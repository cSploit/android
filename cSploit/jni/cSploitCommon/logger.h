/* LICENSE
 * 
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>

enum loglevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL,
};

extern void std_logger(int, char *, ...);

extern pthread_mutex_t print_lock;
extern void (*_print)(int, char *, ...);

#define print( level, fmt, args...) _print( level, "%s: " fmt, __func__, ##args)
#define set_logger(func) _print = func

#endif