/* LICENSE
 * 
 * 
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "control_messages.h"
#include "log.h"
#include "connection.h"

#include "child.h"

struct child_list children;

child_node *create_child(uint16_t pending_seq_no) {
  child_node *c;
  
  c = malloc(sizeof(child_node));
  
  if(!c) {
    LOGE("%s: malloc: %s", __func__, strerror(errno));
    return NULL;
  }
  
  memset(c, 0, sizeof(child_node));
  c->pending = 1;
  c->seq = pending_seq_no;
  c->id = CTRL_ID;
  
  return c;
}

/**
 * @brief get a child from ::children by it's @p id
 * @param id the id of the wanted child
 * @returns the found child on success, NULL on error
 */
inline child_node *get_child_by_id(uint16_t id) {
  register child_node *c;
  for(c=(child_node *) children.list.head;c && c->id != id; c= (child_node *) c->next);
  return c;
}

/**
 * @brief get a child from ::children by it's pending @p seq
 * @param seq the pensing sequence number of the wanted child
 * @returns the found child on success, NULL on error
 */
inline child_node *get_child_by_pending_seq(uint16_t seq) {
  register child_node *c;
  for(c=(child_node *) children.list.head;c && (!c->pending || c->seq != seq); c= (child_node *) c->next);
  return c;
}

void free_child(child_node *c) {
  release_buffer(&(c->buffer));
  free(c);
}

/**
 * @brief send raw bytes to child
 * @param id the child id
 * @param array the byte array to send
 * @returns 0 on success, -1 on error.
 */
jboolean send_to_child(JNIEnv *env, jclass clazz __attribute__((unused)), int id, jbyteArray array) {
  jboolean ret;
  uint16_t seq;
  jbyte *buff;
  jsize len;
  message *m;
  child_node *c;
  
  ret = JNI_FALSE;
  m = NULL;
  c = NULL;
  buff = NULL;
  
  pthread_mutex_lock(&(children.control.mutex));
  for(c=(child_node *) children.list.head;c && c->id != id;c=(child_node *) c->next);
  if(c) seq=c->seq++;
  pthread_mutex_unlock(&(children.control.mutex));
  
  if(!c) {
    LOGE("%s: child #%d not found", __func__, id);
    return -1;
  }
  
  buff = (*env)->GetByteArrayElements(env, array, NULL);
  
  if(!buff) goto jni_error;
  
  len = (*env)->GetArrayLength(env, array); // no error specified
  
  LOGD("%s: len=%d", __func__, len);
  
  m = create_message(seq, len, c->id);
  
  if(!m) {
    LOGE("%s: cannot craete messages", __func__);
    goto cleanup;
  }
  
  memcpy(m->data, buff, len);
  
  pthread_mutex_lock(&write_lock);
  if(!send_message(sockfd, m))
    ret = JNI_TRUE;
  pthread_mutex_unlock(&write_lock);
  
  if(!ret) {
    LOGE("%s: cannot send messages", __func__);
  }
  
  goto cleanup;
  
  jni_error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  cleanup:
  
  if(m)
    free_message(m);
  
  if(buff) {
    LOGD("%s: releasing array", __func__);
    (*env)->ReleaseByteArrayElements(env, array, buff, JNI_ABORT);
  }
  
  return ret;
}