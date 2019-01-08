// -*- coding: utf-8 -*-

#include "midi.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <mmsystem.h>
#include "callback.h"

#define MIDI_TABLE_SIZE 4096

typedef struct {
  HMIDIIN handle;
  callback_table_t callback_table;
} midi_in_info_t;
typedef struct {
  HMIDIOUT handle;
} midi_out_info_t;

static midi_in_info_t  midi_ins[MIDI_TABLE_SIZE];
static midi_out_info_t midi_outs[MIDI_TABLE_SIZE];

callback_table_t global_callback_table;

void system_init_midi()
{
  memset(midi_ins, 0, sizeof(midi_ins));
  memset(midi_outs, 0, sizeof(midi_outs));
  initialize_callback_table(&global_callback_table);
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


void free_enum_device_names(char **names)
{
  for(char **p=names; *p; p++)
    free(*p);
  free(names);
}

char** mi_enum_device_names()
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

char** mo_enum_device_names()
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

      
      global_lock();
      if( 0 == len )
        printf("0 %03X RECVERROR\n");
      else{
        char signal[7];
        switch( len ){
        case 1: sprintf(signal,"%02X",data[0]); break;
        case 2: sprintf(signal,"%02X%02X",data[0], data[1]); break;
        case 3: sprintf(signal,"%02X%02X%02X",data[0], data[1], data[2]); break;
        }
        dispatch_callbacks(index, signal, &info->callback_table, &global_callback_table);
      }
      fflush(stdout);
      global_unlock();
      //midiOutShortMsg(hmo, dwParam1);
    }
    break;
  case MIM_LONGDATA:
    {
      MIDIHDR *hdr = (MIDIHDR*)param;
      int len = (int)hdr->dwBytesRecorded;
      if( len > 0 ){
        //midi->deliver(midi->_buffer_of_header, len);
        midiInPrepareHeader(info->handle, hdr, sizeof(MIDIHDR));
        midiInAddBuffer(info->handle, hdr, sizeof(MIDIHDR));
      }
    }
    break;
  }
}

midi_t mi_open(int device_index)
{
  HMIDIIN h;
  midi_t reserved = alloc_midi_in(NULL); // 仮確保
  if( midiInOpen(&h, (UINT)device_index, (DWORD_PTR)MidiInProc, reserved, CALLBACK_FUNCTION) == MMSYSERR_NOERROR ){
    //midiInStart(h);
    midi_ins[reserved].handle = h;
    initialize_callback_table(&midi_ins[reserved].callback_table);
    return reserved;
  }
  else
    return 0;
}

void mi_close(midi_t indev)
{
  midiInClose(CASTIN(indev));
  midi_ins[indev].handle = NULL;
  finalize_callback_table(&midi_ins[indev].callback_table);
}

void mi_listen(midi_t indev)
{
  midiInStart(CASTIN(indev));
}

void mi_reset(midi_t indev)
{
  midiInReset(CASTIN(indev));
}

void mi_stop(midi_t indev)
{
  midiInStop(CASTIN(indev));
}

midi_t mo_open(int device_index)
{
  HMIDIOUT h=0;
  if(midiOutOpen(&h, (UINT)device_index, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR )
    return alloc_midi_out(h);
  else
    return 0;
}

void mo_close(midi_t outdev)
{
  midiOutClose(CASTOUT(outdev));
  midi_outs[outdev].handle = NULL;
}

void mo_reset(midi_t outdev)
{
  midiOutReset(CASTOUT(outdev));
}

void mo_send1(midi_t outdev, uint8_t status_byte)
{
  midiOutShortMsg(CASTOUT(outdev), status_byte);
}
void mo_send2(midi_t outdev, uint8_t status_byte, uint8_t data_byte)
{
  midiOutShortMsg(CASTOUT(outdev), status_byte|(data_byte<<8));
}
void mo_send3(midi_t outdev, uint8_t status_byte, uint8_t data_byte_1, uint8_t data_byte_2)
{
  midiOutShortMsg(CASTOUT(outdev), status_byte|(data_byte_1<<8)|(data_byte_2<<16));
}


bool register_global_callback(const char *signal_pattern, const char *callback_id)
{
  add_callback(signal_pattern, callback_id, &global_callback_table);
  return true;
}
bool register_local_callback(midi_t indev, const char *signal_pattern, const char *callback_id)
{
  add_callback(signal_pattern, callback_id, &midi_ins[indev].callback_table);
}
