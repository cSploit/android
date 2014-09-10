/* LICENSE
 * 
 * 
 */

#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include "list.h"
#include "control.h"

typedef struct msg_node {
  node *next;
  message *msg;
} msg_node;

extern struct msgqueue {
  data_control control;
  list list;
} outcoming_messages, ///< output message queue
  incoming_messages;  ///< input message queue

  
int enqueue_message(struct msgqueue *, message *);

int start_sender(void);
int start_dispatcher(void);

int stop_sender(void);
int stop_dispatcher(void);
#endif