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
 * @brief notify the command received that a child has exited
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
    fprintf(stderr, "%s: cannot create messages\n", __func__);
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
      fprintf(stderr, "%s: child exited unexpectly. status=%0*X\n",
            __func__, (int) sizeof(int), status);
      died_info->signal = 0;
    }
  }
  
  if(enqueue_message(&(c->conn->outcoming), msg)) {
    fprintf(stderr, "%s: cannot enqueue messages\n", __func__);
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
    fprintf(stderr, "%s: empty argv", __func__);
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
    fprintf(stderr, "%s: malloc: %s", __func__, strerror(errno));
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
  
  fprintf(stderr, "%s: strndup: %s\n", __func__, strerror(errno));
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
  int pin[2],pout[2],pexec[2];
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
  pin[0] = pin[1] = pout[0] = pout[1] = pexec[0] = pexec[1] = -1;
  request_info = (struct cmd_start_info *)msg->data;
  argv=NULL;
  free_argv0 = 1;
  pid = 0;
  seq = msg->head.seq;
  
  c = create_child(conn);
  if(!c) {
    fprintf(stderr, "%s: cannot craete a new child\n", __func__);
    goto start_fail;
  }
  
  for(h=(handler *)handlers.head;h && h->id != request_info->hid;h=(handler *)h->next);
  c->handler = h;
  
  if(!c->handler) {
    fprintf(stderr, "%s: handler #%d not found\n", __func__, request_info->hid);
    goto start_fail;
  }
  
  cmd = (char *)c->handler->argv0;
  
  i = offsetof(struct cmd_start_info, argv);
  argv = parse_argv(msg, (cmd ? 1 : 0));
  if(!argv) {
    fprintf(stderr, "%s: cannot parse argv\n", __func__);
    goto start_fail;
  }
  
  if(!cmd) {
    // argv[0] in message, must resolve it
    cmd = parse_cmd(argv[0]);
    
    if(!cmd) {
      fprintf(stderr, "%s: command not found: '%s'\n", __func__, argv[0]);
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
    fprintf(stderr, "%s: exec pipe: %s\n", __func__, strerror(errno));
    goto start_fail;
  }
  
  if(h->have_stdin && pipe(pin)) {
    fprintf(stderr, "%s: input pipe: %s\n", __func__, strerror(errno));
    goto start_fail;
  }
  
  if(h->have_stdout && pipe(pout)) {
    fprintf(stderr, "%s: output pipe: %s\n", __func__, strerror(errno));
    goto start_fail;
  }
  
  if((pid = fork()) < 0) {
    fprintf(stderr, "%s: fork: %s\n", __func__, strerror(errno));
    goto start_fail;
  } else if(!pid) {
    // child
    close(pexec[0]);
    close(pin[1]);
    close(pout[0]);
    
    i= open("/dev/null", O_RDWR);
    
    if(pin[0] == -1)
      pin[0] = i;
    if(pout[1] == -1)
      pout[1] = i;
    
    if(h->workdir)
      chdir(h->workdir);
    
    dup2(pin[0], STDIN_FILENO);
    dup2(pout[1], STDOUT_FILENO);
#ifdef NDEBUG
    dup2(i, STDERR_FILENO);
#endif

    close(i);
    close(pin[0]);
    close(pout[1]);
    
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
    
    if(free_argv0)
      free(argv[0]);
    for(i=1;argv[i];i++)
      free(argv[i]);
    free(argv);
    argv=NULL;
    
    if(read(pexec[0],&exec_errno, sizeof(int))) {
      fprintf(stderr, "%s: execv: %s\n", __func__, strerror(exec_errno));
      waitpid(pid, NULL, 0);
      goto start_fail;
    }
#ifndef NDEBUG
    printf("%s: successfully started a child for '%s' (pid=%d)\n", __func__, h->name, pid);
#endif
    close(pexec[0]);
  }
  
  c->pid = pid;
  
  if(h->have_stdin)
    c->stdin_fd = pin[1];
  
  if(h->have_stdout)
    c->stdout_fd = pout[0];
  
  pthread_mutex_lock(&(conn->children.control.mutex));
  if(pthread_create(&(c->tid), NULL, &handle_child, c)) {
    i=errno;
    pthread_mutex_unlock(&(conn->children.control.mutex));
    fprintf(stderr, "%s: pthread_craete: %s\n", __func__, strerror(i));
    goto start_fail;
  }
  list_add(&(conn->children.list), (node *)c);
  pthread_mutex_unlock(&(conn->children.control.mutex));
  
  pthread_cond_broadcast(&(conn->children.control.cond));
  
  msg = create_message(seq, sizeof(struct cmd_started_info), CTRL_ID);
  
  if(!msg) {
    fprintf(stderr, "%s: cannot create messages\n", __func__);
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
  
  if(msg)
    dump_message(msg);
  
  if(pid)
    kill(pid, 9);
  
  msg = create_message(seq, sizeof(struct cmd_fail_info), CTRL_ID);
  
  if(!msg) {
    fprintf(stderr, "%s: cannot create messages\n", __func__);
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
    fprintf(stderr, "%s: kill(%du, %d): %s\n", __func__, pid, info->signal, strerror(errno));
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
      fprintf(stderr, "%s: unknown command '%02hhX'\n", __func__, msg->data[0]);
      reply = NULL;
      break;
  }
  
  if(!reply)
    return 0;
  
  if(enqueue_message(&(c->outcoming), reply)) {
    fprintf(stderr, "%s: cannot enqueue message", __func__);
    dump_message(reply);
    free_message(reply);
    return -1;
  }
  return 0;
}
