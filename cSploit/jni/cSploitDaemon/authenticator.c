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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "logger.h"
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
    print( ERROR, "colon (':') not found" );
    return -1;
  }
  
  l = malloc(sizeof(login));
  
  if(!l) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return -1;
  }  
    
  *colon = '\0';
  colon++;
  
  if(!(l->user = strdup(line)) || !(l->pswd_hash = strdup(colon))) {
    print( ERROR, "strdup: %s", strerror(errno) );
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
    print( ERROR, "cannot open \"" USERS_FILE "\": %s", strerror(errno) );
    return -1;
  }
  
  while((count = read(fd, buff, 255)) > 0) {
    if(append_to_buffer(&b, buff, count)) {
      print( ERROR, "cannot append to buffer" );
      goto exit;
    }
    while((line = get_line_from_buffer(&b))) {
      if(load_user(line)) {
        print( ERROR, "cannot load user from \"%s\"", line );
      }
      free(line);
    }
  }
  
  if(count == -1) {
    print( ERROR, "read: %s", strerror(errno) );
  } else {
    ret = 0;
  }
  
  exit:
  release_buffer(&b);
  close(fd);
  
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
  
  print( ERROR, "received a control message fron an unauthorized client" );
  
  reply = create_message(get_sequence(&(c->ctrl_seq), &(c->control.mutex)),
                         sizeof(struct auth_status_info),
                         CTRL_ID);
  if(!reply) {
    print( ERROR, "cannot create messages" );
    return -1;
  }
  
  fail_info = (struct auth_status_info *) reply->data;
  
  fail_info->auth_code = AUTH_FAIL;
  
  if(enqueue_message(&(c->outcoming), reply)) {
    print( ERROR, "cannot enqueue messages" );
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
    print( ERROR, "missing authentication credentials" );
    dump_message(m);
    goto send_response;
  }
  
  for(l=(login *) users.head;l && strncmp(l->user, user, l->user_len + 1);l=(login *) l->next);
  
  if(!l) {
    print( ERROR, "unkown username: \"%s\"", user );
  } else if(strncmp(l->pswd_hash, pswd_hash, l->hash_len + 1)) {
    print( ERROR, "wrong password hash" );
  } else {
    reply_code = AUTH_OK;
    pthread_mutex_lock(&(c->control.mutex));
    c->logged = 1;
    pthread_mutex_unlock(&(c->control.mutex));
#ifndef NDEBUG
    print( DEBUG, "user \"%s\" logged in", user );
#endif
  }
  
  send_response:
  
  reply = create_message(get_sequence(&(c->ctrl_seq), &(c->control.mutex)),
                         sizeof(struct auth_status_info), CTRL_ID);
  
  if(!reply) {
    print( ERROR, "cannot create messages" );
    ret = -1;
    goto exit;
  }
  
  auth_info = (struct auth_info *) reply->data;
  
  auth_info->auth_code = reply_code;
  
  if(enqueue_message(&(c->outcoming), reply)) {
    print( ERROR, "cannot enqueue message" );
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