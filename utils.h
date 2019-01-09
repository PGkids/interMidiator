// -*- coding: utf-8 -*-

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdbool.h>

void system_init_utils();

char* duplicate_string(const char *s);
unsigned hexes_to_unsigned(const char *hexes, bool *error_r);

void errorf(const char *fmt, ...);
void warnf(const char *fmt, ...);

void global_lock(), global_unlock();

#endif //_UTILS_H_
