/*
    ettercap -- connection buffer module

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ec_connbuf.c,v 1.7 2004/02/08 19:58:37 alor Exp $
*/

#include <ec.h>
#include <ec_packet.h>
#include <ec_connbuf.h>

/* mutexes */

#define CONNBUF_INIT_LOCK(x)  do{ pthread_mutex_init(&x, NULL); }while(0)
#define CONNBUF_LOCK(x)       do{ pthread_mutex_lock(&x); }while(0)
#define CONNBUF_UNLOCK(x)     do{ pthread_mutex_unlock(&x); }while(0)

/* protos */

void connbuf_init(struct conn_buf *cb, size_t size);
int connbuf_add(struct conn_buf *cb, struct packet_object *po);
void connbuf_wipe(struct conn_buf *cb);
int connbuf_print(struct conn_buf *cb, void (*)(u_char *, size_t, struct ip_addr *L3_src));

/************************************************/

/*
 * initialize the buffer
 */
void connbuf_init(struct conn_buf *cb, size_t size)
{
   DEBUG_MSG("connbuf_init");
  
   /* init the size */
   cb->size = 0;
   cb->max_size = size;
   /* init the tail */
   TAILQ_INIT(&cb->connbuf_tail);
   /* init the mutex */
   CONNBUF_INIT_LOCK(cb->connbuf_mutex);
}

/* 
 * add the packet to the conn_buf.
 * check if the buffer has reached the max size
 * and in case delete the oldest elements to 
 * fit the predefined size.
 *
 * the tail has the newer packet in the head and
 * older in the tail.
 */
int connbuf_add(struct conn_buf *cb, struct packet_object *po)
{
   struct conn_pck_list *p;
   struct conn_pck_list *e;

   SAFE_CALLOC(p, 1, sizeof(struct conn_pck_list));
   
   /* 
    * we add the sizeof because if the packets have 0 length
    * (ack packets) the real memory occupation will overflow
    * the max_size
    */
   p->size = sizeof(struct conn_pck_list) + po->DATA.disp_len;
  
   memcpy(&p->L3_src, &po->L3.src, sizeof(struct ip_addr));

   /* 
    * we cant handle the packet, the buffer
    * is too small
    */
   if (p->size > cb->max_size) {
      DEBUG_MSG("connbuf_add: buffer too small %d %d\n", (int)cb->max_size, (int)p->size);      
      SAFE_FREE(p);
      return 0;
   }
      
   /* copy the buffer */
   SAFE_CALLOC(p->buf, po->DATA.disp_len, sizeof(u_char));
   
   memcpy(p->buf, po->DATA.disp_data, po->DATA.disp_len);

   CONNBUF_LOCK(cb->connbuf_mutex);
   
   /* 
    * check the total size and make adjustment 
    * if we have to free some packets
    */
   if (cb->size + p->size > cb->max_size) {
      struct conn_pck_list *tmp = NULL;
      
      TAILQ_FOREACH_REVERSE_SAFE(e, &cb->connbuf_tail, next, connbuf_head, tmp) {
         
         /* we have freed enough bytes */
         if (cb->size + p->size <= cb->max_size)
            break;
         
         /* calculate the new size */
         cb->size -= e->size;
         /* remove the elemnt */
         SAFE_FREE(e->buf);
         TAILQ_REMOVE(&cb->connbuf_tail, e, next);
         SAFE_FREE(e);
      }
   }
   
   /* insert the packet in the tail */
   TAILQ_INSERT_HEAD(&cb->connbuf_tail, p, next);
      
   /* update the total buffer size */
   cb->size += p->size;

   CONNBUF_UNLOCK(cb->connbuf_mutex);

   return 0;
}

/*
 * empty a give buffer.
 * all the elements in the list are deleted
 */
void connbuf_wipe(struct conn_buf *cb)
{
   struct conn_pck_list *e;

   DEBUG_MSG("connbuf_wipe");
   
   CONNBUF_LOCK(cb->connbuf_mutex);
   
   /* delete the list */
   while ((e = TAILQ_FIRST(&cb->connbuf_tail)) != TAILQ_END(&cb->connbuf_tail)) {
      TAILQ_REMOVE(&cb->connbuf_tail, e, next);
      SAFE_FREE(e->buf);
      SAFE_FREE(e);
   }

   /* reset the buffer */
   cb->size = 0;
   TAILQ_INIT(&cb->connbuf_tail);
   
   CONNBUF_UNLOCK(cb->connbuf_mutex);
}

/* 
 * print the content of a buffer
 * you can print only one side of the communication
 * by specifying the L3_src address, or NULL to 
 * print all the packet in order (joined view).
 *
 * returns the number of printed chars
 */
int connbuf_print(struct conn_buf *cb, void (*func)(u_char *, size_t, struct ip_addr *L3_src))
{
   struct conn_pck_list *e;
   int n = 0;
  
   CONNBUF_LOCK(cb->connbuf_mutex);
   
   /* print the buffer */
   TAILQ_FOREACH_REVERSE(e, &cb->connbuf_tail, next, connbuf_head) {
      /* 
       * remember that the size is comprehensive
       * of the struct size
       */
      func(e->buf, e->size - sizeof(struct conn_pck_list), &e->L3_src);
      n += e->size - sizeof(struct conn_pck_list);
   }
   
   CONNBUF_UNLOCK(cb->connbuf_mutex);
   
   return n;
}

/* EOF */

// vim:ts=3:expandtab

