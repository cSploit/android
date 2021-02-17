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

int init_csploit_core_childmanager_cache(JNIEnv *env) {
  struct core_chlidmanager_cache *c;
  
  c = &(cache.csploit.core.childmanager);
  
  if(init_class_cache(env, (struct class_cache *) c, "org/csploit/android/core/ChildManager"))
    return -1;
  
  c->on_event = (*env)->GetStaticMethodID(env, c->class, "onEvent",
                                                 "(ILorg/csploit/android/events/Event;)V");
  
  if(!c->on_event) goto error;
  
  return 0;
  
  error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return -1;
}
  
int init_csploit_core_client_cache(JNIEnv *env) {
  struct class_cache *c;
  
  c = &(cache.csploit.core.client);
  
  if(init_class_cache(env, c, "org/csploit/android/core/Client"))
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

int init_csploit_events_cache(JNIEnv *env) {
  struct class_and_ctor_signature {
    const char *class_name;
    const char *ctor_signature;
  } events[] = {
    { "org/csploit/android/events/Newline", "(Ljava/lang/String;)V" },
    { "org/csploit/android/events/ChildEnd", "(I)V" },
    { "org/csploit/android/events/ChildDied", "(I)V" },
    { "org/csploit/android/events/StderrNewline", "(Ljava/lang/String;)V" },
    { "org/csploit/android/events/Hop", "(IJLjava/net/InetAddress;)V" },
    { "org/csploit/android/events/Port", "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;)V" },
    { "org/csploit/android/events/Os", "(SLjava/lang/String;Ljava/lang/String;)V" },
    { "org/csploit/android/events/Ready", "()V" },
    { "org/csploit/android/events/Account", "(Ljava/net/InetAddress;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" },
    { "org/csploit/android/events/Message", "(Ljava/lang/String;Ljava/lang/String;)V" },
    { "org/csploit/android/events/Login", "(ILjava/net/InetAddress;Ljava/lang/String;Ljava/lang/String;)V" },
    { "org/csploit/android/events/Attempts", "(JJJJ)V" },
    { "org/csploit/android/events/Packet", "(Ljava/net/InetAddress;Ljava/net/InetAddress;I)V" },
    { "org/csploit/android/events/FuseBind", "(Ljava/lang/String;Ljava/lang/String;)V" },
    { "org/csploit/android/events/Host", "([BLjava/net/InetAddress;Ljava/lang/String;)V" },
    { "org/csploit/android/events/HostLost", "(Ljava/net/InetAddress;)V" },
  };
  struct class_and_ctor_cache *ptr;
  register int i;
  
  ptr=(struct class_and_ctor_cache *) &(cache.csploit.events);
  for(i=0;i<NUMELEM(events);i++, ptr++)
    if(init_class_and_ctor_cache(env, ptr, events[i].class_name, events[i].ctor_signature))
      return -1;
  
  return 0;
}

int init_csploit_core_cache(JNIEnv *env) {
  if(init_csploit_core_childmanager_cache(env))
    return -1;
  if(init_csploit_core_client_cache(env))
    return -1;
  return 0;
}

int init_csploit_cache(JNIEnv *env) {
  if(init_csploit_core_cache(env))
    return -1;
  if(init_csploit_events_cache(env))
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
  if(init_csploit_cache(env))
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
    &(cache.csploit.core.childmanager.class),
    &(cache.csploit.core.client.class),
    &(cache.csploit.events.newline.class),
    &(cache.csploit.events.child_end.class),
    &(cache.csploit.events.child_died.class),
    &(cache.csploit.events.hop.class),
    &(cache.csploit.events.port.class),
    &(cache.csploit.events.os.class),
    &(cache.csploit.events.ready.class),
    &(cache.csploit.events.account.class),
    &(cache.csploit.events.message.class),
    &(cache.csploit.events.login.class),
    &(cache.csploit.events.attempts.class),
    &(cache.csploit.events.packet.class),
    &(cache.csploit.events.fusebind.class),
    &(cache.csploit.events.host.class),
    &(cache.csploit.events.hostlost.class),
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
