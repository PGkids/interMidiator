// -*- coding: utf-8 -*-

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include "midi.h"

typedef struct {
  char *signal_pattern;
  char *callback_id;
}callback_info_t;

typedef struct {
  size_t max;
  size_t count;
  callback_info_t *infos;

  char *filter_pattern;
  char *callback_id_for_begin;
  char *callback_id_for_end;
  char *callback_id_for_denied;
  char *callback_id_for_otherwise;
} callback_table_t;

void initialize_callback_table(callback_table_t *table);
void finalize_callback_table(callback_table_t *table);
void add_callback(const char *signal_pattern, const char *callback_id, callback_table_t *table);
void del_callback(const char *callback_id, callback_table_t *table);
//const char* find_callbacks(const char *signal, callback_table_t *table);

void dispatch_callbacks(midi_t indev, const char *signal,
                        callback_table_t *global_table,
                        callback_table_t *local_table);

#endif
