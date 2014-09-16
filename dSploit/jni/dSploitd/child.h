/* LICENSE
 * 
 * 
 */

#ifndef CHILD_H
#define CHILD_H

#include "list.h"
#include "control.h"
#include "buffer.h"

struct handler;

/// info about one child
typedef struct child_node {
  node *next;
  
  uint16_t id;                  ///< child id
  pthread_t tid;                ///< stdout reader thread
  struct handler *handler;      ///< handler for this child
  pid_t pid;                    ///< process managed by this child
  int stdin;                    ///< stdin file descritor of the process
  int stdout;                   ///< stdout file descritor of the process
  
  uint16_t seq;                 ///< sent messages sequence number
  buffer output_buff;           ///< ::buffer for ::handler.raw_output_parser
} child_node;

child_node *create_child(void);
void *handle_child(void *);
void stop_children(void);

extern struct child_list {
  data_control control;
  list list;
} children;

/// buffer size for process stdout
#define STDOUT_BUFF_SIZE 2048

/// usecs to wait between waitpid() calls
#define CHILD_WAIT_INTERVAL 1000

#endif