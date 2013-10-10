/*
    ettercap -- ARP decoder module

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

    $Id: ec_arp.c,v 1.12 2003/10/27 21:25:45 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>

/* globals */

struct arp_header {
   u_int16  ar_hrd;          /* Format of hardware address.  */
   u_int16  ar_pro;          /* Format of protocol address.  */
   u_int8   ar_hln;          /* Length of hardware address.  */
   u_int8   ar_pln;          /* Length of protocol address.  */
   u_int16  ar_op;           /* ARP opcode (command).  */
#define ARPOP_REQUEST   1    /* ARP request.  */
#define ARPOP_REPLY     2    /* ARP reply.  */
#define ARPOP_RREQUEST  3    /* RARP request.  */
#define ARPOP_RREPLY    4    /* RARP reply.  */
};

struct arp_eth_header {
   u_int8   arp_sha[MEDIA_ADDR_LEN];     /* sender hardware address */
   u_int8   arp_spa[IP_ADDR_LEN];      /* sender protocol address */
   u_int8   arp_tha[MEDIA_ADDR_LEN];     /* target hardware address */
   u_int8   arp_tpa[IP_ADDR_LEN];      /* target protocol address */
};

/* protos */

FUNC_DECODER(decode_arp);
void arp_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init arp_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_ARP, decode_arp);
}


FUNC_DECODER(decode_arp)
{
   struct arp_header *arp;

   arp = (struct arp_header *)DECODE_DATA;

   /*
    * size of the whole packet is the size of the
    * header + size of the hard address + proto address
    * for the sender and the target
    */
   
   DECODED_LEN = sizeof(struct arp_header) +
                 2 * (arp->ar_hln + arp->ar_pln);

   PACKET->L3.len = DECODED_LEN;
   PACKET->L3.header = (u_char *)DECODE_DATA;
   PACKET->L3.options = NULL;
   
   PACKET->L3.proto = htons(LL_TYPE_ARP);
   
   /* ARP discovered hosts are always local ;) */
   PACKET->PASSIVE.flags |= FP_HOST_LOCAL;
   
   if (arp->ar_hln == MEDIA_ADDR_LEN && arp->ar_pln == IP_ADDR_LEN) {
   
      struct arp_eth_header *earp;
      earp = (struct arp_eth_header *)(arp + 1);
      
      ip_addr_init(&PACKET->L3.src, AF_INET, (char *)&earp->arp_spa);
      ip_addr_init(&PACKET->L3.dst, AF_INET, (char *)&earp->arp_tpa);
           

      /* 
       * for ARP packets we can overwrite the L2 addresses with the 
       * information within the ARP header 
       */
      memcpy(PACKET->L2.src, (char *)&earp->arp_sha, MEDIA_ADDR_LEN);
      memcpy(PACKET->L2.dst, (char *)&earp->arp_tha, MEDIA_ADDR_LEN);
      
      /* 
       * HOOK POINT:  HOOK_PACKET_ARP 
       * differentiate between REQUEST and REPLY
       */
      if (ntohs(arp->ar_op) == ARPOP_REQUEST)
         hook_point(HOOK_PACKET_ARP_RQ, po);
      else if (ntohs(arp->ar_op) == ARPOP_REPLY)
         hook_point(HOOK_PACKET_ARP_RP, po);
      
      /* ARP packets are always local (our machine is at distance 0) */
      if (!ip_addr_cmp(&po->L3.src, &GBL_IFACE->ip))
         PACKET->L3.ttl = 0;
      else
         PACKET->L3.ttl = 1;
   
      /* HOOK_PACKET_ARP is for all type of arp, no distinctions */
      hook_point(HOOK_PACKET_ARP, po);
   }
   
   return NULL;
}

/* EOF */

// vim:ts=3:expandtab

