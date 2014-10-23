/* LICENSE
 * 
 */
#ifndef AUTH_H
#define AUTH_H

#include <jni.h>

#include "control.h"

int on_auth_status(message *);
inline int authenticated(void);
jboolean is_authenticated(JNIEnv *, jclass);
jboolean jlogin(JNIEnv *, jclass, jstring, jstring );
void auth_on_disconnect(void);
void auth_on_connect(void);

enum login_status {
  LOGIN_WAIT,
  LOGIN_OK,
  LOGIN_FAIL
};

extern struct login_data {
  data_control control;
  enum login_status status;
} logged;

#endif
