// -*- coding: utf-8 -*-

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <stdbool.h>

/* typedef struct */
/* { */
/*   char *filter_pattern; */
/*   char * */
/* } sigrule_t; */


bool sigcmp(const char *signal_pattern, const char *signal);
bool verify_signal_pattern(const char *signal_pattern);

#endif

