// -*- coding: utf-8 -*-

#ifndef _MIDI_H_
#define _MIDI_H_

#include <stdint.h>
#include <stdbool.h>

/* <<OS depend>>
   os_* ... os function
   mi_* ... midi in 
   mo_* ... midi out
*/

typedef unsigned short midi_t;

void os_initialize();
void os_exclusive_lock(), os_exclusive_unlock();
void os_run_thread( void (*proc)(void) );
void os_msec_sleep(int msec);
void os_abort_with_alert(const char *alert_msg);

void os_midi_initialize();

char** mi_enum_device_names();
char** mo_enum_device_names();
void free_enum_device_names(char **names);

bool register_global_callback(const char *signal_pattern, const char *callback_id);
bool register_local_callback(midi_t indev, const char *signal_pattern, const char *callback_id);

bool register_global_filter(const char *signal_pattern);
bool register_local_filter(midi_t indev, const char *signal_pattern);


#define MIDIIN_TRANSLATE_TO_8  8
#define MIDIIN_TRANSLATE_TO_9  9
#define MIDIIN_TRANSLATE_NONE  0

midi_t mi_open(int device_index, int translate_to);
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
void mo_send_sysex(midi_t outdev, const uint8_t *msg, int msg_len);

#endif //_MIDI_H_
