/* LICENSE
 * 
 */
#include <ctype.h>
#include <pthread.h>
#include <errno.h>

#include "message.h"

#include "child.h"
#include "connection.h"
#include "event.h"
#include "log.h"
#include "control_messages.h"
#include "handler.h"
#include "sequence.h"
#include "controller.h"

#include "command.h"

int on_cmd_started(message *m) {
  struct cmd_started_info *started_info;
  child_node *c;
  
  started_info = (struct cmd_started_info *) m->data;
  
  pthread_mutex_lock(&(children.control.mutex));
  
  c = get_child_by_pending_seq(m->head.seq);
  
  if(!c) {
    pthread_mutex_unlock(&(children.control.mutex));
    LOGW("%s: no child pending on sequence #%u", __func__, m->head.seq);
    return -1;
  }
  
  c->pending=0;
  c->id = started_info->id;
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  return 0;
}

int on_cmd_fail(message *m) {
  child_node *c;
  pthread_mutex_lock(&(children.control.mutex));
  
  c = get_child_by_pending_seq(m->head.seq);
  
  if(!c) {
    pthread_mutex_unlock(&(children.control.mutex));
    LOGW("%s: no child pending on sequence #%u", __func__, m->head.seq);
    return -1;
  }
  
  c->pending=0;
  c->id = CTRL_ID;
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  return 0;
}

int on_cmd_end(JNIEnv *env, message *m) {
  struct cmd_end_info *end_info;
  child_node *c;
  jobject event;
  int ret;
  
  ret = -1;
  end_info = (struct cmd_end_info *) m->data;
  
  pthread_mutex_lock(&(children.control.mutex));
  
  c = get_child_by_id(end_info->id);
  
  if(!c) {
    pthread_mutex_unlock(&(children.control.mutex));
    LOGW("%s: child #%u not found", __func__, end_info->id);
    return -1;
  }
  
  list_del(&(children.list), (node *) c);
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  event = create_child_end_event(env, c, &(end_info->exit_value));
  
  if(!event) {
    LOGE("%s: cannot create event", __func__);
  } else if(send_event(env, event)) {
    LOGE("%s: cannot send event", __func__);
  } else {
    ret = 0;
  }
  
  if(event)
    (*env)->DeleteLocalRef(env, event);
  
  free_child(c);
  
  return ret;
}

int on_cmd_died(JNIEnv *env, message *m) {
  struct cmd_died_info *died_info;
  child_node *c;
  jobject event;
  int ret;
  
  ret = -1;
  died_info = (struct cmd_died_info *) m->data;
  
  pthread_mutex_lock(&(children.control.mutex));
  
  for(c=(child_node *) children.list.head;c && c->id != died_info->id;c= (child_node *) c->next);
  
  if(!c) {
    pthread_mutex_unlock(&(children.control.mutex));
    LOGW("%s: child #%u not found", __func__, died_info->id);
    return -1;
  }
  
  list_del(&(children.list), (node *) c);
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  event = create_child_died_event(env, c, &(died_info->signal));
  
  if(!event) {
    LOGE("%s: cannot create event", __func__);
  } else if(send_event(env, event)) {
    LOGE("%s: cannot send event", __func__);
  } else {
    ret = 0;
  }
  
  if(event)
    (*env)->DeleteLocalRef(env, event);
  
  free_child(c);
  
  return ret;
}

/**
 * @brief handle a command message
 * @param m the received message
 * @returns 0 on success, -1 on error.
 */
int on_command(JNIEnv *env, message *m) {
  switch(m->data[0]) {
    case CMD_STARTED:
      return on_cmd_started(m);
    case CMD_FAIL:
      return on_cmd_fail(m);
    case CMD_END:
      return on_cmd_end(env, m);
    case CMD_DIED:
      return on_cmd_died(env, m);
    default:
      LOGW("%s: unkown command code: %02hhX", __func__, m->data[0]);
      return -1;
  }
}

#define INSIDE_SINGLE_QUOTE 1
#define INSIDE_DOUBLE_QUOTE 2
#define ESCAPE_FOUND 4
#define END_OF_STRING 8

int start_command(JNIEnv *env, jclass clazz __attribute__((unused)), jstring jhandler, jstring jcmd, jstring jtarget) {
  char status;
  char *pos, *start, *end;
  const char *utf;
  jstring *utf_parent;
  handler *h;
  uint32_t msg_size; // big enought to check for uint16_t overflow
  int id;
  size_t arg_len;
  struct cmd_start_info *start_info;
  message *m;
  child_node *c;
  
  id = -1;
  m=NULL;
  c=NULL;
  
  utf = (*env)->GetStringUTFChars(env, jhandler, NULL);
  utf_parent = &jhandler;
  
  if(!utf) {
    LOGE("%s: cannot get handler name", __func__);
    goto jni_error;
  }
  
  arg_len = (*env)->GetStringUTFLength(env, jhandler);
  
  if(!arg_len) {
    LOGE("%s: empty handler name", __func__);
    goto jni_error;
  }
  
  arg_len++; // test even the '\0'
  
  for(h=(handler *) handlers.list.head;h && strncmp(utf, h->name, arg_len);h=(handler *) h->next);
  
  if(!h) {
    LOGE("%s: handler \"%s\" not found", __func__, utf);
    goto exit;
  }
  
  (*env)->ReleaseStringUTFChars(env, jhandler, utf);
  utf = (*env)->GetStringUTFChars(env, jcmd, NULL);
  utf_parent = &jcmd;
  
  if(!utf) {
    LOGE("%s: cannot get command string", __func__);
    goto jni_error;
  }
  
  LOGD("%s: parsing \"%s\"", __func__, utf);
  
  msg_size = sizeof(struct cmd_start_info);
  m = create_message(get_sequence(&ctrl_seq, &ctrl_seq_lock),
                      msg_size, CTRL_ID);
  
  if(!m) {
    LOGE("%s: cannot create messages", __func__);
    goto exit;
  }
  
  start_info = (struct cmd_start_info *) m->data;
  start_info->cmd_action = CMD_START;
  start_info->hid = h->id;
  
  status = 0;
  arg_len = 0;
  start = end = NULL;
  
  for(pos=(char *) utf;!(status & END_OF_STRING);pos++) {
    
    // string status parser
    switch (*pos) {
      case '"':
        if(status & (INSIDE_SINGLE_QUOTE | ESCAPE_FOUND)) {
          // copy it as a normal char
        } else if(status & INSIDE_DOUBLE_QUOTE) {
          status &= ~INSIDE_DOUBLE_QUOTE;
          end = pos -1;
        } else {
          status |= INSIDE_DOUBLE_QUOTE;
        }
        break;
      case '\'':
        if(status & (INSIDE_DOUBLE_QUOTE | ESCAPE_FOUND)) {
          // copy it as a normal char
        } else if(status & INSIDE_SINGLE_QUOTE) {
          status &= ~INSIDE_SINGLE_QUOTE;
          end = pos - 1;
        } else {
          status |= INSIDE_SINGLE_QUOTE;
        }
        break;
      case '\\':
        if(status & ESCAPE_FOUND) {
          status &= ~ESCAPE_FOUND;
          // copy it as normal char
        } else {
          status |= ESCAPE_FOUND;
        }
        break;
      case ' ': // if(isspace(*pos))
      case '\t':
        if(!status && start)
          end=pos;
        break;
      case '\0':
        status |= END_OF_STRING;
        end=pos;
        break;
      default:
        if(!start)
          start=pos;
    }
    
    // copy the arg if found
    if(start && end) {
      
      LOGD("%s: argument found: start=%d, end=%d", __func__, (start-utf), (end-utf));
      arg_len=(end-start);
      
      msg_size+=arg_len + 1;
      
      if(msg_size > UINT16_MAX) {
        LOGW("%s: command too long: \"%s\"", __func__, utf);
        goto exit;
      }
      
      m->data = realloc(m->data, msg_size);
      
      if(!m->data) {
        LOGE("%s: realloc: %s", __func__, strerror(errno));
        goto exit;
      }
      
      memcpy(m->data + m->head.size, start, arg_len);
      *(m->data + msg_size -1) = '\0';
      
      m->head.size = msg_size;
      
      start = end = NULL;
    }
  }
  
  // create child
  
  c = create_child(m->head.seq);
  
  if(!c) {
    LOGE("%s: cannot craete child", __func__);
    goto exit;
  }
  
  c->handler = h;
  // check for optional target argument
  if(jtarget) {
    (*env)->ReleaseStringUTFChars(env, jcmd, utf);
    utf = (*env)->GetStringUTFChars(env, jtarget, NULL);
    utf_parent = &jtarget;
    
    
    if(!utf) {
      LOGE("%s: cannot get target string", __func__);
      goto jni_error;
    }
    
    if(!inet_pton(AF_INET, utf, &(c->target))) {
      LOGE("%s: invalid IPv4 address: \"%s\"", __func__, utf);
      goto exit;
    }
  }
  
  // add child to list
  
  pthread_mutex_lock(&(children.control.mutex));
  list_add(&(children.list), (node *) c);
  pthread_mutex_unlock(&(children.control.mutex));
  
  // send message to dSploitd
  
  pthread_mutex_lock(&write_lock);
  // OPTIMIZATION: use m->head.id to store return value for later check
  m->head.id = send_message(sockfd, m);
  pthread_mutex_unlock(&write_lock);
  
  if(m->head.id) {
    LOGE("%s: cannot send messages", __func__);
    goto exit;
  }
  
  // wait for CMD_STARTED or CMD_FAIL
  
  pthread_mutex_lock(&(children.control.mutex));
  
  while(c->pending && children.control.active)
    pthread_cond_wait(&(children.control.cond), &(children.control.mutex));
  
  if(c->id == CTRL_ID || c->pending) { // command failed
    list_del(&(children.list), (node *) c);
  } else {
    id = c->id;
  }
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  if(id != -1) {
    LOGI("%s: child #%d started", __func__, id);
  } else if(c->pending) {
    LOGW("%s: pending child cancelled", __func__);
  } else {
    LOGW("%s: cannot start command", __func__);
  }
  
  goto exit;
  
  jni_error:
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  exit:
  if(c && id==-1) {
    pthread_mutex_lock(&(children.control.mutex));
    list_del(&(children.list), (node *) c);
    pthread_mutex_unlock(&(children.control.mutex));
    free_child(c);
  }
  
  if(m)
    free_message(m);
  
  if(utf)
    (*env)->ReleaseStringUTFChars(env, *utf_parent, utf);
  
  return id;
}

void kill_child(JNIEnv *env, jclass clazz __attribute__((unused)), int id, int signal) {
  message *m;
  int send_error;
  struct cmd_signal_info *signal_info;
  
  m = create_message(get_sequence(&ctrl_seq, &ctrl_seq_lock),
                     sizeof(struct cmd_signal_info), CTRL_ID);
  
  if(!m) {
    LOGE("%s: cannot crate messages", __func__);
    return;
  }
  
  signal_info = (struct cmd_signal_info *) m->data;
  
  signal_info->id = id;
  signal_info->signal = signal;
  
  pthread_mutex_lock(&write_lock);
  send_error = send_message(sockfd, m);
  pthread_mutex_unlock(&write_lock);
  
  if(send_error) {
    LOGE("%s: cannot send messages", __func__);
  }
}