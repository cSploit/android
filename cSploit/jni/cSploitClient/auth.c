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
#include <stddef.h>

#include "control_messages.h"
#include "message.h"
#include "command.h"
#include "log.h"
#include "control.h"
#include "connection.h"
#include "sequence.h"
#include "controller.h"
#include "str_array.h"

#include "auth.h"

struct login_data logged;

/**
 * @brief login to to cSploitd
 * @param jusername the username
 * @param jhash the password hash
 * @returns true on success, false on error.
 */
jboolean jlogin(JNIEnv *env, jclass clazz, jstring jusername, jstring jhash) {
  struct auth_info *auth_info;
  message *m;
  char *username, *pswd_hash;
  char auth_ok;
  
  pthread_mutex_lock(&logged.control.mutex);
  auth_ok = logged.status == LOGIN_OK;
  pthread_mutex_unlock(&logged.control.mutex);
  
  if(auth_ok) {
    LOGI("%s: already logged in", __func__);
    return JNI_TRUE;
  }
  
  auth_ok = 0;
  m=NULL;
  username=NULL;
  pswd_hash=NULL;
  
  username = (char *) (*env)->GetStringUTFChars(env, jusername, NULL);
  
  if(!username) goto jni_error;
  
  pswd_hash = (char *) (*env)->GetStringUTFChars(env, jhash, NULL);
  
  if(!pswd_hash) goto jni_error;
  
  m = create_message(get_sequence(&ctrl_seq, &ctrl_seq_lock), sizeof(struct auth_info), CTRL_ID);
  
  if(!m) {
    LOGE("%s: cannot create messages", __func__);
    goto exit;
  }
  
  auth_info = (struct auth_info *) m->data;
  auth_info->auth_code = AUTH;
  
  if( string_array_add(m, offsetof(struct auth_info, data), username) ||
      string_array_add(m, offsetof(struct auth_info, data), pswd_hash)) {
    LOGE("%s: cannot append string to message", __func__);
    goto exit;
  }
  
  pthread_mutex_lock(&logged.control.mutex);
  logged.status = LOGIN_WAIT;
  pthread_mutex_unlock(&logged.control.mutex);
  
  pthread_mutex_lock(&write_lock);
  if(send_message(sockfd, m)) {
    pthread_mutex_unlock(&write_lock);
    LOGE("%s: cannot send message", __func__);
    goto exit;
  }
  pthread_mutex_unlock(&write_lock);
  
  pthread_mutex_lock(&logged.control.mutex);
  while(logged.status == LOGIN_WAIT && logged.control.active) {
    pthread_cond_wait(&(logged.control.cond), &(logged.control.mutex));
  }
  auth_ok = logged.status == LOGIN_OK;
  pthread_mutex_unlock(&logged.control.mutex);
  
  
  jni_error:
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  exit:
  
  if(m)
    free_message(m);
  
  if(username)
    (*env)->ReleaseStringUTFChars(env, jusername, username);
  
  if(pswd_hash)
    (*env)->ReleaseStringUTFChars(env, jhash, pswd_hash);
  
  if(auth_ok)
    return JNI_TRUE;
  else
    return JNI_FALSE;
}

/**
 * @brief try to find and terminate the task that triggered this AUTH_FAIL
 * 
 * if we received AUTH_FAIL and we are not trying to login, 
 * some task has sent control data without checking login status.
 * this is a bug.
 */
void on_unexcpected_auth_fail() {
  LOGE("%s: received unexcpected AUTH_FAIL", __func__);
}

int on_auth_status(message *m) {
  struct auth_status_info *status_info;
  enum login_status previous_status;
  
  status_info = (struct auth_status_info *) m->data;
  
  pthread_mutex_lock(&(logged.control.mutex));
  previous_status = logged.status;
  if(status_info->auth_code == AUTH_OK) {
    logged.status = LOGIN_OK;
  } else {
    logged.status = LOGIN_FAIL;
  }
  pthread_mutex_unlock(&(logged.control.mutex));
  
  pthread_cond_broadcast(&(logged.control.cond));
  
  if(status_info->auth_code != AUTH_OK && previous_status != LOGIN_WAIT) {
    on_unexcpected_auth_fail();
  }
  
  return 0;
}

/**
 * @brief check if we are authenticated into the server
 * @returns 1 if logged in, 0 if not.
 */
inline int authenticated() {
  register int ret;
  
  pthread_mutex_lock(&(logged.control.mutex));
  if (logged.control.active && logged.status == LOGIN_OK)
    ret = 1;
  else
    ret = 0;
  pthread_mutex_unlock(&(logged.control.mutex));
  
  return ret;
}

jboolean is_authenticated(JNIEnv *env _U_, jclass clazz _U_) {
  return (authenticated() ? JNI_TRUE : JNI_FALSE);
}

/**
 * @brief reset and enable auth data.
 */
void auth_on_connect() {
  pthread_mutex_lock(&(logged.control.mutex));
  logged.control.active = 1;
  logged.status = LOGIN_FAIL;
  pthread_mutex_unlock(&(logged.control.mutex));
}

/**
 * @brief reset and disable auth data.
 */
void auth_on_disconnect() {
  pthread_mutex_lock(&(logged.control.mutex));
  logged.control.active = 0;
  logged.status = LOGIN_FAIL;
  pthread_mutex_unlock(&(logged.control.mutex));
}
