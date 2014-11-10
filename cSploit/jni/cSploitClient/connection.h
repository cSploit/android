/* LICENSE
 * 
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include <jni.h>
#include <pthread.h>

extern int sockfd;
extern pthread_mutex_t write_lock;

jboolean connect_unix(JNIEnv *, jclass, jstring);
jboolean is_unix_connected(JNIEnv *, jclass);
void disconnect_unix(JNIEnv *, jclass);
void on_disconnect(void);

#endif
