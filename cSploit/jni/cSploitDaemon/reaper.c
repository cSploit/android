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
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "list.h"
#include "control.h"
#include "logger.h"
#include "reaper.h"

struct graveyard graveyard;
pthread_t reaper_tid = 0;

/**
 * @brief the reaper. he will join all threads in the graveyard.
 */
void *reaper(void *arg) {
  dead_node *zombie;
  void *exit_val;
  
  pthread_mutex_lock(&(graveyard.control.mutex));
  
  while(graveyard.control.active || graveyard.list.head) {
  
    while(!(graveyard.list.head) && graveyard.control.active) {
      pthread_cond_wait(&(graveyard.control.cond), &(graveyard.control.mutex));
    }
    
    zombie = (dead_node *) queue_get(&(graveyard.list));
    
    
    if(zombie) {
      pthread_mutex_unlock(&(graveyard.control.mutex));
      
      if(pthread_join(zombie->tid, &exit_val)) {
        print( ERROR, "pthread_join: %s", strerror(errno) );
      }
#ifdef NDEBUG
      else if(exit_val)
        print( WARNING, "thread joined. (exit_val=%p)", exit_val );
#else
      else {
        print( DEBUG, "thread \"%s\" joined. (exit_val=%p, tid=%lu)", zombie->name, exit_val, zombie->tid );
        free(zombie->name);
      }
#endif
      free(zombie);
      
      pthread_mutex_lock(&(graveyard.control.mutex));
    }
  }
  
  return 0;
}

/**
 * @brief start the @reaper thread
 * @returns 0 on success, -1 on error
 */
int start_reaper() {
  
  if(reaper_tid)
    return 0;
  
  if(control_activate(&(graveyard.control))) {
    print( ERROR, "cannot activate the graveyard" );
    return -1;
  }
  
  if(pthread_create(&(reaper_tid), NULL, &reaper, NULL)) {
    print( ERROR, "pthread_create: %s", strerror(errno) );
    reaper_tid=0;
    return -1;
  }
  
  return 0;
}

/**
 * @brief stop the @reaper thread
 * @returns 0 on success, -1 on error
 */
int stop_reaper() {
  
  void *exit_val;
  
  if(!reaper_tid)
    return 0;
  
  if(control_deactivate(&(graveyard.control))) {
    print( ERROR, "cannot deactivate the graveyard" );
    return -1;
  }
  
  if(pthread_join(reaper_tid, &exit_val)) {
    print( ERROR, "pthread_join: %s", strerror(errno) );
    return -1;
  }
#ifdef NDEBUG
  else if(exit_val)
    print( WARNING, "reaper joined. (exit_val=%p)", exit_val );
#else
  else
    print( DEBUG, "reaper joined. (exit_val=%p)", exit_val );
#endif
  
  return 0;
}

/**
 * @brief send a thread to the graveyard.
 * @param tid the thread id
 * @param name the thread name
 * @returns 0 on success, -1 on error
 */
 #ifdef NDEBUG
void send_to_graveyard(pthread_t tid) {
#else
void _send_to_graveyard(pthread_t tid, char *name) {
#endif  
  dead_node *d;
  
  d=malloc(sizeof(dead_node));
  
  if(!d) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return;
  }
  memset(d, 0, sizeof(dead_node));
  d->tid = tid;
#ifndef NDEBUG
  d->name = name;
#endif
  
  pthread_mutex_lock(&(graveyard.control.mutex));
  queue_put(&(graveyard.list), (node *) d);
  pthread_mutex_unlock(&(graveyard.control.mutex));
  
  pthread_cond_broadcast(&(graveyard.control.cond));
}
