/* LICENSE
 * 
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "list.h"
#include "control.h"
#include "reaper.h"

struct graveyard graveyard;
pthread_t reaper_tid = 0;

/**
 * @brief the reaper. he will join all threads in the graveyard.
 */
void *reaper(void *arg) {
  dead_node *zombie;
  void *exit_val;
  
  pthread_mutex_lock(&(graveyard.control.mutex));
  
  while(graveyard.control.active || graveyard.list.head) {
  
    while(!(graveyard.list.head) && graveyard.control.active) {
      pthread_cond_wait(&(graveyard.control.cond), &(graveyard.control.mutex));
    }
    
    zombie = (dead_node *) queue_get(&(graveyard.list));
    
    
    if(zombie) {
      pthread_mutex_unlock(&(graveyard.control.mutex));
      
      if(pthread_join(zombie->tid, &exit_val)) {
        fprintf(stderr, "%s: pthread_join: %s\n", __func__, strerror(errno));
      }
#ifdef NDEBUG
      else if(exit_val)
        fprintf(stderr, "%s: thread joined. (exit_val=%p)\n", __func__, exit_val);
#else
      else {
        printf("%s: thread \"%s\" joined. (exit_val=%p, tid=%lu)\n", __func__, zombie->name, exit_val, zombie->tid);
        free(zombie->name);
      }
#endif
      free(zombie);
      
      pthread_mutex_lock(&(graveyard.control.mutex));
    }
  }
  
  return 0;
}

/**
 * @brief start the @reaper thread
 * @returns 0 on success, -1 on error
 */
int start_reaper() {
  
  if(reaper_tid)
    return 0;
  
  if(control_activate(&(graveyard.control))) {
    fprintf(stderr, "%s: cannot activate the graveyard\n", __func__);
    return -1;
  }
  
  if(pthread_create(&(reaper_tid), NULL, &reaper, NULL)) {
    fprintf(stderr, "%s: pthread_create: %s\n", __func__, strerror(errno));
    reaper_tid=0;
    return -1;
  }
  
  return 0;
}

/**
 * @brief stop the @reaper thread
 * @returns 0 on success, -1 on error
 */
int stop_reaper() {
  
  void *exit_val;
  
  if(!reaper_tid)
    return 0;
  
  if(control_deactivate(&(graveyard.control))) {
    fprintf(stderr, "%s: cannot deactivate the graveyard\n", __func__);
    return -1;
  }
  
  if(pthread_join(reaper_tid, &exit_val)) {
    fprintf(stderr, "%s: pthread_join: %s\n", __func__, strerror(errno));
    return -1;
  }
#ifdef NDEBUG
  else if(exit_val)
    fprintf(stderr, "%s: reaper joined. (exit_val=%p)\n", __func__, exit_val);
#else
  else
    printf("%s: reaper joined. (exit_val=%p)\n", __func__, exit_val);
#endif
  
  return 0;
}

/**
 * @brief send a thread to the graveyard.
 * @param tid the thread id
 * @param name the thread name
 * @returns 0 on success, -1 on error
 */
 #ifdef NDEBUG
void send_to_graveyard(pthread_t tid) {
#else
void _send_to_graveyard(pthread_t tid, char *name) {
#endif  
  dead_node *d;
  
  d=malloc(sizeof(dead_node));
  
  if(!d) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    return;
  }
  memset(d, 0, sizeof(dead_node));
  d->tid = tid;
#ifndef NDEBUG
  d->name = name;
#endif
  
  pthread_mutex_lock(&(graveyard.control.mutex));
  queue_put(&(graveyard.list), (node *) d);
  pthread_mutex_unlock(&(graveyard.control.mutex));
  
  pthread_cond_broadcast(&(graveyard.control.cond));
}
