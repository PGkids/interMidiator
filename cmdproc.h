// -*- coding: utf-8 -*-

#ifndef _CMDPROC_H_
#define _CMDPROC_H_

typedef int (*cmdproc_t)(int n_args, char** args);

#define CMDPROC_SUCCESS 0
#define CMDPROC_FATAL (-1)

#define CPDECL(name) int cmdproc##name(int, char**);

CPDECL(LIST)
CPDECL(OPEN)
CPDECL(QUIT)
CPDECL(ECHO)
CPDECL(CALLBACK)

CPDECL(CLOSE)
CPDECL(LISTEN)
CPDECL(STOP)
CPDECL(SEND)
CPDECL(RESET)



#endif

