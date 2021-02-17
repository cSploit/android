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
#ifndef HANDLER_H
#define HANDLER_H

#include <stdint.h>
#include <jni.h>

#include "list.h"
#include "control.h"

typedef struct handler {
  node *next;
  
  uint8_t id;             ///< handler id
  
  uint8_t have_stdin:1;   ///< does it have stdin ?
  uint8_t have_stdout:1;  ///< does it have stdout ?
  uint8_t reserved:6;
  
  char *name;             ///< an human readable identifier
} handler;

enum handlers_loading_status {
  HANDLER_UNKOWN, // inital value ( 0x0 )
  HANDLER_WAIT,
  HANDLER_OK
};

extern struct handlers_list {
  data_control control;
  list list;
  struct {
    handler *blind;
    handler *raw;
    handler *nmap;
    handler *ettercap;
    handler *hydra;
    handler *arpspoof;
    handler *tcpdump;
    handler *fusemounts;
    handler *network_radar;
    handler *msfrpcd;
  } by_name;              ///< access handlers by name
  enum handlers_loading_status status;
} handlers;

struct message;

jboolean jload_handlers(JNIEnv *, jclass);
void unload_handlers(void);
int on_handler(struct message *);
jobjectArray get_handlers(JNIEnv *, jclass);
handler *get_handler_by_name(const char *);

#endif
