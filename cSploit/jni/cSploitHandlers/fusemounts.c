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
#include <regex.h>

#include "handler.h"
#include "logger.h"
#include "fusemounts.h"
#include "message.h"
#include "str_array.h"

handler handler_info = {
  NULL,                           // next
  7,                              // handler id
  0,                              // have_stdin
  1,                              // have_stdout
  1,                              // enabled
  NULL,                           // raw_output_parser
  &fusemounts_output_parser,      // output_parser
  NULL,                           // input_parser
  "tools/fusemounts/fusemounts",  // argv[0]
  NULL,                           // workdir
  "fusemounts"                    // handler name
};

/**
 * @brief extract fusemount source and destination from fusemounts output
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *fusemounts_output_parser(char *line) {
  message *m;
  char *dst, *ptr;
  
  if(!*line) {
    return NULL;
  }
  
  for(dst=line;*dst!=' ' && *dst!='\0';dst++);
  
  if(*dst) {
    *dst='\0';
    dst++;
  }
  
  for(;*dst==' ';dst++);
  
  if(!*dst)
    return NULL;
  
  for(ptr=dst;*ptr!=' ' && *ptr!='\0';ptr++);
  *ptr='\0';
  
  m = create_message(0, sizeof(struct fusemount_bind_info), 0);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return NULL;
  }
  
  m->data[0] = FUSEMOUNT_BIND;
  
  if(string_array_add(m, offsetof(struct fusemount_bind_info, data), line)) {
    print( ERROR, "cannot append string to message" );
    goto error;
  }
  
  if(string_array_add(m, offsetof(struct fusemount_bind_info, data), dst)) {
    print( ERROR, "cannot append string to message" );
    goto error;
  }
  
  return m;
  
  error:
  free_message(m);
  
  return NULL;
}