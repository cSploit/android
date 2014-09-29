/* LICENSE
 * 
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <pthread.h>
#include <jni.h>

struct message;

int on_control(JNIEnv *, struct message *);

extern pthread_mutex_t ctrl_seq_lock;
extern uint16_t ctrl_seq;

#endif