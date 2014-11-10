/* LICENSE
 * 
 */
#include <pthread.h>
#include <jni.h>

#include "log.h"
#include "controller.h"
#include "connection.h"
#include "command.h"
#include "auth.h"
#include "handler.h"
#include "message.h"
#include "control_messages.h"
#include "sequence.h"

pthread_mutex_t ctrl_seq_lock = PTHREAD_MUTEX_INITIALIZER;
uint16_t ctrl_seq = 0;

/**
 * @brief ask daemon to stop itself
 */
void request_shutdown(JNIEnv *env _U_, jclass clazz _U_ ) {
  int ret;
  message *m;
  
  if(!authenticated()) {
    LOGE("%s: not authenticated", __func__);
    return;
  }
  
  m = create_message(get_sequence(&ctrl_seq, &ctrl_seq_lock),
                     sizeof(struct dmon_stop_info), CTRL_ID);
  if(!m) {
    LOGE("%s: cannot create messages", __func__);
    return;
  }
  
  m->data[0] = DMON_STOP;
  
  pthread_mutex_lock(&write_lock);
  ret = send_message(sockfd, m);
  pthread_mutex_unlock(&write_lock);
  
  if(ret) {
    LOGE("%s: cannot send messages", __func__);
  }
  free_message(m);
}

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