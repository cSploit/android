/* LICENSE
 * 
 * 
 */

#ifndef LIST_H
#define LIST_H

/* part of this code has been taken from:
 * Gentoo Linux Documentation - POSIX threads explained
 */

/**
 * @brief a node of the list
 * 
 * a very smart way to handle lists, just have a struct with the usual "next" pointer as first field.
 * in this way any other struct that have "next" as first field can safely casted to this one.
 */
typedef struct node {
  struct node *next;
} node;

/**
 * @brief a struct for hold list head and tail.
 */
typedef struct list {
  node *head, *tail;
} list;

void list_del(list *, node *);
void list_add(list *, node *);
void list_ins(list *, node *);

node *queue_get(list *);
#define queue_put(l, n) list_add(l, n)

#define stack_push(l, n) list_ins(l, n)
#define stack_pop(l, n)  queue_get(l, n)

#endif
