/*
 * LICENSE
 * 
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

/// the head of a message
struct msg_header {
  uint16_t seq;   ///< the message sequence number
  uint16_t size;  ///< the message size
  uint16_t id;    ///< the received id
};

/// the message
typedef struct message {
  struct msg_header head; ///< the message head
  char * data;            ///< the message body
} message;

/// byte to separate strings inside message->data
#define STRING_SEPARATOR '\0'

message *create_message(uint16_t , uint16_t , uint16_t );
message *read_message(int);
void free_message(message *);
void dump_message(message *);
message *msgdup(message *);
int send_message(int, message *);

#endif
