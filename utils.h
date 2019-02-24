// -*- coding: utf-8 -*-

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdbool.h>

void system_init_utils();

char* duplicate_string(const char *s);
unsigned hexes_to_unsigned(const char *hexes, bool *error_r);

/* これらは自動的に改行とフラッシュが行われる
   単一行を出力することを目的としているため、決して中途改行しないこと */
void errorf(const char *fmt, ...);
void warnf(const char *fmt, ...);
void infof(const char *fmt, ...);

extern bool __intermidiator_debug__;
#define DEBUGF(...) if( __intermidiator_debug__ ){ infof("[DEBUG] " __VA_ARGS__); }

void set_debug(bool enable);

void global_lock(), global_unlock();

#endif //_UTILS_H_
