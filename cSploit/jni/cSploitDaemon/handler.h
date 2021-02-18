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

#ifndef HANDLER_H
#define HANDLER_H

#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>

#include "list.h"
#include "control.h"
#include "message.h"

struct conn_node;
struct child_node;
struct msg_node;

typedef struct handler {
  node *next;
  uint8_t id;                   ///< handler id
  
  char    have_stdin:1;         ///< does it have stdin ?
  char    have_stdout:1;        ///< does it have stdout ?
  char    enabled:1;            ///< is it enabled ?
  
  /**
   * @brief function that parse child binary output
   * @param c current child
   * @param b buffer containing child output
   * @param cn number of bytes to process
   * @returns 0 on success, -1 on error
   */
  int (* raw_output_parser)(struct child_node *c, char *b, int cn);
  /**
   * @brief function that parse child output, line by line.
   * @param l the line to parse
   * @returns a message to send or NULL
   */
  message *(* output_parser)(char *l);
  /**
   * @brief function that pre-parse child input
   * @param c current child
   * @param m received message
   */
  void (* input_parser)(struct child_node *c, message *m);
  
  /// path to the process binary
  const char *argv0;
  /// path to chdir into before executing program
  const char *workdir;
  
  /// an human readable identifier of the handler
  const char *name;
  
  /// handle returned by dlopen
  void *dl_handle;
} handler;

/// list containing loaded handlers
extern list handlers;

int load_handlers(void);
void unload_handlers(void);
int on_handler_request(struct conn_node *, message *);
  
#define HANDLERS_DIR "handlers"

#endif
