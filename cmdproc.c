// -*- coding: utf-8 -*-

#include "cmdproc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "midi.h"
#include "cmdproc.h"
#include "utils.h"

// convert string to boolean
static bool to_boolean(const char *s, bool *result_r)
{
  if( 0==strcmp(s,"ENABLE") || 0==strcmp(s,"ON") || 0==strcmp(s,"YES") ||
      0==strcmp(s,"TRUE") || 0==strcmp(s,"1") )
    *result_r = true;
  else if( 0==strcmp(s,"DISABLE") || 0==strcmp(s,"OFF") || 0==strcmp(s,"NO") ||
            0==strcmp(s,"FALSE") || 0==strcmp(s,"0") )
    *result_r = false;
  else
    return false;

  return true;
}

int cmdprocDEBUG(int n_args, char **args)
{
  if( n_args != 1 )
    return CMDPROC_FATAL;

  bool bl;
  if( to_boolean(args[0], &bl) )
    set_debug(bl);
  else
    return CMDPROC_FATAL;

  DEBUGF("debug mode enabled");
  
  return CMDPROC_SUCCESS;
}

// 主に同期を取るために使われる
int cmdprocECHO(int n_args, char **args)
{
  DEBUGF("ECHO with %d argument(s)", n_args);
  printf("< ");
  for(int i=0; i<n_args; i++)
    printf((i+1 == n_args?"%s":"%s "), args[i]);
  printf("\n");
  fflush(stdout);
}

int cmdprocQUIT(int n_arg, char **args)
{
  DEBUGF("QUIT");
  exit(0);

  // 
  return 0;
}


static void list_names(char **names)
{
  printf("{\n");

  for(char **p = names; *p; p++)
    printf("%s\n", *p);
  
  printf("}\n");
  fflush(stdout);
  free_enum_device_names(names);
}

static void list_input_devices()
{
  list_names(mi_enum_device_names());
}

static void list_output_devices()
{
  list_names(mo_enum_device_names());
}


int cmdprocLIST(int n_argc, char **args)
{
  if( n_argc != 1 )
    return CMDPROC_FATAL;

  const char *direction = args[0];

  if( 0 == strcmp("INPUT", direction) )
    list_input_devices();
  else if( 0 == strcmp("OUTPUT", direction) )
    list_output_devices();
  else{
    return CMDPROC_FATAL;
  }

  DEBUGF("LIST %s", direction);
  return CMDPROC_SUCCESS;
}

static inline bool check_direction(const char *direction, const char *cmd, bool ext)
{
  if( 0 == strcmp(direction,"INPUT") ||
      0 == strcmp(direction,"OUTPUT") ||
      // 以下はOPENの場合のみ有効な指定子である
      (ext && (0 == strcmp(direction,"INPUT8T") ||
               0 == strcmp(direction,"INPUT9T")))
      ){
    return true;
  }

  return false;
}

static inline bool check_dev(const char *hexes, midi_t *result_r, const char *cmd)
{
  bool has_error;
  int x = hexes_to_unsigned(hexes, &has_error);
  if( has_error ){
    return false;
  }
  *result_r = x;
  return true;
}

int cmdprocOPEN(int n_args, char **args)
{
  if( n_args != 2 )
    return CMDPROC_FATAL;

  const char *direction = args[0];
  midi_t device_index;
  if( ! check_direction(direction, "OPEN", true) || ! check_dev(args[1], &device_index, "OPEN") )
    return CMDPROC_FATAL;
    
  midi_t internal_dev; 
  if( direction[0] == 'I' ){
    int translate_to;
    if( 0 == strcmp(direction, "INPUT") )
      translate_to = MIDIIN_TRANSLATE_NONE;
    else if( 0 == strcmp(direction, "INPUT8T") )
        translate_to = MIDIIN_TRANSLATE_TO_8;
    else if( 0 == strcmp(direction, "INPUT9T") )
      translate_to = MIDIIN_TRANSLATE_TO_9;
    else
      return CMDPROC_FATAL;
  
    internal_dev = mi_open(device_index, translate_to);
  }else if( 0 == strcmp(direction, "OUTPUT") )
    internal_dev = mo_open(device_index);
  else
    return CMDPROC_FATAL;
  
  // internal_dev == 0 は失敗を表す
  
  printf("< %03X\n",internal_dev);
  fflush(stdout);

  DEBUGF("OPEN %s %d --> %03X", direction, device_index, internal_dev);
  
  return CMDPROC_SUCCESS;
    
}

int cmdprocCLOSE(int n_args, char **args)
{
  if( n_args != 2 )
    return CMDPROC_FATAL;

  const char *direction = args[0];
  midi_t dev;
  if( ! check_direction(direction, "CLOSE", false) || ! check_dev(args[1], &dev, "CLOSE") )
    return CMDPROC_FATAL;

  DEBUGF("CLOSE %s %d", direction, dev);

  if( direction[0] == 'I' )
    mi_close(dev);
  else 
    mo_close(dev);

  return CMDPROC_SUCCESS;

}

int cmdprocRESET(int n_args, char **args)
{
  if( n_args != 2 )
    return CMDPROC_FATAL;

  const char *direction = args[0];
  midi_t dev;
  if( ! check_direction(direction, "RESET", false) || ! check_dev(args[1], &dev, "RESET") )
    return CMDPROC_FATAL;

  DEBUGF("CLOSE %s %d", direction, dev);

  if( direction[0] == 'I' )
    mi_reset(dev);
  else
    mo_reset(dev);

  return CMDPROC_SUCCESS;

}

int cmdprocSTOP(int n_args, char **args)
{
  if( n_args != 1 )
    return CMDPROC_FATAL;

  midi_t dev;

  DEBUGF("STOP %d", dev);

  if( ! check_dev(args[0], &dev, "STOP") )
    return CMDPROC_FATAL;

  mi_stop(dev);
  return CMDPROC_SUCCESS;

}

int cmdprocLISTEN(int n_args, char **args)
{
  if( n_args != 1 )
    return CMDPROC_FATAL;
  
  midi_t dev;

  DEBUGF("STOP %d", dev);

  if( ! check_dev(args[0], &dev, "LISTEN") )
    return CMDPROC_FATAL;

  
  mi_listen(dev);
  return CMDPROC_SUCCESS;
}

static inline int hex2int(char c)
{
  if( c >= '0' && c <= '9' )
    return c-'0';
  else if( c >= 'A' && c <= 'F' )
    return c-'A'+10;
  else
    return -1;
}

static int try_send_sysex(midi_t dev, char *msg, size_t msg_len)
{
  // msg_len >= 2 は保証されていることに注意

  // 'F7' terminator not found
  if( ! (msg[msg_len-2]=='F' && msg[msg_len-1]=='7') )
    return CMDPROC_FATAL;

  uint8_t *bytes = (uint8_t*)msg;
  *bytes++ = 0xF0;

  for(char *s=msg+2,*end =msg+(msg_len-2); s!=end; s+=2)
    *bytes++ = (uint8_t)(hex2int(s[0])*16 + hex2int(s[1]));

  *bytes = 0xF7;

  mo_send_sysex(dev, (uint8_t*)msg, (int)msg_len/2);
  
  return CMDPROC_SUCCESS;
}

int cmdprocSEND(int n_args, char **args)
{
  if( n_args != 2 )
    return CMDPROC_FATAL;
  
  midi_t dev;
  if( ! check_dev(args[0], &dev, "SEND") )
    return CMDPROC_FATAL;

  char *msg = args[1];
  size_t msg_len = strlen(msg);

  DEBUGF("SEND %d %s", dev, msg);

  // メッセージはHEX表記であるため、
  // 正しい表記は必ず偶数個のキャラクタから構成されねばならない
  if( msg_len % 2 == 1 )
    return CMDPROC_FATAL;
  
  bool passed = true;
  for(const char *p=msg; *p; *p++)
    if( 0 > hex2int(*p) ){
      passed = false;
      break;
    }

  if( msg[0]=='F' && msg[1]=='0' ){
    // System-Exclusive Message
    return try_send_sysex(dev, msg, msg_len);
  }

  /***************************************************************************/
  /********************  Except System-Exclusive Message *********************/
  /***************************************************************************/

  // SysExでなければ、最長6桁である
  if( msg_len > 6 )
    return CMDPROC_FATAL;
  
  
  // ステータスバイトの基本チェック
  if( ! (hex2int(msg[0])&0x8) )
    passed = false;
  // データバイトの基本チェック
  if( msg_len >= 4 && (hex2int(msg[2])&0x8) )
    passed = false;
  if( msg_len == 6 && (hex2int(msg[4])&0x8) )
    passed = false;
  
  if( passed ){
    int st_1 = hex2int(msg[0]);
    int st_2 = hex2int(msg[1]);
    if( st_1 < 0xF ){
      switch( st_1 ){
      case 0x8: case 0x9: case 0xA: case 0xB: case 0xE:
        if( msg_len != 6 )
          passed = false;
        break;
      default:
        if( msg_len != 4 )
          passed = false;
        break;
      }
    }else{
      switch( (st_1<<4)|st_2 ){
      case 0xF2:
        if( msg_len != 6 )
          passed = false;
        break;
      case 0xF1: case 0xF3:
        if( msg_len != 4 )
          passed = false;
        break;
      case 0xF6: case 0xF8: case 0xFA: case 0xFB:
      case 0xFC: case 0xFE: case 0xFF:
        if( msg_len != 2 )
          passed = false;
        break;
      default:
        // 0xF4,0xF5,0xF9,0xFD are undefined
        // 0xF7 is perfectory wrong
        passed = false; break; // err
        }
    }
  }
        
  if( passed ){
    uint8_t status_byte = (uint8_t)((hex2int(msg[0])<<4)|hex2int(msg[1]));
    if( msg_len == 2 )
      mo_send1(dev, status_byte);
    else{
      uint8_t data_1 = (uint8_t)((hex2int(msg[2])<<4)|hex2int(msg[3]));
      if( msg_len == 4 )
        mo_send2(dev, status_byte, data_1);
      else{
        uint8_t data_2 = (uint8_t)((hex2int(msg[4])<<4)|hex2int(msg[5]));
        mo_send3(dev, status_byte, data_1, data_2);
      }
    }
  }else{
    return CMDPROC_FATAL;
  }

  return CMDPROC_SUCCESS;

}

int cmdprocCALLBACK(int n_args, char **args)
{
  if( n_args != 3 )
    return CMDPROC_FATAL;

  const char *device = args[0];
  const char *signal_pattern = args[1];
  const char *callback_id = args[2];

  DEBUGF("CALLBACK %s %s %s", device, signal_pattern, callback_id);

  if( 0 == strcmp(device, "*") )
    register_global_callback(signal_pattern, callback_id);
  else{
    int indev = atoi(device);
    register_local_callback(indev, signal_pattern, callback_id);
  }

  return CMDPROC_SUCCESS;
}

