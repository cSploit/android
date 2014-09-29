/* LICENSE
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "list.h"
#include "control.h"
#include "connection.h"
#include "message.h"
#include "controller.h"
#include "buffer.h"
#include "control_messages.h"
#include "sequence.h"
#include "str_array.h"

#include "authenticator.h"

/// seconds to freeze the connection if authentication fails
#define AUTH_FAIL_DELAY 3

/// path to file containing logins
#define USERS_FILE "users"

list users;

/**
 * @brief load a user from a line
 * @param line the line to parse
 * @returns 0 on success, -1 on error.
 * 
 * expected line format is:
 *   "username:password_hash"
 */
int load_user(char *line) {
  char *colon;
  login *l;
  
  colon = strchr(line, ':');
  
  if(!colon) {
    fprintf(stderr, "%s: colon (':') not found\n", __func__);
    return -1;
  }
  
  l = malloc(sizeof(login));
  
  if(!l) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    return -1;
  }  
    
  *colon = '\0';
  colon++;
  
  if(!(l->user = strdup(line)) || !(l->pswd_hash = strdup(colon))) {
    fprintf(stderr, "%s: strdup: %s\n", __func__, strerror(errno));
  } else {
    l->user_len = strlen(l->user);
    l->hash_len = strlen(l->pswd_hash);
    list_add(&users, (node *) l);
    return 0;
  }
  free(l);
  return -1;
}

/**
 * @brief load users from ::USERS_FILE
 * @returns 0 on success, -1 on error.
 */
int load_users() {
  char buff[255], *line;
  buffer b;
  int fd, count, ret;
  
  memset(&users, 0, sizeof(list));
  memset(&b, 0, sizeof(buffer));
  ret = -1;
  
  fd = open(USERS_FILE, O_RDONLY);
  
  if(fd == -1) {
    fprintf(stderr, "%s: cannot open \"" USERS_FILE "\": %s\n", __func__, strerror(errno));
    return -1;
  }
  
  while((count = read(fd, buff, 255)) > 0) {
    if(append_to_buffer(&b, buff, count)) {
      fprintf(stderr, "%s: cannot append to buffer\n", __func__);
      goto exit;
    }
    while((line = get_line_from_buffer(&b))) {
      if(load_user(line)) {
        fprintf(stderr, "%s: cannot load user from \"%s\"\n", __func__, line);
      }
      free(line);
    }
  }
  
  if(count == -1) {
    fprintf(stderr, "%s: read: %s\n", __func__, strerror(errno));
  } else {
    ret = 0;
  }
  
  exit:
  release_buffer(&b);
  
  return ret;
}

void unload_users() {
  login *l;
  
  while((l=(login *) queue_get(&users))) {
    free(l->user);
    free(l->pswd_hash);
    free(l);
  }
}

int on_unathorized_request(conn_node *c, message *m __attribute__((unused))) {
  struct auth_status_info *fail_info;
  message *reply;
  
  fprintf(stderr, "%s: received a control message fron an unauthorized client\n", __func__);
  
  reply = create_message(get_sequence(&(c->ctrl_seq), &(c->control.mutex)),
                         sizeof(struct auth_status_info),
                         CTRL_ID);
  if(!reply) {
    fprintf(stderr, "%s: cannot create messages\n", __func__);
    return -1;
  }
  
  fail_info = (struct auth_status_info *) reply->data;
  
  fail_info->auth_code = AUTH_FAIL;
  
  if(enqueue_message(&(c->outcoming), reply)) {
    fprintf(stderr, "%s: cannot enqueue messages", __func__);
    dump_message(reply);
    free_message(reply);
    return -1;
  }
  return 0;
}

/**
 * @brief handle an ::AUTH request message
 * @param c the connection that send the message
 * @param m the message
 * @returns 0 on success, -1 on error
 */
int on_auth_request(conn_node *c, message *m) {
  char *user;
  char *pswd_hash;
  char reply_code;
  int ret;
  login *l;
  struct auth_info * auth_info;
  message *reply;
  
  auth_info = (struct auth_info *) m->data;
  reply_code = AUTH_FAIL;
  ret = 0;
  
  user = string_array_next(m, auth_info->data, NULL);
  pswd_hash = string_array_next(m, auth_info->data, user);
  
  if(!user || !pswd_hash) {
    fprintf(stderr, "%s: missing authentication credentials\n", __func__);
    dump_message(m);
    goto send_response;
  }
  
  for(l=(login *) users.head;l && strncmp(l->user, user, l->user_len + 1);l=(login *) l->next);
  
  if(!l) {
    fprintf(stderr, "%s: unkown username: \"%s\"\n", __func__, user);
  } else if(strncmp(l->pswd_hash, pswd_hash, l->hash_len + 1)) {
    fprintf(stderr, "%s: wrong password hash\n", __func__);
  } else {
    reply_code = AUTH_OK;
    pthread_mutex_lock(&(c->control.mutex));
    c->logged = 1;
    pthread_mutex_unlock(&(c->control.mutex));
#ifndef NDEBUG
    printf("%s: user \"%s\" logged in\n", __func__, user);
#endif
  }
  
  send_response:
  
  reply = create_message(get_sequence(&(c->ctrl_seq), &(c->control.mutex)),
                         sizeof(struct auth_status_info), CTRL_ID);
  
  if(!reply) {
    fprintf(stderr, "%s: cannot create messages\n", __func__);
    ret = -1;
    goto exit;
  }
  
  auth_info = (struct auth_info *) reply->data;
  
  auth_info->auth_code = reply_code;
  
  if(enqueue_message(&(c->outcoming), reply)) {
    fprintf(stderr, "%s: cannot enqueue message\n", __func__);
    dump_message(reply);
    free_message(reply);
    ret = -1;
  }
  
  exit:
  
  if(reply_code == AUTH_FAIL) {
    pthread_mutex_lock(&(c->control.mutex));
    c->freeze = 1;
    pthread_mutex_unlock(&(c->control.mutex));
    
    pthread_cond_broadcast(&(c->control.cond));
    
    sleep(AUTH_FAIL_DELAY);
    
    pthread_mutex_lock(&(c->control.mutex));
    c->freeze = 0;
    pthread_mutex_unlock(&(c->control.mutex));
    
    pthread_cond_broadcast(&(c->control.cond));
  }
  return ret;
}