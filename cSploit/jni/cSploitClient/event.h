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

#ifndef EVENT_H
#define EVENT_H

#include <jni.h>
#include "message.h"


jobject create_newline_event(JNIEnv *, void *);
jobject create_child_end_event(JNIEnv *, void *);
jobject create_child_died_event(JNIEnv *, int);
jobject create_stderrnewline_event(JNIEnv *, void *);
jobject create_hop_event(JNIEnv *, void *);
jobject create_port_event(JNIEnv *, void *);
jobject create_os_event(JNIEnv *, void *);
jobject create_ready_event(JNIEnv *, void *);
jobject create_account_event(JNIEnv *, void *);
jobject create_message_event(JNIEnv *, message *);
jobject create_login_event(JNIEnv *, message *);
jobject create_attempts_event(JNIEnv *, message *);
jobject create_packet_event(JNIEnv *, message *);
jobject create_fusebind_event(JNIEnv *, message *);
jobject create_host_event(JNIEnv *, message *);
jobject create_hostlost_event(JNIEnv *, message *);
int send_event(JNIEnv *, child_node *, jobject);

#endif
