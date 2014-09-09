#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "list.h"
#include "control.h"

/// internal struct for manage connection data.
typedef struct conn_node {
  node *next;
  
  pthread_t tid; ///< thread id that read and write from this connection
  
  int fd;        ///< file descriptor associated with that connection.
} conn_node;

void close_connections(void);
int serve_new_client(int);

extern struct connection_list {
  data_control control;
  list list;
} connections;


#endif
