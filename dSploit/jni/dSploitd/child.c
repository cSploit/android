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
#include "msgqueue.h"
#include "reaper.h"
#include "command.h"
#include "buffer.h"

struct child_list children;

#define MIN(a, b) (a < b ? a : b)

/**
 * @brief create a new unique child
 * @returns a pointer to the new child or NULL on error.
 */
child_node *create_child() {
  static unsigned short uid = 1;
  register child_node *c;
  
  c = malloc(sizeof(child_node));
  if(!c) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    return NULL;
  }
  memset(c, 0, sizeof(child_node));
  c->stdout = c->stdin = -1;
  
  c->id = uid++;
  if(c->id == COMMAND_RECEIVER_ID)
    c->id = uid++;
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
    while((count=read(c->stdout, buff, STDOUT_BUFF_SIZE)) > 0) {
      if(c->handler->raw_output_parser(c, buff, count)) {
        ret = -1;
        break;
      }
    }
  } else if(c->handler->output_parser) { // parse lines
    while((count=read(c->stdout, buff, STDOUT_BUFF_SIZE)) > 0 && !ret) {
      if(append_to_buffer(&(c->output_buff), buff, count)) {
        ret = -1;
      }
      while((line = get_line_from_buffer(&(c->output_buff))) && !ret) {
        m = c->handler->output_parser(line);
        if(m) {
          m->head.id = c->id;
          m->head.seq = c->seq + 1;
          
          if(enqueue_message(&(outcoming_messages), m)) {
            fprintf(stderr, "%s [%s]: cannot enqueue the following message.\n", __func__, c->handler->name);
            dump_message(m);
            free_message(m);
            ret = -1;
          } else {
            c->seq++;
          }
        }
        free(line);
      }
    }
  } else { // send raw bytes
    while((count=read(c->stdout, buff, STDOUT_BUFF_SIZE)) > 0) {
      m = create_message(c->seq + 1, count, c->id);
      if(!m) {
        buff[MIN(count, STDOUT_BUFF_SIZE -1)] = '\0';
        fprintf(stderr, "%s [%s]: cannot send the following output: '%s'\n", __func__, c->handler->name, buff);
        ret = -1;
        break;
      }
      memcpy(m->data, buff, count);
      if(enqueue_message(&(outcoming_messages), m)) {
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
  
  if(c->output_buff.buffer)
    free(c->output_buff.buffer);
  
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
  
  if(waitpid(c->pid, &status, 0) < 0) {
    fprintf(stderr, "%s [%s]: waitpid: %s\n", __func__, c->handler->name, strerror(errno));
  } else if(notify_child_done(c, status)) {
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
  
  pthread_mutex_lock(&(children.control.mutex));
  list_del(&(children.list), (node *)c);
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  if(c->handler->have_stdout)
    close(c->stdout);
  if(c->handler->have_stdin)
    close(c->stdin);
  
  if(wait_res!=c->pid)
    kill(c->pid, 9);
  
  free(c);
  
  return ret;
}

/**
 * @brief kill all forked processes and wait childs to exit gracefully.
 */
void stop_children() {
  child_node *c;
  
  pthread_mutex_lock(&(children.control.mutex));
  
  for(c=(child_node *)children.list.head;c;c=(child_node *) c->next) {
    if(c->handler->have_stdout)
      close(c->stdout);
    kill(c->pid, 9);
  }
  
  while(children.list.head) {
    pthread_cond_wait(&(children.control.cond), &(children.control.mutex));
  }
  
  pthread_mutex_unlock(&(children.control.mutex));
}
