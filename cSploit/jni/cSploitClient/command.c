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
#include "auth.h"
#include "str_array.h"

#include "command.h"


void cmd_on_disconnect() {
  child_node *c;
  char found = 0;
  
  pthread_mutex_lock(&(children.control.mutex));
  
  for(c =(child_node *) children.list.head;c;c=(child_node *) c->next) {
    if(c->pending) {
      c->seq = 0;
      c->id = CTRL_ID;
      found = 1;
    }
  }
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  if(found) {
    pthread_cond_broadcast(&(children.control.cond));
  }
}

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
  
  c->seq=0;
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
  
  c->seq=0;
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
  
  // ensure that start_command consumed the started notification
  while(c->pending && children.control.active) {
    pthread_cond_wait(&(children.control.cond), &(children.control.mutex));
  }
  
  list_del(&(children.list), (node *) c);
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  event = create_child_end_event(env, &(end_info->exit_value));
  
  if(!event) {
    LOGE("%s: cannot create event", __func__);
  } else if(send_event(env, c, event)) {
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
  
  // ensure that start_command consumed the started notification
  while(c->pending && children.control.active) {
    pthread_cond_wait(&(children.control.cond), &(children.control.mutex));
  }
  
  list_del(&(children.list), (node *) c);
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  event = create_child_died_event(env, died_info->signal);
  
  if(!event) {
    LOGE("%s: cannot create event", __func__);
  } else if(send_event(env, c, event)) {
    LOGE("%s: cannot send event", __func__);
  } else {
    ret = 0;
  }
  
  if(event)
    (*env)->DeleteLocalRef(env, event);
  
  free_child(c);
  
  return ret;
}

int on_cmd_stderr(JNIEnv *env, message *m) {
  jobject event;
  struct cmd_stderr_info *stderr_info;
  child_node *c;
  int ret;
  
  ret = -1;
  stderr_info = (struct cmd_stderr_info *) m->data;
  
  pthread_mutex_lock(&(children.control.mutex));
  
  for(c=(child_node *) children.list.head;c && c->id != stderr_info->id;c= (child_node *) c->next);
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  if(!c) {
    LOGW("%s: child #%u not found", __func__, stderr_info->id);
    return -1;
  }
  
  event = create_stderrnewline_event(env, stderr_info->line);
  
  if(!event) {
    LOGE("%s: cannot create event", __func__);
  } else if(send_event(env, c, event)) {
    LOGE("%s: cannot send event", __func__);
  } else {
    ret = 0;
  }
  
  if(event)
    (*env)->DeleteLocalRef(env, event);
  
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
    case CMD_STDERR:
      return on_cmd_stderr(env, m);
    default:
      LOGW("%s: unkown command code: %02hhX", __func__, m->data[0]);
      return -1;
  }
}

#define INSIDE_SINGLE_QUOTE 1
#define INSIDE_DOUBLE_QUOTE 2
#define ESCAPE_FOUND 4
#define END_OF_STRING 8

int parse_cmd(JNIEnv *env, handler *h, jstring jcmd, message *m) {
  const char *utf;
  struct cmd_start_info * start_info;
  char status;
  char *pos, *start, *end, *rpos, *wpos;
  size_t arg_len;
  uint32_t msg_size; // big enought to check for uint16_t overflow
  int escapes;
  int ret;
  
  ret = -1;
  msg_size=m->head.size;
  
  utf = (*env)->GetStringUTFChars(env, jcmd, NULL);
  
  if(!utf) {
    LOGE("%s: cannot get command string", __func__);
    goto jni_error;
  }
  
  LOGD("%s: parsing \"%s\"", __func__, utf);
  
  status = 0;
  arg_len = 0;
  escapes = 0;
  start = end = NULL;
  
  for(pos=(char *) utf;!(status & END_OF_STRING);pos++) {
    
    // string status parser
    switch (*pos) {
      case '"':
        if(status & ESCAPE_FOUND) {
          escapes++;
        } else if(status & (INSIDE_SINGLE_QUOTE)) {
          // copy it as a normal char
        } else if(status & INSIDE_DOUBLE_QUOTE) {
          status &= ~INSIDE_DOUBLE_QUOTE;
          end = pos;
        } else {
          status |= INSIDE_DOUBLE_QUOTE;
        }
        break;
      case '\'':
        if(status & ESCAPE_FOUND) {
          escapes++;
        } else if(status & INSIDE_DOUBLE_QUOTE) {
          // copy it as a normal char
        } else if(status & INSIDE_SINGLE_QUOTE) {
          status &= ~INSIDE_SINGLE_QUOTE;
          end = pos;
        } else {
          status |= INSIDE_SINGLE_QUOTE;
        }
        break;
      case '\\':
        if(status & ESCAPE_FOUND) {
          // copy it as normal char
          escapes++;
        } else {
          status |= ESCAPE_FOUND;
          continue;
        }
        break;
      case ' ': // if(isspace(*pos))
      case '\t':
        if(status & ESCAPE_FOUND) {
          escapes++;
        } else if(!status && start) {
          end=pos;
        }
        break;
      case '\0':
        status |= END_OF_STRING;
        end=pos;
        break;
      default:
        if(!start)
          start=pos;
    }
    
    status &= ~ESCAPE_FOUND;
    
    // copy the arg if found
    if(start && end) {
      
      LOGD("%s: argument found: start=%d, end=%d", __func__, (start-utf), (end-utf));
      arg_len=(end-start);
      arg_len-= escapes;
      
      msg_size+=arg_len + 1;
      
      if(msg_size > UINT16_MAX) {
        LOGW("%s: command too long: \"%s\"", __func__, utf);
        goto exit;
      }
      
      m->data = realloc(m->data, msg_size);
      start_info = (struct cmd_start_info *) m->data;
      
      if(!m->data) {
        LOGE("%s: realloc: %s", __func__, strerror(errno));
        goto exit;
      }
      
      wpos = m->data + m->head.size;
      for(rpos=start;rpos<end;rpos++) {
        if(status & ESCAPE_FOUND) {
          status &= ~ESCAPE_FOUND;
          if( *rpos != '\\' &&
              *rpos != '"' &&
              *rpos != '\'' &&
              *rpos != ' ' &&
              *rpos != '\t') {
            // unrecognized escape sequence, copy the backslash as it is.
            *wpos = '\\';
            wpos++;
          }
        } else if(*rpos == '\\') {
          status |= ESCAPE_FOUND;
          continue;
        }
        *wpos=*rpos;
        wpos++;
      }
      *(m->data + msg_size -1) = '\0';
      
      m->head.size = msg_size;
      
      start_info->argc++;
      
      start = end = NULL;
      escapes = 0;
    }
  }
  
  ret = 0;
  
  goto exit;
  
  jni_error:
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  exit:
  
  if(utf)
    (*env)->ReleaseStringUTFChars(env, jcmd, utf);
  
  return ret;
}

int parse_env(JNIEnv *env, jobjectArray jenv, message *m) {
  size_t len;
  const char *utf;
  jstring *jstr;
  int i, ret;
  
  len = (*env)->GetArrayLength(env, jenv);
  ret = -1;
  utf=NULL;
  
  for(i=0;i < len; i++) {
    
    jstr = (jstring) (*env)->GetObjectArrayElement(env, jenv, i);
    
    if(!jstr) goto jni_error;
    
    utf = (*env)->GetStringUTFChars(env, jstr, NULL);
    
    if(!utf) goto jni_error;
    
    if(string_array_add(m, offsetof(struct cmd_start_info, data), (char *) utf)) {
      LOGE("%s: cannot append '%s' to message", __func__, utf);
      goto exit;
    }
    
    (*env)->ReleaseStringUTFChars(env, jstr, utf);
    utf=NULL;
  }
  
  ret = 0;
  
  goto exit;
  
  jni_error:
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  exit:
  
  if(utf)
    (*env)->ReleaseStringUTFChars(env, jstr, utf);
  
  return ret;
  
}

int start_command(JNIEnv *env, jclass clazz __attribute__((unused)), jstring jhandler, jstring jcmd, jobjectArray jenv) {
  const char *utf;
  handler *h;
  int id;
  struct cmd_start_info *start_info;
  message *m;
  child_node *c;
  
  id = -1;
  m=NULL;
  c=NULL;
  utf=NULL;
  
  if(!authenticated()) {
    LOGE("%s: not authenticated", __func__);
    return -1;
  }
  
  if(!jhandler) {
    LOGE("%s: handler cannot be null", __func__);
    return -1;
  }
  
  utf = (*env)->GetStringUTFChars(env, jhandler, NULL);
  
  if(!utf) {
    LOGE("%s: cannot get handler name", __func__);
    goto jni_error;
  }
  
  h = get_handler_by_name(utf);
  
  if(!h) {
    LOGE("%s: handler \"%s\" not found", __func__, utf);
    goto exit;
  }
  
  (*env)->ReleaseStringUTFChars(env, jhandler, utf);
  utf=NULL;
  
  m = create_message(get_sequence(&ctrl_seq, &ctrl_seq_lock),
                      sizeof(struct cmd_start_info), CTRL_ID);
  
  if(!m) {
    LOGE("%s: cannot create messages", __func__);
    goto exit;
  }
  
  start_info = (struct cmd_start_info *) m->data;
  start_info->cmd_action = CMD_START;
  start_info->hid = h->id;
  
  if(jcmd && parse_cmd(env, h, jcmd, m)) {
    LOGE("%s: cannot parse command", __func__);
    goto exit;
  }
  
  if(jenv && parse_env(env, jenv, m)) {
    LOGE("%s: cannot parse environment", __func__);
    goto exit;
  }
  
  // create child
  
  c = create_child(m->head.seq);
  
  if(!c) {
    LOGE("%s: cannot craete child", __func__);
    goto exit;
  }
  
  c->handler = h;
  
  // add child to list
  
  pthread_mutex_lock(&(children.control.mutex));
  list_add(&(children.list), (node *) c);
  pthread_mutex_unlock(&(children.control.mutex));
  
  // send message to cSploitd
  
  pthread_mutex_lock(&write_lock);
  // OPTIMIZATION: use id to store return value for later check
  id = send_message(sockfd, m);
  pthread_mutex_unlock(&write_lock);
  
  if(id) {
    LOGE("%s: cannot send messages", __func__);
    // mark it as failed
    c->id = CTRL_ID;
    c->seq = 0;
  }
  
  id=-1;
  
  // wait for CMD_STARTED or CMD_FAIL
  
  pthread_mutex_lock(&(children.control.mutex));
  
  while(c->seq && children.control.active)
    pthread_cond_wait(&(children.control.cond), &(children.control.mutex));
  
  if(c->id == CTRL_ID || c->seq) { // command failed
    list_del(&(children.list), (node *) c);
  } else {
    id = c->id;
  }
  
  c->pending = 0;
  
  pthread_mutex_unlock(&(children.control.mutex));
  
  pthread_cond_broadcast(&(children.control.cond));
  
  if(id != -1) {
    LOGI("%s: child #%d started", __func__, id);
  } else if(c->seq) {
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
  
  if(m)
    free_message(m);
  
  if(utf)
    (*env)->ReleaseStringUTFChars(env, jhandler, utf);
  
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
  
  signal_info->cmd_action = CMD_SIGNAL;
  signal_info->id = id;
  signal_info->signal = signal;
  
  pthread_mutex_lock(&write_lock);
  send_error = send_message(sockfd, m);
  pthread_mutex_unlock(&write_lock);
  
  if(send_error) {
    LOGE("%s: cannot send messages", __func__);
  }
}
