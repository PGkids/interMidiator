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


bool register_global_callback(const char *signal_pattern, const char *callback_id)
{
  add_callback(signal_pattern,
               callback_id,
               os_get_global_callback_table());
  return true;
}

bool register_local_callback(midi_t indev, const char *signal_pattern, const char *callback_id)
{
  add_callback(signal_pattern,
               callback_id,
               os_get_local_callback_table(indev));
}
