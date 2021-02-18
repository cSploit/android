/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>

#include "logger.h"

void (*_print)(int, char *, ...) = std_logger;

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

void std_logger(int level, char *fmt, ...) {
  va_list argptr;
  FILE *stream;
  
  pthread_mutex_lock(&print_lock);
  
  va_start(argptr,fmt);
  
  switch(level) {
    case DEBUG:
      stream = stdout;
      fprintf(stdout, "[DEBUG  ] ");
      break;
    case INFO:
      stream = stdout;
      fprintf(stdout, "[INFO   ] ");
      break;
    case WARNING:
      stream = stderr;
      fprintf(stderr, "[WARNING] ");
      break;
    case ERROR:
      stream = stderr;
      fprintf(stderr, "[ERROR  ] ");
      break;
    case FATAL:
      stream = stderr;
      fprintf(stderr, "[FATAL  ] ");
      break;
    default:
      stream = stderr;
      fprintf(stderr, "[UNKNOWN] ");
      break;
  }
  
  vfprintf(stream, fmt, argptr);
  fprintf(stream, "\n");
  fflush(stream);
  
  va_end(argptr);
  
  pthread_mutex_unlock(&print_lock);
}