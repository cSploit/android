/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
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
