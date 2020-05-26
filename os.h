// -*- coding: utf-8 -*-

#ifndef _OS_H_
#define _OS_H_

void os_initialize();
void os_exclusive_lock(), os_exclusive_unlock();
void os_run_thread( void (*proc)(void) );
void os_msec_sleep(int msec);
void os_abort_with_alert(const char *alert_msg);

#endif //_OS_H_
