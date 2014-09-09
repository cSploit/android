/* LICENSE
 * 
 */

#ifndef CONTROL_H
#define CONTROL_H

typedef struct data_control {
  pthread_mutex_t mutex;  ///< mutex to access associated data
  pthread_cond_t cond;    ///< sync threads that use that data
  char active;            ///< activate the associated data
} data_control;

int control_init(data_control *);
int control_destroy(data_control *);

int control_activate(data_control *);
int control_deactivate(data_control *);

#endif