/* LICENSE
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "msgqueue.h"
#include "message.h"
#include "connection.h"

/// output message queue
struct msgqueue outcoming_messages;
/// id of the sender thread
pthread_t sender_tid = 0;

/**
 * @brief the thread that will send outcoming messages
 */
void *sender(void *arg) {
  
  msg_node *m;
  conn_node *c;
  
  pthread_mutex_lock(&(outcoming_messages.control.mutex));
  
  while(outcoming_messages.control.active || outcoming_messages.list.head) {
    
    while(!(outcoming_messages.list.head) && outcoming_messages.control.active) {
      pthread_cond_wait(&(outcoming_messages.control.cond), &(outcoming_messages.control.mutex));
    }
    
    m = (msg_node *) queue_get(&(outcoming_messages.list));
    
    if(!m)
      continue;
    
    pthread_mutex_unlock(&(outcoming_messages.control.mutex));
    
    pthread_mutex_lock(&(connections.control.mutex));
    
    while(!(connections.list.head) && connections.control.active) {
      pthread_cond_wait(&(connections.control.cond), &(connections.control.mutex));
    }
    
    for(c=(conn_node *)connections.list.head;c;c=(conn_node *) c->next) {
      if(send_message(c->fd, m->msg)) {
#ifdef NDEBUG
        fprintf(stderr, "%s: cannot send message\n", __func__);
#else
        fprintf(stderr, "%s: cannot send message (fd=%d)\n", __func__, c->fd);
#endif
        dump_message(m->msg);
      }
    }
    
    free_message(m->msg);
    free(m);
    
    pthread_mutex_unlock(&(connections.control.mutex));
    
    pthread_mutex_lock(&(outcoming_messages.control.mutex));
  }
  
  pthread_mutex_unlock(&(outcoming_messages.control.mutex));
  
  return 0;
}

/**
 * @brief start the @sender thread
 * @returns 0 on success, -1 on error
 */
int start_sender() {
  
  if(sender_tid)
    return 0;
  
  if(control_activate(&(outcoming_messages.control))) {
    fprintf(stderr, "%s: cannot activate outcoming messages queue\n", __func__);
    return -1;
  }
  
  if(pthread_create(&(sender_tid), NULL, &sender, NULL)) {
    fprintf(stderr, "%s: pthread_create: %s\n", __func__, strerror(errno));
    sender_tid=0;
    return -1;
  }
  
  return 0;
}

/**
 * @brief stop the @sender thread
 * @returns 0 on success, -1 on error
 */
int stop_sender() {
  
  if(!sender_tid)
    return 0;
  
  if(control_deactivate(&(outcoming_messages.control))) {
    fprintf(stderr, "%s: cannot deactivate outcoming messages queue\n", __func__);
    return -1;
  }
  
  if(pthread_join(sender_tid, NULL)) {
    fprintf(stderr, "%s: pthread_join: %s\n", __func__, strerror(errno));
    return -1;
  }
  
  return 0;
}