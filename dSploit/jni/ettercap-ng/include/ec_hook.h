
/* $Id: ec_hook.h,v 1.20 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_HOOK_H
#define EC_HOOK_H

#include <ec_packet.h>

EC_API_EXTERN void hook_point(int point, struct packet_object *po);
   #define HOOK_RECEIVED      0     /* raw packet, the L* structures are not filled */
   #define HOOK_DECODED       1     /* all the packet after the protocol stack parsing */
   #define HOOK_PRE_FORWARD   2     /* right before the forward (if it has to be forwarded) */
   #define HOOK_HANDLED       3     /* top of the stack but before the decision of PO_INGORE */
   #define HOOK_FILTER        4     /* the content filtering point */
   #define HOOK_DISPATCHER    5     /* in the TOP HALF (the packet is a copy) */

   /* these are used the hook received packets */
   #define HOOK_PACKET_BASE      50
   #define HOOK_PACKET_ETH       (HOOK_PACKET_BASE + 1)
   #define HOOK_PACKET_FDDI      (HOOK_PACKET_BASE + 2)
   #define HOOK_PACKET_TR        (HOOK_PACKET_BASE + 3)
   #define HOOK_PACKET_WIFI      (HOOK_PACKET_BASE + 4)
   #define HOOK_PACKET_ARP       (HOOK_PACKET_BASE + 5)
   #define HOOK_PACKET_ARP_RQ    (HOOK_PACKET_BASE + 6)
   #define HOOK_PACKET_ARP_RP    (HOOK_PACKET_BASE + 7)
   #define HOOK_PACKET_IP        (HOOK_PACKET_BASE + 8)
   #define HOOK_PACKET_IP6       (HOOK_PACKET_BASE + 9)
   #define HOOK_PACKET_UDP       (HOOK_PACKET_BASE + 10)
   #define HOOK_PACKET_TCP       (HOOK_PACKET_BASE + 11)
   #define HOOK_PACKET_ICMP      (HOOK_PACKET_BASE + 12)
   #define HOOK_PACKET_LCP       (HOOK_PACKET_BASE + 13)
   #define HOOK_PACKET_ECP       (HOOK_PACKET_BASE + 14)
   #define HOOK_PACKET_IPCP      (HOOK_PACKET_BASE + 15)
   #define HOOK_PACKET_PPP       (HOOK_PACKET_BASE + 16)
   #define HOOK_PACKET_GRE       (HOOK_PACKET_BASE + 17)
   #define HOOK_PACKET_VLAN      (HOOK_PACKET_BASE + 18)

   /* high level protocol hooks */
   #define HOOK_PROTO_BASE          100
   #define HOOK_PROTO_SMB           (HOOK_PROTO_BASE + 1)
   #define HOOK_PROTO_SMB_CHL       (HOOK_PROTO_BASE + 2)
   #define HOOK_PROTO_DHCP_REQUEST  (HOOK_PROTO_BASE + 3)
   #define HOOK_PROTO_DHCP_DISCOVER (HOOK_PROTO_BASE + 4)
   #define HOOK_PROTO_DHCP_PROFILE  (HOOK_PROTO_BASE + 5)
   #define HOOK_PROTO_DNS           (HOOK_PROTO_BASE + 6)
   #define HOOK_PROTO_HTTP          (HOOK_PROTO_BASE + 7)

EC_API_EXTERN void hook_add(int point, void (*func)(struct packet_object *po) );
EC_API_EXTERN int hook_del(int point, void (*func)(struct packet_object *po) );

#endif

/* EOF */

// vim:ts=3:expandtab

