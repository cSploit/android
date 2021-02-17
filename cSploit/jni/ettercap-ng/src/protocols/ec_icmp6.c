/*
        ettercap - ICMPv6 decoder module

        Copyright (C) the braindamaged entities collective
        GPL blah-blah-blah
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_checksum.h>

struct icmp6_hdr {
   u_int8 type;
   u_int8 code;
   u_int16 csum;

   /* other stuff lies here */
};

struct lla {
   u_int8 type;
#define LLA_SOURCE   1
#define LLA_TARGET   2
   u_int8 len;
};

FUNC_DECODER(decode_icmp6);
void icmp6_init(void);

void __init icmp6_init(void)
{
   add_decoder(PROTO_LAYER, NL_TYPE_ICMP6, decode_icmp6);
}

FUNC_DECODER(decode_icmp6)
{
   struct icmp6_hdr *icmp6;
   u_int16 csum;

   icmp6 = (struct icmp6_hdr*)DECODE_DATA;

   PACKET->L4.proto = NL_TYPE_ICMP6;
   /* since ICMPv6 header has no such field */
   PACKET->L4.len = PACKET->L3.payload_len;
   PACKET->L4.header = (u_char *)DECODE_DATA;
   /* This fields may be initialized later */
   PACKET->L4.options = NULL;
   PACKET->L4.optlen = 0;
   /* Why not? */
   PACKET->L4.flags = icmp6->type;

   DECODED_LEN = sizeof(struct icmp6_hdr);

   /* this sucked. Now it sucks less. */ 
   if(GBL_CONF->checksum_check && !GBL_OPTIONS->unoffensive) {
      if((csum = L4_checksum(PACKET)) != CSUM_RESULT) {
         if(GBL_CONF->checksum_warning) {
            char tmp[MAX_ASCII_ADDR_LEN];
            USER_MSG("Invalid ICMPv6 packet from %s : csum [%#x] should be (%#x)\n",
                     ip_addr_ntoa(&PACKET->L3.src, tmp), ntohs(icmp6->csum),
                     checksum_shouldbe(icmp6->csum, csum));
         }
         return NULL;
      }
   }

   /*
    * trying to detect a router
    */
   switch(icmp6->type) {
      case ICMP6_ROUTER_ADV:
      case ICMP6_PKT_TOO_BIG:
         PACKET->PASSIVE.flags |= FP_ROUTER;
         break;
   }

   hook_point(HOOK_PACKET_ICMP6, po);

   if(icmp6->type == ICMP6_NEIGH_SOL || icmp6->type == ICMP6_NEIGH_ADV) {
      PACKET->L4.options = (u_char*)(icmp6) + 4;
      PACKET->L4.optlen = PACKET->L4.len - sizeof(struct icmp6_hdr) - 4;
   }

   /*
    * Here come the hooks
    */
   switch(icmp6->type) {
      case ICMP6_NEIGH_SOL:
         hook_point(HOOK_PACKET_ICMP6_NSOL, PACKET);
         break;
      case ICMP6_NEIGH_ADV:
         hook_point(HOOK_PACKET_ICMP6_NADV, PACKET);
         break;
   }

   return NULL;
}

