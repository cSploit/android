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
#ifndef CHILD_H
#define CHILD_H

#include <jni.h>
#include <arpa/inet.h>

#include "list.h"
#include "control.h"
#include "buffer.h"

struct handler;

typedef struct {
  node *next;
  
  uint16_t  id;             ///< child id
  uint8_t   pending;        ///< is this child pending for CMD_STARTED / CMD_FAIL ?
  uint16_t  seq;            ///< pending message sequence number
  
  struct handler *handler;  ///< handler for this child
  
  buffer    buffer;         ///< ::buffer for received output
  
} child_node;

extern struct child_list {
  list list;
  data_control control;
} children;

child_node *create_child(uint16_t );
inline child_node *get_child_by_id(uint16_t );
inline child_node *get_child_by_pending_seq(uint16_t);
void free_child(child_node *);
void free_all_childs(void);
extern jboolean send_to_child(JNIEnv *, jclass, int, jbyteArray);

#endif
