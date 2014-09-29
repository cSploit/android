/* LICENSE
 * 
 * 
 */

#ifndef READER_H
#define READER_H

#include "msgqueue.h"

int start_reader(void);
void stop_reader(void);

extern struct msgqueue incoming_messages;

#endif