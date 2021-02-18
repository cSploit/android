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
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "logger.h"
#include "io.h"

/**
 * @brief ensure to read @p size bytes from @p fd into @p buff
 * @param fd the file descriptor to read from
 * @param buff the buffer where store the readed bytes
 * @param size the number of bytes to read
 * @returns 0 on success, -1 on error
 */
int read_wrapper(int fd, void *buff, unsigned int size) {
  void *pos, *end;
  int read_bytes;
  
  pos=buff;
  end=(buff + size);
  read_bytes=0;
  do {
    read_bytes=read(fd, pos, end-pos);
    pos+=read_bytes;
  } while(read_bytes > 0 && pos < end);
  
  if(read_bytes < 0)
    print( ERROR, "read: %s", strerror(errno) );
  else if(pos == end)
    return 0;
  else if(pos > buff) // 0 bytes read == fd closed
    print( ERROR, "cannot read %d bytes, got %d", size, (unsigned int)(pos - buff) );
    
  
  return -1;
}

/**
 * @brief ensure to write @p size bytes to @p fd from @p buff
 * @param fd the file descriptor to write to
 * @param buff the source buffer
 * @param size the number of bytes to write
 * @returns 0 on success, -1 on error
 */
int write_wrapper(int fd, void *buff, unsigned int size) {
  void *pos, *end;
  int write_bytes;
  
  pos=buff;
  end=(buff + size);
  write_bytes=0;
  do {
    write_bytes=write(fd, pos, end-pos);
    pos+=write_bytes;
  } while(write_bytes > 0 && pos < end);
  
  if(write_bytes < 0)
    print( ERROR, "write: %s", strerror(errno) );
  else if(pos == end)
    return 0;
  else if(pos>buff) // 0 bytes wrote == fd closed
    print( ERROR, "cannot write %d bytes, sent %d", size, (unsigned int)(pos - buff) );
  
  return -1;
}

/**
 * @brief read a @p size of bytes from a @p fd.
 * @param fd the file descriptor to read from
 * @param size the amount of bytes to read
 * @returns a pointer to the readed bytes, or NULL on error.
 */
char *read_chunk(int fd, unsigned int size) {
  char *data;
  
  data = malloc(size);
  if(!data) {
    print( ERROR, "malloc: %s", strerror(errno) );
    return NULL;
  }
  
  if(read_wrapper(fd, data, size)) {
    free(data);
    return NULL;
  }
  
  return data;
}