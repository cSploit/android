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
#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "logger.h"
#include "list.h"
#include "control.h"
#include "message.h"
#include "connection.h"
#include "reaper.h"
#include "child.h"
#include "control_messages.h"
#include "controller.h"

struct connection_list connections;

/**
 * @brief stop a connection
 * @param c the connection to stop
 */
void stop_connection(conn_node *c) {
  stop_children(c); // stop children and wait them to gracefully exits.
  close(c->fd); // stop the reader
  control_deactivate(&(c->control));
  control_deactivate(&(c->incoming.control));   // stop the worker
  control_deactivate(&(c->outcoming.control));  // stop the writer
}

/**
 * @brief free resources used by a connection
 * @param c the connection to free
 * 
 * all connection threads and owned childs must be ended before this call,
 * otherwise they will access to free'd memory.
 */
void free_connection(conn_node *c) {
  control_destroy(&(c->control));
  control_destroy(&(c->incoming.control));
  control_destroy(&(c->outcoming.control));
  free(c);
}

/**
 * @brief close all connections from ::connections
 */
void close_connections() {
  register conn_node *c;
  
  pthread_mutex_lock(&(connections.control.mutex));
  
  // equivalent to control_deactivate
  connections.control.active = 0;
  
  for(c=(conn_node *) connections.list.head;c;c=(conn_node *) c->next) {
    stop_connection(c);
  }
  
  // wait that all connection threads ends
  while(connections.list.head) {
    pthread_cond_wait(&(connections.control.cond), &(connections.control.mutex));
  }
  
  pthread_mutex_unlock(&(connections.control.mutex));
}

void *connection_writer(void *arg) {
  msg_node *mn;
  message *msg;
  conn_node *c;
  
  c=(conn_node *) arg;
  
#ifndef NDEBUG
  print( DEBUG, "started (fd=%d, tid=%lu)", c->fd, pthread_self() );
#endif
  
  pthread_mutex_lock(&(c->outcoming.control.mutex));
  
  while(c->outcoming.control.active || c->outcoming.list.head) {
    
    while(!(c->outcoming.list.head) && c->outcoming.control.active) {
      pthread_cond_wait(&(c->outcoming.control.cond), &(c->outcoming.control.mutex));
    }
    
    mn = (msg_node *) queue_get(&(c->outcoming.list));
    
    if(!mn)
      continue;
    
    pthread_mutex_unlock(&(c->outcoming.control.mutex));
    
    pthread_mutex_lock(&(c->control.mutex));
    
    while(c->freeze) {
      pthread_cond_wait(&(c->control.cond), &(c->control.mutex));
    }
    
    pthread_mutex_unlock(&(c->control.mutex));
    
    msg = mn->msg;
    
    if(send_message(c->fd, msg)) {
      print( ERROR, "cannot send the following message" );
      dump_message(msg);
    }
    
    free_message(msg);
    free(mn);
    
    pthread_mutex_lock(&(c->outcoming.control.mutex));
  }
  
  pthread_mutex_unlock(&(c->outcoming.control.mutex));
  
  pthread_mutex_lock(&(c->control.mutex));
  c->writer_done=1;
  pthread_mutex_unlock(&(c->control.mutex));
  
  pthread_cond_broadcast(&(c->control.cond));
  
  send_me_to_graveyard();
  
  return 0;
}

void *connection_worker(void *arg) {
  msg_node *mn;
  message *msg;
  conn_node *c;
  
  c=(conn_node *) arg;
  
#ifndef NDEBUG
  print( DEBUG, "started (fd=%d, tid=%lu)", c->fd, pthread_self() );
#endif
  
  pthread_mutex_lock(&(c->incoming.control.mutex));
  
  while(c->incoming.control.active || c->incoming.list.head) {
    
    while(!(c->incoming.list.head) && c->incoming.control.active) {
      pthread_cond_wait(&(c->incoming.control.cond), &(c->incoming.control.mutex));
    }
    
    mn = (msg_node *) queue_get(&(c->incoming.list));
    
    if(!mn)
      continue;
    
    pthread_mutex_unlock(&(c->incoming.control.mutex));
    
    msg = mn->msg;
    
    if(IS_CTRL(msg)) {
      if(on_control_request(c, msg)) {
        print( ERROR, "cannot handle the following control message" );
        dump_message(msg);
      }
    } else {
      if(on_child_message(c, msg)) {
        print( ERROR, "cannot handle the following message" );
        dump_message(msg);
      }
    }
    
    free_message(msg);
    free(mn);
    
    pthread_mutex_lock(&(c->incoming.control.mutex));
  }
  
  pthread_mutex_unlock(&(c->incoming.control.mutex));
  
  pthread_mutex_lock(&(c->control.mutex));
  c->worker_done=1;
  pthread_mutex_unlock(&(c->control.mutex));
  
  pthread_cond_broadcast(&(c->control.cond));
  
  send_me_to_graveyard();
  
  return 0;
}

/**
 * @brief read from a connection
 */
void *connection_reader(void *arg) {
  
  struct message *msg;
  conn_node *c;
  
  c=(conn_node *) arg;
  
#ifndef NDEBUG
  print( DEBUG, "started (fd=%d, tid=%lu)", c->fd, pthread_self() );
#endif
  
  while((msg = read_message(c->fd))) {
    pthread_mutex_lock(&(c->control.mutex));
    
    while(c->freeze) {
      pthread_cond_wait(&(c->control.cond), &(c->control.mutex));
    }
    
    pthread_mutex_unlock(&(c->control.mutex));
    
    if(enqueue_message(&(c->incoming), msg)) {
      print( ERROR, "cannot enqueue received message" );
      dump_message(msg);
      free_message(msg);
    }
  }
  
  stop_connection(c);
  
  pthread_mutex_lock(&(c->control.mutex));
  while(!c->writer_done || !c->worker_done) {
    pthread_cond_wait(&(c->control.cond), &(c->control.mutex));
  }
  pthread_mutex_unlock(&(c->control.mutex));
  
  pthread_mutex_lock(&(connections.control.mutex));
  list_del(&(connections.list), (node *)c);
  pthread_mutex_unlock(&(connections.control.mutex));
  
  pthread_cond_broadcast(&(connections.control.cond));
  
  close(c->fd);

#ifndef NDEBUG
  print( DEBUG, "connection closed (fd=%d)", c->fd );
#endif
  
  // add me to the death list
  send_me_to_graveyard();
  
  free_connection(c);
  
  return 0;
}

/**
 * @brief start a @connection_handler for a new client.
 * @param fd file descriptor of the new client
 * @returns 0 on success, -1 on error.
 */
int serve_new_client(int fd) {
  conn_node *c;
  pthread_t dummy;
  int writer_created, worker_created;
  
  
  pthread_mutex_lock(&(connections.control.mutex));
  writer_created = connections.control.active; // OPTIMIZATION
  pthread_mutex_unlock(&(connections.control.mutex));
  
  if(!writer_created) {
    print( ERROR, "connections disabled" );
    return -1;
  }
  
  writer_created = worker_created = 0;
  
  c = malloc(sizeof(conn_node));
  
  if(!c) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return -1;
  }

  memset(c, 0, sizeof(conn_node));
  
  if(control_init(&(c->control)) || control_init(&(c->incoming.control)) ||
    control_init(&(c->outcoming.control))) {
    print( ERROR, "cannot init cotnrols" );
    free_connection(c);
    return -1;
  }

  c->fd = fd;
  
  pthread_mutex_lock(&(connections.control.mutex));
  
  if( (writer_created = pthread_create(&dummy, NULL, &connection_writer, (void *)c)) ||
      (worker_created = pthread_create(&dummy, NULL, &connection_worker, (void *)c)) ||
      pthread_create(&dummy, NULL, &connection_reader, (void *)c)) {
    pthread_mutex_unlock(&(connections.control.mutex));
    print( ERROR, "pthread_create: %s", strerror(errno) );
    goto create_error;
  }
  
  list_add(&(connections.list), (node *) c);
  
  pthread_mutex_unlock(&(connections.control.mutex));

  pthread_cond_broadcast(&(connections.control.cond));
  
  return 0;
  
  create_error:
  
  if(writer_created || worker_created) {
    stop_connection(c);
    pthread_mutex_lock(&(c->control.mutex));
    while((writer_created && !c->writer_done) || (worker_created && !c->worker_done)) {
      pthread_cond_wait(&(c->control.cond), &(c->control.mutex));
    }
    pthread_mutex_unlock(&(c->control.mutex));
  }
  
  free_connection(c);
  
  return -1;
}