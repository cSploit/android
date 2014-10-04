/* LICENSE
 * 
 */

#include <jni.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#include "log.h"
#include "control.h"
#include "reader.h"
#include "notifier.h"
#include "handler.h"
#include "auth.h"

#include "connection.h"

int sockfd;
pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;

void on_connect() {
  auth_on_connect();
}

void on_disconnect() {
  auth_on_disconnect();
}

/**
 * @brief connect to a dSploitd UNIX socket
 * @param jsocket_path path to the UNIX socket
 * @returns 
 * @throws 
 */
jboolean connect_unix(JNIEnv *env, jclass clazz __attribute__((unused)), jstring jsocket_path) {
  const char *socket_path;
  jboolean ret;
  struct sockaddr_un addr;
  
  sockfd=-1;
  ret = JNI_FALSE;
  
  socket_path = (*env)->GetStringUTFChars(env, jsocket_path, NULL);
  
  if(!socket_path) goto jni_error;
  
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  
  if(sockfd<0) {
    LOGE("%s: socket: %s", __func__, strerror(errno));
    goto cleanup; // nothing to close
  }
  
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
  
  if(connect(sockfd, (struct sockaddr *) &addr, sizeof(addr))) {
    LOGE("%s: connect: %s", __func__, strerror(errno));
    goto error;
  }
  
  if(start_reader()) {
    LOGE("%s: cannot start reader", __func__);
    goto error;
  }
  
  if(start_notifier()) {
    LOGE("%s: cannot start notifier", __func__);
    goto error;
  }
  
  ret = JNI_TRUE;
  
  goto cleanup;
  
  jni_error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  error:
  
  stop_notifier();
  
  close(sockfd);
  
  stop_reader();
  
  sockfd = -1;
  
  cleanup:
  
  if(socket_path)
    (*env)->ReleaseStringUTFChars(env, jsocket_path, socket_path);
  
  return ret;
}

void disconnect_unix(JNIEnv *env __attribute__((unused)), jclass clazz __attribute__((unused))) {
  stop_notifier();
  
  pthread_mutex_lock(&write_lock);
  close(sockfd);
  sockfd=-1;
  pthread_mutex_unlock(&write_lock);
  
  stop_reader();
  
  unload_handlers();
}