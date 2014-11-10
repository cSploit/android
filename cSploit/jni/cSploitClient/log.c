/* LICENSE
 * 
 */

#include <android/log.h>
#include <pthread.h>

#include "log.h"
#include "logger.h"

void android_logger(int level, char *fmt, ...) {
  va_list argptr;
  int prio;
  
  switch(level) {
    case DEBUG:
      prio = ANDROID_LOG_DEBUG;
      break;
    case INFO:
      prio = ANDROID_LOG_INFO;
      break;
    case WARNING:
      prio = ANDROID_LOG_WARN;
      break;
    case ERROR:
      prio = ANDROID_LOG_ERROR;
      break;
    case FATAL:
      prio = ANDROID_LOG_FATAL;
      break;
    default:
      prio = ANDROID_LOG_VERBOSE;
      break;
  }
  
  va_start(argptr,fmt);
  
  pthread_mutex_lock(&print_lock);
  
  __android_log_vprint(prio, LOG_TAG, fmt, argptr);
  
  pthread_mutex_unlock(&print_lock);
}