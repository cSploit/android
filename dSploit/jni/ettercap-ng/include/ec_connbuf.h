
/* $Id: ec_connbuf.h,v 1.6 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_CONNBUF_H
#define EC_CONNBUF_H

#include <ec_inet.h>
#include <ec_threads.h>

struct conn_buf {
   /* the lock */
   pthread_mutex_t connbuf_mutex;
   /* max buffer size */
   size_t max_size;
   /* actual buffer size */
   size_t size;
   /* the real buffer made up of a tail of packets */
   TAILQ_HEAD(connbuf_head, conn_pck_list) connbuf_tail;
};

/* an entry in the tail */
struct conn_pck_list {
   /* size of the element (including the struct size) */
   size_t size;
   /* the source of the packet */
   struct ip_addr L3_src;
   /* the data */
   u_char *buf;
   /* the link to the next element */
   TAILQ_ENTRY(conn_pck_list) next;
};

/* functions */
EC_API_EXTERN void connbuf_init(struct conn_buf *cb, size_t size);
EC_API_EXTERN int connbuf_add(struct conn_buf *cb, struct packet_object *po);
EC_API_EXTERN void connbuf_wipe(struct conn_buf *cb);
EC_API_EXTERN int connbuf_print(struct conn_buf *cb, void (*)(u_char *, size_t, struct ip_addr *));


#endif

/* EOF */

// vim:ts=3:expandtab

