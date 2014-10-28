/* LICENSE
 * 
 */
#include <pthread.h>
#include <jni.h>

#include "log.h"
#include "controller.h"
#include "command.h"
#include "auth.h"
#include "handler.h"
#include "message.h"
#include "control_messages.h"

pthread_mutex_t ctrl_seq_lock = PTHREAD_MUTEX_INITIALIZER;
uint16_t ctrl_seq = 0;

/**
 * @brief handle a control message response
 * @param m the received mesasge
 */
int on_control(JNIEnv *env, message *m) {
  switch(m->data[0]) {
    case AUTH_OK:
    case AUTH_FAIL:
      return on_auth_status(m);
    case CMD_STARTED:
    case CMD_FAIL:
    case CMD_END:
    case CMD_DIED:
    case CMD_STDERR:
      return on_command(env, m);
    case HNDL_LIST:
      return on_handler(m);
    default:
      LOGW("%s: unknown conrtol code: %02hhX", __func__, m->data[0]);
      return -1;
  }
}