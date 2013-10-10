/*
    ettercap -- dissector portmap 

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

    $Id: ec_portmap.c,v 1.7 2004/01/21 20:20:06 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */
typedef struct {
   u_int32 xid;
   u_int32 prog;
   u_int32 ver;
   u_int32 proto;
   u_int32 status;
   u_int32 next_offs;
} portmap_session;
#define DUMP 1
#define MAP_LEN 20
#define FIRST_FRAG 0
#define MORE_FRAG 1
#define LAST_FRAG 0x80000000

typedef struct {
   u_int32 program;
   u_int32 version;
   u_char name[32];
   FUNC_DECODER_PTR(dissector);
} RPC_DISSECTOR;

extern FUNC_DECODER(dissector_mountd);

RPC_DISSECTOR Available_RPC_Dissectors[] = {
   {100005,  1, "mountd", dissector_mountd },
   {100005,  2, "mountd", dissector_mountd },
   {100005,  3, "mountd", dissector_mountd },
   {     0,  0, "", NULL }
};

/* protos */
FUNC_DECODER(dissector_portmap);
void portmap_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init portmap_init(void)
{
   /* UDP come first for the precedence in the dissector list */
   dissect_add("portmap", APP_LAYER_UDP, 111, dissector_portmap);
   dissect_add("portmap", APP_LAYER_TCP, 111, dissector_portmap);
}

FUNC_DECODER(dissector_portmap)
{
   DECLARE_DISP_PTR_END(ptr, end);
   u_int32 type, xid, proc, port, proto, program, version, offs, i;
   portmap_session *pe;
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* don't complain about unused var */
   (void)end;

   /* skip unuseful packets */
   if (PACKET->DATA.len < 24)  
      return NULL;
   
   DEBUG_MSG("portmap --> dissector_portmap");

   /* Offsets differs from TCP to UDP (?) */
   if (PACKET->L4.proto == NL_TYPE_TCP)
      ptr += 4;

   xid  = pntol(ptr);
   proc = pntol(ptr + 20);
   type = pntol(ptr + 4);

   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_portmap));

   /* CALL */
   if (FROM_CLIENT("portmap", PACKET)) {
      if (type != 0 || session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         SAFE_FREE(ident); 
         return NULL;
      }
      
      SAFE_FREE(ident);
      dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_portmap));
      SAFE_CALLOC(s->data, 1, sizeof(portmap_session));
      pe = (portmap_session *)s->data;

      pe->xid   = xid;
      pe->prog  = pntol(ptr + 40);
      pe->ver   = pntol(ptr + 44);
      pe->proto = pntol(ptr + 48);

      /* DUMP */
      if ( proc == 4 ) 
         pe->prog = DUMP;

      session_put(s);

      return NULL;
   }

   /* REPLY */
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
      return NULL;
   }

   SAFE_FREE(ident);
   pe = (portmap_session *)s->data;
   if (!pe)
      return NULL;
   
   /* Unsuccess or not a reply */
   if ( (pe->xid != xid || pntol(ptr + 8) != 0 || type != 1) 
        && pe->status != MORE_FRAG) 
      return NULL;
      
   /* GETPORT Reply */
   if (pe->prog != DUMP) {
      port = pntol(ptr + 24);
      
      for (i=0; Available_RPC_Dissectors[i].program != 0; i++ ) {
         if ( Available_RPC_Dissectors[i].program == pe->prog &&
              Available_RPC_Dissectors[i].version == pe->ver ) {

            if (pe->proto == IPPROTO_TCP) {
               if (dissect_on_port_level(Available_RPC_Dissectors[i].name, port, APP_LAYER_TCP) == ESUCCESS)
                  break;
               dissect_add(Available_RPC_Dissectors[i].name, APP_LAYER_TCP, port, Available_RPC_Dissectors[i].dissector);
               DISSECT_MSG("portmap : %s binds [%s] on port %d TCP\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                                                       Available_RPC_Dissectors[i].name, 
                                                                       port);
            } else {
               if (dissect_on_port_level(Available_RPC_Dissectors[i].name, port, APP_LAYER_UDP) == ESUCCESS)
                  break;
               dissect_add(Available_RPC_Dissectors[i].name, APP_LAYER_UDP, port, Available_RPC_Dissectors[i].dissector);
               DISSECT_MSG("portmap : %s binds [%s] on port %d UDP\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                                                       Available_RPC_Dissectors[i].name, 
                                                                       port);
            }
            break;
         }
      }
   } else { /* DUMP Reply */
      /* XXX - It jumps the fragmented entry (if any) */
      if (pe->status == MORE_FRAG)
         offs = pe->next_offs;
      else
         offs = 24;
      while ( (PACKET->DATA.len - offs) >= MAP_LEN ) {
         program = pntol(ptr + offs + 4);
         version = pntol(ptr + offs + 8);
         proto   = pntol(ptr + offs + 12);
         port    = pntol(ptr + offs + 16);
         for (i=0; Available_RPC_Dissectors[i].program != 0; i++) {

            if ( Available_RPC_Dissectors[i].program == program &&
                 Available_RPC_Dissectors[i].version == version ) {

               if (proto == IPPROTO_TCP) {

                  if (dissect_on_port_level(Available_RPC_Dissectors[i].name, port, APP_LAYER_TCP) == ESUCCESS)
                     break;

                  dissect_add(Available_RPC_Dissectors[i].name, APP_LAYER_TCP, port, Available_RPC_Dissectors[i].dissector);
                  DISSECT_MSG("portmap : %s binds [%s] on port %d TCP\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                                                          Available_RPC_Dissectors[i].name, 
                                                                          port);
               } else {

                  if (dissect_on_port_level(Available_RPC_Dissectors[i].name, port, APP_LAYER_UDP) == ESUCCESS)
                     break;

                  dissect_add(Available_RPC_Dissectors[i].name, APP_LAYER_UDP, port, Available_RPC_Dissectors[i].dissector);
                  DISSECT_MSG("portmap : %s binds [%s] on port %d UDP\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                                                          Available_RPC_Dissectors[i].name, 
                                                                          port);
               }	 
               break;
            }
         }
         offs += MAP_LEN;
      }
      /* 
       * Offset to the beginning of the first 
       * valid structure in the next packet
       */
      pe->next_offs = (MAP_LEN + 4) - (PACKET->DATA.len - offs);
   }

   /* Check if we have to wait for more reply fragments */
   if ( PACKET->L4.proto == NL_TYPE_TCP && !(pntol(ptr - 4)&LAST_FRAG) )
      pe->status = MORE_FRAG;
   else
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_portmap));      

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

