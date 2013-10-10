/*
    ettercap -- IP decoder module

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

    $Id: ec_ip.c,v 1.43 2004/09/28 09:56:13 alor Exp $
*/

#include <ec.h>
#include <ec_inet.h>
#include <ec_decode.h>
#include <ec_fingerprint.h>
#include <ec_checksum.h>
#include <ec_session.h>
#include <ec_inject.h>


/* globals */

struct ip_header {
#ifndef WORDS_BIGENDIAN
   u_int8   ihl:4;
   u_int8   version:4;
#else 
   u_int8   version:4;
   u_int8   ihl:4;
#endif
   u_int8   tos;
   u_int16  tot_len;
   u_int16  id;
   u_int16  frag_off;
#define IP_DF 0x4000
#define IP_MF 0x2000
#define IP_FRAG 0x1fff
   u_int8   ttl;
   u_int8   protocol;
   u_int16  csum;
   u_int32  saddr;
   u_int32  daddr;
/*The options start here. */
};

/* Session data structure */
struct ip_status {
   u_int16  last_id;
   int16    id_adj;
};

/* Ident structure for ip sessions */
struct ip_ident {
   u_int32 magic;
      #define IP_MAGIC  0x0300e77e
   struct ip_addr L3_src;
};

#define IP_IDENT_LEN sizeof(struct ip_ident)


/* protos */

FUNC_DECODER(decode_ip);
FUNC_INJECTOR(inject_ip);
FUNC_INJECTOR(stateless_ip);
void ip_init(void);
int ip_match(void *id_sess, void *id_curr);
void ip_create_session(struct ec_session **s, struct packet_object *po);
size_t ip_create_ident(void **i, struct packet_object *po);            


/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ip_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_IP, decode_ip);
   add_injector(CHAIN_LINKED, IP_MAGIC, inject_ip);
   add_injector(CHAIN_LINKED, STATELESS_IP_MAGIC, stateless_ip);
}


FUNC_DECODER(decode_ip)
{
   FUNC_DECODER_PTR(next_decoder);
   struct ip_header *ip;
   struct ec_session *s = NULL;
   void *ident = NULL;
   struct ip_status *status = NULL;
   u_int32 t_len;
   u_int16 sum;

   ip = (struct ip_header *)DECODE_DATA;
  
   DECODED_LEN = (u_int32)(ip->ihl * 4);

   /* IP addresses */
   ip_addr_init(&PACKET->L3.src, AF_INET, (char *)&ip->saddr);
   ip_addr_init(&PACKET->L3.dst, AF_INET, (char *)&ip->daddr);
   
   /* this is needed at upper layer to calculate the tcp payload size */
   t_len = (u_int32) ntohs(ip->tot_len);
   if (t_len < (u_int32)DECODED_LEN)
      return NULL;
   PACKET->L3.payload_len = t_len - DECODED_LEN;

   /* other relevant infos */
   PACKET->L3.header = (u_char *)DECODE_DATA;
   PACKET->L3.len = DECODED_LEN;
   
   /* parse the options */
   if ( (u_int32)(ip->ihl * 4) > sizeof(struct ip_header)) {
      PACKET->L3.options = (u_char *)(DECODE_DATA) + sizeof(struct ip_header);
      PACKET->L3.optlen = (u_int32)(ip->ihl * 4) - sizeof(struct ip_header);
   } else {
      PACKET->L3.options = NULL;
      PACKET->L3.optlen = 0;
   }
   
   PACKET->L3.proto = htons(LL_TYPE_IP);
   PACKET->L3.ttl = ip->ttl;

   /* First IP decoder set its header as packet to be forwarded */
   if (PACKET->fwd_packet == NULL) {
      /* Don't parse packets we have forwarded */
      EXECUTE(GBL_SNIFF->check_forwarded, PACKET);
      if (PACKET->flags & PO_FORWARDED)
         return NULL;
      /* set PO_FORWARDABLE flag */
      EXECUTE(GBL_SNIFF->set_forwardable, PACKET);
      /* set the pointer to the data to be forwarded at layer 3 */
      PACKET->fwd_packet = (u_char *)DECODE_DATA;
      /* the len will be adjusted later...just in case of a brutal return */
      PACKET->fwd_len = t_len; 
   }
   
   /* XXX - implement the handling of fragmented packet */
   /* don't process fragmented packets */
   if (ntohs(ip->frag_off) & IP_FRAG || ntohs(ip->frag_off) & IP_MF) 
      return NULL;
   
   /* 
    * if the checsum is wrong, don't parse it (avoid ettercap spotting) 
    * the checksum should be 0 ;)
    *
    * don't perform the check in unoffensive mode
    */
   if (GBL_CONF->checksum_check) {
      if (!GBL_OPTIONS->unoffensive && (sum = L3_checksum(PACKET->L3.header, PACKET->L3.len)) != CSUM_RESULT) {
         if (GBL_CONF->checksum_warning)
            USER_MSG("Invalid IP packet from %s : csum [%#x] should be (%#x)\n", int_ntoa(ip->saddr), 
                              ntohs(ip->csum), checksum_shouldbe(ip->csum, sum));      
         return NULL;
      }
   }
   
   /* if it is a TCP packet, try to passive fingerprint it */
   if (ip->protocol == NL_TYPE_TCP) {
      /* initialize passive fingerprint */
      fingerprint_default(PACKET->PASSIVE.fingerprint);
  
      /* collect infos for passive fingerprint */
      fingerprint_push(PACKET->PASSIVE.fingerprint, FINGER_TTL, ip->ttl);
      fingerprint_push(PACKET->PASSIVE.fingerprint, FINGER_DF, ntohs(ip->frag_off) & IP_DF);
      fingerprint_push(PACKET->PASSIVE.fingerprint, FINGER_LT, ip->ihl * 4);
   }

   /* calculate if the dest is local or not */
   switch (ip_addr_is_local(&PACKET->L3.src)) {
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
   
   /* HOOK POINT: HOOK_PACKET_IP */
   hook_point(HOOK_PACKET_IP, po);

   /* don't save the sessions in unoffensive mode */
   if (!GBL_OPTIONS->unoffensive && !GBL_OPTIONS->read) {
   
      /* Find or create the correct session */
      ip_create_ident(&ident, PACKET);
   
      if (session_get(&s, ident, IP_IDENT_LEN) == -ENOTFOUND) {
         ip_create_session(&s, PACKET);
         session_put(s);
      }
      SAFE_FREE(ident);
      
      SESSION_PASSTHRU(s, PACKET);
   
      /* Record last packet's ID */
      status = (struct ip_status *)s->data;
      status->last_id = ntohs(ip->id);
   }
   
   /* Jump to next Layer */
   next_decoder = get_decoder(PROTO_LAYER, ip->protocol);
   EXECUTE_DECODER(next_decoder);
   
   /* don't save the sessions in unoffensive mode */
   if (!GBL_OPTIONS->unoffensive && !GBL_OPTIONS->read && (PACKET->flags & PO_FORWARDABLE)) {
      /* 
       * Modification checks and adjustments.
       * - ip->id according to number of injected/dropped packets
       * - ip->len according to upper layer's payload modification
       */
      if (PACKET->flags & PO_DROPPED)
         status->id_adj--;
      else if ((PACKET->flags & PO_MODIFIED) || (status->id_adj != 0)) {
         
         /* se the correct id for this packet */
         ORDER_ADD_SHORT(ip->id, status->id_adj);
         /* adjust the packet length */
         ORDER_ADD_SHORT(ip->tot_len, PACKET->DATA.delta);

         /* 
          * In case some upper level encapsulated 
          * ip decoder modified it... (required for checksum)
          */
         PACKET->L3.header = (u_char *)ip;
         PACKET->L3.len = (u_int32)(ip->ihl * 4);
      
         /* ...recalculate checksum */
         ip->csum = CSUM_INIT; 
         ip->csum = L3_checksum(PACKET->L3.header, PACKET->L3.len);
      }
   }
   /* Last ip decoder in cascade will set the correct fwd_len */
   PACKET->fwd_len = ntohs(ip->tot_len);
      
   return NULL;
}

/*******************************************/
FUNC_INJECTOR(inject_ip)
{
   struct ec_session *s = NULL;
   struct ip_status *status;
   struct ip_header *iph;
   size_t further_len, payload_len;
   u_int32 magic;
   
   /* Paranoid check */
   if (LENGTH + sizeof(struct ip_header) > GBL_IFACE->mtu)
      return -ENOTHANDLED;

   /* Make space for ip header on packet stack... */      
   PACKET->packet -= sizeof(struct ip_header);

   /* ..and fill it */  
   iph = (struct ip_header *)PACKET->packet;
   
   iph->ihl      = 5;
   iph->version  = 4;
   iph->tos      = 0;
   iph->csum     = CSUM_INIT;
   iph->frag_off = 0;            
   iph->ttl      = 125;   
   iph->protocol = PACKET->L4.proto; 
   iph->saddr    = ip_addr_to_int32(PACKET->L3.src.addr);   
   iph->daddr    = ip_addr_to_int32(PACKET->L3.dst.addr);   

   /* Take the session and fill remaining fields */
   s = PACKET->session;
   status = (struct ip_status *)s->data;
   iph->id = htons(status->last_id + status->id_adj + 1);

   /* Renew session timestamp (XXX it locks the sessions) */
   if (session_get(&s, s->ident, IP_IDENT_LEN) == -ENOTFOUND) 
      return -ENOTFOUND;
   
   /* Adjust headers length */   
   LENGTH += sizeof(struct ip_header);
   
   /* Rember length of further headers */
   further_len = LENGTH;
   
   if (s->prev_session != NULL) {
      /* Prepare data for next injector */
      PACKET->session = s->prev_session;
      memcpy(&magic, s->prev_session->ident, 4);
      
      /* Go deeper into injectors chain */
      EXECUTE_INJECTOR(CHAIN_LINKED, magic);
   }

   /* Update session */
   status->id_adj ++;
   
   /* Guess payload_len that will be used by ENTRY injector */
   payload_len = GBL_IFACE->mtu - LENGTH;
   if (payload_len > PACKET->DATA.inject_len)
      payload_len = PACKET->DATA.inject_len;

   /* Set tot_len field as further header's len + payload */
   PACKET->L3.len = further_len + payload_len;
   iph->tot_len = htons(PACKET->L3.len);
   
   /* Calculate checksum */
   PACKET->L3.header = (u_char *)iph;
   iph->csum = L3_checksum(PACKET->L3.header, PACKET->L3.len);

   /* Set fields to forward the packet (only if the chain is ended) */
   if (s->prev_session == NULL) {
      PACKET->fwd_packet = PACKET->packet;
      PACKET->fwd_len = PACKET->L3.len;
   }
   
   return ESUCCESS;
}

/* Used to link sessionless udp with correct ip session */
FUNC_INJECTOR(stateless_ip)
{
   struct ec_session *s = NULL;
   void *ident = NULL;

   /* Find the correct IP session */
   ip_create_ident(&ident, PACKET);
   if (session_get(&s, ident, IP_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
      return -ENOTFOUND;
   }

   PACKET->session = s;
   
   /* Execute IP injector */
   EXECUTE_INJECTOR(CHAIN_LINKED, IP_MAGIC);

   SAFE_FREE(ident);
   return ESUCCESS;
}

/*******************************************/

/* Sessions' stuff for ip packets */


/*
 * create the ident for a session
 */
 
size_t ip_create_ident(void **i, struct packet_object *po)
{
   struct ip_ident *ident;
   
   /* allocate the ident for that session */
   SAFE_CALLOC(ident, 1, sizeof(struct ip_ident));
  
   /* the magic */
   ident->magic = IP_MAGIC;
      
   /* prepare the ident */
   memcpy(&ident->L3_src, &po->L3.src, sizeof(struct ip_addr));

   /* return the ident */
   *i = ident;

   /* return the lenght of the ident */
   return sizeof(struct ip_ident);
}


/*
 * compare two session ident
 *
 * return 1 if it matches
 */

int ip_match(void *id_sess, void *id_curr)
{
   struct ip_ident *ids = id_sess;
   struct ip_ident *id = id_curr;

   /* sanity check */
   BUG_IF(ids == NULL);
   BUG_IF(id == NULL);
  
   /* 
    * is this ident from our level ?
    * check the magic !
    */
   if (ids->magic != id->magic)
      return 0;
   
   /* Check the source */
   if ( !ip_addr_cmp(&ids->L3_src, &id->L3_src) ) 
      return 1;

   return 0;
}

/*
 * prepare the ident and the pointer to match function
 * for ip layer.
 */

void ip_create_session(struct ec_session **s, struct packet_object *po)
{
   void *ident;

   DEBUG_MSG("ip_create_session");
   
   /* allocate the session */
   SAFE_CALLOC(*s, 1, sizeof(struct ec_session));
   
   /* create the ident */
   (*s)->ident_len = ip_create_ident(&ident, po);
   
   /* link to the session */
   (*s)->ident = ident;

   /* the matching function */
   (*s)->match = &ip_match;
   
   /* alloc of data element */
   SAFE_CALLOC((*s)->data, 1, sizeof(struct ip_status));
}

/* EOF */

// vim:ts=3:expandtab

