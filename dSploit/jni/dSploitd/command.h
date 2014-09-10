/*
 * 
 * LICENSE
 */

#ifndef COMMAND_H
#define COMMAND_H

#include "message.h"
#include "child.h"

/// actions performed by the command manager
enum cmd_action {
  CMD_START,    ///< start a command
  CMD_STARTED,  ///< command started
  CMD_FAIL,     ///< command not started
  CMD_SIGNAL,   ///< send signal to command
  CMD_END,      ///< command ends
  CMD_DIED,     ///< command died
};

/// id of the command receiver
#define COMMAND_RECEIVER_ID 0x00

/// message is a command ?
#define IS_COMMAND(message) (message->head.id == COMMAND_RECEIVER_ID)

/// start a command
struct cmd_start_info {
  char cmd_action;  ///< must be set to @CMD_START
  char hid;         ///< id of the @handler for this command
  char argv[];      ///< command argv, separed with ::STRING_SEPARATOR
};

/// command has been successfully started
struct cmd_started_info {
  char cmd_action;  ///< must be set to @CMD_STARTED
  uint16_t id;      ///< child id for the the started command
};

/// command start request has failed
struct cmd_fail_info {
  char cmd_action;  ///< must be set to @CMD_FAIL
};

/// send a signal to a command
struct cmd_signal_info {
  char cmd_action;  ///< must be set to @CMD_SIGNAL
  uint16_t id;      ///< child id to send the signal to
  int signal;       ///< the signal to send
};

/// command has exited
struct cmd_end_info {
  char cmd_action;    ///< must be set to @CMD_END
  uint16_t id;        ///< id of the child that managed the exited command
  int8_t exit_value;  ///< the value returned by the command
};

/// command has died ( killed by a signal )
struct cmd_died_info {
  char cmd_action;  ///< must be set to @CMD_DIED
  uint16_t id;      ///< id of the child that managed the dead command
  int signal;       ///< the signal that killed the command
};

int handle_command(struct message *);
int notify_child_done(child_node *, int );

#endif
