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
#include "auth.h"

#include "handler.h"

struct handlers_list handlers;

/**
 * @brief load handlers from cSploitd
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
  
  if(!authenticated()) {
    LOGE("%s: not authenticated", __func__);
    return JNI_FALSE;
  }
  
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
 * @brief remove an handler by it's id.
 * @param id of the handler to remove
 * 
 * take care of freeing it and removing any "by_name" reference.
 */
void remove_handler_by_id(uint16_t id) {
  handler *h, **hh, **end;
  
  for(h=(handler *) handlers.list.head;h && h->id != id;h=(handler *) h->next);
  
  if(!h) return;
  
  LOGD("%s: removing handler #%u", __func__, id);
  
  list_del(&(handlers.list), (node *) h);
  
  hh=(handler **) &(handlers.by_name);
  end= hh + sizeof(handlers.by_name);
  for(;hh < end; hh++) {
    if(*hh == h)
      memset(hh, 0, sizeof(handler *));
  }
  free(h->name);
  free(h);
}

handler *get_handler_by_name(const char *name) {
  handler *h;
  size_t len;
  
  len = strlen(name) + 1;
  
  pthread_mutex_lock(&(handlers.control.mutex));
  
  for(h=(handler *) handlers.list.head;h && strncmp(h->name, name, len); h=(handler *) h->next);
  
  pthread_mutex_unlock(&(handlers.control.mutex));
  
  return h;
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
         
    remove_handler_by_id(handler_info->id);
    
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
    } else if(!handlers.by_name.arpspoof && !strncmp(h->name, "arpspoof", 9)) {
      handlers.by_name.arpspoof = h;
    } else if(!handlers.by_name.tcpdump && !strncmp(h->name, "tcpdump", 8)) {
      handlers.by_name.tcpdump = h;
    } else if(!handlers.by_name.fusemounts && !strncmp(h->name, "fusemounts", 11)) {
      handlers.by_name.fusemounts = h;
    } else if(!handlers.by_name.network_radar && !strncmp(h->name, "network-radar", 14)) {
      handlers.by_name.network_radar = h;
    } else if(!handlers.by_name.msfrpcd && !strncmp(h->name, "msfrpcd", 8)) {
      handlers.by_name.msfrpcd = h;
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
 */
void unload_handlers() {
  struct handler *h;
  
  pthread_mutex_lock(&(handlers.control.mutex));
  
  handlers.control.active = 0;
  
  while((h=(handler *) queue_get(&handlers.list))) {
    free(h->name);
    free(h);
  }
  
  memset(&(handlers.by_name), 0, sizeof(handlers.by_name));
  
  pthread_mutex_unlock(&(handlers.control.mutex));
  
  pthread_cond_broadcast(&(handlers.control.cond));
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

jobjectArray get_handlers(JNIEnv *env, jclass clazz) {
  jobjectArray jarray;
  jstring jstr;
  size_t size,i;
  handler *h;
  char locked;
  
  clazz = NULL;
  jstr = NULL;
  jarray = NULL;
  locked = 0;
  
  //HACK: use function parameter as variable
  clazz = (*env)->FindClass(env, "java/lang/String");
  
  if(!clazz) goto jni_error;
  
  pthread_mutex_lock(&(handlers.control.mutex));
  
  locked = 1;
  
  h = (handler *) handlers.list.head;
  for(size=0;h;h=(handler *) h->next, size++);
  
  if(!size) goto exit;
  
  jarray = (*env)->NewObjectArray(env, size, clazz, NULL);
  
  if(!jarray) goto jni_error;
  
  h = (handler *) handlers.list.head;
  
  for(i=0;h && i < size;h=(handler *) h->next, i++) {
    jstr = (*env)->NewStringUTF(env, h->name);
    
    if(!jstr) goto jni_error;
    
    (*env)->SetObjectArrayElement(env, jarray, i, jstr);
    
    if((*env)->ExceptionCheck(env)) goto jni_error;
  }
  
  goto exit;
  
  jni_error:
  
  // unlock mutex ASAP
  if(locked) {
    pthread_mutex_unlock(&(handlers.control.mutex));
    locked = 0;
  }
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  if(jarray) {
    (*env)->DeleteLocalRef(env, jarray);
    jarray = NULL;
  }
  
  exit:
  
  if(locked) {
    pthread_mutex_unlock(&(handlers.control.mutex));
  }
  
  if(jstr) {
    (*env)->DeleteLocalRef(env, jstr);
  }
  
  return jarray;
}
