/*
    ettercap -- UDP decoder module

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

    $Id: ec_udp.c,v 1.21 2004/09/28 09:56:13 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_checksum.h>
#include <ec_inject.h>

/* globals */

struct udp_header {
   u_int16  sport;           /* source port */
   u_int16  dport;           /* destination port */
   u_int16  ulen;            /* udp length */
   u_int16  csum;            /* udp checksum */
};

/* protos */

FUNC_DECODER(decode_udp);
FUNC_INJECTOR(inject_udp);
void udp_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init udp_init(void)
{
   add_decoder(PROTO_LAYER, NL_TYPE_UDP, decode_udp);
   add_injector(CHAIN_ENTRY, NL_TYPE_UDP, inject_udp);
}


FUNC_DECODER(decode_udp)
{
   FUNC_DECODER_PTR(next_decoder);
   struct udp_header *udp;
   u_int16 sum;

   udp = (struct udp_header *)DECODE_DATA;

   DECODED_LEN = sizeof(struct udp_header);

   /* source and dest port */
   PACKET->L4.src = udp->sport;
   PACKET->L4.dst = udp->dport;

   PACKET->L4.len = DECODED_LEN;
   PACKET->L4.header = (u_char *)DECODE_DATA;
   PACKET->L4.options = NULL;
   
   /* this is UDP */
   PACKET->L4.proto = NL_TYPE_UDP;

   /* set up the data poiters */
   PACKET->DATA.data = ((u_char *)udp) + sizeof(struct udp_header);
   if (ntohs(udp->ulen) < (u_int16)sizeof(struct udp_header))
      return NULL;
   PACKET->DATA.len = ntohs(udp->ulen) - (u_int16)sizeof(struct udp_header);
  
   /* create the buffer to be displayed */
   packet_disp_data(PACKET, PACKET->DATA.data, PACKET->DATA.len);

   /* 
    * if the checsum is wrong, don't parse it (avoid ettercap spotting) 
    * the checksum is should be CSUM_RESULT and not equal to udp->csum ;)
    *
    * don't perform the check in unoffensive mode
    */
   if (GBL_CONF->checksum_check) {
      if (!GBL_OPTIONS->unoffensive && (sum = L4_checksum(PACKET)) != CSUM_RESULT) {
         char tmp[MAX_ASCII_ADDR_LEN];
#if defined(OS_DARWIN) || defined(OS_WINDOWS) || defined(__APPLE__)
         /* 
          * XXX - hugly hack here !  Mac OS X really sux
          * 
          * Packets transmitted on interfaces with TCP checksum offloading
          * don't have valid checksums as presented to the machine's packet-capture
          * mechanism, as those packets are wrapped around internally rather
          * than being captured after passing through the network interface, as
          * the OS doesn't bother computing the checksum and adding it to the packet
          * it leaves that up to the network interface.
          *                (taken from a bug report by Guy Harris - libpcap engineer)
          * 
          * For Windows at least, TCP checksum off-loading can be disabled with a
          * registry setting.
          *
          * if the source is the ettercap host, don't display the message 
          */
         if (!ip_addr_cmp(&PACKET->L3.src, &GBL_IFACE->ip))
            return NULL;
#endif
         if (GBL_CONF->checksum_warning)
            USER_MSG("Invalid UDP packet from %s:%d : csum [%#x] should be (%#x)\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                    ntohs(udp->sport), ntohs(udp->csum), checksum_shouldbe(udp->csum, sum));
         return NULL;
      }
   }

   /* HOOK POINT: HOOK_PACKET_UDP */
   hook_point(HOOK_PACKET_UDP, po);
   
   /* get the next decoder */
   next_decoder =  get_decoder(APP_LAYER, PL_DEFAULT);
   EXECUTE_DECODER(next_decoder);
   
   /* 
    * in unoffensive mode the filters don't touch
    * the packet, so we don't have to check here
    * for unoffensive option
    */

   /* Adjustments after filters */
   if ((PACKET->flags & PO_MODIFIED) && (PACKET->flags & PO_FORWARDABLE)) {
            
      /* Recalculate checksum */
      udp->csum = CSUM_INIT; 
      udp->csum = L4_checksum(PACKET);
   }

   return NULL;
}

/*******************************************/

FUNC_INJECTOR(inject_udp)
{
   struct udp_header *udph;
   u_char *udp_payload;
       
   /* Rember where the payload has to start */
   udp_payload = PACKET->packet;

   /* Allocate stack for tcp header */
   PACKET->packet -= sizeof(struct udp_header);

   /* Create the udp header */
   udph = (struct udp_header *)PACKET->packet;

   udph->sport = PACKET->L4.src;
   udph->dport = PACKET->L4.dst;
   udph->csum  = CSUM_INIT;
      
   /* 
    * Go deeper into injectors chain. 
    * XXX We assume next layer is IP.
    */
   LENGTH += sizeof(struct udp_header);     
   PACKET->session = NULL;
   EXECUTE_INJECTOR(CHAIN_LINKED, STATELESS_IP_MAGIC);

   /* 
    * Attach the data (LENGTH was adjusted by LINKED injectors).
    * Set LENGTH to injectable data len.
    */
   LENGTH = GBL_IFACE->mtu - LENGTH;
   if (LENGTH > PACKET->DATA.inject_len)
      LENGTH = PACKET->DATA.inject_len;
   memcpy(udp_payload, PACKET->DATA.inject, LENGTH);   

   /* Set datagram len and calculate checksum */
   PACKET->L4.header = (u_char *)udph;
   PACKET->L4.len = sizeof(struct udp_header);
   PACKET->DATA.len = LENGTH; 
   udph->ulen = htons(PACKET->DATA.len + PACKET->L4.len);
   udph->csum = L4_checksum(PACKET);
      
   return ESUCCESS;
}

/* EOF */

// vim:ts=3:expandtab

