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
#include "arpspoof.h"
#include "tcpdump.h"
#include "fusemounts.h"
#include "network-radar.h"

#include "event.h"

/**
 * @brief create an org.csploit.android.events.Newline
 * @param arg the new line
 */
jobject create_newline_event(JNIEnv *env, void *arg) {
  jstring str;
  jobject newline;
  
  str = NULL;
  newline = NULL;
  
  str = (*env)->NewStringUTF(env, (const char *)arg);
  
  if(!str) goto jni_error;
  
  newline =  (*env)->NewObject(env,
                               cache.csploit.events.newline.class,
                               cache.csploit.events.newline.ctor,
                               str);
  if(newline) goto cleanup;
  
  jni_error:
  
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
 * @brief create an org.csploit.android.events.StderrNewline
 * @param arg the new line
 */
jobject create_stderrnewline_event(JNIEnv *env, void *arg) {
  jstring str;
  jobject stderr_newline;
  
  str = NULL;
  stderr_newline = NULL;
  
  str = (*env)->NewStringUTF(env, (const char *) arg);
  
  if(!str) goto jni_error;
  
  stderr_newline = (*env)->NewObject(env,
                                     cache.csploit.events.stderrnewline.class,
                                     cache.csploit.events.stderrnewline.ctor,
                                     str);
  
  if(stderr_newline) goto cleanup;
  
  jni_error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  cleanup:
  
  if(str)
    (*env)->DeleteLocalRef(env, str);
  
  return stderr_newline;
}

/**
 * @brief create an org.csploit.android.events.ChildEnd
 * @param arg a pointer to the exit status
 * @returns the jobject on success, NULL on error.
 */
jobject create_child_end_event(JNIEnv *env, void *arg) {
  jobject event;
  
  event = (*env)->NewObject(env,
                            cache.csploit.events.child_end.class,
                            cache.csploit.events.child_end.ctor,
                            *((uint8_t *) arg));
  
  if(event)
    return event;
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  return NULL;
}

/**
 * @brief create an org.csploit.android.events.ChildDied
 * @param arg a poitner to the signal that caused the death
 * @returns the jobject on success, NULL on error.
 */
jobject create_child_died_event(JNIEnv *env, void *arg) {
  jobject event;
  
  event = (*env)->NewObject(env,
                            cache.csploit.events.child_died.class,
                            cache.csploit.events.child_died.ctor,
                            *((unsigned int *) arg));
  
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
 * @brief create an org.csploit.android.events.Hop
 * @param arg a pointer to an ::nmap_hop_info
 * @returns the jobject on success, NULL on error.
 */
jobject create_hop_event(JNIEnv *env, void *arg) {
  jobject addr, res;
  struct nmap_hop_info *hop_info;
  
  hop_info = (struct nmap_hop_info*)arg;
  
  addr = inaddr_to_inetaddress(env, hop_info->address);
  
  if(!addr)
    return NULL;
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.hop.class,
                          cache.csploit.events.hop.ctor,
                          hop_info->hop, hop_info->usec, addr);
  
  (*env)->DeleteLocalRef(env, addr);
  
  if(!res && (*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Port
 * @param arg a poiner to the received ::message
 * @returns a new object on success, NULL on error.
 */
jobject create_port_event(JNIEnv *env, void *arg) {
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
                          cache.csploit.events.port.class,
                          cache.csploit.events.port.ctor,
                          jproto, service_info->port, jservice, jversion);
  
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
 * @brief create an org.csploit.android.events.Os
 * @param arg a pointer to ::nmap_os_info
 * @returns a new object on success, NULL on error.
 */
jobject create_os_event(JNIEnv *env, void *arg) {
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
                          cache.csploit.events.os.class,
                          cache.csploit.events.os.ctor,
                          os_info->accuracy, jos, jtype);
  
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
 * @brief create an org.csploit.android.events.Ready
 * @param arg unused
 * @returns a new object on success, NULL on error.
 */
jobject create_ready_event(JNIEnv *env, void *arg _U_) {
  jobject res;
  
  res = (*env)->NewObject(env,
                        cache.csploit.events.ready.class,
                        cache.csploit.events.ready.ctor);
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Account
 * @param arg the ::message containing the account info
 * @returns a new object on success, NULL on error.
 */
jobject create_account_event(JNIEnv *env, void *arg) {
  jstring jproto, juser, jpswd;
  jobject res, addr;
  struct ettercap_account_info *account_info;
  message *m;
  char *pos;
  
  res = NULL;
  jproto = juser = jpswd = NULL;
  m = (message *) arg;
  
  account_info = (struct ettercap_account_info *) m->data;
  
  addr = inaddr_to_inetaddress(env, account_info->address);
  
  if(!addr) return NULL;
  
  pos = account_info->data;
  
  jproto = (*env)->NewStringUTF(env, pos);
  
  if(!jproto) goto cleanup;
  
  pos = string_array_next(m, account_info->data, pos);
  
  juser = (*env)->NewStringUTF(env, pos);
  
  if(!juser) goto cleanup;
  
  pos = string_array_next(m, account_info->data, pos);
  
  jpswd = (*env)->NewStringUTF(env, pos);
  
  if(!jpswd) goto cleanup;
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.account.class,
                          cache.csploit.events.account.ctor,
                          addr, jproto, juser, jpswd);
  
  cleanup:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  if(addr)
    (*env)->DeleteLocalRef(env, addr);
  
  if(jproto)
    (*env)->DeleteLocalRef(env, jproto);
  
  if(juser)
    (*env)->DeleteLocalRef(env, juser);
  
  if(jpswd)
    (*env)->DeleteLocalRef(env, jpswd);
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Ready
 * @param m the received ::message
 * @returns a new object on success, NULL on error.
 */
jobject create_message_event(JNIEnv *env, message *m) {
  jobject res;
  char *severity, *message;
  jstring jseverity, jmessage;
  
  jseverity = jmessage = NULL;
  res = NULL;
  
  switch(m->data[0]) {
    case HYDRA_WARNING:
      severity = "WARNING";
      message = ((struct hydra_warning_info *) m->data)->text;
      break;
    case ARPSPOOF_ERROR:
    case HYDRA_ERROR:
      severity = "ERROR";
      message = ((struct hydra_error_info *) m->data)->text;
      break;
    default:
      LOGE("%s: unknown message code %02hhX", __func__, m->data[0]);
      return NULL;
  }
  
  jseverity = (*env)->NewStringUTF(env, severity);
  if(!jseverity) goto cleanup;
  
  jmessage = (*env)->NewStringUTF(env, message);
  if(!jmessage) goto cleanup;
      
  
  res = (*env)->NewObject(env,
                        cache.csploit.events.message.class,
                        cache.csploit.events.message.ctor,
                        jseverity, jmessage);
  
  cleanup:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  if(jseverity)
    (*env)->DeleteLocalRef(env, jseverity);
  
  if(jmessage)
    (*env)->DeleteLocalRef(env, jmessage);
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Attempts
 * @param arg a pointer to a ::message containing an ::hydra_attempts_info
 * @returns a new object on success, NULL on error.
 */
jobject create_attempts_event(JNIEnv *env, message *m) {
  jobject res;
  struct hydra_attempts_info *attempts_info;
  
  attempts_info = (struct hydra_attempts_info *) m->data;
  
  res = (*env)->NewObject(env,
                        cache.csploit.events.attempts.class,
                        cache.csploit.events.attempts.ctor,
                        attempts_info->sent, attempts_info->left,
                        attempts_info->rate, attempts_info->elapsed,
                        attempts_info->eta);
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Login
 * @param m the received ::message
 * @returns a new object on success, NULL on error.
 */
jobject create_login_event(JNIEnv *env, message *m) {
  jstring jlogin, jpswd;
  jobject res, addr;
  struct hydra_login_info *login_info;
  char *pos;
  
  res = addr = NULL;
  pos = NULL;
  jlogin = jpswd = NULL;
  
  login_info = (struct hydra_login_info *) m->data;
  
  if(login_info->contents & HAVE_ADDRESS) {
    addr = inaddr_to_inetaddress(env, login_info->address);
    if(!addr) return NULL;
  }
  
  if(login_info->contents & HAVE_LOGIN) {
    pos = string_array_next(m, login_info->data, pos);
    
    jlogin = (*env)->NewStringUTF(env, pos);
    if(!jlogin) goto cleanup;
  }
  
  if(login_info->contents & HAVE_PASSWORD) {
    pos = string_array_next(m, login_info->data, pos);
    
    jpswd = (*env)->NewStringUTF(env, pos);
    if(!jpswd) goto cleanup;
  }
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.login.class,
                          cache.csploit.events.login.ctor,
                          login_info->port, addr, jlogin, jpswd);
  
  cleanup:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  if(addr)
    (*env)->DeleteLocalRef(env, addr);
  
  if(jlogin)
    (*env)->DeleteLocalRef(env, jlogin);
  
  if(jpswd)
    (*env)->DeleteLocalRef(env, jpswd);
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Packet
 * @param m the received message
 * @returns the jobject on success, NULL on error.
 */
jobject create_packet_event(JNIEnv *env, message *m) {
  jobject src, dst, res;
  struct tcpdump_packet_info *packet_info;
  
  packet_info = (struct tcpdump_packet_info *) m->data;
  
  src = inaddr_to_inetaddress(env, packet_info->src);
  if(!src) return NULL;
  
  dst = inaddr_to_inetaddress(env, packet_info->dst);
  if(!dst) return NULL;
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.packet.class,
                          cache.csploit.events.packet.ctor,
                          src, dst, packet_info->len);
  
  (*env)->DeleteLocalRef(env, src);
  (*env)->DeleteLocalRef(env, dst);
  
  if(!res && (*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.FuseBind
 * @param m the received message
 * @returns the jobject on success, NULL on error.
 */
jobject create_fusebind_event(JNIEnv *env, message *m) {
  jobject res;
  char *src, *mnt;
  jstring *jsrc, *jmnt;
  struct fusemount_bind_info *bind_info;
  
  bind_info = (struct fusemount_bind_info *) m->data;
  jsrc = jmnt = NULL;
  
  src = string_array_next(m, bind_info->data, NULL);
  
  if(!src) {
    LOGE("%s: source not found", __func__);
    return NULL;
  }
  
  mnt = string_array_next(m, bind_info->data, src);
  
  if(!mnt) {
    LOGE("%s: mountpoint not found", __func__);
    return NULL;
  }
  
  jsrc = (*env)->NewStringUTF(env, src);
  
  if(!jsrc) goto jni_error;
  
  jmnt = (*env)->NewStringUTF(env, mnt);
  
  if(!jmnt) goto jni_error;
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.fusebind.class,
                          cache.csploit.events.fusebind.ctor,
                          jsrc, jmnt);
  
  goto cleanup;
  
  jni_error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  cleanup:
  
  if(jsrc)
    (*env)->DeleteLocalRef(env, jsrc);
  
  if(jmnt)
    (*env)->DeleteLocalRef(env, jmnt);
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.Host
 * @param m the received message
 * @returns the jobject on success, NULL on error.
 */
jobject create_host_event(JNIEnv *env, message *m) {
  jstring jname;
  jobject res, ip_addr;
  jbyteArray eth_addr;
  struct nrdr_host_info *hinfo;
  char *pos;
  
  res = ip_addr = NULL;
  pos = NULL;
  jname = NULL;
  
  hinfo = (struct nrdr_host_info *) m->data;
  
  ip_addr = inaddr_to_inetaddress(env, hinfo->ip_addr);
  if(!ip_addr) return NULL;
  
  eth_addr = (*env)->NewByteArray(env, 6);
  if(!eth_addr) goto cleanup;
  
  (*env)->SetByteArrayRegion(env, eth_addr, 0, 6, (const jbyte *) hinfo->eth_addr);
  
  pos = string_array_next(m, hinfo->name, pos);
  
  if(pos) {
    jname = (*env)->NewStringUTF(env, hinfo->name);
    if(!jname) goto cleanup;
  }
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.host.class,
                          cache.csploit.events.host.ctor,
                          eth_addr, ip_addr, jname);
  
  cleanup:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  if(ip_addr)
    (*env)->DeleteLocalRef(env, ip_addr);
  
  if(eth_addr)
    (*env)->DeleteLocalRef(env, eth_addr);
  
  if(jname)
    (*env)->DeleteLocalRef(env, jname);
  
  return res;
}

/**
 * @brief create an org.csploit.android.events.HostLost
 * @param m the received message
 * @returns the jobject on success, NULL on error.
 */
jobject create_hostlost_event(JNIEnv *env, message *m) {
  jobject res, addr;
  struct nrdr_host_del_info *hinfo;
  
  hinfo = (struct nrdr_host_del_info *) m->data;
  
  addr = inaddr_to_inetaddress(env, hinfo->ip_addr);
  if(!addr) return NULL;
  
  res = (*env)->NewObject(env,
                          cache.csploit.events.hostlost.class,
                          cache.csploit.events.hostlost.ctor,
                          addr);
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  (*env)->DeleteLocalRef(env, addr);
  
  return res;
}

/**
 * @brief send an event to java.
 * @param c the child that generate this event
 * @param e the event to send
 * @returns 0 on success, -1 on error.
 */
int send_event(JNIEnv *env, child_node *c, jobject e) {
  (*env)->CallStaticVoidMethod(env, cache.csploit.core.childmanager.class,
                               cache.csploit.core.childmanager.on_event, c->id, e);
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return -1;
  }
  return 0;
}
