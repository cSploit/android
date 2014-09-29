/* LICENSE
 * 
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "child.h"
#include "handler.h"
#include "message.h"
#include "reaper.h"
#include "command.h"
#include "buffer.h"
#include "connection.h"
#include "sequence.h"
#include "handler.h"
#include "io.h"

#define MIN(a, b) (a < b ? a : b)

/**
 * @brief create a new unique child
 * @returns a pointer to the new child or NULL on error.
 */
child_node *create_child(conn_node *conn) {
  register child_node *c;
  
  c = malloc(sizeof(child_node));
  if(!c) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    return NULL;
  }
  
  memset(c, 0, sizeof(child_node));
  
  c->stdout_fd = c->stdin_fd = -1;
  c->id = get_id(&(conn->child_id), &(conn->control.mutex));
  c->conn = conn;
  
  return c;
}

/**
 * @brief read from a child stdout.
 * @param c the child from read to
 * @returns 0 on success, -1 on error
 */
int read_stdout(child_node *c) {
  int count;
  int ret;
  char *buff;
  char *line;
  message *m;
  
  buff = malloc(STDOUT_BUFF_SIZE);
    
  if(!buff) {
    fprintf(stderr, "%s [%s]: malloc: %s\n", __func__, c->handler->name, strerror(errno));
    return -1;
  }
  
  ret = 0;
  
  if(c->handler->raw_output_parser) { // parse raw bytes
    while((count=read(c->stdout_fd, buff, STDOUT_BUFF_SIZE)) > 0) {
      if(c->handler->raw_output_parser(c, buff, count)) {
        ret = -1;
        break;
      }
    }
  } else if(c->handler->output_parser) { // parse lines
    while(!ret && (count=read(c->stdout_fd, buff, STDOUT_BUFF_SIZE)) > 0) {
      if(append_to_buffer(&(c->output_buff), buff, count)) {
        ret = -1;
      }
      while(!ret && (line = get_line_from_buffer(&(c->output_buff)))) {
        m = c->handler->output_parser(line);
        if(m) {
          m->head.id = c->id;
          m->head.seq = c->seq + 1;
          
#ifdef BROADCAST_EVENTS
          if(broadcast_message(m)) {
            fprintf(stderr, "%s [%s]: cannot broadcast message.\n", __func__, c->handler->name);
#else
          if(enqueue_message(&(c->conn->outcoming), m)) {
            fprintf(stderr, "%s [%s]: cannot enqueue the following message.\n", __func__, c->handler->name);
#endif
            dump_message(m);
            free_message(m);
            ret = -1;
            // events not sent are not fatal, just a missed event.
            // while in raw connection a missed message will de-align the output/input.
            // this is why a "break;" is missing here
          } else {
            c->seq++;
          }
        }
        free(line);
      }
    }
  } else { // send raw bytes
    while((count=read(c->stdout_fd, buff, STDOUT_BUFF_SIZE)) > 0) {
      m = create_message(c->seq + 1, count, c->id);
      if(!m) {
        buff[MIN(count, STDOUT_BUFF_SIZE -1)] = '\0';
        fprintf(stderr, "%s [%s]: cannot send the following output: '%s'\n", __func__, c->handler->name, buff);
        ret = -1;
        break;
      }
      memcpy(m->data, buff, count);
      if(enqueue_message(&(c->conn->outcoming), m)) {
        fprintf(stderr, "%s [%s]: cannot enqueue the following message.\n", __func__, c->handler->name);
        dump_message(m);
        free_message(m);
        ret = -1;
        break;
      } else {
        c->seq++;
      }
    }
  }
  
  free(buff);
  
  release_buffer(&(c->output_buff));
  
  if(count<0) {
    fprintf(stderr, "%s [%s]: read: %s\n", __func__, c->handler->name, strerror(errno));
    ret = -1;
  }
  return ret;
}

/**
 * @brief thread routine that handle a forked process
 * @param arg the ::child_node for the forked child
 * @return 0 on success, -1 on error.
 */
void *handle_child(void *arg) {
  child_node *c;
  int status;
  pid_t wait_res;
  void *ret;
  int stdout_error;
#ifndef NDEBUG
  char *name;
#endif
  
  c = (child_node *)arg;
  ret = (void *) -1;
  stdout_error = 0;
  wait_res = 0;
  
#ifndef NDEBUG
  printf("%s: new child started. (name=%s, pid=%d)\n", __func__, c->handler->name, c->pid);
#endif
  
  if(c->handler->have_stdout) {
    if((stdout_error = read_stdout(c))) {
      fprintf(stderr, "%s [%s]: cannot read process stdout\n", __func__, c->handler->name);
    }
  }
  
  if((wait_res = waitpid(c->pid, &status, 0)) != c->pid) {
    fprintf(stderr, "%s [%s]: waitpid: %s\n", __func__, c->handler->name, strerror(errno));
  } else if(on_child_done(c, status)) {
    fprintf(stderr, "%s [%s]: cannot notify child termination.\n", __func__, c->handler->name);
  } else if(!stdout_error)
    ret = 0;
  
  
#ifdef NDEBUG
  send_to_graveyard(c->tid);
#else
  if(asprintf(&name, "%s(name=%s, pid=%d)", __func__, c->handler->name, c->pid)<0) {
    fprintf(stderr, "%s: asprintf: %s\n", __func__, strerror(errno));
    name=NULL;
  }
  _send_to_graveyard(c->tid, name);
#endif
  
  pthread_mutex_lock(&(c->conn->children.control.mutex));
  list_del(&(c->conn->children.list), (node *)c);
  pthread_mutex_unlock(&(c->conn->children.control.mutex));
  
  pthread_cond_broadcast(&(c->conn->children.control.cond));
  
  if(c->handler->have_stdout)
    close(c->stdout_fd);
  if(c->handler->have_stdin)
    close(c->stdin_fd);
  
  if(wait_res!=c->pid)
    kill(c->pid, 9); // SIGKILL, process didn't returned as expected
  
  free(c);
  
  return ret;
}

/**
 * @brief kill all connection's forked processes and wait childs to exit gracefully.
 * @param conn the owner of the children to stop
 */
void stop_children(conn_node *conn) {
  child_node *c;
  
  pthread_mutex_lock(&(conn->children.control.mutex));
  
  for(c=(child_node *)conn->children.list.head;c;c=(child_node *) c->next) {
    if(c->handler->have_stdin)
      close(c->stdin_fd);
    if(c->handler->have_stdout)
      close(c->stdout_fd);
    kill(c->pid, CHILD_STOP_SIGNAL);
  }
  
  while(conn->children.list.head) {
    pthread_cond_wait(&(conn->children.control.cond), &(conn->children.control.mutex));
  }
  
  pthread_mutex_unlock(&(conn->children.control.mutex));
}


/**
 * @brief handle a received message for a child
 * @param conn the connection that sent @p m
 * @param m the received ::message
 * @returns 0 on success, -1 on error.
 */
int on_child_message(conn_node *conn, message *m) {
  child_node *c;
  int write_error, blind_error;
  
  write_error = blind_error = 0;
  
  pthread_mutex_lock(&(conn->children.control.mutex));
  
  for(c=(child_node *) conn->children.list.head;c && c->id != m->head.id; c=(child_node *) c->next);
  
  if(c) {
    if(c->handler->input_parser) {
      c->handler->input_parser(c, m);
    } else if(c->handler->have_stdin) {
      write_error = write_wrapper(c->stdin_fd, m->data, m->head.size);
    } else {
      blind_error = 1;
    }
  }
  
  pthread_mutex_unlock(&(conn->children.control.mutex));
  
  if(!c) {
    fprintf(stderr, "%s: child #%u not found\n", __func__, m->head.id);
  } else if(write_error) {
    fprintf(stderr, "%s: cannot send message to child #%u\n", __func__, m->head.id);
  } else if(blind_error) {
    fprintf(stderr, "%s: received message for blind child #%u\n", __func__, m->head.id);
  } else {
    return 0;
  }
  
  return -1;
}