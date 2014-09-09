/* LICENSE
 * 
 * 
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "connection.h"
#include "message.h"
#include "dSploitd.h"
#include "msgqueue.h"
#include "command.h"
#include "child.h"
#include "handler.h"
#include "common.h"

struct msgqueue incoming_messages;

struct msgqueue outcoming_messages;
pthread_t sender_tid = 0;
pthread_t dispatcher_tid = 0;

/**
 * @brief add @p msg to @p queue
 * @param queue the queue to deal with
 * @param msg the @message to add
 * @returns 0 on success, -1 on error.
 */
int enqueue_message(struct msgqueue *queue, message *msg) {
  msg_node *m;
  
  assert(msg!=NULL);
  
  pthread_mutex_lock(&(queue->control.mutex));
  if(queue->control.active) {
    if(!(m=malloc(sizeof(msg_node)))) {
      pthread_mutex_unlock(&(queue->control.mutex));
      fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
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
    
    return -1;
  }
}

/*
 * @brief check if a message is pending on output queue
 * @param msg message to search for
 * @returns 1 if message is enqueued, 0 if not.
 */
/*
int message_enqueued(message *msg) {
  msg_node *m;
  
  pthread_mutex_lock(&(outcoming_messages.control.mutex));
  
  for(m=(msg_node *) outcoming_messages.list.head;m && m->msg != msg;m=(msg_node *) m->next);
  
  pthread_mutex_unlock(&(outcoming_messages.control.mutex));
  
  if(m)
    return 1;
  return 0;
  
} */

/**
 * @brief the thread that will dispatch incoming maessages
 * 
 */
void *dispatcher(void *arg) {
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
    
    pthread_mutex_lock(&(incoming_messages.control.mutex));
  }
  
  pthread_mutex_unlock(&(incoming_messages.control.mutex));
  
  return 0;
}

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

#ifndef NDEBUG
    dump_message(m->msg);
#endif
    
    free_message(m->msg);
    free(m);
    
    pthread_mutex_unlock(&(connections.control.mutex));
    
    pthread_mutex_lock(&(outcoming_messages.control.mutex));
  }
  
  pthread_mutex_unlock(&(outcoming_messages.control.mutex));
  
  return 0;
}

/**
 * @brief start the @dispatcher thread
 * @returns 0 on success, -1 on error.
 */
int start_dispatcher() {
  
  if(dispatcher_tid)
    return 0;
  
  if(control_activate(&(incoming_messages.control))) {
    fprintf(stderr, "%s: cannot activate incoming messages queue\n", __func__);
    return -1;
  }
  
  if(pthread_create(&(dispatcher_tid), NULL, &dispatcher, NULL)) {
    fprintf(stderr, "%s: pthread_create: %s\n", __func__, strerror(errno));
    dispatcher_tid=0;
    return -1;
  }
  
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
 * @brief stop the @dispatcher thread
 * @returns 0 on success, -1 on error
 */
int stop_dispatcher() {
  
  if(!dispatcher_tid)
    return 0;
  
  if(control_deactivate(&(incoming_messages.control))) {
    fprintf(stderr, "%s: cannot deactivate incoming messages queue\n", __func__);
    return -1;
  }
  
  if(pthread_join(dispatcher_tid, NULL)) {
    fprintf(stderr, "%s: pthread_join: %s\n", __func__, strerror(errno));
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
