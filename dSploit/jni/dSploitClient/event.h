/* LICENSE
 * 
 */

#ifndef EVENT_H
#define EVENT_H

#include <jni.h>

jobject create_newline_event(JNIEnv *, void *);
jobject create_child_end_event(JNIEnv *, void *);
jobject create_child_died_event(JNIEnv *, void *);
jobject create_hop_event(JNIEnv *, void *);
jobject create_port_event(JNIEnv *, void *);
jobject create_os_event(JNIEnv *, void *);
jobject create_ready_event(JNIEnv *, void *);
jobject create_account_event(JNIEnv *, void *);
int send_event(JNIEnv *, child_node *, jobject);

#endif