/* LICENSE
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "msgqueue.h"
#include "command.h"
#include "child.h"
#include "handler.h"
#include "message.h"
#include "io.h"

/// input message queue
struct msgqueue incoming_messages;

/// id of the receiver thread.
pthread_t receiver_tid = 0;


/**
 * @brief the thread that will dispatch incoming maessages
 * 
 */
void *receiver(void *arg) {
  msg_node *m;
  child_node *c;
  
  pthread_mutex_lock(&(incoming_messages.control.mutex));
  
  while(incoming_messages.control.active || incoming_messages.list.head) {
    
    while(!(incoming_messages.list.head) && incoming_messages.control.active) {
      pthread_cond_wait(&(incoming_messages.control.cond), &(incoming_messages.control.mutex));
    }
    
    m = (msg_node *) queue_get(&(incoming_messages.list));
    
    if(!m)
      continue;
    
    pthread_mutex_unlock(&(incoming_messages.control.mutex));
    
    if(IS_COMMAND(m->msg)) {
      handle_command(m->msg);
    } else {
      pthread_mutex_lock(&(children.control.mutex));
      
      for(c=(child_node *) children.list.head;c && c->id != m->msg->head.id;c=(child_node *) c->next);
      
      if(c) {
        if(c->handler->input_parser) {
          c->handler->input_parser(c, m->msg);
        } else if(c->handler->have_stdin) {
          if(write_wrapper(c->stdin, m->msg->data, m->msg->head.size)) {
            fprintf(stderr, "%s: cannot send message to child\n", __func__);
            dump_message(m->msg);
          }
        } else {
          fprintf(stderr, "%s: received message for blind child\n", __func__);
          dump_message(m->msg);
        }
      }
        
      pthread_mutex_unlock(&(children.control.mutex));
    }
    
    free_message(m->msg);
    free(m);
    
    pthread_mutex_lock(&(incoming_messages.control.mutex));
  }
  
  pthread_mutex_unlock(&(incoming_messages.control.mutex));
  
  return 0;
}


/**
 * @brief start the @receiver thread
 * @returns 0 on success, -1 on error.
 */
int start_receiver() {
  
  if(receiver_tid)
    return 0;
  
  if(control_activate(&(incoming_messages.control))) {
    fprintf(stderr, "%s: cannot activate incoming messages queue\n", __func__);
    return -1;
  }
  
  if(pthread_create(&(receiver_tid), NULL, &receiver, NULL)) {
    fprintf(stderr, "%s: pthread_create: %s\n", __func__, strerror(errno));
    receiver_tid=0;
    return -1;
  }
  
  return 0;
}



/**
 * @brief stop the @receiver thread
 * @returns 0 on success, -1 on error
 */
int stop_receiver() {
  
  if(!receiver_tid)
    return 0;
  
  if(control_deactivate(&(incoming_messages.control))) {
    fprintf(stderr, "%s: cannot deactivate incoming messages queue\n", __func__);
    return -1;
  }
  
  if(pthread_join(receiver_tid, NULL)) {
    fprintf(stderr, "%s: pthread_join: %s\n", __func__, strerror(errno));
    return -1;
  }
  
  return 0;
}
