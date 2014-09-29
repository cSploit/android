/* LICENSE
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <jni.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "init.h"
#include "cache.h"
#include "command.h"
#include "handler.h"
#include "connection.h"
#include "reader.h"
#include "child.h"
#include "msgqueue.h"
#include "auth.h"

#define NUMELEM(a) (sizeof(a)/sizeof(a[0]))

JavaVM *jvm;

void init_structs() {
  memset(&handlers, 0, sizeof(struct handlers_list));
  memset(&cache, 0, sizeof(struct cache));
  memset(&children, 0, sizeof(struct child_list));
  memset(&incoming_messages, 0, sizeof(struct msgqueue));
}

int init_controls() {
  if(control_init(&(children.control)))
    return -1;
  if(control_init(&(incoming_messages.control)))
    return -1;
  if(control_init(&(handlers.control)))
    return -1;
  if(control_init(&(logged.control)))
    return -1;
  return 0;
}

/**
 * @brief this function redirect stderr and stdio to a logfile.
 * @param jpath path to the file to write to
 * 
 * it's useful for debug purposes. all function from "dSploitCommon"
 * use stderr for reporting errors. on android these info will be lost.
 */
void redirect_stdio(JNIEnv *env, jclass clazz _U_, jstring jpath) {
  const char *utf;
  int fd;
  
  utf = (*env)->GetStringUTFChars(env, jpath, NULL);
  
  if(!utf) goto jni_error;
  
  fd = open(utf, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC );
  
  if(fd == -1) {
    LOGE("%s: open(\"%s\"): %s", __func__, utf, strerror(errno));
    goto exit;
  }
  
  if(dup2(fd, fileno(stderr)) == -1 || dup2(fd, fileno(stdout)) == -1) {
    LOGE("%s: dup2: %s", __func__, strerror(errno));
    goto exit;
  }
  
  if(setvbuf(stderr, NULL, _IOLBF, 1024) || setvbuf(stdout, NULL, _IOLBF, 1024)) {
    LOGW("%s: setvbuf: %s", __func__, strerror(errno));
  }
  
  goto exit;
  
  jni_error:
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  exit:
  if(utf)
    (*env)->ReleaseStringUTFChars(env, jpath, utf);
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* pVm, void* reserved _U_) {
  JNIEnv *env;
  jint ret;
  JNINativeMethod nm[] =  {
    { "LoadHandlers", "()Z", jload_handlers },
    { "StartCommand", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I", start_command },
    { "Connect", "(Ljava/lang/String;)Z", connect_unix },
    { "Login", "(Ljava/lang/String;Ljava/lang/String;)Z", jlogin },
    { "Disconnect", "()V", disconnect_unix },
    { "Kill", "(II)V", kill_child },
    { "SendTo", "(I[B)Z", send_to_child },
    { "SetCoreLogfile", "(Ljava/lang/String;)V", redirect_stdio },
  };
  
  ret = (*pVm)->GetEnv(pVm, (void **)&env, JNI_VERSION_1_6);
  
  if(ret != JNI_OK) {
    LOGF("%s: GetEnv: %d", __func__, ret);
    return -1;
  }
  
  init_structs();
  
  if(init_controls()) {
    LOGF("%s: cannot init controls", __func__);
    return -1;
  }
  
  if(init_cache(env)) {
    LOGF("%s: cannot init cache", __func__);
    return -1;
  }
  
  ret = (*env)->RegisterNatives(env, cache.dsploit.core.client.class, nm, NUMELEM(nm));
  
  if(ret != JNI_OK) {
    
    if((*env)->ExceptionCheck(env)) {
      (*env)->ExceptionDescribe(env);
      (*env)->ExceptionClear(env);
    }
    
    LOGF("%s: cannot register native methods: %d", __func__, ret);
    
    return -1;
  }
  
  jvm = pVm;
  
  return JNI_VERSION_1_6;
}

