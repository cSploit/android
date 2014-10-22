/* LICENSE
 * 
 */
#ifndef COMMAND_H
#define COMMAND_H

#include "message.h"

int on_command(JNIEnv *, message *);
int start_blind_command(JNIEnv *, jclass, jstring);
int start_command(JNIEnv *, jclass, jstring, jstring);
void kill_child(JNIEnv *, jclass, int, int);
#endif