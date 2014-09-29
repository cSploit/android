/* LICENSE
 * 
 * 
 * 
 */

#include <stddef.h>
#include <assert.h>

#include "export.h"
#include "list.h"

/* part of this code has been taken from:
 * Gentoo Linux Documentation - POSIX threads explained
 */

/**
 * @brief insert @p n in @p l
 * @param l the list
 * @param n the node
 */
void list_ins(list *l, node *n) {
  
  n->next = l->head;
  l->head = n;
  if(!n->next)
    l->tail = n;
}

/**
 * @brief add @p n to @p l
 * @param l the list
 * @param n the node
 */
void list_add(list *l, node *n) {
  
  n->next=NULL;
  
  if(l->tail)
    l->tail->next=n;
  
  l->tail = n;
  
  if(!l->head)
    l->head=n;
}

/**
 * @brief remove @p n from @p l
 * @param l the list
 * @param n the node
 */
void list_del(list *l, node *n) {
  register node *tmp;
  
  assert(n!=NULL);
  
  if(!l->head) {
    return;
  } else if(n==l->head) {
    if(n==l->tail) {
      l->tail = l->head = NULL;
    } else {
      l->head=l->head->next;
    }
  } else {
    for(tmp=l->head;tmp && tmp->next != n;tmp=tmp->next);
    if(tmp) {
      tmp->next=n->next;
      if(l->tail==n)
        l->tail=tmp;
    }
  }
}

/**
 * @brief pop a node from the queue. (FIFO)
 * @param q the queue
 * @returns the first node of @p q
 */
node *queue_get(list *q) {
  node *n;
  
  n=q->head;
  
  if(q->head) {
    q->head = q->head->next;
    if(!q->head)
      q->tail = NULL;
  }
  
  return n;
}
