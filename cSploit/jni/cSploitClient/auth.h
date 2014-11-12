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
