/*
    ettercap -- TCP/UDP injection module

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

    $Id: ec_inject.c,v 1.16 2004/05/27 10:59:52 alor Exp $
*/

#include <ec.h>
#include <ec_packet.h>
#include <ec_inject.h>
#include <ec_send.h>
#include <ec_session_tcp.h>

/* globals */
static SLIST_HEAD (, inj_entry) injectors_table;

struct inj_entry {
   u_int32 type;
   u_int8 level;
   FUNC_INJECTOR_PTR(injector);
   SLIST_ENTRY (inj_entry) next;
};


/* proto */

int inject_buffer(struct packet_object *po);
void add_injector(u_int8 level, u_int32 type, FUNC_INJECTOR_PTR(injector));
void * get_injector(u_int8 level, u_int32 type);
size_t inject_protocol(struct packet_object *po);
void inject_split_data(struct packet_object *po);

int user_kill(struct conn_object *co);
int user_inject(u_char *buf, size_t size, struct conn_object *co, int which);

/*******************************************/

/*
 * add an injector to the injector table 
 */
void add_injector(u_int8 level, u_int32 type, FUNC_INJECTOR_PTR(injector))
{
   struct inj_entry *e;

   SAFE_CALLOC(e, 1, sizeof(struct inj_entry));

   e->level = level;
   e->type = type;
   e->injector = injector;

   SLIST_INSERT_HEAD (&injectors_table, e, next); 
   
   return;
}

/*
 * get an injector from the injector table 
 */

void * get_injector(u_int8 level, u_int32 type)
{
   struct inj_entry *e;
   
   SLIST_FOREACH (e, &injectors_table, next) {
      if (e->level == level && e->type == type) 
         return (void *)e->injector;
   }

   return NULL;
}


size_t inject_protocol(struct packet_object *po)
{
   FUNC_INJECTOR_PTR(injector);
   size_t len = 0;
      
   injector = get_injector(CHAIN_ENTRY, po->L4.proto);
   
   if (injector == NULL) 
      return 0;

   /* Start the injector chain */
   if (injector(po, &len) == ESUCCESS)
      return len;
      
   /* if there's an error */
   return 0;              
}


/*
 * the idea is that the application will pass a buffer
 * and a len, and this function will split up the 
 * buffer to fit the MTU and inject the resulting packet(s).
 */
int inject_buffer(struct packet_object *po)
{

   /* the packet_object passed is a fake.
    * it is used only to pass:
    *    - IP source and dest
    *    - IPPROTO
    *    - (tcp/udp) port source and dest
    * all the field have to be filled in and the buffer
    * has to be alloc'd
    */       
   struct packet_object *pd;
   size_t injected;
   u_char *pck_buf;
   int ret = ESUCCESS;
  
   /* we can't inject in unoffensive mode or in bridge mode */
   if (GBL_OPTIONS->unoffensive || GBL_OPTIONS->read || GBL_OPTIONS->iface_bridge) {
      return -EINVALID;
   }
   
   /* Duplicate the packet to modify the payload buffer */
   pd = packet_dup(po, PO_DUP_NONE);

   /* Allocate memory for the packet (double sized)*/
   SAFE_CALLOC(pck_buf, 1, (GBL_IFACE->mtu * 2));
         
   /* Loop until there's data to send */
   do {

      /* 
       * Slide to middle. First part is for header's stack'ing.
       * Second part is for packet data. 
       */
      pd->packet = pck_buf + GBL_IFACE->mtu;

      /* Start the injector cascade */
      injected = inject_protocol(pd);

      if (injected == 0) {
         ret = -ENOTHANDLED;
         break;
      }
      
      /* Send on the wire */ 
      send_to_L3(pd);

      /* Ready to inject the rest */
      pd->DATA.inject_len -= injected;
      pd->DATA.inject += injected;
   } while (pd->DATA.inject_len);
   
   /* we cannot use packet_object_destroy because
    * the packet is not yet in the queue to tophalf.
    * so we have to free the duplicates by hand.
    */ 
   SAFE_FREE(pck_buf);
   SAFE_FREE(pd->DATA.disp_data);
   SAFE_FREE(pd);
   
   return ret;
}

void inject_split_data(struct packet_object *po) 
{
   size_t max_len;
   
   max_len = GBL_IFACE->mtu - (po->L4.header - (po->packet + po->L2.len) + po->L4.len);

   /* the packet has exceeded the MTU */
   if (po->DATA.len > max_len) {
      po->DATA.inject = po->DATA.data + max_len;
      po->DATA.inject_len = po->DATA.len - max_len;
      po->DATA.delta -= po->DATA.len - max_len;
      po->DATA.len = max_len;
   } 
}

/*
 * kill the connection on user request
 */
int user_kill(struct conn_object *co)
{
   struct ec_session *s = NULL;
   void *ident;
   struct packet_object po;
   size_t ident_len, direction;
   struct tcp_status *status;

   /* we can kill only tcp connection */
   if (co->L4_proto != NL_TYPE_TCP)
      return -EFATAL;
   
   DEBUG_MSG("user_kill");
   
   /* prepare the fake packet object */
   memcpy(&po.L3.src, &co->L3_addr1, sizeof(struct ip_addr));
   memcpy(&po.L3.dst, &co->L3_addr2, sizeof(struct ip_addr));

   po.L4.src = co->L4_addr1;
   po.L4.dst = co->L4_addr2;
   po.L4.proto = co->L4_proto;

   /* retrieve the ident for the session */
   ident_len = tcp_create_ident(&ident, &po);

   /* get the session */
   if (session_get(&s, ident, ident_len) == -ENOTFOUND) {
      SAFE_FREE(ident); 
      return -EINVALID;
   }
   
   DEBUG_MSG("user_kill: session found");
      
   /* Select right comunication way */
   direction = tcp_find_direction(s->ident, ident);
   SAFE_FREE(ident); 

   status = (struct tcp_status *)s->data;
     
   /* send the reset. at least one should work */
   send_tcp(&po.L3.src, &po.L3.dst, po.L4.src, po.L4.dst, htonl(status->way[!direction].last_ack), 0, TH_RST);
   send_tcp(&po.L3.dst, &po.L3.src, po.L4.dst, po.L4.src, htonl(status->way[direction].last_ack), 0, TH_RST);

   return ESUCCESS;
}

/*
 * inject from user
 */
int user_inject(u_char *buf, size_t size, struct conn_object *co, int which)
{
   struct packet_object po;
   
   DEBUG_MSG("user_inject");

   /* prepare the fake packet object */
   if (which == 1) {
      memcpy(&po.L3.src, &co->L3_addr1, sizeof(struct ip_addr));
      memcpy(&po.L3.dst, &co->L3_addr2, sizeof(struct ip_addr));

      po.L4.src = co->L4_addr1;
      po.L4.dst = co->L4_addr2;
   } else {
      memcpy(&po.L3.dst, &co->L3_addr1, sizeof(struct ip_addr));
      memcpy(&po.L3.src, &co->L3_addr2, sizeof(struct ip_addr));

      po.L4.dst = co->L4_addr1;
      po.L4.src = co->L4_addr2;
   }
   po.L4.proto = co->L4_proto;
   
   po.DATA.inject = buf;
   po.DATA.inject_len = size;

   po.packet = NULL;
   po.DATA.disp_data = NULL;
      
   /* do the dirty job */
   inject_buffer(&po);

   /* mark the connection as injected */
   co->flags = CONN_INJECTED;

   return ESUCCESS;
}

/* EOF */

// vim:ts=3:expandtab

