// -*- coding: utf-8 -*-

#include "cmdproc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "midi.h"
#include "cmdproc.h"
#include "utils.h"


int cmdprocECHO(int n_args, char **args)
{
  printf("[ECHO]:");
  for(int i=0; i<n_args; i++)
    printf((i+1 == n_args?"%s":"%s "), args[i]);
  printf("\n");
  fflush(stdout);
}

int cmdprocQUIT(int n_arg, char **args)
{
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
  if( n_argc != 1 ){
    errorf("LIST: arg");
  }

  const char *direction = args[0];

  if( 0 == strcmp("INPUT", direction) )
    list_input_devices();
  else if( 0 == strcmp("OUTPUT", direction) )
    list_output_devices();
  else{
    errorf("LIST: arg errrr\n");
    return CMDPROC_FATAL;
  }

  return CMDPROC_SUCCESS;
}

static inline bool check_direction(const char *direction, const char *cmd)
{
  if( 0 == strcmp(direction,"INPUT") ||
      0 == strcmp(direction,"OUTPUT") ){
    return true;
  }

  printf("! %s: devindex arg errrr\n", cmd);
  fflush(stdout);
  return false;
}

static inline bool check_dev(const char *hexes, midi_t *result_r, const char *cmd)
{
  bool has_error;
  int x = hexes_to_unsigned(hexes, &has_error);
  if( has_error ){
    printf("! %s: devindex arg errrr\n", cmd);
    fflush(stdout);
    return false;
  }
  *result_r = x;
  return true;
}

int cmdprocOPEN(int n_args, char **args)
{
  if( n_args != 2 ){
    printf("! OPEN: invalid number of arguments");
    fflush(stdout);
    return CMDPROC_FATAL;
  }

  const char *direction = args[0];
  midi_t device_index;
  if( ! check_direction(direction, "OPEN") || ! check_dev(args[1], &device_index, "OPEN") )
    return CMDPROC_FATAL;
    
  midi_t internal_dev; 
  if( direction[0] == 'I' )
    internal_dev = mi_open(device_index);
  else
    internal_dev = mo_open(device_index);

  // internal_dev == 0 は失敗を表す
  
  printf("< %03X\n",internal_dev);
  fflush(stdout);

  return CMDPROC_SUCCESS;
    
}

int cmdprocCLOSE(int n_args, char **args)
{
  if( n_args != 2 ){
    printf("! CLOSE: invalid number of arguments");
    fflush(stdout);
    return CMDPROC_FATAL;
  }

  const char *direction = args[0];
  midi_t dev;
  if( ! check_direction(direction, "CLOSE") || ! check_dev(args[1], &dev, "CLOSE") )
    return CMDPROC_FATAL;

  if( direction[0] == 'I' )
    mi_close(dev);
  else 
    mo_close(dev);

    return CMDPROC_SUCCESS;

}

int cmdprocRESET(int n_args, char **args)
{
  if( n_args != 2 ){
    printf("! RESET: invalid number of arguments");
    fflush(stdout);
    return CMDPROC_FATAL;
  }

  const char *direction = args[0];
  midi_t dev;
  if( ! check_direction(direction, "RESET") || ! check_dev(args[1], &dev, "RESET") )
    return CMDPROC_FATAL;

  if( direction[0] == 'I' )
    mi_reset(dev);
  else
    mo_reset(dev);

    return CMDPROC_SUCCESS;

}

int cmdprocSTOP(int n_args, char **args)
{
  if( n_args != 1 ){
    printf("! STOP: invalid number of arguments");
    fflush(stdout);
    return CMDPROC_FATAL;
  }

  midi_t dev;
  if( ! check_dev(args[0], &dev, "STOP") )
    return CMDPROC_FATAL;

  mi_stop(dev);
  return CMDPROC_SUCCESS;

}

int cmdprocLISTEN(int n_args, char **args)
{
  if( n_args != 1 ){
    printf("F LISTEN: invalid number of arguments");
    fflush(stdout);
    return CMDPROC_FATAL;
  }
  
  midi_t dev;
  if( ! check_dev(args[0], &dev, "LISTEN") )
    return CMDPROC_FATAL;

  errorf("F LISTEN %d %s", dev,args[0]);
  fflush(stdout);
  
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

int cmdprocSEND(int n_args, char **args)
{
  if( n_args != 2 ){
    printf("! SEND: invalid number of arguments");
    fflush(stdout);
    return CMDPROC_FATAL;
  }

  
  
  midi_t dev;
  if( ! check_dev(args[0], &dev, "SEND") )
    return CMDPROC_FATAL;

  const char *msg = args[1];
  size_t msg_len = strlen(msg);
  bool passed = true;
  for(const char *p=msg; *p; *p++)
    if( 0 > hex2int(*p) ){
      passed = false;
      break;
    }

  // ステータスバイトの基本チェック
  if( ! (hex2int(msg[0])&0x8) )
    passed = false;
  // データバイトの基本チェック
  if( msg_len >= 4 && (hex2int(msg[2])&0x8) )
    passed = false;
  if( msg_len >= 6 && (hex2int(msg[4])&0x8) )
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
        case 0xF6: case 0xF8: case 0xFA: case 0xFB: 
        case 0xFC: case 0xFE: case 0xFF:
        if( msg_len != 4 )
          passed = false;
        break;
        case 0xF1: case 0xF3:
        if( msg_len != 2 )
          passed = false;
        break;
        default:
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
    errorf("! SEND illegal message: %s", msg);
    return CMDPROC_FATAL;
  }

  return CMDPROC_SUCCESS;

}

int cmdprocCALLBACK(int n_args, char **args)
{
  if( n_args != 3 ){
    errorf("! CALLBACK");
    return CMDPROC_FATAL;
  }

  const char *device = args[0];
  const char *signal_pattern = args[1];
  const char *callback_id = args[2];

  
  if( 0 == strcmp(device, "*") )
    register_global_callback(signal_pattern, callback_id);
  else{
    int indev = atoi(device);
    register_local_callback(indev, signal_pattern, callback_id);
  }

  return CMDPROC_SUCCESS;
}


