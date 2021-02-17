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
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "control.h"
#include "logger.h"

/* part of this code has been taken from:
 * Gentoo Linux Documentation - POSIX threads explained
 */

/**
 * @brief initialize a ::data_control
 * @param mycontrol the ::data_control to initilize
 * @returns 0 on success, -1 on error
 */
int control_init(data_control *mycontrol) {
  if (pthread_mutex_init(&(mycontrol->mutex),NULL)) {
    print( ERROR, "pthread_mutex_init: %s", strerror(errno) );
    return -1;
  }
  if (pthread_cond_init(&(mycontrol->cond),NULL)) {
    print( ERROR, "pthread_cond_init: %s", strerror(errno) );
    return -1;
  }
  mycontrol->active=1;
  return 0;
}

/**
 * @brief destroy a ::data_control
 * @param mycontrol the ::data_control to destroy
 * @returns 0 on success, -1 on error
 */
int control_destroy(data_control *mycontrol) {
  if (pthread_cond_destroy(&(mycontrol->cond))) {
    print( ERROR, "pthread_cond_destroy: %s", strerror(errno) );
    return -1;
  }
  if (pthread_mutex_destroy(&(mycontrol->mutex))) {
    print( ERROR, "pthread_mutex_destroy: %s", strerror(errno) );
    return -1;
  }
  mycontrol->active=0;
  return 0;
}

/**
 * @brief set ::data_control.active to 1
 * @param mycontrol the ::data_control to activate
 * @returns 0 on success, -1 on error
 */
int control_activate(data_control *mycontrol) {
  if (pthread_mutex_lock(&(mycontrol->mutex))) {
    print( ERROR, "pthread_mutex_lock: %s", strerror(errno) );
    return -1;
  }
  mycontrol->active=1;
  pthread_mutex_unlock(&(mycontrol->mutex));
  pthread_cond_broadcast(&(mycontrol->cond));
  return 0;
}

/**
 * @brief set ::data_control.active to 0
 * @param mycontrol the ::data_control to deactivate
 * @returns 0 on success, -1 on error.
 */
int control_deactivate(data_control *mycontrol) {
  if (pthread_mutex_lock(&(mycontrol->mutex))) {
    print( ERROR, "pthread_mutex_lock: %s", strerror(errno) );
    return -1;
  }
  mycontrol->active=0;
  pthread_mutex_unlock(&(mycontrol->mutex));
  pthread_cond_broadcast(&(mycontrol->cond));
  return 0;
}