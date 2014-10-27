/* LICENSE
 * 
 * 
 */

#include <jni.h>
#include <string.h>

#include "log.h"
#include "cache.h"

JavaVM *jvm = NULL;
struct cache cache;

int init_class_cache(JNIEnv *env, struct class_cache *c, const char *class_name) {
  jclass local_class;
  int ret;
  
  ret = -1;
  
  local_class = (*env)->FindClass(env, class_name);
  if(!local_class) goto error;
  
  c->class = (*env)->NewGlobalRef(env, local_class);
  if(!c->class) goto error;
  
  ret = 0;
  goto cleanup;
  
  error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  cleanup:
  if(local_class)
    (*env)->DeleteLocalRef(env, local_class);
  
  return ret;
}

int init_dsploit_core_childmanager_cache(JNIEnv *env) {
  struct core_chlidmanager_cache *c;
  
  c = &(cache.dsploit.core.childmanager);
  
  if(init_class_cache(env, (struct class_cache *) c, "it/evilsocket/dsploit/core/ChildManager"))
    return -1;
  
  c->on_event = (*env)->GetStaticMethodID(env, c->class, "onEvent",
                                                 "(ILit/evilsocket/dsploit/events/Event;)V");
  
  if(!c->on_event) goto error;
  
  return 0;
  
  error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return -1;
}
  
int init_dsploit_core_client_cache(JNIEnv *env) {
  struct class_cache *c;
  
  c = &(cache.dsploit.core.client);
  
  if(init_class_cache(env, c, "it/evilsocket/dsploit/core/Client"))
    return -1;
  
  return 0;
}

int init_class_and_ctor_cache(JNIEnv *env, struct class_and_ctor_cache *c,
                              const char *class, const char *ctor_signature) {
  
  if(init_class_cache(env, (struct class_cache *) c, class))
    return -1;
  
  c->ctor = (*env)->GetMethodID(env, c->class, "<init>", ctor_signature);
  if(!c->ctor) goto error;
  
  return 0;
  
  error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return -1;
}

#define NUMELEM(a) (sizeof(a)/sizeof(a[0]))

int init_dsploit_events_cache(JNIEnv *env) {
  struct class_and_ctor_signature {
    const char *class_name;
    const char *ctor_signature;
  } events[] = {
    { "it/evilsocket/dsploit/events/Newline", "(Ljava/lang/String;)V" },
    { "it/evilsocket/dsploit/events/ChildEnd", "(I)V" },
    { "it/evilsocket/dsploit/events/ChildDied", "(I)V" },
    { "it/evilsocket/dsploit/events/Hop", "(IJLjava/net/InetAddress;)V" },
    { "it/evilsocket/dsploit/events/Port", "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;)V" },
    { "it/evilsocket/dsploit/events/Os", "(SLjava/lang/String;Ljava/lang/String;)V" },
    { "it/evilsocket/dsploit/events/Ready", "()V" },
    { "it/evilsocket/dsploit/events/Account", "(Ljava/net/InetAddress;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" },
    { "it/evilsocket/dsploit/events/Message", "(Ljava/lang/String;Ljava/lang/String;)V" },
    { "it/evilsocket/dsploit/events/Login", "(ILjava/net/InetAddress;Ljava/lang/String;Ljava/lang/String;)V" },
    { "it/evilsocket/dsploit/events/Attempts", "(JJJJJ)V" },
    { "it/evilsocket/dsploit/events/Packet", "(Ljava/net/InetAddress;Ljava/net/InetAddress;S)V" },
  };
  struct class_and_ctor_cache *ptr;
  register int i;
  
  ptr=(struct class_and_ctor_cache *) &(cache.dsploit.events);
  for(i=0;i<NUMELEM(events);i++, ptr++)
    if(init_class_and_ctor_cache(env, ptr, events[i].class_name, events[i].ctor_signature))
      return -1;
  
  return 0;
}

int init_dsploit_core_cache(JNIEnv *env) {
  if(init_dsploit_core_childmanager_cache(env))
    return -1;
  if(init_dsploit_core_client_cache(env))
    return -1;
  return 0;
}

int init_dsploit_cache(JNIEnv *env) {
  if(init_dsploit_core_cache(env))
    return -1;
  if(init_dsploit_events_cache(env))
    return -1;
  return 0;
}

int init_java_net_inetaddress_cache(JNIEnv *env) {
  struct java_net_inetaddress_cache *c;
  
  c = &(cache.java.net.inetaddress);
  
  if(init_class_cache(env, (struct class_cache *) c, "java/net/InetAddress"))
    return -1;
  
  c->getByAddress = (*env)->GetStaticMethodID(env, c->class,
                                       "getByAddress", "(Ljava/lang/String;[BI)Ljava/net/InetAddress;");
  
  if(!c->getByAddress) goto error;
  
  return 0;
  
  error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return -1;
}

int init_java_net_cache(JNIEnv *env) {
  if(init_java_net_inetaddress_cache(env))
    return -1;
  return 0;
}

int init_java_cache(JNIEnv *env) {
  if(init_java_net_cache(env))
    return -1;
  return 0;
}

int _init_cache(JNIEnv *env) {
  if(init_java_cache(env))
    return -1;
  if(init_dsploit_cache(env))
    return -1;
  return 0;
}

int init_cache(JavaVM *pVm) {
  int ret;
  JNIEnv *env;
  
  ret = (*pVm)->GetEnv(pVm, (void **)&env, JNI_VERSION_1_6);
  
  if(ret != JNI_OK) {
    LOGF("%s: GetEnv: %d", __func__, ret);
    return -1;
  }
  
  if(_init_cache(env))
    return -1;  
  
  jvm = pVm;
  
  return 0;
  
}

void _free_cache(JNIEnv *env) {
  int i;
  jclass *global_refs[] = {
    &(cache.java.net.inetaddress.class),
    &(cache.dsploit.core.childmanager.class),
    &(cache.dsploit.core.client.class),
    &(cache.dsploit.events.newline.class),
    &(cache.dsploit.events.child_end.class),
    &(cache.dsploit.events.child_died.class),
    &(cache.dsploit.events.hop.class),
    &(cache.dsploit.events.port.class),
    &(cache.dsploit.events.os.class),
  };
  
  for(i=0;i<NUMELEM(global_refs); i++) {
    (*env)->DeleteGlobalRef(env, *(global_refs[i]));
  }
}

void free_cache() {
  JNIEnv *env;
  int i;
  
  if(!jvm)
    return;
  
  i=(*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6);
  
  if(i != JNI_OK) {
    LOGE("%s: GetEnv: %d", __func__, i);
    return;
  }
  
  _free_cache(env);
  
  jvm = NULL;
}
