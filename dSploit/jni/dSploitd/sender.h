/* LICENSE
 * 
 */

#ifndef SENDER_H
#define SENDER_H

#include "msgqueue.h"

int start_sender(void);
int stop_sender(void);

extern struct msgqueue outcoming_messages; 

#endif