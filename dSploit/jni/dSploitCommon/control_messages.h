/* LICENSE
 * 
 * 
 */

/**
 * @file control_messages.h
 * contains control messages definitions.
 */

#ifndef CONTROL_MESSAGES_H
#define CONTROL_MESSAGES_H

#include <stdint.h>

/// actions performed by the controller
enum ctrl_code {
  AUTH,         ///< authenticate
  AUTH_OK,      ///< authentication complete
  AUTH_FAIL,    ///< authentication failed
  CMD_START,    ///< start a command
  CMD_STARTED,  ///< command started
  CMD_FAIL,     ///< command not started
  CMD_SIGNAL,   ///< send signal to command
  CMD_END,      ///< command ends
  CMD_DIED,     ///< command died
  HNDL_LIST,    ///< get handler definitions
  MOD_LIST,     ///< get modules definitions
  MOD_INFO,     ///< module definition
  MOD_START,    ///< start a module
  MOD_END,      ///< stop a module
};

/// id of the controller receiver
#define CTRL_ID 0x00

/// message is for controller ?
#define IS_CTRL(message) (message->head.id == CTRL_ID)

/* --- command structures --- */

/// start a command
struct cmd_start_info {
  char cmd_action;  ///< must be set to ::CMD_START
  char hid;         ///< id of the ::handler for this command
  char argv[];      ///< command argv, a list of null-terminated strings
};

/// command has been successfully started
struct cmd_started_info {
  char cmd_action;  ///< must be set to ::CMD_STARTED
  uint16_t id;      ///< child id for the the started command
};

/// command start request has failed
struct cmd_fail_info {
  char cmd_action;  ///< must be set to ::CMD_FAIL
};

/// send a signal to a command
struct cmd_signal_info {
  char cmd_action;  ///< must be set to ::CMD_SIGNAL
  uint16_t id;      ///< child id to send the signal to
  int signal;       ///< the signal to send
};

/// command has exited
struct cmd_end_info {
  char cmd_action;    ///< must be set to ::CMD_END
  uint16_t id;        ///< id of the child that managed the exited command
  int8_t exit_value;  ///< the value returned by the command
};

/// command has died ( killed by a signal )
struct cmd_died_info {
  char cmd_action;  ///< must be set to ::CMD_DIED
  uint16_t id;      ///< id of the child that managed the dead command
  int signal;       ///< the signal that killed the command
};

/* --- authenticator structures --- */

struct auth_info {
  char auth_code;   ///< must be set to ::AUTH
  char data[];      ///< string array of username and password hash
};

struct auth_status_info {
  char auth_code;   ///< must be set to ::AUTH_FAIL or ::AUTH_OK
};

/* --- handler structures --- */

struct hndl_info {
  uint8_t id;             ///< handler id
  
  uint8_t have_stdin:1;   ///< does it have stdin ?
  uint8_t have_stdout:1;  ///< does it have stdout ?
  uint8_t reserved:6;
  
  char name[];            ///< an human readable identifier
};

struct hndl_list_info {
  char hndl_code;           ///< must be set to ::HNDL_LIST
  struct hndl_info list[];  ///< a list of ::hndl_info
};

/* --- modules structures --- */

struct mod_info {
  //TODO
};

struct mod_list_info {
  //TODO
};

struct mod_start_info {
  //TODO
};

struct mod_end_info {
  //TODO
};

#endif