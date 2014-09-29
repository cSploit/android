/* LICENSE
 * 
 */
#ifndef CHILD_H
#define CHILD_H

#include <jni.h>
#include <arpa/inet.h>

#include "list.h"
#include "control.h"
#include "buffer.h"

struct handler;

typedef struct {
  node *next;
  
  uint16_t  id;             ///< child id
  uint8_t   pending;        ///< is this child pending for CMD_STARTED / CMD_FAIL ?
  uint16_t  seq;            ///< pending message sequence number
  
  struct handler *handler;  ///< handler for this child
  
  in_addr_t target;         ///< target of this child
  
  buffer    buffer;         ///< ::buffer for received output
  
} child_node;

extern struct child_list {
  list list;
  data_control control;
} children;

child_node *create_child(uint16_t );
inline child_node *get_child_by_id(uint16_t );
inline child_node *get_child_by_pending_seq(uint16_t);
void free_child(child_node *);
extern jboolean send_to_child(JNIEnv *, jclass, int, jbyteArray);

#endif