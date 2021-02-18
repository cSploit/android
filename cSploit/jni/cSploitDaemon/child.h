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

#ifndef CHILD_H
#define CHILD_H

#include "list.h"
#include "control.h"
#include "buffer.h"

struct handler;
struct conn_node;
struct message;

/// info about one child
typedef struct child_node {
  node *next;
  
  uint16_t id;                  ///< child id
  pthread_t tid;                ///< stdout reader thread
  struct handler *handler;      ///< handler for this child
  struct conn_node *conn;       ///< connection that own this child
  pid_t pid;                    ///< process managed by this child
  int stdin_fd;                 ///< stdin file descritor of the process
  int stdout_fd;                ///< stdout file descritor of the process
  int stderr_fd;                ///< stderr file descritor of the process
  
  uint16_t seq;                 ///< sent messages sequence number
  buffer output_buff;           ///< ::buffer for ::handler.raw_output_parser
} child_node;

child_node *create_child(struct conn_node *);
void *handle_child(void *);
void stop_children(struct conn_node *);
int on_child_message(struct conn_node *, struct message *);

/// buffer size for read stdout
#define STDOUT_BUFF_SIZE 2048

/// buffer size for read stderr
#define STDERR_BUFF_SIZE 255

/// signal to send to forked processes to stop them
#define CHILD_STOP_SIGNAL 15

#endif