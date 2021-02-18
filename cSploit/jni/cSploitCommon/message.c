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
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "logger.h"
#include "message.h"
#include "io.h"

/**
 * @brief create a message
 * @param seq the sequence number of this message.
 * @param size the size in bytes of the message.
 * @param id the message receiver id
 */
message *create_message(uint16_t seq, uint16_t size, uint16_t id) {
  message *m;
  
  assert(size>0);
  
  m=malloc(sizeof(struct message));
  if(!m) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  
  m->data = malloc(size);
  if(!m->data) {
    free(m);
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  
  m->head.seq = seq;
  m->head.id = id;
  m->head.size = size;
  memset(m->data, 0, size);
  
  return m;
}

/**
 * @brief read a message from an open file desctriptor.
 * @param fd the file desctriptor to read from
 * @returns a pointer to the readed message, or NULL on error.
 */
message *read_message(int fd) {
  struct message *res;
  
  res = malloc(sizeof(struct message));
  
  if(!res) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  
  memset(res, 0, sizeof(struct message));
  
  if(!read_wrapper(fd, &(res->head), sizeof(struct msg_header))) {
    if(res->head.size) {
      res->data = read_chunk(fd, res->head.size);
      if(res->data) {
#ifndef NDEBUG
        print( DEBUG, "received a message (fd=%d)", fd );
        dump_message(res);
#endif
        return res;
      }
    } else {
      print( ERROR, "received a 0-length message" );
      dump_message(res);
    }
  }
  
  free(res);
  return NULL;
}

/**
 * @brief convert a message into a readable format.
 * @param msg the message to convert
 */
char *message_to_string(message *msg) {
  char *pos, *end, *res, *write;
  size_t len;
  
  len = snprintf(NULL, 0, "{ seq = %d, size = %d, id = %d, data = '",
          msg->head.seq, msg->head.size, msg->head.id);
  pos=msg->data;
  end=pos+msg->head.size;
  for(;pos<end;pos++) {
    if(31 < *pos && *pos < 127)
      len++;
    else
      len+=4;
  }
  
  len+=3;
  
  res = malloc(len + 1);
  
  if(!res)
    return NULL;
  
  write = res;
  
  write += snprintf(write, len, "{ seq = %d, size = %d, id = %d, data = '",
          msg->head.seq, msg->head.size, msg->head.id);
  
  for(pos=msg->data;pos<end;pos++) {
    if(31 < *pos && *pos < 127)
      write += snprintf(write, 2, "%c", *pos);
    else
      write += snprintf(write, 5, "\\x%02hhX", *pos);
  }
  
  write += snprintf(write, 4, "' }");
  
  *write = '\0';
  
  return res;
}

/**
 * @brief dump a message to stderr
 * @param m the message to dump
 */
void _dump_message(const char *func, message *m) {
  char *str;
  
  str = message_to_string(m);
  
  _print(DEBUG, "%s: %s", func, str);
  
  if(str)
    free(str);
}

/**
 * @brief release all the resources used by a message
 * @param msg the message to free
 */
void free_message(message *msg) {
  if(!msg)
    return;
  
  if(msg->data)
    free(msg->data);
  free(msg);
}

/**
 * @brief make a copy of a message and all it's content
 * @param msg the message to copy
 * @returns the new message on success, NULL on error.
 */
message *msgdup(message *msg) {
  message *m;
  
  m=malloc(sizeof(message));
  
  if(!m) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  
  memcpy(m, msg, sizeof(message));
  
  m->data = malloc(msg->head.size);
  if(!m->data) {
    free(m);
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  memcpy(m->data, msg->data, msg->head.size);
  
  return m;
}

/**
 * @brief send a message to the @p fd file descriptor
 * @param fd the file descritor to write on
 * @param msg the message to send
 * @returns 0 on success, -1 on error.
 */
int send_message(int fd, message *msg) {
  if(write_wrapper(fd, &(msg->head), sizeof(struct msg_header)))
    return -1;
  if(write_wrapper(fd, msg->data, msg->head.size))
    return -1;

#ifndef NDEBUG
  print( DEBUG, "message sent (fd=%d)", fd );
  dump_message(msg);
#endif

  return 0;
}
