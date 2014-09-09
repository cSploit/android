/*
 * the usual list handling in C
 * 
 * LICENSE
 */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "reader.h"

r_info *readers = NULL;

pthread_mutex_t r_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief the reader thread. it keep reading from a file descriptor for us.
 */
void *fd_reader(void *arg) {
  r_info *info = ((r_info *)arg);
  
  while((info->size = read(info->fd, info->buff, READER_BUFF_SIZE)) > 0) {
    sem_wait(&(info->consumed));
    info->size=0;
  }
  if(info->size) {
    fprintf(stderr, "%s(%d): read: %s\n", __func__, info->fd, strerror(errno));
    return (void *) -1;
  }
  return 0;
}

/**
 * @brief create a reader thread
 * @returns a ::r_info shared with the created thread, NULL on error.
 */
r_info *create_reader(int fd) {
  r_info *info;
  
  info = malloc(sizeof(r_info));
  
  if(!info) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    return NULL;
  }
  
  info->fd = fd;
  info->buff = malloc(READER_BUFF_SIZE);
  info->size = 0;
  
  if(!(info->buff)) {
    fprintf(stderr, "%s: malloc: %s\n", __func__, strerror(errno));
    free(info);
    return NULL;
  }
  
  if(sem_init(&(info->consumed), 0, 0)) {
    fprintf(stderr, "%s: sem_init: %s\n", __func__, strerror(errno));
    free(info->buff);
    free(info);
    return NULL;
  }
  
  if(pthread_create(&(info->tid), NULL, &fd_reader, (void *)info)) {
    fprintf(stderr, "%s: pthread_create: %s\n", __func__, strerror(errno));
    free(info->buff);
    sem_destroy(&(info->consumed));
    free(info);
    return NULL;
  }
  return info;
}


/**
 * @brief add a reader to the list
 * @param head the list head
 * @param item the reader to add
 * @returns the new head
 */
r_info *add_reader(r_info * head, r_info *item) {
  register r_info *r;
  
  if(head)
    for(r=head;r->next;r=r->next);
  else
    return item;
  r->next = item;
  return head;
}

/**
 * @brief remove a reader from the list. it does not free the removed @p item.
 * @param head the list head
 * @param item the connectino to remove
 * @returns the new head
 */
r_info *del_reader(r_info *head, r_info *item) {
  register r_info *r;
  
  if(head && item) {
    if(head->tid == item->tid)
      head = head->next;
    else {
      for(r=head;r->next;r=r->next) {
        if(r->next->tid == item->tid) {
          r->next=item->next;
          break;
        }
      }
    }
  }
  return head;
}

/**
 * @brief free all readers from list
 * @param head the list head
 */
void free_readers(r_info *head) {
  register r_info *r,*p;
  
  for(r=head;r;) {
    p=r;
    r=r->next;
    free_reader(p);
  }
}

/**
 * @brief fre a ::r_info, without any check.
 * @param item the r_info th free
 */
void _free_reader(r_info *item) {
  if(item->buff)
    free(item->buff);
  
  free(item);
}

/**
 * @brief free a ::r_info
 * @param item the reader to free
 */
void free_reader(r_info *item) {
  
  if(!item)
    return;
  
  close(item->fd);

  if(IS_ALIVE(item->tid)) {  
    // reader thread keeps blocks on read(2),
    // no chance of a clear shutdown, we must cancel it.
    pthread_cancel(item->tid);
  }
  pthread_join(item->tid, NULL);
  
#if DEBUG
  printf("%s: fd_reader(%d) joined\n", __func__, item->fd);
#endif
  
  _free_reader(item);
}
