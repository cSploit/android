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

#define _GNU_SOURCE // required for pipe2
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stddef.h>

#include "cSploitd.h"
#include "logger.h"
#include "handler.h"
#include "message.h"
#include "child.h"
#include "connection.h"
#include "command.h"
#include "sequence.h"
#include "control_messages.h"
#include "str_array.h"

/**
 * @brief notify the command receiver that a chid printed a line on it's stderr.
 * @param c the child hat generated @p line
 * @param line the line received from the child stderr
 * @returns 0 on success, -1 on error.
 */
int on_cmd_stderr(child_node *c, char *line) {
  message *m;
  struct cmd_stderr_info *stderr_info;
  size_t len;
  uint16_t seq;
  
  seq = get_sequence(&(c->conn->ctrl_seq), &(c->conn->control.mutex));
  
  len = strlen(line) + 1;
  
  m = create_message(seq, sizeof(struct cmd_stderr_info) + len, CTRL_ID);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return -1;
  }
  
  stderr_info = (struct cmd_stderr_info *) m->data;
  stderr_info->cmd_action = CMD_STDERR;
  stderr_info->id = c->id;
  memcpy(stderr_info->line, line, len);
  
  if(enqueue_message(&(c->conn->outcoming), m)) {
    print(ERROR, "cannot enqueue messages");
    dump_message(m);
    free_message(m);
    return -1;
  }
  
  return 0;
}


/**
 * @brief notify the command receiver that a child has exited
 * @param c the terminated child
 * @param status the child status returned by waitpid()
 * @returns 0 on success, -1 on error.
 */
int on_child_done(child_node *c, int status) {
  struct message *msg;
  struct cmd_end_info *end_info;
  struct cmd_died_info *died_info;
  uint16_t seq;
  
  seq = get_sequence(&(c->conn->ctrl_seq), &(c->conn->control.mutex));
  
  if(WIFEXITED(status)) {
    msg = create_message(seq, sizeof(struct cmd_end_info), CTRL_ID);
  } else {
    msg = create_message(seq, sizeof(struct cmd_died_info), CTRL_ID);
  }
  
  if(!msg) {
    print( ERROR, "cannot create messages" );
    return -1;
  }
  
  if(WIFEXITED(status)) {
    end_info = (struct cmd_end_info *) msg->data;
    end_info->cmd_action = CMD_END;
    end_info->id = c->id;
    end_info->exit_value = WEXITSTATUS(status);
  } else {
    died_info = (struct cmd_died_info *) msg->data;
    died_info->cmd_action = CMD_DIED;
    died_info->id = c->id;
    
    if(WIFSIGNALED(status)) {
      died_info->signal = WTERMSIG(status);
    } else {
      print(ERROR, "child exited unexpectly. status=%0*X", (int) sizeof(int), status);
      died_info->signal = 0;
    }
  }
  
  if(enqueue_message(&(c->conn->outcoming), msg)) {
    print( ERROR, "cannot enqueue messages" );
    dump_message(msg);
    free_message(msg);
    return -1;
  }
  
  return 0;
}

/// data extracted from ::cmd_start_info
struct cmd_start_data {
  int argc;         ///< ::argv size
  char free_argv0;  ///< should argv[0] be freed ?
  char **argv;      ///< command arguments
  /**
   * @brief start of environment inside the received message body.
   * 
   * while using another char* array can seems more clear,
   * it is very unefficient.
   * we have to read the environment from the message,
   * store it allocating memory,
   * read it again and call putenv for every argument.
   * 
   * we can just read the environment from the message
   * and directly call putenv. and ge very higher performance.
   */
  char *env_start;
};

/**
 * @brief free all resources used ::cmd_start_data
 * @param data a the ::cmd_start_dta to free
 */
void free_start_data(struct cmd_start_data *data) {
  int i;
  
  if(data->argv) {
    i = (data->free_argv0 ? 0 : 1);
    for(;i < data->argc;i++)
      if(data->argv[i])
        free(data->argv[i]);
    free(data->argv);
  }
  
  free(data);
}

/**
 * @brief read ::cmd_start_data from @p m
 * @param m the message to parse
 * @param skip_argv0 start building from argv[1]
 * @returns a ::cmd_start_data pointer on success, NULL on error.
 */
struct cmd_start_data *extract_start_data(message *m, char skip_argv0) {
  char *arg, *end;
  int i, n;
  size_t size;
  struct cmd_start_data *data;
  struct cmd_start_info *start_info;
  
  start_info = (struct cmd_start_info *) m->data;
  
  n = string_array_size(m, start_info->data);
  
  if(!n && !skip_argv0) {
    print( ERROR, "empty array");
    return NULL;
  }
  
  data = malloc(sizeof(struct cmd_start_data));
  
  if(!data) {
    print( ERROR, "malloc: %s", strerror(errno));
    return NULL;
  }
  
  memset(data, 0, sizeof(struct cmd_start_data));
  
  if(start_info->argc > n) {
    print( ERROR, "argc out of bounds ( argc=%d, n=%d )", start_info->argc, n);
    goto error;
  }
  
  data->argc = start_info->argc;
  
  if(skip_argv0) {
    i=1;
    data->argc++;
  } else {
    i=0;
    data->free_argv0 = 1;
  }
  
  size = (sizeof(char *) * (data->argc+1));
  data->argv = malloc(size);
  
  if(!data->argv) {
    print( ERROR, "malloc: %s", strerror(errno) );
    goto error;
  }
  
  memset(data->argv, 0, size);
  
  end = m->data + m->head.size;
  arg=NULL;
  
  for(;i < data->argc && (arg=string_array_next(m, start_info->data, arg)); i++) {
    data->argv[i] = strndup(arg, (end-arg));
    
    if(!data->argv[i]) {
      print( ERROR, "strndup: %s", strerror(errno));
      goto error;
    }
  }
  
  if(arg) {
    data->env_start = string_array_next(m, start_info->data, arg);
  }
  
  return data;

  error:
  
  free_start_data(data);
  
  return NULL;
}

/**
 * @brief handle a start command request.
 * @param msg the request ::message.
 * @param conn the connection that send the @p message
 * @returns a reply message on success, NULL on error.
 */
message *on_cmd_start(conn_node *conn, message *msg) {
  struct cmd_start_data *data;
  char *cmd;
  handler *h;
  child_node *c;
  int pin[2],pout[2],perr[2],pexec[2];
  int i;
  struct cmd_start_info *request_info;
  struct cmd_started_info *reply_info;
  pid_t pid;
  uint16_t seq;
  
  c = NULL;
  cmd = NULL;
  // ensure to set piped fd to invalid one,
  // or close(2) will close unexpected fd. ( like our stdin if 0 )
  pin[0] = pin[1] = pout[0] = pout[1] = perr[0] = perr[1] = pexec[0] = pexec[1] = -1;
  request_info = (struct cmd_start_info *)msg->data;
  data=NULL;
  pid = 0;
  seq = msg->head.seq;
  
  c = create_child(conn);
  if(!c) {
    print( ERROR, "cannot craete a new child" );
    goto start_fail;
  }
  
  for(h=(handler *)handlers.head;h && h->id != request_info->hid;h=(handler *)h->next);
  c->handler = h;
  
  if(!c->handler) {
    print( ERROR, "handler #%d not found", request_info->hid );
    goto start_fail;
  }
  
  cmd = (char *)c->handler->argv0;
  
  data = extract_start_data(msg, (cmd ? 1 : 0));
  if(!data) {
    print( ERROR, "cannot extract data" );
    goto start_fail;
  }
  
  if(cmd) {
    data->argv[0] = cmd;
  }
  
  if(pipe2(pexec, O_CLOEXEC)) {
    print( ERROR, "exec pipe: %s", strerror(errno) );
    goto start_fail;
  }
  
  if(h->have_stdin && pipe(pin)) {
    print( ERROR, "input pipe: %s", strerror(errno) );
    goto start_fail;
  }
  
  if(h->have_stdout && pipe(pout)) {
    print( ERROR, "output pipe: %s", strerror(errno) );
    goto start_fail;
  }
  
  if(pipe(perr)) {
    print( ERROR, "error pipe: %s", strerror(errno));
    goto start_fail;
  }
  
  
  if((pid = fork()) < 0) {
    print( ERROR, "fork: %s", strerror(errno) );
    goto start_fail;
  } else if(!pid) {
    // child
    close(pexec[0]);
    close(pin[1]);
    close(pout[0]);
    close(perr[0]);
    
    if(pin[0] == -1 || pout[1] == -1) {
      i= open("/dev/null", O_RDWR);
      
      if(pin[0] == -1)
        pin[0] = dup(i);
      if(pout[1] == -1)
        pout[1] = dup(i);
      
      close(i);
    }
    
    if(h->workdir && chdir(h->workdir)) {
      print ( ERROR, "chdir: %s", strerror(errno));
      goto error;
    }
    
    dup2(pin[0], STDIN_FILENO);
    
    close(pin[0]);
    
    dup2(pout[1], STDOUT_FILENO);
    
    close(pout[1]);
    
    dup2(perr[1], STDERR_FILENO);
    
    close(perr[1]);
    
    cmd = data->env_start;
    
    while(cmd) {
      if(putenv(cmd)) {
        print( ERROR, "putenv(\"%s\"): %s", cmd, strerror(errno));
        goto error;
      }
      cmd=string_array_next(msg, request_info->data, cmd);
    }
    
    execvp(data->argv[0], data->argv);
    print( ERROR, "execvp: %s", strerror(errno));
    
    error:
    
    write(pexec[1], "!", 1);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    close(pexec[1]);
    exit(-1);
  } else {
    // parent
    close(pexec[1]);
    close(pin[0]);
    close(pout[1]);
    close(perr[1]);
    
    free_start_data(data);
    data = NULL;
    
    if(read(pexec[0], &i, 1)) {
      waitpid(pid, NULL, 0);
      goto start_fail;
    }
#ifndef NDEBUG
    print( DEBUG, "successfully started a child for '%s' (pid=%d)", h->name, pid );
#endif
    close(pexec[0]);
  }
  
  c->pid = pid;
  
  if(h->have_stdin)
    c->stdin_fd = pin[1];
  
  if(h->have_stdout)
    c->stdout_fd = pout[0];
  
  c->stderr_fd = perr[0];
  
  pthread_mutex_lock(&(conn->children.control.mutex));
  if(pthread_create(&(c->tid), NULL, &handle_child, c)) {
    i=errno;
    pthread_mutex_unlock(&(conn->children.control.mutex));
    print( ERROR, "pthread_craete: %s", strerror(i) );
    goto start_fail;
  }
  list_add(&(conn->children.list), (node *)c);
  pthread_mutex_unlock(&(conn->children.control.mutex));
  
  pthread_cond_broadcast(&(conn->children.control.cond));
  
  msg = create_message(seq, sizeof(struct cmd_started_info), CTRL_ID);
  
  if(!msg) {
    print( ERROR, "cannot create messages" );
    goto start_fail;
  }
  
  reply_info = (struct cmd_started_info *)msg->data;
  reply_info->cmd_action = CMD_STARTED;
  reply_info->id = c->id;
  
  return msg;
  
  start_fail:
  if(c)
    free(c);
  if(data)
    free_start_data(data);
  // no care about EBADF, ensure to close all opened fd
  close(pexec[0]);
  close(pexec[1]);
  close(pin[0]);
  close(pin[1]);
  close(pout[0]);
  close(pout[1]);
  close(perr[0]);
  close(perr[1]);
  
  if(msg)
    dump_message(msg);
  
  if(pid)
    kill(pid, 9);
  
  msg = create_message(seq, sizeof(struct cmd_fail_info), CTRL_ID);
  
  if(!msg) {
    print( ERROR, "cannot create messages" );
    return NULL;
  }
  
  ((struct cmd_fail_info *)msg->data)->cmd_action = CMD_FAIL;
  return msg;
}

/**
 * @brief handle a command signal request.
 * @param msg the request message.
 * @returns NULL, does not respond to this request.
 */
message *on_cmd_signal(conn_node *conn, message *msg) {
  child_node *c;
  pid_t pid;
  struct cmd_signal_info *info;
  
  info = (struct cmd_signal_info *)msg->data;
  
  pid = 0;
  
  pthread_mutex_lock(&(conn->children.control.mutex));
  for(c=(child_node *)conn->children.list.head;c && c->id != info->id;c=(child_node *) c->next);
  if(c)
    pid = c->pid;
  pthread_mutex_unlock(&(conn->children.control.mutex));
  
  if(pid && kill(pid, info->signal)) {
    print( ERROR, "kill(%du, %d): %s", pid, info->signal, strerror(errno) );
  }
  
  return NULL;
}

/**
 * @brief handle a command request.
 * @param c the connection that send @p msg
 * @param msg the request ::message
 * @returns 0 on success, -1 on error.
 */
int on_command_request(conn_node *c, message *msg) {
  message *reply;
  
  switch(msg->data[0]) {
    case CMD_START:
      reply = on_cmd_start(c, msg);
      break;
    case CMD_SIGNAL:
      reply = on_cmd_signal(c, msg);
      break;
    default:
      print( ERROR, "unknown command '%02hhX'", msg->data[0] );
      reply = NULL;
      break;
  }
  
  if(!reply)
    return 0;
  
  if(enqueue_message(&(c->outcoming), reply)) {
    print( ERROR, "cannot enqueue message" );
    dump_message(reply);
    free_message(reply);
    return -1;
  }
  return 0;
}
