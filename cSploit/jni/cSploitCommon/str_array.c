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

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "logger.h"
#include "message.h"

#include "str_array.h"

/**
 * @file str_array.c
 * 
 * this file contain a set of function to work with serialized string array.
 * serialized string arrays are appended to messages, which defines their termination
 * using ::mesg_header.size .
 */

/**
 * @brief append @p string to the string array in @p m
 * @param m the ::message to work with.
 * @param array_offset start offset of the string array insise ::message.data
 * @param string the string to append
 * @returns 0 on success, -1 on error.
 * 
 * append @p string to the array of null-terminated string at @p array_offset
 * 
 * WARNING: any previous reference to m->data will be invalid
 *          use this function as last write call on you struct
 */
int string_array_add(message *m, size_t array_offset, char *string) {
  char *end, *prev, *array_start, *array_end;
  int32_t needed_bytes;
  
  assert(m!=NULL);
  
  end = m->data + m->head.size;
  array_start = m->data + array_offset;
  
  assert(m->data <= array_start && array_start <= end);
  
  for(array_end=array_start;
      array_end<end && ( *array_end!='\0' || ( prev && prev!='\0' ))
      ;prev=array_end++);
  
  needed_bytes = (strlen(string) + 1) - (end - array_end);
  
  if(needed_bytes > 0) {
    needed_bytes += m->head.size;
    if(needed_bytes > UINT16_MAX) {
      print( ERROR, "message too long (string=\"%s\")", string );
      return -1;
    }
    
    prev = realloc(m->data, needed_bytes);
    if(!prev) {
      print( ERROR, "realloc: %s", string );
      return -1;
    }
    m->data = prev;
    m->head.size = (uint16_t) needed_bytes;
    // recompute it. realloc can move memory around
    array_end = m->data + array_offset + (array_end - array_start);
  }
  
  strncpy(array_end, string, strlen(string) + 1);
  
  return 0;
}

/**
 * @brief get number of strings contained in the string array.
 * @param m the message containing the string array
 * @param array_start start of the array in @p m
 * @returns the number of strings stored in the string array
 */
int string_array_size(message *m, char *array_start) {
  
  char *end,*prev,*pos;
  int n;
  
  assert(m!=NULL);
  
  end = m->data + m->head.size;
  
  assert(m->data <= array_start && array_start <= end);
  
  prev = NULL;
  n=0;
  for(pos=array_start;pos<end;prev=pos++)
    if(*pos == '\0' && (!prev || *prev != '\0'))
      n++;
  
  return n;
}

/**
 * @brief get next string from string array contained in @p m
 * @param m the ::message to work with
 * @param array_start start of the string array
 * @param current_string pointer to current position in the array
 * @returns a pointer to next string on success, NULL if empty.
 */
char *string_array_next(message *m, char *array_start, char *current_string) {
  char *end, *pos;
  
  assert(m!=NULL);
  
  end = m->data + m->head.size;
  
  assert(m->data <= array_start && array_start <= end);
  
  if(array_start == end)
    return NULL;
  
  if(!current_string)
    return array_start;
  
  assert(array_start <= current_string && current_string < end);
  
  for(pos=current_string;pos<end && *pos!='\0'; pos++);
  
  if(pos==end || ++pos == end)
    return NULL;
  
  return pos;
}
