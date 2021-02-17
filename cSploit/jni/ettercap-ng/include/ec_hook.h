

#ifndef EC_HOOK_H
#define EC_HOOK_H

#include <ec_packet.h>

EC_API_EXTERN void hook_point(int point, struct packet_object *po);

enum {
   HOOK_RECEIVED     = 0,     /* raw packet, the L* structures are not filled */
   HOOK_DECODED      = 1,     /* all the packet after the protocol stack parsing */
   HOOK_PRE_FORWARD  = 2,     /* right before the forward (if it has to be forwarded) */
   HOOK_HANDLED      = 3,     /* top of the stack but before the decision of PO_INGORE */
   HOOK_FILTER       = 4,     /* the content filtering point */
   HOOK_DISPATCHER   = 5,     /* in the TOP HALF (the packet is a copy) */
};

   /* these are used the hook received packets */
enum {
   HOOK_PACKET_BASE  = 50,
   HOOK_PACKET_ETH,
   HOOK_PACKET_FDDI,      
   HOOK_PACKET_TR,
   HOOK_PACKET_WIFI,
   HOOK_PACKET_ARP,
   HOOK_PACKET_ARP_RQ,
   HOOK_PACKET_ARP_RP,
   HOOK_PACKET_IP,
   HOOK_PACKET_IP6,
   HOOK_PACKET_UDP,
   HOOK_PACKET_TCP,
   HOOK_PACKET_ICMP,
   HOOK_PACKET_LCP,
   HOOK_PACKET_ECP,
   HOOK_PACKET_IPCP,
   HOOK_PACKET_PPP,
   HOOK_PACKET_GRE,
   HOOK_PACKET_VLAN,
   HOOK_PACKET_ICMP6,
   HOOK_PACKET_ICMP6_NSOL,
   HOOK_PACKET_ICMP6_NADV,
   HOOK_PACKET_PPPOE,
   HOOK_PACKET_PPP_PAP,
   HOOK_PACKET_MPLS,
   HOOK_PACKET_ERF,

   /* high level protocol hooks */
   HOOK_PROTO_BASE   = 100,
   HOOK_PROTO_SMB,
   HOOK_PROTO_SMB_CHL,
   HOOK_PROTO_SMB_CMPLT,
   HOOK_PROTO_DHCP_REQUEST,
   HOOK_PROTO_DHCP_DISCOVER,
   HOOK_PROTO_DHCP_PROFILE,
   HOOK_PROTO_DNS,
   HOOK_PROTO_MDNS,
   HOOK_PROTO_NBNS,
   HOOK_PROTO_HTTP,
};

EC_API_EXTERN void hook_add(int point, void (*func)(struct packet_object *po) );
EC_API_EXTERN int hook_del(int point, void (*func)(struct packet_object *po) );

#endif

/* EOF */

// vim:ts=3:expandtab

