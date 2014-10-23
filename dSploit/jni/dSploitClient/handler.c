/* LICENSE
 * 
 */

#include <jni.h>
#include <errno.h>
#include <string.h>

#include "control_messages.h"
#include "controller.h"
#include "list.h"
#include "control.h"
#include "message.h"
#include "sequence.h"
#include "log.h"
#include "connection.h"

#include "handler.h"

struct handlers_list handlers;

/* TODO. need internet for writing it.
jobjectarray jloaded_handlers(JNIEnv *env, jclass clazz _U_) {
  
} */

/**
 * @brief load handlers from dSploitd
 * @returns true on success, false on error.
 * 
 * call it once a time for connection,
 * the code is not thread-safe because it's intended to run
 * one time after the connection has been established.
 */
jboolean jload_handlers(JNIEnv *env _U_, jclass clazz _U_) {
  message *m;
  int send_error;
  jboolean ret;
  
  send_error = -1;
  
  m = create_message(get_sequence(&ctrl_seq, &ctrl_seq_lock),
                     sizeof(struct hndl_list_info), CTRL_ID);
  
  if(!m) {
    LOGE("%s: cannot create messages", __func__);
    return JNI_FALSE;
  }
  
  m->data[0] = HNDL_LIST;
  
  pthread_mutex_lock(&(handlers.control.mutex));
  handlers.control.active = 1;
  handlers.status = HANDLER_WAIT;
  pthread_mutex_unlock(&(handlers.control.mutex));
  
  pthread_mutex_lock(&write_lock);
  send_error = send_message(sockfd, m);
  pthread_mutex_unlock(&write_lock);
  
  free_message(m);
  
  if(send_error) {
    LOGE("%s: cannot send messages", __func__);
    return JNI_FALSE;
  }
  
  
  pthread_mutex_lock(&(handlers.control.mutex));
  while(handlers.status == HANDLER_WAIT && handlers.control.active) {
    pthread_cond_wait(&(handlers.control.cond), &(handlers.control.mutex));
  }
  
  ret = ((handlers.status == HANDLER_OK && handlers.list.head) ? JNI_TRUE : JNI_FALSE);
  
  pthread_mutex_unlock(&(handlers.control.mutex));
  
  return ret;
}

/**
 * @brief load handlers from an ::HNDL_LIST ::message
 * @param m the received message
 * @returns 0 on success, -1 on error.
 */
int on_handler_list(message *m) {
  char *end;
  int ret;
  size_t name_len;
  struct hndl_info *handler_info;
  handler *h;
  
  ret = 0;
  end = m->data + m->head.size;
  handler_info = (struct hndl_info *) (m->data +1);
  
  pthread_mutex_lock(&(handlers.control.mutex));
  
  while(((char *) handler_info) < end) {
    
    LOGD("%s: id=%u, have_stdin=%u, have_stdout=%u, name=\"%s\"", __func__, handler_info->id,
         handler_info->have_stdin, handler_info->have_stdout, handler_info->name);
         
    for(h=(handler *) handlers.list.head;h && h->id != handler_info->id;h=(handler *) h->next);
    
    if(h) {
      LOGI("%s: updating handler #%u", __func__, handler_info->id);
      list_del(&(handlers.list), (node *)h);
    }
    
    h = malloc(sizeof(handler));
    
    if(!h) {
      LOGE("%s: malloc: %s", __func__, strerror(errno));
      ret = -1;
      break;
    }
    
    memset(h, 0, sizeof(handler));
    
    h->name = strndup(handler_info->name, (end - handler_info->name));
    
    if(!h->name) {
      free(h);
      LOGE("%s: strndup: %s", __func__, strerror(errno));
      ret = -1;
      break;
    }
    
    name_len = strlen(h->name);
    
    if(!handlers.by_name.blind && !strncmp(h->name, "blind", 6)) {
      handlers.by_name.blind = h;
    } else if(!handlers.by_name.raw && !strncmp(h->name, "raw", 4)) {
      handlers.by_name.raw = h;
    } else if(!handlers.by_name.nmap && !strncmp(h->name, "nmap", 5)) {
      handlers.by_name.nmap = h;
    } else if(!handlers.by_name.ettercap && !strncmp(h->name, "ettercap", 9)) {
      handlers.by_name.ettercap = h;
    } else if(!handlers.by_name.hydra && !strncmp(h->name, "hydra", 6)) {
      handlers.by_name.hydra = h;
    }
    
    h->id = handler_info->id;
    h->have_stdin = handler_info->have_stdin;
    h->have_stdout = handler_info->have_stdout;
    
    list_add(&handlers.list, (node *) h);
    
    handler_info =  (struct hndl_info *) (
                    ((char *) handler_info) + sizeof(struct hndl_info) + name_len + 1);
  }
  
  name_len = ( handlers.status == HANDLER_WAIT );
  handlers.status = HANDLER_OK;
  pthread_mutex_unlock(&(handlers.control.mutex));
  
  pthread_cond_broadcast(&(handlers.control.cond));
  
  if(!name_len) {
    LOGW("%s: received handlers list but no one is waiting it.", __func__);
  }
  
  return ret;
}

/**
 * @brief unload handlers
 * 
 * this function in NOT thread-safe.
 * it's intended to run once at connection shutdown
 */
void unload_handlers() {
  struct handler *h;
  
  control_deactivate(&(handlers.control));
  
  while((h=(handler *) queue_get(&handlers.list))) {
    free(h->name);
    free(h);
  }
}

/**
 * @brief mange an handler response
 * @param m the response message
 * @returns 0 on success, -1 on error.
 */
int on_handler(message *m) {
  switch(m->data[0]) {
    case HNDL_LIST:
      return on_handler_list(m);
    default:
      LOGW("%s: unknown handler code: %02hhX", __func__, m->data[0]);
      return -1;
  }
}
