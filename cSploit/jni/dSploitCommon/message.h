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

message *create_message(uint16_t , uint16_t , uint16_t );
message *read_message(int);
void free_message(message *);
char *message_to_string(message *);
void _dump_message(const char *, message *);
message *msgdup(message *);
int send_message(int, message *);

#define dump_message(m) _dump_message(__func__, m)

#endif
