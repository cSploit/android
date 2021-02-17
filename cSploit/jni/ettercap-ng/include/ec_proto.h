
#ifndef EC_PROTO_H
#define EC_PROTO_H

#include <ec_inet.h>

/* interface layer types */
enum {
   IL_TYPE_NULL     = 0x00,  /* bsd loopback (used by some wifi cards in monitor mode) */
   IL_TYPE_ETH      = 0x01,  /* ethernet */
   IL_TYPE_TR       = 0x06,  /* token ring */
   IL_TYPE_PPP      = 0x09,  /* PPP */
   IL_TYPE_FDDI     = 0x0a,  /* fiber distributed data interface */
   IL_TYPE_RAWIP    = 0x0c,  /* raw ip dump file */
   IL_TYPE_WIFI     = 0x69,  /* wireless */
   IL_TYPE_COOK     = 0x71,  /* linux cooked */
   IL_TYPE_PRISM    = 0x77,  /* prism2 header for wifi dumps */
   IL_TYPE_RADIO    = 0x7f,  /* radiotap header for wifi dumps */
   IL_TYPE_PPI      = 0xc0,  /* per packet information */
   IL_TYPE_ERF      = 0xc5,  /* ERF endace format */
};

/* link layer types */
enum {
   LL_TYPE_PPP_IP = 0x0021,
   LL_TYPE_IP     = 0x0800,
   LL_TYPE_ARP    = 0x0806,
   LL_TYPE_VLAN   = 0x8100,
   LL_TYPE_IP6    = 0x86DD,
   LL_TYPE_PPP    = 0x880B,
   LL_TYPE_PPPOE  = 0x8864,
   LL_TYPE_PAP    = 0xc023,
   LL_TYPE_MPLS   = 0x8847,
   LL_TYPE_8021x  = 0x888E
};

/* network layer types */
enum {
   NL_TYPE_ICMP  = 0x01,
   NL_TYPE_IPIP  = 0x04,
   NL_TYPE_IP6   = 0x29,
   NL_TYPE_ICMP6 = 0x3a,
   NL_TYPE_TCP   = 0x06,
   NL_TYPE_UDP   = 0x11,
   NL_TYPE_GRE   = 0x2f,
   NL_TYPE_OSPF  = 0x59,
   NL_TYPE_VRRP  = 0x70,
};

/* proto layer types */
enum {
   PL_DEFAULT  = 0x0000,
};

/* IPv6 options types */
/* NOTE: they may (but should not) conflict with network layer types!   */
/*       double check new definitions of either types.                  */

enum {
   LO6_TYPE_HBH = 0,   /* Hop-By-Hop */
   LO6_TYPE_RT  = 0x2b,  /* Routing */
   LO6_TYPE_FR  = 0x2c,  /* Fragment */
   LO6_TYPE_DST = 0x3c,  /* Destination */
   LO6_TYPE_NO  = 0x3b,  /* No Next Header */
};


/* TCP flags */
enum {
   TH_FIN = 0x01,
   TH_SYN = 0x02,
   TH_RST = 0x04,
   TH_PSH = 0x08,
   TH_ACK = 0x10,
   TH_URG = 0x20,
};

/* ICMP types */
enum {
   ICMP_ECHOREPLY       = 0,
   ICMP_DEST_UNREACH    = 3,
   ICMP_REDIRECT        = 5,
   ICMP_ECHO            = 8,
   ICMP_TIME_EXCEEDED   = 11,
   ICMP_NET_UNREACH     = 0,
   ICMP_HOST_UNREACH    = 1,
};

/* ICMPv6 types */
enum {
   /* Errors */
   ICMP6_DEST_UNREACH   = 1,
   ICMP6_PKT_TOO_BIG    = 2,
   ICMP6_TIME_EXCEEDED  = 3,
   ICMP6_BAD_PARAM      = 4,
   
   /* Info */
   ICMP6_ECHO           = 128,
   ICMP6_ECHOREPLY      = 129,
   ICMP6_ROUTER_SOL     = 133,
   ICMP6_ROUTER_ADV     = 134,
   ICMP6_NEIGH_SOL      = 135,
   ICMP6_NEIGH_ADV      = 136,
};

/* DHCP options */
enum {
   DHCP_MAGIC_COOKIE    = 0x63825363,
   DHCP_DISCOVER        = 0x01,
   DHCP_OFFER           = 0x02,
   DHCP_REQUEST         = 0x03,
   DHCP_ACK             = 0x05,
   DHCP_OPT_NETMASK     = 0x01,
   DHCP_OPT_ROUTER      = 0x03,
   DHCP_OPT_DNS         = 0x06,
   DHCP_OPT_DOMAIN      = 0x0f,
   DHCP_OPT_RQ_ADDR     = 0x32,
   DHCP_OPT_LEASE_TIME  = 0x33,
   DHCP_OPT_MSG_TYPE    = 0x35,
   DHCP_OPT_SRV_ADDR    = 0x36,
   DHCP_OPT_RENEW_TIME  = 0x3a,
   DHCP_OPT_CLI_IDENT   = 0x3d,
   DHCP_OPT_FQDN        = 0x51,
   DHCP_OPT_END         = 0xff,
   DHCP_OPT_MIN_LEN     = 0x12c,
};

#endif

/* EOF */

// vim:ts=3:expandtab

