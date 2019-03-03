// -*- coding: utf-8 -*-

#include "signal.h"
#include <string.h>

bool verify_signal_pattern(const char *signal_pattern)
{
  return true;
}

bool sigcmp(const char *signal_pattern, const char *signal)
{
  if( signal_pattern[0] == '!' )
    return (! sigcmp(signal_pattern+1, signal));
  
  bool found;
  const char *p=signal_pattern;
  while(1){
    found = false;
    const char *s;
    for(s=signal; *p && *s ; p++,s++){
      char ptn = *p;
      if( ptn == ',' ){
        found = true;
        break;
      }
      else if( ptn == '.' ){
        break;
          }
      else if( ptn != '*' && ptn != *s ){
        break;
          }
    }

    if( found ){
      break;          
    }else if( *p == '.'){
      if( *s == '\0'){
        found = true;
        break;
      }
    }else if( *p == '\0' ){
      found = true;
      break;
    }
    
    const char *p_delim = strchr(p,',');
    if( p_delim )
      p = p_delim + 1;
    else
      break;
  }

  return found;
}

