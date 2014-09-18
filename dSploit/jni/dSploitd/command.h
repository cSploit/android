/*
 * 
 * LICENSE
 */

#ifndef COMMAND_H
#define COMMAND_H

#include "message.h"
#include "child.h"
#include "cmd.h"

int handle_command(message *);
int notify_child_done(child_node *, int );

#endif
