// -*- coding: utf-8 -*-

/// test callback

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "callback.h"
#include "utils.h"
#include "signal.h"

#define isDENIED(s) (0==strcmp(s,"DENIED"))
#define isOTHERWISE(s) (0==strcmp(s,"OTHERWISE"))
#define isBEGIN(s) (0==strcmp(s,"BEGIN"))
#define isEND(s) (0==strcmp(s,"END"))

void finalize_callback_info(callback_info_t *info)
{
  if( info->signal_pattern ){
    free(info->signal_pattern);
    info->signal_pattern = NULL;
  }
  if( info->callback_id ){
    free(info->callback_id);
    info->callback_id = NULL;
  }
}


  
void initialize_callback_table(callback_table_t *table)
{
  const size_t size = 2; // must be an even number
  table->max = size;
  table->count = 0;
  table->infos = (callback_info_t*)calloc(table->max, sizeof(callback_info_t));

  table->filter_pattern = NULL;
  table->callback_id_for_begin = NULL;
  table->callback_id_for_end = NULL;
  table->callback_id_for_denied = NULL;
  table->callback_id_for_otherwise = NULL;
}

void finalize_callback_table(callback_table_t *table)
{
  size_t n = table->max;
  size_t cnt = table->count;
    
  for(int i=0; i<n && cnt>0; i++){
    callback_info_t *info = &table->infos[i];
    if( info->signal_pattern ){
      finalize_callback_info(info);
      cnt--;
    }
  }

  free(table->infos);
  if( table->filter_pattern ) free(table->filter_pattern);
  if( table->callback_id_for_denied ) free(table->callback_id_for_denied);
  if( table->callback_id_for_otherwise ) free(table->callback_id_for_otherwise);
  if( table->callback_id_for_begin ) free(table->callback_id_for_begin);
  if( table->callback_id_for_end ) free(table->callback_id_for_end);

  memset(table, 0, sizeof(callback_table_t));
}

static void reset_string(char **place, const char *new_s)
{
  if( *place ) free( *place );
  *place = new_s ? duplicate_string(new_s) : NULL;
}

void add_callback(const char *signal_pattern, const char *callback_id, callback_table_t *table)
{
  if( isDENIED(signal_pattern) ){
    reset_string(&table->callback_id_for_denied, callback_id);
    return;
  }
  if( isOTHERWISE(signal_pattern) ){
    reset_string(&table->callback_id_for_otherwise, callback_id);
    return;
  }
  if( isBEGIN(signal_pattern) ){
    reset_string(&table->callback_id_for_begin, callback_id);
    return;
  }
  if( isEND(signal_pattern) ){
    reset_string(&table->callback_id_for_end, callback_id);
    return;
  }
  
  
  if( table->max == table->count ){
    callback_info_t *new_infos = realloc(table->infos, sizeof(callback_info_t)*table->max*2);
    memset(&new_infos[table->max], 0, sizeof(callback_info_t)*table->max);
    table->infos = new_infos;
    table->max *= 2;
  }

  size_t n = table->max;
  size_t cnt = table->count;
  int free_cell_index = -1;
  
  for(int i=0; i<n && cnt>0; i++){
    callback_info_t *info = &table->infos[i];
    if( info->callback_id ){
      //printf("C INFO: %s %s\n",info->callback_id, info->signal_pattern);

      cnt--;
      if( 0 == strcmp(callback_id, info->callback_id) ){
        if( 0 != strcmp(signal_pattern, info->signal_pattern ) ){
          free(info->signal_pattern);
          info->signal_pattern = duplicate_string(signal_pattern);
        }
        return;
      }
    }else if( free_cell_index < 0 ){
      free_cell_index = i;
    }
  }


  if( free_cell_index < 0 ){
    if( 0 == table->count )
      free_cell_index = 0;
    else
      free_cell_index = table->count;
  }

  callback_info_t *cell = &table->infos[free_cell_index];
  cell->signal_pattern = duplicate_string(signal_pattern);
  cell->callback_id = duplicate_string(callback_id);
  table->count++;
   
}

void del_callback(const char *callback_id, callback_table_t *table)
{
  if( isDENIED(callback_id) ){
    reset_string(&table->callback_id_for_denied, NULL);
    return;
  }
  if( isOTHERWISE(callback_id) ){
    reset_string(&table->callback_id_for_otherwise, NULL);
    return;
  }
  if( isBEGIN(callback_id) ){
    reset_string(&table->callback_id_for_begin, NULL);
    return;
  }
  if( isEND(callback_id) ){
    reset_string(&table->callback_id_for_end, NULL);
    return;
  }

  size_t n = table->max;
  size_t cnt = table->count;

  for(int i=0; i<n && cnt>0; i++){
    callback_info_t *info = &table->infos[i];
    if( info->callback_id &&
        0 == strcmp(callback_id, info->callback_id) ){
      finalize_callback_info(&table->infos[i]);
      table->count--;
      return;
    }
  }

  // warning
}

static const char *msg_signal = NULL;
static midi_t msg_indev = 0;
static inline void send_msg_signal_if_needed()
{

  fprintf(stderr,"debug---\n"); fflush(stderr);
  if( msg_signal ){
    const char *signal = msg_signal;
    midi_t indev = msg_indev;
    if( signal[0]=='F' && signal[1]=='0' ){
      // sysex
    }else{
      int len = signal[2] ? (signal[4] ? 3 : 2) : 1;
      char tmp[64];
      printf("%d %03X %s\n",len, indev, signal);
    }

    //printf("%s\n", msg_signal);
    msg_signal = NULL;
  }      
}

static inline void send_callback_id(const char *id)
{
  send_msg_signal_if_needed();
  printf("C %s\n", id);
}

#define CB_OTHERWISE 0x01
#define CB_DENIED    0x80
#define CB_FOUND     0x20
#define CB_APPLIED   0x10

static int find_callbacks(const char *signal, callback_table_t *table)
{
  int st = 0;
  size_t sent = 0;

  if( table->filter_pattern == NULL || sigcmp(table->filter_pattern, signal) ){
  
    size_t n = table->max;
    size_t cnt = table->count;
    
    for(int i=0; i<n && cnt>0; i++){
      callback_info_t *info = &table->infos[i];
      if( info->signal_pattern ){
        cnt--;
        if( sigcmp(info->signal_pattern, signal) ){
          send_callback_id(info->callback_id);
          sent++;
          st |= CB_FOUND;
        }
      }
    }

    if( sent == 0 && table->callback_id_for_otherwise ){
      send_callback_id(table->callback_id_for_otherwise);
      sent = 1;
      st |= CB_OTHERWISE;
    }
  }else{
    st |= CB_DENIED;
    if( table->callback_id_for_denied ){
      send_callback_id(table->callback_id_for_denied);
      sent = 1;
    }
  }
  
  if( sent )
    st |= CB_APPLIED;  
  
  return st;
}



void dispatch_callbacks(midi_t indev, const char *signal,
                        callback_table_t *global_table,
                        callback_table_t *local_table)
{
  msg_signal = signal;
  msg_indev = indev;
  //int len = signal[2] ? (signal[4] ? 3 : 2) : 1;
  //char tmp[64];
  //sprintf(tmp, "%d %03X %s\n",len, indev, signal);
  //msg_signal = tmp;
  int st1 = find_callbacks(signal, global_table);
  int st2 = st1&CB_DENIED ? 0 : find_callbacks(signal, local_table);
  if( (st1|st2)&CB_APPLIED )
    fflush(stdout);
}

//todo
//replace_callbackの創設
