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
#include <string.h>

#include "handler.h"
#include "logger.h"
#include "msfrpcd.h"
#include "message.h"

handler handler_info = {
  NULL,                       // next
  9,                          // handler id
  0,                          // have_stdin
  1,                          // have_stdout
  1,                          // enabled
  NULL,                       // raw_output_parser
  &msfrpcd_output_parser,     // output_parser
  NULL,                       // input_parser
  "msfrpcd",                  // argv[0]
  NULL,                       // workdir
  "msfrpcd"                   // handler name
};

#define READY_MESSAGE "[*] MSGRPC ready at"
#define READY_MESSAGE_LENGTH 19

/**
 * @brief search for the ready message in msfrpcd output.
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *msfrpcd_output_parser(char *line) {
  message *m;
  
  if(strncmp(READY_MESSAGE, line, READY_MESSAGE_LENGTH))
    return NULL;
  
  m = create_message(0, 1, 0);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return NULL;
  }
  
  m->data[0] = MSFRPCD_READY;
  
  return m;
}
