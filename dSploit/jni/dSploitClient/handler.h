/* LICENSE
 * 
 */
#ifndef HANDLER_H
#define HANDLER_H

#include <stdint.h>
#include <jni.h>

#include "list.h"
#include "control.h"

struct message;

jboolean jload_handlers(JNIEnv *, jclass);
void unload_handlers(void);
int on_handler(struct message *);
jobjectArray get_handlers(JNIEnv *, jclass);

typedef struct handler {
  node *next;
  
  uint8_t id;             ///< handler id
  
  uint8_t have_stdin:1;   ///< does it have stdin ?
  uint8_t have_stdout:1;  ///< does it have stdout ?
  uint8_t reserved:6;
  
  char *name;             ///< an human readable identifier
} handler;

enum handlers_loading_status {
  HANDLER_UNKOWN, // inital value ( 0x0 )
  HANDLER_WAIT,
  HANDLER_OK
};

extern struct handlers_list {
  data_control control;
  list list;
  struct {
    handler *blind;
    handler *raw;
    handler *nmap;
    handler *ettercap;
    handler *hydra;
    handler *arpspoof;
  } by_name;              ///< access handlers by name
  enum handlers_loading_status status;
} handlers;

#endif
