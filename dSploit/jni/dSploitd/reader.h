#ifndef READER_H
#define READER_H

#include <pthread.h>

#include "handler.h"

/// struct for share data with the readers
typedef struct reader_info {
  pthread_t tid;    ///< the reader thread
  int fd;           ///< the file descriptor to read from
  
  char *buff;       ///< the shared memory for readed data
  int size;         ///< amount of readed bytes
  
  sem_t consumed;   ///< semaphore to syncronize buffer reader and writer.
  
  h_info *parent;   ///< the handler that read from this thread.
  
  struct reader_info *next;
} r_info;

r_info *create_reader(int);
r_info *add_reader(r_info *, r_info *);
r_info *del_reader(r_info *, r_info *);
void free_readers(r_info *);
void _free_reader(r_info *);
void free_reader(r_info *);

/// the size of the read buffer
#define READER_BUFF_SIZE 4096

/// readers list
extern r_info *readers;

/// mutex to access readers list
extern pthread_mutex_t r_lock;

#endif
