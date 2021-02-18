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

#include <stdio.h>

#include "connection.h"
#include "message.h"
#include "controller.h"
#include "authenticator.h"
#include "command.h"
#include "handler.h"
#include "logger.h"
#include "cSploitd.h"

#include "control_messages.h"

//TODO #incude "module.h"

//TODO
#define on_module_request(c, m) 0

/**
 * @brief handle a daemon control request
 * @param m the received control message
 * @returns 0 on success, -1 on error.
 */
int on_daemon_request(conn_node *c, message *m) {
  switch(m->data[0]) {
    case DMON_STOP:
      stop_daemon();
      return 0;
    default:
      print( ERROR, "unkown daemon request code: %02hhX", m->data[0] );
      return -1;
  }
}

int on_control_request(conn_node *c, message *m) {
  char logged;
  
  if(m->data[0] == AUTH)
    return on_auth_request(c, m);
  
  pthread_mutex_lock(&(c->control.mutex));
  logged = c->logged;
  pthread_mutex_unlock(&(c->control.mutex));
  
  if(!logged) {
    return on_unathorized_request(c, m);
  }
  
  switch (m->data[0]) {
    case CMD_START:
    case CMD_SIGNAL:
      return on_command_request(c, m);
    case HNDL_LIST:
      return on_handler_request(c, m);
    case MOD_LIST:
    case MOD_START:
    case MOD_END:
      return on_module_request(c, m);
    case DMON_STOP:
      return on_daemon_request(c, m);
    default:
      print( ERROR, "unkown control code: %02hhX", m->data[0] );
      return -1;
  }
}