/* LICENSE
 * 
 */

#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>

#include "logger.h"

void default_logger(int, char *, ...);

void (*_print)(int, char *, ...) = default_logger;

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

void default_logger(int level, char *fmt, ...) {
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
  
  va_end(argptr);
  
  pthread_mutex_unlock(&print_lock);
}