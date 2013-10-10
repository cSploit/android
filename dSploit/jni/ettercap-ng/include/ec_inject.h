
/* $Id: ec_inject.h,v 1.11 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_INJECT_H
#define EC_INJECT_H

#include <ec_packet.h>
#include <ec_conntrack.h>

#define FUNC_INJECTOR_PTR(func) int (*func)(struct packet_object *, size_t *)
#define FUNC_INJECTOR(func) int func(struct packet_object *po, size_t *len)

#define LENGTH *len
#define LENGTH_PTR len

#define SESSION_PASSTHRU(x,y) do{   \
   x->prev_session = y->session;    \
   y->session = x;                  \
} while(0)

#define SESSION_CLEAR(x) x->session=NULL

#define EXECUTE_INJECTOR(x,y) do{                     \
   FUNC_INJECTOR_PTR(prev_injector);                  \
   prev_injector = get_injector(x, y);                \
   if (prev_injector == NULL)                         \
      return -ENOTFOUND;                              \
   if (prev_injector(PACKET, LENGTH_PTR) != ESUCCESS) \
      return -ENOTHANDLED;                            \
} while(0)

EC_API_EXTERN int inject_buffer(struct packet_object *po);
EC_API_EXTERN void add_injector(u_int8 level, u_int32 type, FUNC_INJECTOR_PTR(injector));
EC_API_EXTERN void * get_injector(u_int8 level, u_int32 type);
EC_API_EXTERN void inject_split_data(struct packet_object *po);

EC_API_EXTERN int user_kill(struct conn_object *co);
EC_API_EXTERN int user_inject(u_char *buf, size_t size, struct conn_object *co, int which);

#define CHAIN_ENTRY 1
#define CHAIN_LINKED 2

/* Used to trace inject from udp to ip */
#define STATELESS_IP_MAGIC 0x0304e77e

#endif

/* EOF */

// vim:ts=3:expandtab

