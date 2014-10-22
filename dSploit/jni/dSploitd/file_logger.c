/* LICENSE
 * 
 */

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "logger.h"
#include "file_logger.h"

/// log file
FILE *logfile = NULL;

/**
 * @brief use @p filepath as log file
 * @param filepath path to the file to use as logfile
 * @returns 0 on success, -1 on error.
 */
int open_logfile(char *filepath) {
  logfile = fopen(filepath, "w");
  
  if(!logfile) {
    print(ERROR, "fopen: %s", strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * @brief print log messages to ::logfile
 */
void file_logger(int level, char *fmt, ...) {
  va_list argptr;
  
  if(!logfile)
    return;
  
  pthread_mutex_lock(&print_lock);
  
  va_start(argptr, fmt);
  
  switch(level) {
    case DEBUG:
      fprintf(logfile, "[DEBUG  ] ");
      break;
    case INFO:
      fprintf(logfile, "[INFO   ] ");
      break;
    case WARNING:
      fprintf(logfile, "[WARNING] ");
      break;
    case ERROR:
      fprintf(logfile, "[ERROR  ] ");
      break;
    case FATAL:
      fprintf(logfile, "[FATAL  ] ");
      break;
    default:
      fprintf(logfile, "[UNKNOWN] ");
      break;
  }
  
  vfprintf(logfile, fmt, argptr);
  fprintf(logfile, "\n");
  fflush(logfile);
  
  va_end(argptr);
  
  pthread_mutex_unlock(&print_lock);
}