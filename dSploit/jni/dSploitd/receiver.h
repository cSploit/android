/* LICENSE
 * 
 */

#ifndef RECEIVER_H
#define RECEIVER_H

#include "msgqueue.h"

int start_receiver(void);
int stop_receiver(void);

extern struct msgqueue incoming_messages;

#endif