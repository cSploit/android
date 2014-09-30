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

void register_logger(void (*)(int, char *, ...));

extern pthread_mutex_t print_lock;
extern void (*_print)(int, char *, ...);

#define print( level, fmt, args...) _print( level, "%s: " fmt, __func__, ##args)

#endif