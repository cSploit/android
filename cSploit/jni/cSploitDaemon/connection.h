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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "list.h"
#include "control.h"
#include "msgqueue.h"

/// internal struct for manage connection data.
typedef struct conn_node {
  node *next;
  
  pthread_t tid;                ///< thread id that read and write from this connection
  
  int fd;                       ///< file descriptor associated with that connection.
  
  data_control control;         ///< control struct for write/read connection data.
  
  uint16_t ctrl_seq;            ///< control messages sequence number.
  uint16_t child_id;            ///< child id counter.
  
  char logged:1;                ///< is that client logged in ?
  char freeze:1;                ///< freeze connection ?
  char worker_done:1;           ///< is the worker thread finished ?
  char writer_done:1;           ///< is the writer thread finished ?
  char reserved:4;
  
  struct child_list {
    data_control control;
    list list;
  } children;                   ///< list of children started by this connection
  
  struct msgqueue incoming;     ///< incoming messages queue
  struct msgqueue outcoming;    ///< outcoming messages queue
  
} conn_node;

void close_connections(void);
int serve_new_client(int);

extern struct connection_list {
  data_control control;
  list list;
} connections;


#endif
