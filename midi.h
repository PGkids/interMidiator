// -*- coding: utf-8 -*-

#ifndef _MIDI_H_
#define _MIDI_H_

#include <stdint.h>
#include <stdbool.h>

/* <<OS depend>>
   os_* ... os function
   mi_* ... midi in 
   os_midi_out_* ... midi out
*/

typedef unsigned short midi_t;

void os_midi_initialize();

char** os_midi_in_enum_device_names();
char** os_midi_out_enum_device_names();
void os_midi_free_enum_device_names(char **names);

bool register_global_callback(const char *signal_pattern, const char *callback_id);
bool register_local_callback(midi_t indev, const char *signal_pattern, const char *callback_id);

//bool register_global_filter(const char *signal_pattern);
//bool register_local_filter(midi_t indev, const char *signal_pattern);


#define MIDIIN_TRANSLATE_TO_8  8
#define MIDIIN_TRANSLATE_TO_9  9
#define MIDIIN_TRANSLATE_NONE  0

midi_t os_midi_in_open(int device_index, int translate_to);
void os_midi_in_close(midi_t);
void os_midi_in_listen(midi_t);
void os_midi_in_stop(midi_t);
void os_midi_in_reset(midi_t);

midi_t os_midi_out_open(int device_index);
void os_midi_out_close(midi_t);
void os_midi_out_reset(midi_t);

void os_midi_out_send1(midi_t outdev, uint8_t status_byte);
void os_midi_out_send2(midi_t outdev, uint8_t status_byte, uint8_t data_byte);
void os_midi_out_send3(midi_t outdev, uint8_t status_byte, uint8_t data_byte_1, uint8_t data_byte_2);
void os_midi_out_send_sysex(midi_t outdev, const uint8_t *msg, int msg_len);


#endif //_MIDI_H_
