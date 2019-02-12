// -*- coding: utf-8 -*-

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdproc.h"
#include "utils.h"
#include "midi.h"

void dispatch_command(const char *cmd, int n_args, char **args)
{
#define EQL(s) (0==strcmp(s+1,cmd+1))

  cmdproc_t proc;

  switch( cmd[0] ){
  case 'C':
    if( EQL("CALLBACK") )
      proc = cmdprocCALLBACK;
    else if( EQL("CLOSE") )
      proc = cmdprocCLOSE;
  case 'E':
    if( EQL("ECHO") )
      proc = cmdprocECHO;
    break;
  case 'L':
    if( EQL("LIST") )
      proc = cmdprocLIST;
    else if( EQL("LISTEN") )
      proc = cmdprocLISTEN;
    break;
  case 'O':
    if( EQL("OPEN") )
      proc = cmdprocOPEN;
    break;
  case 'Q':
    if( EQL("QUIT") )
      proc = cmdprocQUIT;
    break;
  case 'R':
    if( EQL("RESET") )
      proc = cmdprocRESET;
    break;
  case 'S':
    if( EQL("SEND") )
      proc = cmdprocSEND;
    else if( EQL("STOP") )
      proc = cmdprocSTOP;
    break;
  default:
      proc = NULL;
  }
      
  if( NULL == proc ){
    errorf("unknown: %s",cmd);
  }else
    (*proc)(n_args, args);
}

static int main_loop();

int main(int argc, char **argv)
{
  fprintf(stderr,
          "interMidiator version 1.0\n"
          "Copyright (c) PGkids Laboratory\n") ;
  fflush(stderr);
  
  // initialize modules
  system_init_utils(); // utils.c
  system_init_midi(); // midi.c

  printf("READY\n");
  fflush(stdout);
  
  return main_loop();
}


int rd_chr()
{
  int c = getchar();
  if( c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c== ' ' || c == '*' || c == '.' || c == ',' || c == '\n')
    return c;
  else if( c >= 'a' && c <= 'z' )
    return 'A' + (c-'a');
  else if( c == '\r' || c == '\t' )
    return ' ';
  else if( c == EOF ){
    exit(-1);
  }
  else{
    errorf("err[%c]\n",c);
    fflush(stderr);
    return -1;
  }
}

int rd_line(char *buf, size_t max)
{
  int c;
  while( ' ' == (c = rd_chr()) )
    ;

  if( c < 0 )
    return c;
  
  int cnt = 1;
  buf[0] = (char)c;
  while( cnt < max ){
    c = rd_chr();
    if( c == '\n' ){
      buf[cnt] = '\0';
      return cnt;
    }
    buf[cnt++] = (char)c;
  }

  return -1;
}

  
//
#define CMD_ELEM_MAX 6
char* cmd_elems[CMD_ELEM_MAX];
int n_cmd_elem = 0;
static void parse_cmd(char *cmdline)
{
  int cnt = 0;
  while( *cmdline ){
    char ch = *cmdline;
    if( ch == ' ' )
      *cmdline++ = '\0';
    else{
      if( cnt+1 > CMD_ELEM_MAX ){
        errorf("too many args for %s",cmd_elems[0]);
        break;
      }
      cmd_elems[cnt++] = cmdline;
      while( *cmdline && *cmdline != ' ' )
        cmdline++;
    }
  }

  n_cmd_elem = cnt;
}

static int main_loop()
{
  while(1){
    char cmdline[256];
    rd_line(cmdline, sizeof(cmdline));
    //printf("[%s]\n",cmdline);
    parse_cmd(cmdline);
    //for(int i=0; i<n_cmd_elem; i++)
    //  printf("{%s}\n", cmd_elems[i]);
    dispatch_command(cmd_elems[0], n_cmd_elem-1, &cmd_elems[1]);
  }
}
