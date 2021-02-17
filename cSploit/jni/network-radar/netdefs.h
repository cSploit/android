/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NETDEFS_H
#define NETDEFS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* required headers */
#include <sys/types.h>
#include <sys/socket.h>

#ifndef PF_PACKET
# error PF_PACKET support is required, please upgrade your kernel
#endif

#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>

#ifdef HAVE_LINUX_IF_ETHER_H
#include <linux/if_ether.h>
#else
#define _LINUX_IF_ETHER_H 1

#define ETH_P_ALL 0x0003          /* Every packet (be careful!!!) */
#define ETH_P_IP  0x0800          /* Internet Protocol packet     */
#define ETH_P_ARP 0x0806          /* Address Resolution packet    */

#define ETH_ALEN 6                /* Address length  */
#define ETH_HLEN 14               /* Header length   */

#endif /* HAVE_LINUX_IF_ETHER_H */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else

#define _NET_ETHERNET_H 1

/* 10Mb/s ethernet header */
struct ether_header
{
  u_int8_t  ether_dhost[ETH_ALEN];      /* destination eth addr */
  u_int8_t  ether_shost[ETH_ALEN];      /* source ether addr    */
  u_int16_t ether_type;                 /* packet type ID field */
} __attribute__ ((__packed__));

#endif /* HAVE_NET_ETHERNET_H */

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#else

#define _NET_IF_ARP_H 1

/* ARP protocol opcodes. */
#define ARPOP_REQUEST   1               /* ARP request.  */
#define ARPOP_REPLY     2               /* ARP reply.  */
#define ARPOP_RREQUEST  3               /* RARP request.  */
#define ARPOP_RREPLY    4               /* RARP reply.  */

struct arphdr
{
  unsigned short int ar_hrd;          /* Format of hardware address.  */
  unsigned short int ar_pro;          /* Format of protocol address.  */
  unsigned char ar_hln;               /* Length of hardware address.  */
  unsigned char ar_pln;               /* Length of protocol address.  */
  unsigned short int ar_op;           /* ARP opcode (command).  */
};

#define ARPHRD_ETHER    1               /* Ethernet 10/100Mbps.  */

#endif /* HAVE_NET_IF_ARP_H */

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#else

#define _NETINET_IF_ETHER_H 1

struct  ether_arp {
        struct  arphdr ea_hdr;          /* fixed-size header */
        u_int8_t arp_sha[ETH_ALEN];     /* sender hardware address */
        u_int8_t arp_spa[4];            /* sender protocol address */
        u_int8_t arp_tha[ETH_ALEN];     /* target hardware address */
        u_int8_t arp_tpa[4];            /* target protocol address */
};

#define arp_hrd ea_hdr.ar_hrd
#define arp_pro ea_hdr.ar_pro
#define arp_hln ea_hdr.ar_hln
#define arp_pln ea_hdr.ar_pln
#define arp_op  ea_hdr.ar_op

#endif /* HAVE_NETINET_IF_ETHER_H */

struct arp_packet {
  struct ether_header eh;
  struct arphdr ah;
  u_int8_t arp_sha[ETH_ALEN];
  u_int8_t arp_spa[4];
  u_int8_t arp_tha[ETH_ALEN];
  u_int8_t arp_tpa[4];
};

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#else

#define _NETINET_IP_H 1

#include <endian.h>

struct iphdr
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error "Please fix <bits/endian.h>"
#endif
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
    /*The options start here. */
};
#endif

#ifdef HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#else

#define _NETINET_UDP_H 1

struct udphdr
{
  __extension__ union
  {
    struct
    {
      u_int16_t uh_sport;               /* source port */
      u_int16_t uh_dport;               /* destination port */
      u_int16_t uh_ulen;                /* udp length */
      u_int16_t uh_sum;         /* udp checksum */
    };
    struct
    {
      u_int16_t source;
      u_int16_t dest;
      u_int16_t len;
      u_int16_t check;
    };
  };
};

#endif /* HAVE_NETINET_UDP_H */

#endif /* NETDEFS_H */
