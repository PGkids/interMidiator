// -*- coding: utf-8 -*-

#include "os.h"
#include <windows.h>

#include "midi.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mmsystem.h>
#include "callback.h"

void os_msec_sleep(int msec)
{
  Sleep(msec);
}

void os_abort_with_alert(const char *alert_msg)
{
  MessageBox(NULL, TEXT(alert_msg), TEXT("OS ABORTED"), MB_OK|MB_ICONERROR);
  exit(-1);
}

static CRITICAL_SECTION _os_exclusive_mutex;

void os_initialize()
{
  InitializeCriticalSection(&_os_exclusive_mutex);
}

void os_exclusive_lock()
{
  EnterCriticalSection(&_os_exclusive_mutex);
}

void os_exclusive_unlock()
{
  LeaveCriticalSection(&_os_exclusive_mutex);
}

DWORD WINAPI _wrapped_thread(LPVOID p)
{
  ((void (*)(void))p)();
}

void os_run_thread(void (*proc)(void))
{
  CreateThread(NULL,
               1024,
               _wrapped_thread,
               proc,
               0,
               NULL);               
}

#define MIDI_TABLE_SIZE 256
#define RDBUFF_SIZE 4096

typedef struct {
  bool listening;
  HMIDIIN handle;
  MIDIHDR hdr;
  int translate_to; // 8 or 9 or (0 default) 
  uint8_t rdbuff[RDBUFF_SIZE];
  callback_table_t callback_table;
} midi_in_info_t;

typedef struct {
  HMIDIOUT handle;
  HANDLE   thread_handle;
  DWORD    thread_id;
} midi_out_info_t;

static midi_in_info_t  midi_ins[MIDI_TABLE_SIZE];
static midi_out_info_t midi_outs[MIDI_TABLE_SIZE];

callback_table_t global_callback_table;

void os_midi_initialize()
{
  memset(midi_ins, 0, sizeof(midi_ins));
  memset(midi_outs, 0, sizeof(midi_outs));
  initialize_callback_table(&global_callback_table);
}

callback_table_t* os_get_local_callback_table(midi_t indev)
{
  return &midi_ins[indev].callback_table;
}

callback_table_t* os_get_global_callback_table()
{
  return &global_callback_table;
}


static midi_t alloc_midi_in(HMIDIIN h)
{
  for(int i=1; i<MIDI_TABLE_SIZE; i++)
    if( NULL == midi_ins[i].handle ){
      midi_ins[i].handle = h;
      return i;
    }

  return 0;
}
static midi_t alloc_midi_out(HMIDIOUT h)
{
  for(int i=1; i<MIDI_TABLE_SIZE; i++)
    if( NULL == midi_outs[i].handle ){
      midi_outs[i].handle = h;
      return i;
    }

  return 0;
}

static inline HMIDIIN CASTIN(midi_t indev)
{
  return midi_ins[indev].handle;
}

static inline HMIDIOUT CASTOUT(midi_t outdev)
{
  return midi_outs[outdev].handle;
}


static char** alloc_ppchars(int n)
{
  return (char**)malloc(sizeof(void*)*n);
}


void os_midi_free_enum_device_names(char **names)
{
  for(char **p=names; *p; p++)
    free(*p);
  free(names);
}

char** os_midi_in_enum_device_names()
{
  int n_dev = midiInGetNumDevs();
  char **names = alloc_ppchars(n_dev+1);
  names[n_dev] = NULL;
  for(int id=0; id<n_dev; id++){
    MIDIINCAPS caps;
    midiInGetDevCaps(id, &caps, sizeof(caps));
    names[id] = duplicate_string(caps.szPname);
  }
  return names;
}

char** os_midi_out_enum_device_names()
{
  int n_dev = midiOutGetNumDevs();
  char **names = alloc_ppchars(n_dev+1);
  names[n_dev] = NULL;
  for(int id=0; id<n_dev; id++){
    MIDIOUTCAPS caps;
    midiOutGetDevCaps(id, &caps, sizeof(caps));
    names[id] = duplicate_string(caps.szPname);
  }
  return names;
}

static inline void shex02(char *s, int h)
{
  s[0] = (h&0xF0) < 0xA0 ? (char)(h>>4) + '0' : (char)(h>>4) + ('A'-10);
  s[1] = (h&0x0F) < 0x0A ? (char)(h&0x0F) + '0' : (char)(h&0x0F) + ('A'-10);
}

void CALLBACK MidiInProc(HMIDIIN hmi, UINT message, DWORD index, DWORD param, DWORD param2)
{
  midi_in_info_t *info = &midi_ins[index];
  
  
  switch (message) {
  case MIM_DATA:
    {
      uint8_t status_byte = (uint8_t)param; // == (midi_num_t)(data&0xFF)
      uint8_t val1 = (uint8_t)(param>>8);
      uint8_t val2 = (uint8_t)(param>>16);
      uint8_t data[3];
      data[0] = status_byte;
      data[1] = val1;
      int len;
      if( status_byte < 0xF0 ){
        switch( status_byte >> 4 ){
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xE:
          data[2] = val2;
          len = 3;

          // translation
          if((status_byte&0xF0)==0x90 &&
              info->translate_to==MIDIIN_TRANSLATE_TO_8
              && data[2]==0 ){
            // NOTEON --> NOTEOFF
            data[0] = 0x80 | (data[0]&0x0F);
          }else if ((status_byte&0xF0)==0x80 && info->translate_to==MIDIIN_TRANSLATE_TO_9){
            // NOTEOFF --> NOTEON
            data[0] = 0x90 | (data[0]&0x0F);
            data[2] = 0;
          }
            
          break;
        default:
          len = 2; break;
        }
      }else{
        switch( status_byte ){
        case 0xF2:
          data[2] = val2;
          len = 3;
          break;
        case 0xF6: case 0xF8: case 0xFA: case 0xFB: 
        case 0xFC: case 0xFE: case 0xFF:
          len = 1; break;
        case 0xF1: case 0xF3:
          len = 2; break;
        default:
          len = 0; break; // err
        }
      }

      
      if( 0 == len )
        errorf("RECVERROR\n");
      else{
        char signal[7];
        shex02(signal, data[0]);
        if( len == 1 )
          signal[2] = '\0';
        else{
          shex02(signal+2, data[1]);
          if( len == 2 )
            signal[4] = '\0';
          else{
            shex02(signal+4, data[2]);
            signal[6] = '\0';
          }
        }
        
        /**** DEBUG ***/
        DEBUGF("[Signal]: IN%03X,%s", index, signal);
        /*** END OF DEBUG ***/
        global_lock();
        dispatch_callbacks(index, signal, &info->callback_table, &global_callback_table);
        fflush(stdout);
        global_unlock();
      }
    }
    break;
  case MIM_LONGDATA:
    {
      MIDIHDR *hdr = (MIDIHDR*)param;
      int len = (int)hdr->dwBytesRecorded;
      //printf("DEBUG: [LONGDATA] %d\n",len);
      if( len>0 ){
        if( ((uint8_t*)hdr->lpData)[len-1]==0xF7 ){
          len = &((uint8_t*)hdr->lpData)[len] - info->rdbuff; // 真の長さ          
          hdr->lpData = info->rdbuff; // reset buffer
          uint8_t *begin=info->rdbuff;
          begin[len*2] = '\0';
          for(uint8_t *src=begin+len-1,*dst=begin+(len-1)*2; src>=begin; src--, dst-=2)
            shex02(dst,*src);
          DEBUGF("[SysEx]: IN%03X,%s", index, begin);
          
          global_lock();
          dispatch_callbacks(index, info->rdbuff, &info->callback_table, &global_callback_table);
          fflush(stdout);
          global_unlock();          
        }else
          hdr->lpData = ((uint8_t*)hdr->lpData)+len;

        hdr->dwBytesRecorded = 0;
        

        midiInAddBuffer(info->handle, hdr, sizeof(MIDIHDR));
      }

    }
    break;
  }
}

midi_t os_midi_in_open(int device_index, int translate_to)
{
  HMIDIIN h;
  midi_t reserved = alloc_midi_in(NULL); // 仮確保
  if( midiInOpen(&h, (UINT)device_index, (DWORD_PTR)MidiInProc, reserved, CALLBACK_FUNCTION) == MMSYSERR_NOERROR ){
    //midiInStart(h);
    midi_in_info_t *info = &midi_ins[reserved];
    info->listening = false;
    info->handle = h;
    info->translate_to = translate_to;
    initialize_callback_table(&info->callback_table);

    memset(&info->hdr, 0, sizeof(MIDIHDR));
    info->hdr.lpData = midi_ins[reserved].rdbuff;
    info->hdr.dwBufferLength = RDBUFF_SIZE;
    midiInPrepareHeader(h, &info->hdr, sizeof(MIDIHDR));
    midiInAddBuffer(h, &info->hdr, sizeof(MIDIHDR));
    
    return reserved;
  }
  else
    return 0;
}

void os_midi_in_close(midi_t indev)
{
  midi_in_info_t *info = &midi_ins[indev];
  HMIDIIN hmi = CASTIN(indev);

  // 停止していなければ、クローズの前処理の前に停止させる
  if( info->listening )
    midiInStop(hmi);
  
  midiInUnprepareHeader(hmi, &info->hdr, sizeof(MIDIHDR));
  midiInClose(hmi);

  // クローズ後の処理
  info->handle = NULL;
  finalize_callback_table(&midi_ins[indev].callback_table);
}

void os_midi_in_listen(midi_t indev)
{
  midi_ins[indev].listening = true;
  midiInStart(CASTIN(indev));
}

void os_midi_in_reset(midi_t indev)
{
  midiInReset(CASTIN(indev));
}

void os_midi_in_stop(midi_t indev)
{
  midi_ins[indev].listening = false;
  midiInStop(CASTIN(indev));
}

#define WM_OS_MIDI_OUT_SEND_SHORT WM_USER
#define WM_OS_MIDI_OUT_SEND_LONG  (WM_USER+1)
#define WM_OS_MIDI_OUT_QUIT (WM_USER+2)

DWORD WINAPI _midiout_proc(LPVOID arg)
{
  midi_out_info_t *info = (midi_out_info_t*)arg;
  HMIDIOUT handle = info->handle;
  //midi_out_info_t *info = &midi_outs[(int)arg];
  MSG msg;
  MIDIHDR hdr;
  while(1){
    GetMessage(&msg, NULL, 0,0); //WM_OS_MIDI_OUT_SEND_SHORT, WM_OS_MIDI_OUT_QUIT);
    //printf("[DEBUG]: msg: %d\n",msg.message);

    switch( msg.message ){
    case WM_OS_MIDI_OUT_SEND_SHORT:
      midiOutShortMsg(handle, (DWORD)msg.wParam);
      break;

    case WM_OS_MIDI_OUT_SEND_LONG:
      hdr.lpData = (LPVOID)msg.lParam;
      hdr.dwBufferLength = msg.wParam;
      hdr.dwBytesRecorded = msg.wParam;
      midiOutPrepareHeader(handle, &hdr, sizeof(MIDIHDR));
      midiOutLongMsg(handle, &hdr, sizeof(MIDIHDR));
 
      while ((hdr.dwFlags&MHDR_DONE) != MHDR_DONE)
          Sleep(0);

      midiOutUnprepareHeader(handle, &hdr, sizeof(MIDIHDR));
      free(hdr.lpData);
      
      //printf("[DEBUG] ok cnt=%d\n",cnt);
      break;

      
    case WM_OS_MIDI_OUT_QUIT: // close
      //printf("[DEBUG] WM_OS_MIDI_OUT_QUIT\n");
      return 0;
    }
  }
  
}

midi_t os_midi_out_open(int device_index)
{
  HMIDIOUT h=0;
  if(midiOutOpen(&h, (UINT)device_index, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR ){
    midi_t outdev = alloc_midi_out(h);
    midi_out_info_t *info = &midi_outs[outdev];
    info->thread_handle = CreateThread(NULL, 0, (LPVOID)_midiout_proc, info, 0,
                                       &info->thread_id);
    return outdev;
  }
  else
    return 0;
}

void os_midi_out_close(midi_t outdev)
{
  midiOutClose(CASTOUT(outdev));
  midi_out_info_t *info = &midi_outs[outdev];

  // joining thread
  PostThreadMessage(info->thread_id, WM_OS_MIDI_OUT_QUIT, 0, 0);
  WaitForSingleObject(info->thread_handle,INFINITE); 
  CloseHandle(info->thread_handle);

  info->handle = NULL;
  info->thread_handle = NULL;
}

void os_midi_out_reset(midi_t outdev)
{
  midiOutReset(CASTOUT(outdev));
}

void os_midi_out_send1(midi_t outdev, uint8_t status_byte)
{
  midi_out_info_t *info = &midi_outs[outdev];
  PostThreadMessage(info->thread_id, WM_OS_MIDI_OUT_SEND_SHORT, status_byte, 0);
  
  //midiOutShortMsg(CASTOUT(outdev), status_byte);
}
void os_midi_out_send2(midi_t outdev, uint8_t status_byte, uint8_t data_byte)
{
  midi_out_info_t *info = &midi_outs[outdev];
  PostThreadMessage(info->thread_id, WM_OS_MIDI_OUT_SEND_SHORT,
                    status_byte|(data_byte<<8), 0);

  //midiOutShortMsg(CASTOUT(outdev), status_byte|(data_byte<<8));
}
void os_midi_out_send3(midi_t outdev, uint8_t status_byte, uint8_t data_byte_1, uint8_t data_byte_2)
{
  midi_out_info_t *info = &midi_outs[outdev];
  PostThreadMessage(info->thread_id, WM_OS_MIDI_OUT_SEND_SHORT,
                    status_byte|(data_byte_1<<8)|(data_byte_2<<16), 0);

  //midiOutShortMsg(CASTOUT(outdev), status_byte|(data_byte_1<<8)|(data_byte_2<<16));
}

void os_midi_out_send_sysex(midi_t outdev, const uint8_t *msg, int msg_len)
{
  midi_out_info_t *info = &midi_outs[outdev];
  uint8_t *buf = (uint8_t*)malloc(msg_len);
  memcpy(buf, msg, msg_len);
  PostThreadMessage(info->thread_id, WM_OS_MIDI_OUT_SEND_LONG,
                    msg_len, (LPARAM)buf);
                    
  //printf("[DEBUG] %d\n",(int)msg_len);
  //for(int i=0; i<msg_len;  i++)
  //  printf("%02X", msg[i]);
  //printf("\n");
}

