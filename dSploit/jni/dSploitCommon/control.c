/* LICENSE
 * 
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "export.h"
#include "control.h"

/* part of this code has been taken from:
 * Gentoo Linux Documentation - POSIX threads explained
 */

/**
 * @brief initialize a ::data_control
 * @param mycontrol the ::data_control to initilize
 * @returns 0 on success, -1 on error
 */
int control_init(data_control *mycontrol) {
  if (pthread_mutex_init(&(mycontrol->mutex),NULL)) {
    fprintf(stderr, "%s: pthread_mutex_init: %s\n", __func__, strerror(errno));
    return -1;
  }
  if (pthread_cond_init(&(mycontrol->cond),NULL)) {
    fprintf(stderr, "%s: pthread_cond_init: %s\n", __func__, strerror(errno));
    return -1;
  }
  mycontrol->active=1;
  return 0;
}

/**
 * @brief destroy a ::data_control
 * @param mycontrol the ::data_control to destroy
 * @returns 0 on success, -1 on error
 */
int control_destroy(data_control *mycontrol) {
  if (pthread_cond_destroy(&(mycontrol->cond))) {
    fprintf(stderr, "%s: pthread_cond_destroy: %s\n", __func__, strerror(errno));
    return -1;
  }
  if (pthread_mutex_destroy(&(mycontrol->mutex))) {
    fprintf(stderr, "%s: pthread_mutex_destroy: %s\n", __func__, strerror(errno));
    return -1;
  }
  mycontrol->active=0;
  return 0;
}

/**
 * @brief set ::data_control.active to 1
 * @param mycontrol the ::data_control to activate
 * @returns 0 on success, -1 on error
 */
int control_activate(data_control *mycontrol) {
  if (pthread_mutex_lock(&(mycontrol->mutex))) {
    fprintf(stderr, "%s: pthread_mutex_lock: %s\n", __func__, strerror(errno));
    return -1;
  }
  mycontrol->active=1;
  pthread_mutex_unlock(&(mycontrol->mutex));
  pthread_cond_broadcast(&(mycontrol->cond));
  return 0;
}

/**
 * @brief set ::data_control.active to 0
 * @param mycontrol the ::data_control to deactivate
 * @returns 0 on success, -1 on error.
 */
int control_deactivate(data_control *mycontrol) {
  if (pthread_mutex_lock(&(mycontrol->mutex))) {
    fprintf(stderr, "%s: pthread_mutex_lock: %s\n", __func__, strerror(errno));
    return -1;
  }
  mycontrol->active=0;
  pthread_mutex_unlock(&(mycontrol->mutex));
  pthread_cond_broadcast(&(mycontrol->cond));
  return 0;
}