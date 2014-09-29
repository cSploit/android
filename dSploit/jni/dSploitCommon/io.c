/* LICENSE
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "export.h"
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
    fprintf(stderr, "%s: read: %s\n", __func__, strerror(errno));
  else if(pos == end)
    return 0;
  else if(pos > buff) // 0 bytes read == fd closed
    fprintf(stderr, "%s: cannot read %d bytes, got %d\n", __func__, size, (unsigned int)(pos - buff));
    
  
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
    fprintf(stderr, "%s: write: %s\n", __func__, strerror(errno));
  else if(pos == end)
    return 0;
  else if(pos>buff) // 0 bytes wrote == fd closed
    fprintf(stderr, "%s: cannot write %d bytes, sent %d\n", __func__, size, (unsigned int)(pos - buff));
  
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
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    return NULL;
  }
  
  if(read_wrapper(fd, data, size)) {
    free(data);
    return NULL;
  }
  
  return data;
}