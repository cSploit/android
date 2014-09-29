/*LICENSE
 * 
 * 
 * 
 */

#ifndef BUFFER_H
#define BUFFER_H

#include "export.h"

typedef struct {
  char    *buffer;  ///< pointer to buffer
  size_t  size;     ///< buffer size
} buffer;

char *get_line_from_buffer(buffer *);
int append_to_buffer(buffer *, char *, int);
void release_buffer(buffer *);

#endif
