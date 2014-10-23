/* LICENSE
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
 * @brief login to to dSploitd
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

int on_auth_status(message *m) {
  struct auth_status_info *status_info;
  
  status_info = (struct auth_status_info *) m->data;
  
  pthread_mutex_lock(&(logged.control.mutex));
  if(status_info->auth_code == AUTH_OK) {
    logged.status = LOGIN_OK;
  } else {
    logged.status = LOGIN_FAIL;
  }
  pthread_mutex_unlock(&(logged.control.mutex));
  
  pthread_cond_broadcast(&(logged.control.cond));
  
  return 0;
}

jboolean is_authenticated(JNIEnv *env _U_, jclass clazz _U_) {
  jboolean ret;
  
  pthread_mutex_lock(&(logged.control.mutex));
  ret = (logged.status == LOGIN_OK ? JNI_TRUE : JNI_FALSE);
  pthread_mutex_unlock(&(logged.control.mutex));
  
  return ret;
}

/**
 * @brief allocate resources used in runtime
 */
void auth_on_connect() {
  control_init(&(logged.control));
  control_activate(&(logged.control));
}

/**
 * @brief relase resources because connection is shutting down
 */
void auth_on_disconnect() {
  control_deactivate(&(logged.control));
  control_destroy(&(logged.control));
}
