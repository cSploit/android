/* LICENSE
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

#include "dSploitd.h"
#include "logger.h"
#include "handler.h"
#include "message.h"
#include "child.h"
#include "connection.h"
#include "command.h"
#include "FS.h"
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

/**
 * @brief read @p argv from @p m
 * @param m the message to parse
 * @param skip_argv0 start building from argv[1]
 * @returns the parsed argv on success, NULL on error.
 */
char **parse_argv(message *m, char skip_argv0) {
  char **argv, *arg, *end;
  int i,n;
  size_t size;
  struct cmd_start_info *start_info;
  
  start_info = (struct cmd_start_info *) m->data;
  
  n = string_array_size(m, start_info->argv);
  
  if(!n) {
    print( ERROR, "empty argv" );
    dump_message(m);
    return NULL;
  }
  
  if(skip_argv0) {
    i=1;
    n++;
  } else {
    i=0;
  }
  
  size = (sizeof(char *) * (n+1));
  argv = malloc(size);
  
  if(!argv) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  
  memset(argv, 0, size);
  
  end = m->data + m->head.size;
  arg=NULL;
  
  while((i<n && (arg=string_array_next(m, start_info->argv, arg)))) {
    
    argv[i] = strndup(arg, (end-arg));
    
    if(!argv[i]) goto strndup_error;
    
    i++;
  }
  
  return argv;

  strndup_error:
  
  print( ERROR, "strndup: %s", strerror(errno) );
  for(i=0;i<n;i++)
    if(argv[i])
      free(argv[i]);
  free(argv);
  
  return NULL;
}

/**
 * @brief ensure that @p cmd is a valid command by searching it in CWD and PATH. 
 * @param cmd the command to search
 * @returns the path of the resolved command on success, NULL on error.
 */
char *parse_cmd(char *cmd) {
  int length;
  char *ret;
  
  if(!cmd)
    return NULL;
  
  length = strlen(cmd);
  
  if(!length)
    return NULL;
  
  if(!access(cmd, X_OK)) {
    ret = cmd;
  } else {
    ret = find_cmd_in_path(cmd);
  }
  
  return ret;
}

/**
 * @brief handle a start command request.
 * @param msg the request ::message.
 * @param conn the connection that send the @p message
 * @returns a reply message on success, NULL on error.
 */
message *on_cmd_start(conn_node *conn, message *msg) {
  char **argv;
  char *cmd;
  int  exec_errno;
  handler *h;
  child_node *c;
  int pin[2],pout[2],perr[2],pexec[2];
  int i;
  struct cmd_start_info *request_info;
  struct cmd_started_info *reply_info;
  pid_t pid;
  char free_argv0;
  uint16_t seq;
  
  c = NULL;
  cmd = NULL;
  // ensure to set piped fd to invalid one,
  // or close(2) will close unexpected fd. ( like our stdin if 0 )
  pin[0] = pin[1] = pout[0] = pout[1] = perr[0] = perr[1] = pexec[0] = pexec[1] = -1;
  request_info = (struct cmd_start_info *)msg->data;
  argv=NULL;
  free_argv0 = 1;
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
  
  i = offsetof(struct cmd_start_info, argv);
  argv = parse_argv(msg, (cmd ? 1 : 0));
  if(!argv) {
    print( ERROR, "cannot parse argv" );
    goto start_fail;
  }
  
  if(!cmd) {
    // argv[0] in message, must resolve it
    cmd = parse_cmd(argv[0]);
    
    if(!cmd) {
      print( ERROR, "command not found: '%s'", argv[0] );
      goto start_fail;
    }
    
    if(cmd != argv[0]) {
      free(argv[0]);
      argv[0] = cmd;
    }
  } else {
    argv[0] = cmd;
    free_argv0 = 0;
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
    
    i= open("/dev/null", O_RDWR);
    
    if(pin[0] == -1)
      pin[0] = i;
    if(pout[1] == -1)
      pout[1] = i;
    
    if(h->workdir)
      chdir(h->workdir);
    
    dup2(pin[0], STDIN_FILENO);
    dup2(pout[1], STDOUT_FILENO);
    dup2(perr[1], STDERR_FILENO);

    close(i);
    close(pin[0]);
    close(pout[1]);
    close(perr[1]);
    
    execv(argv[0], argv);
    i=errno;
    write(pexec[1], &i, sizeof(int));
    close(pexec[1]);
    exit(-1);
  } else {
    // parent
    close(pexec[1]);
    close(pin[0]);
    close(pout[1]);
    close(perr[1]);
    
    if(free_argv0)
      free(argv[0]);
    for(i=1;argv[i];i++)
      free(argv[i]);
    free(argv);
    argv=NULL;
    
    if(read(pexec[0],&exec_errno, sizeof(int))) {
      print( ERROR, "execv: %s", strerror(exec_errno) );
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
  if(argv) {
    if(free_argv0)
      free(argv[0]);
    for(i=1;argv[i];i++)
      free(argv[i]);
    free(argv);
  }
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
