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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "logger.h"
#include "message.h"
#include "msgqueue.h"


/**
 * @brief add @p msg to @p queue
 * @param queue the queue to deal with
 * @param msg the ::message to add
 * @returns 0 on success, -1 on error.
 */
int enqueue_message(struct msgqueue *queue, message *msg) {
  msg_node *m;
  
  assert(msg!=NULL);
  
  pthread_mutex_lock(&(queue->control.mutex));
  if(queue->control.active) {
    if(!(m=malloc(sizeof(msg_node)))) {
      pthread_mutex_unlock(&(queue->control.mutex));
      print( ERROR, "malloc: %s", strerror(errno) );
      return -1;
    }
    m->next=NULL;
    m->msg=msg;
    
    queue_put(&(queue->list), (node *)m);
    pthread_mutex_unlock(&(queue->control.mutex));
    
    pthread_cond_broadcast(&(queue->control.cond));
    
    return 0;
  } else {
    pthread_mutex_unlock(&(queue->control.mutex));
    
    print( ERROR, "message queue disabled" );
    
    return -1;
  }
}