/* LICENSE
 * 
 * 
 */

#ifndef REAPER_H
#define REAPER_H

#include <pthread.h>

#include "list.h"
#include "control.h"

typedef struct dead_node {
  node *next;
  pthread_t tid;
#ifndef NDEBUG
  char *name;
#endif
} dead_node;

extern struct graveyard {
  data_control control;
  list list;
} graveyard;

int start_reaper(void);
int stop_reaper(void);
#ifdef NDEBUG
void send_to_graveyard(pthread_t);
#else
void _send_to_graveyard(pthread_t, char *);
#define send_to_graveyard(t) _send_to_graveyard(t, strdup(__func__))
#endif


/// add yourself to the death list
#define send_me_to_graveyard() send_to_graveyard(pthread_self())

#endif
