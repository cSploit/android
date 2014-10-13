/* LICENSE
 * 
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "cache.h"
#include "child.h"
#include "str_array.h"

#include "nmap.h"
#include "hydra.h"
#include "ettercap.h"

#include "event.h"

/**
 * @brief create an it.evilsocket.dsploit.events.Newline
 * @param c the child that send us the line
 * @param arg the new line
 */
jobject create_newline_event(JNIEnv *env, child_node *c, void *arg) {
  jstring str;
  jobject newline;
  
  str = NULL;
  newline = NULL;
  
  str = (*env)->NewStringUTF(env, (const char *)arg);
  
  if(!str) goto error;
  
  newline =  (*env)->NewObject(env,
                               cache.dsploit.events.newline.class,
                               cache.dsploit.events.newline.ctor,
                               c->id, str);
  if(newline) goto cleanup;
  
  error:
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  cleanup:
  
  if(str)
    (*env)->DeleteLocalRef(env, str);
  
  return newline;
}

/**
 * @brief create an it.evilsocket.dsploit.events.ChildEnd
 * @param c the ended child
 * @param arg a pointer to the exit status
 */
jobject create_child_end_event(JNIEnv *env, child_node *c, void *arg) {
  jobject event;
  
  event = (*env)->NewObject(env,
                            cache.dsploit.events.child_end.class,
                            cache.dsploit.events.child_end.ctor,
                            c->id, *((int8_t *) arg));
  
  if(event)
    return event;
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  return NULL;
}

/**
 * @brief create an it.evilsocket.dsploit.events.ChildDied
 * @param c the died child
 * @param arg a poitner to the signal that caused the death
 * @returns 
 */
jobject create_child_died_event(JNIEnv *env, child_node *c, void *arg) {
  jobject event;
  
  event = (*env)->NewObject(env,
                            cache.dsploit.events.child_died.class,
                            cache.dsploit.events.child_died.ctor,
                            c->id, *((int *) arg));
  
  if(event)
    return event;
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  return NULL;
}

/**
 * @brief convert a in_addr_t to a java.net.InetAddress
 * @param a the address to convert
 * @returns a new jobject on success, NULL on error.
 */
jobject inaddr_to_inetaddress(JNIEnv *env, in_addr_t a) {
  jobject res;
  jbyteArray ary;
  
  ary = (*env)->NewByteArray(env, sizeof(a));
  
  if(!ary)
    return NULL;
  
  (*env)->SetByteArrayRegion(env, ary, 0, sizeof(a), (const jbyte *) &a);
  
  res = (*env)->CallStaticObjectMethod(env, cache.java.net.inetaddress.class,
                                       cache.java.net.inetaddress.getByAddress, NULL, ary, 0);
  
  (*env)->DeleteLocalRef(env, ary);
  
  if(!res && (*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return res;
}

/**
 * @brief create an it.evilsocket.dsploit.events.Hop
 * @param c the child that send us this hop
 * @param arg a pointer to an ::nmap_hop_info
 * @returns the jobject on success, NULLl on error.
 */
jobject create_hop_event(JNIEnv *env, child_node *c, void *arg) {
  jobject addr, res;
  struct nmap_hop_info *hop_info;
  
  hop_info = (struct nmap_hop_info*)arg;
  
  addr = inaddr_to_inetaddress(env, hop_info->address);
  
  if(!addr2)
    return NULL;
  
  res = (*env)->NewObject(env,
                          cache.dsploit.events.hop.class,
                          cache.dsploit.events.hop.ctor,
                          c->id, hop_info->hop, hop_info->usec, addr);
  
  (*env)->DeleteLocalRef(env, addr);
  
  if(!res && (*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return res;
}

/**
 * @brief create an it.evilsocket.dsploit.events.Port
 * @param c the child that send us this info
 * @param arg a poiner to the received ::message
 * @returns a new object on success, NULL on error.
 */
jobject create_port_event(JNIEnv *env, child_node *c, void *arg) {
  jobject res;
  jstring jproto;
  jstring jservice, jversion;
  message *m;
  struct nmap_service_info *service_info;
  char *pos;
  
  res = NULL;
  jservice = jversion = NULL;
  m = (message *) arg;
  service_info = (struct nmap_service_info *) m->data;
  
  switch(service_info->proto) {
    case TCP:
      jproto = (*env)->NewStringUTF(env, "tcp");
      break;
    case UDP:
      jproto = (*env)->NewStringUTF(env, "udp");
      break;
    case ICMP:
      jproto = (*env)->NewStringUTF(env, "icmp");
      break;
    case IGMP:
      jproto = (*env)->NewStringUTF(env, "igmp");
      break;
    default:
      LOGW("%s: unrecognized port protocol: %02hhX", __func__, service_info->proto);
    case UNKNOWN:
      jproto = (*env)->NewStringUTF(env, "unknown");
      break;
  }
  
  if(!jproto) goto cleanup;
  
  pos = string_array_next(m, service_info->service, NULL);
  
  if(pos) {
    jservice = (*env)->NewStringUTF(env, pos);
    if(!jservice) goto cleanup;
  }
  
  pos = string_array_next(m, service_info->service, pos);
  
  if(pos) {
    jversion = (*env)->NewStringUTF(env, pos);
    if(!jversion) goto cleanup;
  }
  
  res = (*env)->NewObject(env,
                          cache.dsploit.events.port.class,
                          cache.dsploit.events.port.ctor,
                          c->id, jproto, service_info->port, jservice, jversion);
  
  cleanup:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  
  if(jversion)
    (*env)->DeleteLocalRef(env, jversion);
  
  if(jservice)
    (*env)->DeleteLocalRef(env, jservice);
  
  if(jproto)
    (*env)->DeleteLocalRef(env, jproto);
  
  return res;
}

/**
 * @brief create an it.evilsocket.dsploit.events.Os
 * @param c the child that send us this info
 * @param arg a pointer to ::nmap_os_info
 * @returns a new object on success, NULL on error.
 */
jobject create_os_event(JNIEnv *env, child_node *c, void *arg) {
  jstring jos, jtype;
  jobject res;
  struct nmap_os_info *os_info;
  char *pos;
  
  res = NULL;
  jtype = jos = NULL;
  
  os_info = (struct nmap_os_info *) arg;
  
  for(pos=os_info->os;*pos!='\0';pos++);
  
  jos = (*env)->NewStringUTF(env, os_info->os);
  
  if(!jos) goto cleanup;
  
  jtype = (*env)->NewStringUTF(env, pos + 1);
  
  if(!jtype) goto cleanup;
  
  res = (*env)->NewObject(env,
                          cache.dsploit.events.os.class,
                          cache.dsploit.events.os.ctor,
                          c->id, os_info->accuracy, jos, jtype);
  
  cleanup:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  if(jtype)
    (*env)->DeleteLocalRef(env, jtype);
  
  if(jos)
    (*env)->DeleteLocalRef(env, jos);
  
  return res;
}

/**
 * @brief send an event to java.
 * @param e the event to send
 * @returns 0 on success, -1 on error.
 */
int send_event(JNIEnv *env, jobject e) {
  (*env)->CallStaticVoidMethod(env, cache.dsploit.core.system.class,
                               cache.dsploit.core.system.on_event, e);
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return -1;
  }
  return 0;
}