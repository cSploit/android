/* LICENSE
 * 
 */

#include <stdio.h>

#include "connection.h"
#include "message.h"
#include "controller.h"
#include "authenticator.h"
#include "command.h"
#include "handler.h"

#include "control_messages.h"

//TODO #incude "module.h"

//TODO
#define on_module_request(c, m) 0

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
    default:
      fprintf(stderr, "%s: unkown control code: %02hhX\n", __func__, m->data[0]);
      return -1;
  }
}