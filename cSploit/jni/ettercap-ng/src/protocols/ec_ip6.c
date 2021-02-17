/*
    ettercap -- IPv6 decoder module

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

*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_inject.h>
#include <ec_inet.h>
#include <ec_session.h>
//#include <ec_fingerprint.h>

#define IP6_HDR_LEN 40

/* globals */

struct ip6_header {
#ifndef WORDS_BIGENDIAN
   u_int8   version:4;
   u_int8   priority:4;
#else 
   u_int8   priority:4;
   u_int8   version:4;
#endif
   u_int8   flow_lbl[3];
   u_int16  payload_len;
   u_int8   next_hdr;
   u_int8   hop_limit;

   u_int8   saddr[IP6_ADDR_LEN];
   u_int8   daddr[IP6_ADDR_LEN];
};

struct ip6_ext_header {
    u_int8 next_hdr;
    u_int8 hdr_len;

    /* Here must be options */
};

struct ip6_ident {
   u_int32 magic;
#define IP6_MAGIC    0x0306e77e
   u_int8 flow_lbl[3];
   struct ip_addr L3_src;
};

struct ip6_data {
   u_int8 priority :4;
};

/* protos */

void ip6_init(void);
FUNC_DECODER(decode_ip6);
FUNC_DECODER(decode_ip6_ext);
FUNC_INJECTOR(inject_ip6);
static size_t ip6_create_ident(void **i, struct packet_object *po);
static int ip6_match(void *ident_s, void *ident_c);
static void ip6_create_session(struct ec_session **s, struct packet_object *po);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ip6_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_IP6, decode_ip6);
   add_decoder(PROTO_LAYER, NL_TYPE_IP6, decode_ip6);
   add_decoder(NET6_LAYER, LO6_TYPE_HBH, decode_ip6_ext);
   add_decoder(NET6_LAYER, LO6_TYPE_RT, decode_ip6_ext);
   add_decoder(NET6_LAYER, LO6_TYPE_DST, decode_ip6_ext);

   add_injector(CHAIN_LINKED, IP6_MAGIC, inject_ip6);
}


FUNC_DECODER(decode_ip6)
{
   FUNC_DECODER_PTR(next_decoder);
   struct ip6_header *ip6;
   struct ec_session *s;
   void *ident;

   ip6 = (struct ip6_header *)DECODE_DATA;
  
   if (ip6->payload_len == 0) {
      DEBUG_MSG("IPv6 jumbogram, Hop-By-Hop header should follow");
   }
   DECODED_LEN = IP6_HDR_LEN;

   /* IP addresses */
   ip_addr_init(&PACKET->L3.src, AF_INET6, (u_char *)&ip6->saddr);
   ip_addr_init(&PACKET->L3.dst, AF_INET6, (u_char *)&ip6->daddr);
   
   /* this is needed at upper layer to calculate the tcp payload size */
   PACKET->L3.payload_len = ntohs(ip6->payload_len);
      
   /* other relevant infos */
   PACKET->L3.header = (u_char *)DECODE_DATA;
   PACKET->L3.len = DECODED_LEN;

   PACKET->L3.proto = htons(LL_TYPE_IP6);
   PACKET->L3.ttl = ip6->hop_limit;

   if(PACKET->fwd_packet == NULL) {
      EXECUTE(GBL_SNIFF->check_forwarded, PACKET);
      /* if it is already forwarded */
      if(PACKET->flags & PO_FORWARDED)
         return NULL;
      EXECUTE(GBL_SNIFF->set_forwardable, PACKET);
      PACKET->fwd_packet = (u_char *)DECODE_DATA;
      PACKET->fwd_len = PACKET->L3.payload_len + DECODED_LEN;
   }

   /* calculate if the dest is local or not */
   switch (ip_addr_is_local(&PACKET->L3.src, NULL)) {
      case ESUCCESS:
         PACKET->PASSIVE.flags &= ~FP_HOST_NONLOCAL;
         PACKET->PASSIVE.flags |= FP_HOST_LOCAL;
         break;
      case -ENOTFOUND:
         PACKET->PASSIVE.flags &= ~FP_HOST_LOCAL;
         PACKET->PASSIVE.flags |= FP_HOST_NONLOCAL;
         break;
      case -EINVALID:
         PACKET->PASSIVE.flags = FP_UNKNOWN;
         break;
   }
   
   next_decoder = get_decoder(NET6_LAYER, ip6->next_hdr);
   if(next_decoder == NULL) {
      PACKET->L3.options = NULL;
      PACKET->L3.optlen = 0;
      next_decoder = get_decoder(PROTO_LAYER, ip6->next_hdr);
   } else {
      PACKET->L3.options = (u_char *)&ip6[1];
   }
      
   /* HOOK POINT: HOOK_PACKET_IP6 */
   hook_point(HOOK_PACKET_IP6, po);

   if(!GBL_OPTIONS->unoffensive && !GBL_OPTIONS->read) {
      ip6_create_ident(&ident, PACKET);

      if(session_get(&s, ident, sizeof(struct ip6_ident)) == -ENOTFOUND) {
         ip6_create_session(&s, PACKET);
         session_put(s);
      }
      SAFE_FREE(ident);

      SESSION_PASSTHRU(s, PACKET);
   }
   
   /* passing the packet to options or upper-layer decoder */
   EXECUTE_DECODER(next_decoder);
   
   /*
    * External L3 header sets itself 
    * as the packet to be forwarded.
    */
   /* XXX - recheck this */
   if(!GBL_OPTIONS->unoffensive && !GBL_OPTIONS->read && (PACKET->flags & PO_FORWARDABLE)) {
      if(PACKET->flags & PO_MODIFIED) {
         PACKET->L3.payload_len += PACKET->DATA.delta;
         ip6->payload_len = htons(PACKET->L3.payload_len);

         PACKET->fwd_len = PACKET->L3.payload_len + DECODED_LEN;
      }
   }
   
   return NULL;
}

/* XXX - dirty stuff just to make things work.
 * Rewrite it to handle extension headers (if needed).
 */
FUNC_DECODER(decode_ip6_ext)
{
   FUNC_DECODER_PTR(next_decoder);
   struct ip6_ext_header *ext_hdr;

   ext_hdr = (struct ip6_ext_header *)DECODE_DATA;
   PACKET->L3.optlen += ext_hdr->hdr_len + 1;
   DECODED_LEN = ext_hdr->hdr_len + 1;

   next_decoder = get_decoder(NET6_LAYER, ext_hdr->next_hdr);
   if(next_decoder == NULL) {
      next_decoder = get_decoder(PROTO_LAYER, ext_hdr->next_hdr);
   }

   EXECUTE_DECODER(next_decoder);

   return NULL;
}

FUNC_INJECTOR(inject_ip6)
{
   struct ip6_header *ip6;
   struct ip6_ident *ident;
   struct ip6_data *data;
   struct ec_session *s = NULL;
   u_int32 magic;
   u_int16 plen;
   u_int16 flen;

//   DEBUG_MSG("inject_ip6");

   /* i think im paranoid */
   if(LENGTH + sizeof(struct ip6_header) > GBL_IFACE->mtu)
      return -ENOTHANDLED;

   /* almost copied from ec_ip.c */
   PACKET->packet -= sizeof(struct ip6_header);
   ip6 = (struct ip6_header *)PACKET->packet;

   ip6->version = 6;
   ip6->next_hdr = PACKET->L4.proto;
   ip6->hop_limit = 64;
   memcpy(&ip6->saddr, &PACKET->L3.src.addr, IP6_ADDR_LEN);
   memcpy(&ip6->daddr, &PACKET->L3.dst.addr, IP6_ADDR_LEN);

   s = PACKET->session;
   /* Renew session */
   if(session_get(&s, s->ident, sizeof(struct ip6_ident)) == -ENOTFOUND)
      return -ENOTFOUND;

   ident = s->ident;
   memcpy(&ip6->flow_lbl, &ident->flow_lbl, 3);

   data = s->data;
   ip6->priority = data->priority;
   
   flen = LENGTH;
   LENGTH += sizeof(struct ip6_header);

   if(s->prev_session != NULL) {
      PACKET->session = s->prev_session;
      magic = *(u_int32 *) s->prev_session->ident;

      EXECUTE_INJECTOR(CHAIN_LINKED, magic);
   } 

   plen = GBL_IFACE->mtu - LENGTH < PACKET->DATA.inject_len
          ? GBL_IFACE->mtu - LENGTH : PACKET->DATA.inject_len;
   ip6->payload_len = htons(plen);
   
   PACKET->L3.len = flen + plen;
   PACKET->L3.header = (u_char *)ip6;

   if(s->prev_session == NULL) {
      PACKET->fwd_packet = PACKET->packet;
      PACKET->fwd_len = PACKET->L3.len;
   }

   return ESUCCESS;
}
   
static size_t ip6_create_ident(void **i, struct packet_object *po)
{
   struct ip6_header *ip6;
   struct ip6_ident *ident;

   SAFE_CALLOC(ident, 1, sizeof(struct ip6_ident));

   ip6 = (struct ip6_header *)po->L3.header;

   ident->magic = IP6_MAGIC;
   memcpy(&ident->flow_lbl, ip6->flow_lbl, 3);
   memcpy(&ident->L3_src, &po->L3.src, sizeof(struct ip_addr));

   *i = ident;

   return sizeof(struct ip6_ident);
}

static int ip6_match(void *ident_s, void *ident_c)
{
   struct ip6_ident *ids = ident_s;
   struct ip6_ident *idc = ident_c;

   if(ids->magic != idc->magic)
      return 0;

   if(memcmp(&ids->flow_lbl, &idc->flow_lbl, 3))
      return 0;
   if(ip_addr_cmp(&ids->L3_src, &idc->L3_src))
      return 0;

   return 1;
}

static void ip6_create_session(struct ec_session **s, struct packet_object *po)
{
   void *ident;

   DEBUG_MSG("ip6_create_session");

   SAFE_CALLOC(*s, 1, sizeof(struct ec_session));
   SAFE_CALLOC((*s)->data, 1, sizeof(struct ip6_data));

   (*s)->data_len = sizeof(struct ip6_data);
   (*s)->ident_len = ip6_create_ident(&ident, po);
   (*s)->ident = ident;
   (*s)->match = &ip6_match;

   return;
}

/* EOF */

// vim:ts=3:expandtab

