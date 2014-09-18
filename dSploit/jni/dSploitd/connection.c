/* LICENSE
 * 
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "message.h"
#include "connection.h"
#include "receiver.h"
#include "reaper.h"

struct connection_list connections;

/**
 * @brief close all connections from ::cl
 */
void close_connections() {
  register conn_node *c;
  
  pthread_mutex_lock(&(connections.control.mutex));
  
  for(c=(conn_node *) connections.list.head;c;c=(conn_node *) c->next) {
    close(c->fd); // associated thread will stop soon
  }
  
  // wait that all connection threads ends
  while(connections.list.head) {
    pthread_cond_wait(&(connections.control.cond), &(connections.control.mutex));
  }
  
  pthread_mutex_unlock(&(connections.control.mutex));
}

/**
 * @brief handle a connection
 */
void *connection_handler(void *arg) {
  
  struct message *msg;
  conn_node *c;
#ifndef NDEBUG
  char *name;
#endif
  
  c=(conn_node *) arg;
  
#ifndef NDEBUG
  printf("%s: connection opened (fd=%d)\n", __func__, c->fd);
#endif
  
  while(1) {
    if(!(msg = read_message(c->fd))) {
      // connection closed or broken
      break;
    }
    
    if(enqueue_message(&(incoming_messages), msg)) {
      fprintf(stderr, "%s: cannot enqueue received message\n", __func__);
      dump_message(msg);
      free_message(msg);
    }
  }
  
  pthread_mutex_lock(&(connections.control.mutex));
  list_del(&(connections.list), (node *)c);
  pthread_mutex_unlock(&(connections.control.mutex));
  
  pthread_cond_broadcast(&(connections.control.cond));
  
  close(c->fd);

#ifndef NDEBUG
  printf("%s: connection closed (fd=%d)\n", __func__, c->fd);
#endif
  
  // add me to the death list
#ifdef NDEBUG
  send_to_graveyard(c->tid);
#else
  if(asprintf(&name, "%s(fd=%d)", __func__, c->fd)<0) {
    fprintf(stderr, "%s: asprintf: %s\n", __func__, strerror(errno));
    name = NULL;
  }
  _send_to_graveyard(c->tid, name);
#endif
  
  free(c);
  
  return 0;
}

/**
 * @brief start a @connection_handler for a new client.
 * @param fd file descriptor of the new client
 * @returns 0 on success, -1 on error.
 */
int serve_new_client(int fd) {
  conn_node *c;
  
  pthread_mutex_lock(&(connections.control.mutex));
  
  if(!connections.control.active) {
    pthread_mutex_unlock(&(connections.control.mutex));
    fprintf(stderr, "%s: connections disabled\n", __func__);
    return -1;
  }
  
  c = malloc(sizeof(conn_node));
  
  if(!c) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    pthread_mutex_unlock(&(connections.control.mutex));
    return -1;
  }

  memset(c, 0, sizeof(conn_node));

  c->fd = fd;
  
  if(pthread_create(&(c->tid), NULL, &connection_handler, (void *)c)) {
    pthread_mutex_unlock(&(connections.control.mutex));
    fprintf(stderr, "%s: pthread_create: %s\n", __func__, strerror(errno));
    return -1;
  }
  list_add(&(connections.list), (node *) c);
  
  pthread_mutex_unlock(&(connections.control.mutex));

  pthread_cond_broadcast(&(connections.control.cond));
  
  return 0;
}
