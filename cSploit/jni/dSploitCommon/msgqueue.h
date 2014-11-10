/* LICENSE
 * 
 * 
 */

#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include "list.h"
#include "control.h"
#include "message.h"

typedef struct msg_node {
  node *next;
  message *msg;   ///< the enqueued message
} msg_node;

struct msgqueue {
  data_control control;
  list list;
};
  
int enqueue_message(struct msgqueue *, message *);
#endif