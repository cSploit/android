/*
 * LICENSE
 * 
 * 
 */

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <linux/limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#include "common.h"

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

/**
 * @brief join @p tid or cancel it timed out.
 * @param tid the thread id to join
 * @param milliseconds amount of milliseconds for the timeout.
 * @returns the value returned by the joined thread ( PTHREAD_CANCELLED if timed out ).
 */
void *pthread_join_w_timeout(pthread_t tid, unsigned int milliseconds) {
  struct timeval timeout, cur;
  void *ret;
  
  
  if(IS_ALIVE(tid)) {
    if(gettimeofday(&timeout, NULL)) {
      fprintf(stderr, "%s: gettimeofday: %s\n", __func__, strerror(errno));
    } else {
      cur.tv_sec = 0;
      cur.tv_usec = (1000 * milliseconds);
      timeradd(&timeout, &cur, &timeout);
      while(IS_ALIVE(tid)) {
        if(gettimeofday(&cur, NULL)) {
          fprintf(stderr, "%s: second gettimeofday: %s\n", __func__, strerror(errno));
        } else if(!timercmp(&timeout, &cur, <)) { // bug workaround, see man timercmp
          fprintf(stderr, "%s: thread #%lu timed out\n", __func__, tid);
        } else {
          usleep(1000);
          continue;
        }
        pthread_cancel(tid);
        break;
      }
    }
  }
  
  if(pthread_join(tid, &ret)) {
    fprintf(stderr, "%s: pthread_join: %s\n", __func__, strerror(errno));
    ret = (void *) -1;
  }
  
  return ret;
}
