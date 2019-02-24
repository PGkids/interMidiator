// -*- coding: utf-8 -*-

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include "utils.h"
#include <stdbool.h>
#include <ctype.h>

bool __intermidiator_debug__ = 0; // GLOBAL

void set_debug(bool enable)
{
  __intermidiator_debug__ = enable;
}
  
static CRITICAL_SECTION global_cs;
static bool global_cs_not_initialized = true;

void system_init_utils()
{
  InitializeCriticalSection(&global_cs);
}

void global_lock()
{
  EnterCriticalSection(&global_cs);
}

void global_unlock()
{
  LeaveCriticalSection(&global_cs);
}

void errorf(const char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "*(ERROR) ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

void warnf(const char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "+(WARNING: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

void infof(const char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "- ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

char* duplicate_string(const char *s)
{
  size_t n = strlen(s);
  char *duped = malloc(n+1);
  memcpy(duped, s, n+1);
  return duped;
}

unsigned hexes_to_unsigned(const char *hexes, bool *error_r)
{
  if( '\0' == hexes[0] ){
    *error_r = true;
    return 0;
  }
  
  unsigned result = 0;
  for(const char *p=hexes; *p; p++){
    char c = *p;
    if( isspace(c) ){
      if( hexes != p )
        break;
      else{
        *error_r = true;
        return 0;
      }
    }
    else if( c >= '0' && c <= '9' )
      result = result*16 + (c-'0');
    else if( c >= 'A' && c <= 'F' )
      result = result*16 + (c-'A'+10);
    else if( c >= 'a' && c <= 'f' )
      result = result*16 + (c-'a'+10);
    else{
      *error_r = true;
      return 0;
    }
  }

  *error_r = false;
  return result;
}
