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