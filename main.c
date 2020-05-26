// -*- coding: utf-8 -*-

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cmdproc.h"
#include "utils.h"
#include "os.h"
#include "midi.h"

static void receive_keepalive_signal();
static void keepalive_watchdog(void);
static clock_t keepalive_last_clock;

void dispatch_command(const char *cmd, int n_args, char **args)
{
#define EQL(s) (0==strcmp(s+1,cmd+1))

  cmdproc_t proc = NULL;

  switch( cmd[0] ){
  case 'C':
    if( EQL("CALLBACK") )
      proc = cmdprocCALLBACK;
    else if( EQL("CLOSE") )
      proc = cmdprocCLOSE;
    break;
  case 'D':
    if( EQL("DEBUG") )
      proc = cmdprocDEBUG;
    break;
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
    errorf("unknown command: %s",cmd);
  }else if( CMDPROC_SUCCESS != (*proc)(n_args, args) )
    errorf("fatal error: %s",cmd);
}

static int main_loop();

int main(int argc, char **argv)
{
  infof("interMidiator");
  infof("Copyright (c) PGkids Laboratory.") ;
  
  // initialize modules
  system_init_utils(); // utils.c

  os_initialize(); // midi.c
  os_midi_initialize();    // midi.c

  keepalive_last_clock = clock();

  printf("READY\n");
  fflush(stdout);

  os_run_thread(keepalive_watchdog);
  return main_loop();
}


int rd_chr()
{
  while(1){
    int c = getchar();
    if( c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c== ' ' || c == '*' || c == '.' || c == ',' || c == '\n')
      return c;
    else if( c >= 'a' && c <= 'z' )
      return 'A' + (c-'a');
    else if( c == '\r' || c == '\t' )
      return ' ';
    else if(c == 0xFF){
      /* フロントエンドからのKEEPALIVE信号を受信 */
      receive_keepalive_signal();
    }
    else if( c == EOF ){
      exit(-1);
    }
    else{
      errorf("err[%c]\n",c);
      fflush(stderr);
      return -1;
    }
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

#define CMDLINE_MAX_LEN 32767

static int main_loop()
{
  while(1){
    static char cmdline[CMDLINE_MAX_LEN+1];
    rd_line(cmdline, CMDLINE_MAX_LEN);
    //printf("[%s]\n",cmdline);
    parse_cmd(cmdline);
    //for(int i=0; i<n_cmd_elem; i++)
    //  printf("{%s}\n", cmd_elems[i]);
    dispatch_command(cmd_elems[0], n_cmd_elem-1, &cmd_elems[1]);
  }
}



static void receive_keepalive_signal()
{
  os_exclusive_lock();
  keepalive_last_clock = clock();
  os_exclusive_unlock();
}

static void keepalive_watchdog(void)
{
  for(;;){
    os_msec_sleep(2500);

    clock_t cur_clock = clock();
    clock_t last_clock;

    os_exclusive_lock();
    last_clock = keepalive_last_clock;
    os_exclusive_unlock();
    
    if( (cur_clock - last_clock)/CLOCKS_PER_SEC > 6.0)
      os_abort_with_alert("FRONT-END MAY BE CRUSHED.");
  }
}
