/*LICENSE
 * 
 * 
 * 
 */

#ifndef COMMON_H
#define COMMON_H

#include <signal.h>
#include <pthread.h>

char *read_chunk(int , unsigned int );
int read_wrapper(int , void *, unsigned int);
int write_wrapper(int , void *, unsigned int);
void *pthread_join_w_timeout(pthread_t, unsigned int);

/// is a thread alive?
#define IS_ALIVE(tid) (!pthread_kill(tid, 0))

#endif
