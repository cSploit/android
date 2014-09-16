/*
 * LICENSE
 * 
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"

/**
 * @brief get a line from a buffer.
 * @param b the ::buffer to read from
 * @returns a string containing the newline or NULL if not found.
 */
char *get_line_from_buffer(buffer *b) {
  char *end, *line, *newbuff;
  register char *newline;
  size_t line_len;
  
  if(!b || !b->buffer || !b->size)
    return NULL;
  
  end = b->buffer + b->size;
  for(newline=b->buffer;newline<end && *newline != '\n';newline++);
  
  if(newline == end)
    return NULL;
  
  line_len = (newline - b->buffer);
  b->size -= line_len + 1;
  
  if(b->size) {
    newbuff = malloc(b->size);
    
    if(!newbuff) {
      fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
      b->size += line_len + 1;
      return NULL;
    }
    
    memcpy(newbuff, b->buffer + line_len + 1, b->size);
  } else {
    newbuff = NULL;
  }
  
  line = realloc(b->buffer, line_len + 1);
  if(!line) { 
    // this should be impossible, new size is lesser then the current one.
    // manage it anyway, just in case.
    fprintf(stderr, "%s: realloc: %s\n", __func__, strerror(errno));
    if(newbuff)
      free(newbuff);
    b->size += line_len + 1;
    return NULL;
  }
  
  b->buffer = newbuff; 
  *(line + line_len) = '\0';
  
  return line;
}

/**
 * @brief append @p count bytes from @p buff to @p b.
 * @param b destination ::buffer
 * @param buff source buffer
 * @param count bytes to process
 * @returns 0 on success, -1 on error.
 */
int append_to_buffer(buffer *b, char *buff, int count) {
  char *newbuff;
  
  b->size += count;
  
  newbuff = realloc(b->buffer, b->size);
  
  if (!newbuff) {
    fprintf(stderr, "%s: realloc: %s\n", __func__, strerror(errno));
    free(b->buffer);
    b->buffer = NULL;
    b->size = 0;
    return -1;
  }
  
  b->buffer = newbuff;
  
  memcpy(b->buffer + ( b->size - count ), buff, count);
  
  return 0;
}
