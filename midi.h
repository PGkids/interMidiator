// -*- coding: utf-8 -*-

#ifndef _MIDI_H_
#define _MIDI_H_

#include <stdint.h>
#include <stdbool.h>

/* 
   mi_* ... midi in 
   mo_* ... midi out
*/

typedef unsigned short midi_t;

void system_init_midi();

char** mi_enum_device_names();
char** mo_enum_device_names();
void free_enum_device_names(char **names);

bool register_global_callback(const char *signal_pattern, const char *callback_id);
bool register_local_callback(midi_t indev, const char *signal_pattern, const char *callback_id);

bool register_global_filter(const char *signal_pattern);
bool register_local_filter(midi_t indev, const char *signal_pattern);



midi_t mi_open(int device_index);
void mi_close(midi_t);
void mi_listen(midi_t);
void mi_stop(midi_t);
void mi_reset(midi_t);

midi_t mo_open(int device_index);
void mo_close(midi_t);
void mo_reset(midi_t);

void mo_send1(midi_t outdev, uint8_t status_byte);
void mo_send2(midi_t outdev, uint8_t status_byte, uint8_t data_byte);
void mo_send3(midi_t outdev, uint8_t status_byte, uint8_t data_byte_1, uint8_t data_byte_2);


#endif //_MIDI_H_
