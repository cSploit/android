/*
    ettercap -- send the the wire functions

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

#if defined(OS_DARWIN) || defined(OS_BSD)
   #include <net/bpf.h>
   #include <sys/ioctl.h>
#endif

#include <ec_packet.h>
#include <ec_send.h>
#include <ec_network.h>

#include <pthread.h>
#include <pcap.h>

#include <libnet.h>

#define PCAP_TIMEOUT 10


/* globals */

u_int8 MEDIA_BROADCAST[MEDIA_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
u_int8 ARP_BROADCAST[MEDIA_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static SLIST_HEAD (, build_entry) builders_table;

struct build_entry {
   u_int8 dlt;
   FUNC_BUILDER_PTR(builder);
   SLIST_ENTRY (build_entry) next;
};

/* protos */

int send_to_L3(struct packet_object *po);
int send_to_L2(struct packet_object *po);
int send_to_bridge(struct packet_object *po);
int send_to_iface(struct packet_object *po, struct iface_env *iface);

void capture_only_incoming(pcap_t *p, libnet_t *l);

void add_builder(u_int8 dlt, FUNC_BUILDER_PTR(builder));
libnet_ptag_t ec_build_link_layer(u_int8 dlt, u_int8 *dst, u_int16 proto, libnet_t* l);

int send_arp(u_char type, struct ip_addr *sip, u_int8 *smac, struct ip_addr *tip, u_int8 *tmac);
int send_L2_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac);
int send_L3_icmp(u_char type, struct ip_addr *sip, struct ip_addr *tip);
int send_L3_icmp_echo(struct ip_addr *src, struct ip_addr *tgt);
int send_icmp_redir(u_char type, struct ip_addr *sip, struct ip_addr *gw, struct packet_object *po);
int send_dhcp_reply(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int8 *dhcp_hdr, u_int8 *options, size_t optlen);
int send_dns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 anws_rr, u_int16 auth_rr, u_int16 addi_rr);
int send_mdns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 anws_rr, u_int16 auth_rr, u_int16 addi_rr);
int send_tcp(struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags, u_int8 *payload, size_t length);
int send_udp(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 sport, u_int16 dport, u_int8 *payload, size_t length);
int send_tcp_ether(u_int8 *dmac, struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags);
int send_L3_icmp_unreach(struct packet_object *po);

#ifdef WITH_IPV6
int send_icmp6_echo(struct ip_addr *sip, struct ip_addr *tip);
int send_icmp6_nadv(struct ip_addr *sip, struct ip_addr *tip, struct ip_addr *tgt, u_int8 *macaddr, int router);
#endif

static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
#define SEND_LOCK     do{ pthread_mutex_lock(&send_mutex); } while(0)
#define SEND_UNLOCK   do{ pthread_mutex_unlock(&send_mutex); } while(0)


/*******************************************/

/*
 * send the packet at layer 3
 * the eth header will be handled by the kernel
 */
int send_to_L3(struct packet_object *po)
{
   libnet_ptag_t t;
   libnet_t *l;
   char tmp[MAX_ASCII_ADDR_LEN];
   int c;

   switch(ntohs(po->L3.src.addr_type)) {
      case AF_INET:  l = GBL_LNET->lnet_IP4;
                     break;
      case AF_INET6: l = GBL_LNET->lnet_IP6;
                     break;
      default:       l = NULL;
                     break;
   }

   /* Do not send the packet if corresponding
    * libnet handler is not initialized
    */
   if(l == NULL)
      return -ENOTHANDLED;
   
   SEND_LOCK;

   
   t = libnet_build_data(po->fwd_packet, po->fwd_len, l, 0);
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(l));
   c = libnet_write(l);
   //ON_ERROR(c, -1, "libnet_write %d (%d): %s", po->fwd_len, c, libnet_geterror(l));
   if (c == -1)
      USER_MSG("SEND L3 ERROR: %d byte packet (%04x:%02x) destined to %s was not forwarded (%s)\n", 
            po->fwd_len, ntohs(po->L3.proto), po->L4.proto, ip_addr_ntoa(&po->L3.dst, tmp), 
            libnet_geterror(l));
   
   /* clear the pblock */
   libnet_clear_packet(l);
   
   SEND_UNLOCK;
   
   return c;
}


/*
 * send the packet at layer 2
 * this can be used to send ARP messages
 */

int send_to_L2(struct packet_object *po)
{
   return send_to_iface(po, GBL_IFACE); 
}

/*
 * send the packet to the bridge
 */

int send_to_bridge(struct packet_object *po)
{
   return send_to_iface(po, GBL_BRIDGE);
}

int send_to_iface(struct packet_object *po, struct iface_env *iface)
{
   libnet_ptag_t t;
   int c;
   libnet_t *l;

   if(iface->unoffensive)
      return -EINVALID;

   /* if not lnet warn the developer ;) */
   BUG_IF(iface->lnet == NULL);
   l = iface->lnet;   
   SEND_LOCK;

   t = libnet_build_data(po->packet, po->len, l, 0);

   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(l));
   
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write %d (%d): %s", po->len, c, libnet_geterror(l));
   
   /* clear the pblock */
   libnet_clear_packet(l);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * we MUST not sniff packets sent by us at link layer.
 * expecially useful in bridged sniffing.
 *
 * so we have to find a solution...
 */
void capture_only_incoming(pcap_t *p, libnet_t *l)
{
#ifdef OS_LINUX   
   /*
    * a dirty hack to use the same socket for pcap and libnet.
    * both the structures contains a "int fd" field representing the socket.
    * we can close the fd opened by libnet and use the one already in use by pcap.
   */
   
   DEBUG_MSG("hack_pcap_lnet (before) pcap %d | lnet %d", pcap_fileno(p), l->fd);

   /* needed to avoid double execution (for portstealing) */
   if (pcap_fileno(p) == l->fd)
      return;
      
   /* close the lnet socket */
   close(libnet_getfd(l));
   /* use the socket opened by pcap */
   l->fd = pcap_fileno(p);
   
   DEBUG_MSG("hack_pcap_lnet  (after) pcap %d | lnet %d", pcap_fileno(p), l->fd);
#endif

#ifdef OS_BSD
   /*
    * under BSD we cannot hack the fd as in linux... 
    * pcap opens the /dev/bpf in O_RDONLY and lnet needs O_RDWR
    * 
    * so (if supported: only FreeBSD) we can set the BIOCSSEESENT to 0 to 
    * see only incoming packets
    * but this is unconfortable, because we will not able to sniff ourself.
    */
   #ifdef OS_BSD_FREE
      int val = 0;
   
      DEBUG_MSG("hack_pcap_lnet: setting BIOCSSEESENT on pcap handler");
     
      /* set it to 0 to capture only incoming traffic */
      ioctl(pcap_fileno(p), BIOCSSEESENT, &val);
   #else
      DEBUG_MSG("hack_pcap_lnet: not applicable on this OS");
   #endif
#endif
      
#ifdef OS_DARWIN
   int val = 0;
   
   DEBUG_MSG("hack_pcap_lnet: setting BIOCSSEESENT on pcap handler");
   
   /* not all darwin versions support BIOCSSEESENT */
   #ifdef BIOCSSEESENT
   ioctl(pcap_fileno(p), BIOCSSEESENT, &val);
   #endif
#endif

#ifdef OS_SOLARIS
   DEBUG_MSG("hack_pcap_lnet: not applicable on this OS");
#endif
   
#ifdef OS_CYGWIN
   DEBUG_MSG("hack_pcap_lnet: not applicable on this OS");
#endif
   
}

/*
 * register a builder in the table
 * a builder is a function to create a link layer header.
 */
void add_builder(u_int8 dlt, FUNC_BUILDER_PTR(builder))
{
   struct build_entry *e;

   SAFE_CALLOC(e, 1, sizeof(struct build_entry));
   
   e->dlt = dlt;
   e->builder = builder;

   SLIST_INSERT_HEAD(&builders_table, e, next); 
   
   return;
   
}

/*
 * build the header calling the registered
 * function for the current media
 */
libnet_ptag_t ec_build_link_layer(u_int8 dlt, u_int8 *dst, u_int16 proto, libnet_t* l)
{
   struct build_entry *e;

   SLIST_FOREACH (e, &builders_table, next) {
      if (e->dlt == dlt) {
         return e->builder(dst, proto, l);
      }
   }

   /* on error return -1 */
   return -1;
}


/*
 * helper function to send out an ARP packet
 */
int send_arp(u_char type, struct ip_addr *sip, u_int8 *smac, struct ip_addr *tip, u_int8 *tmac)
{
   libnet_ptag_t t;
   libnet_t *l;
   int c;

   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == NULL);
   l = GBL_IFACE->lnet;

   SEND_LOCK;

   /* ARP uses 00:00:00:00:00:00 broadcast */
   if (type == ARPOP_REQUEST && tmac == MEDIA_BROADCAST)
      tmac = ARP_BROADCAST;
   
   /* create the ARP header */
   t = libnet_build_arp(
           ARPHRD_ETHER,            /* hardware addr */
           ETHERTYPE_IP,            /* protocol addr */
           MEDIA_ADDR_LEN,          /* hardware addr size */
           IP_ADDR_LEN,             /* protocol addr size */
           type,                    /* operation type */
           smac,                    /* sender hardware addr */
           (u_char *)&(sip->addr),  /* sender protocol addr */
           tmac,                    /* target hardware addr */
           (u_char *)&(tip->addr),  /* target protocol addr */
           NULL,                    /* payload */
           0,                       /* payload size */
           l,                       /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_arp: %s", libnet_geterror(l));
   
   /* MEDIA uses ff:ff:ff:ff:ff:ff broadcast */
   if (type == ARPOP_REQUEST && tmac == ARP_BROADCAST)
      tmac = MEDIA_BROADCAST;
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_ARP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* send the packet */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));
   
   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}

/*
 * helper function to send out an ICMP PORT UNREACH packet at layer 3
 */
int send_L3_icmp_unreach(struct packet_object *po)
{
   libnet_ptag_t t;
   int c;
   libnet_t *l;

   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet_IP4 == 0);
   l = GBL_LNET->lnet_IP4;
   SEND_LOCK;


   /* create the ICMP header */
   t = libnet_build_icmpv4_echo(
           3,                       /* type */
           3,                       /* code */
           0,                       /* checksum */
           htons(EC_MAGIC_16),      /* identification number */
           htons(EC_MAGIC_16),      /* sequence number */
           po->L3.header,           /* payload */
           po->L3.len + 8,          /* payload size */
           l,                       /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_echo: %s", libnet_geterror(l));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   t = libnet_build_ipv4(                                                                          
           LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H,       /* length */                                    
           0,                                          /* TOS */                                       
           htons(EC_MAGIC_16),                         /* IP ID */                                     
           0,                                          /* IP Frag */                                   
           64,                                         /* TTL */                                       
           IPPROTO_ICMP,                               /* protocol */                                  
           0,                                          /* checksum */                                  
           ip_addr_to_int32(&po->L3.dst.addr),         /* source IP */                                 
           ip_addr_to_int32(&po->L3.src.addr),         /* destination IP */                            
           NULL,                                       /* payload */                                   
           0,                                          /* payload size */                              
           l,                                          /* libnet handle */                             
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
 
   /* send the packet to Layer 3 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}


/*
 * helper function to send out an ICMP ECHO packet at layer 3
 */
int send_L3_icmp(u_char type, struct ip_addr *sip, struct ip_addr *tip)
{
   libnet_ptag_t t;
   int c;
   libnet_t *l;

   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet_IP4 == 0);
   l = GBL_LNET->lnet_IP4;

   SEND_LOCK;


   /* create the ICMP header */
   t = libnet_build_icmpv4_echo(
           type,                    /* type */
           0,                       /* code */
           0,                       /* checksum */
           htons(EC_MAGIC_16),      /* identification number */
           htons(EC_MAGIC_16),      /* sequence number */
           NULL,                    /* payload */
           0,                       /* payload size */
           l,                       /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_echo: %s", libnet_geterror(l));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   t = libnet_build_ipv4(                                                                          
           LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H,       /* length */                                    
           0,                                          /* TOS */                                       
           htons(EC_MAGIC_16),                         /* IP ID */                                     
           0,                                          /* IP Frag */                                   
           64,                                         /* TTL */                                       
           IPPROTO_ICMP,                               /* protocol */                                  
           0,                                          /* checksum */                                  
           ip_addr_to_int32(&sip->addr),               /* source IP */                                 
           ip_addr_to_int32(&tip->addr),               /* destination IP */                            
           NULL,                                       /* payload */                                   
           0,                                          /* payload size */                              
           l,                                          /* libnet handle */                             
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
 
   /* send the packet to Layer 3 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}

int send_L3_icmp_echo(struct ip_addr *src, struct ip_addr *tgt)
{
   return send_L3_icmp(ICMP_ECHO, src, tgt);
}

/*
 * helper function to send out an ICMP ECHO packet at layer 2
 */
int send_L2_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac)
{
   libnet_ptag_t t;
   int c;
   libnet_t *l;
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == 0);
   l = GBL_IFACE->lnet;
   
   SEND_LOCK;

   /* create the ICMP header */
   t = libnet_build_icmpv4_echo(
           type,                    /* type */
           0,                       /* code */
           0,                       /* checksum */
           htons(EC_MAGIC_16),      /* identification number */
           htons(EC_MAGIC_16),      /* sequence number */
           NULL,                    /* payload */
           0,                       /* payload size */
           l,                       /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_echo: %s", libnet_geterror(l));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   t = libnet_build_ipv4(                                                                          
           LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H,       /* length */                                    
           0,                                          /* TOS */                                       
           htons(EC_MAGIC_16),                         /* IP ID */                                     
           0,                                          /* IP Frag */                                   
           64,                                         /* TTL */                                       
           IPPROTO_ICMP,                               /* protocol */                                  
           0,                                          /* checksum */                                  
           ip_addr_to_int32(&sip->addr),               /* source IP */                                 
           ip_addr_to_int32(&tip->addr),               /* destination IP */                            
           NULL,                                       /* payload */                                   
           0,                                          /* payload size */                              
           l,                                          /* libnet handle */                             
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");

   /* send the packet to Layer 2 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));
 
   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}


/*
 * helper function to send out an ICMP REDIRECT packet
 * the header are created on the source packet PO.
 */
int send_icmp_redir(u_char type, struct ip_addr *sip, struct ip_addr *gw, struct packet_object *po)
{
   libnet_ptag_t t;
   libnet_t *l;
   struct libnet_ipv4_hdr *ip;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == 0);
   l = GBL_IFACE->lnet;
  
   /* retrieve the old ip header */
   ip = (struct libnet_ipv4_hdr *)po->L3.header;
   
   SEND_LOCK;

   /* create the fake ip header for the icmp payload */
   t = libnet_build_ipv4(
            LIBNET_IPV4_H + 8,
            //ntohs(ip->ip_len),                   /* original len */
            ip->ip_tos,                          /* original tos */
            ntohs(ip->ip_id),                    /* original id */
            ntohs(ip->ip_off),                   /* original fragments */
            ip->ip_ttl,                          /* original ttl */
            ip->ip_p,                            /* original proto */
            ip->ip_sum,                          /* original checksum */
            ip_addr_to_int32(&ip->ip_src),       /* original source */
            ip_addr_to_int32(&ip->ip_dst),       /* original dest */
            po->L4.header,                       /* the 64 bit of the original datagram */
            8,                                   /* payload size */
            l,
            0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
   /* create the ICMP header */
   t = libnet_build_icmpv4_redirect(
           ICMP_REDIRECT,                       /* type */
           type,                                /* code */
           0,                                   /* checksum */
           ip_addr_to_int32(&gw->addr),         /* gateway ip */
           NULL,                                /* payload */
           0,                                   /* payload len */
           l,                                   /* libnet handle */
           0);                                  /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_redirect: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   t = libnet_build_ipv4(                                                                          
           LIBNET_IPV4_H + LIBNET_ICMPV4_REDIRECT_H + 
           LIBNET_IPV4_H + 8,                            /* length */
           0,                                            /* TOS */
           htons(EC_MAGIC_16),                           /* IP ID */
           0,                                            /* IP Frag */
           64,                                           /* TTL */
           IPPROTO_ICMP,                                 /* protocol */
           0,                                            /* checksum */
           ip_addr_to_int32(&sip->addr),                 /* source IP */
           ip_addr_to_int32(&po->L3.src.addr),           /* destination IP */
           NULL,                                         /* payload */
           0,                                            /* payload size */
           l,                                            /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
 
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, po->L2.src, ETHERTYPE_IP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
  
   /* 
    * send the packet to Layer 2
    * (sending icmp redirect is not permitted at layer 3)
    */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}

#ifdef WITH_IPV6
int send_icmp6_echo(struct ip_addr *sip, struct ip_addr *tip)
{
   libnet_ptag_t t;
   struct libnet_in6_addr src, dst;
   int c;
   libnet_t *l;

   BUG_IF(GBL_LNET->lnet_IP6 == NULL);
   l = GBL_LNET->lnet_IP6;

   SEND_LOCK;


   memcpy(&src, sip->addr, sizeof(src));
   memcpy(&dst, tip->addr, sizeof(dst));

   t = libnet_build_icmpv6_echo(ICMP6_ECHO_REQUEST,   /* type */
                                0,                    /* code */
                                0,                    /* checksum */
                                EC_MAGIC_16,          /* id */
                                0,                    /* sequence number */
                                NULL,                 /* data */
                                0,                    /* its size */
                                l,                    /* handle */
                                0);
   ON_ERROR(t, -1, "libnet_build_icmpv6_echo: %s", libnet_geterror(l));
   libnet_toggle_checksum(l, t, LIBNET_ON);

   t = libnet_build_ipv6(0,                           /* tc */
                         0,                           /* flow label */
                         LIBNET_ICMPV6_H,             /* next header size */
                         IPPROTO_ICMPV6,              /* next header */
                         64,                          /* hop limit */
                         src,                         /* source */
                         dst,                         /* destination */
                         NULL,                        /* payload and size */
                         0,
                         l,                           /* handle */
                         0);                          /* ptag */
   ON_ERROR(t, -1, "libnet_build_ipv6: %s", libnet_geterror(l));
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write: %s", libnet_geterror(l));

   libnet_clear_packet(l);

   SEND_UNLOCK;

   return c;
}

/* 
 * Sends neighbor solicitation request (like arp request with ipv4)
 * macaddr parameter allows to add sender's mac address. This is an option for unicast requests.
 * See RFC4861 for more information.
 */
int send_icmp6_nsol(struct ip_addr *sip, struct ip_addr *tip, struct ip_addr *req, u_int8 *macaddr)
{  
   libnet_ptag_t t;
   int c, h = 0;
   struct libnet_in6_addr src, dst, r;
   libnet_t *l;

   BUG_IF(GBL_LNET->lnet_IP6 == NULL);
   l = GBL_LNET->lnet_IP6;

   SEND_LOCK;


   memcpy(&src, sip->addr, sizeof(src));
   memcpy(&dst, tip->addr, sizeof(dst));
   memcpy(&r, req->addr, sizeof(r));

   if(macaddr != NULL) {
      t = libnet_build_icmpv6_ndp_opt(ND_OPT_SOURCE_LINKADDR,   /* Address type */
                                      macaddr,                  /* MAC address */
                                      MEDIA_ADDR_LEN,           /* Address length */
                                      l,                        /* libnet handle */
                                      0);                       /* ptag */
      ON_ERROR(t, -1, "libnet_build_icmpv6_ndp_opt: %s", libnet_geterror(l));
      /* base header size + address size rouded to 8 */
      h += LIBNET_ICMPV6_NDP_OPT_H + 8;
   }

   t = libnet_build_icmpv6_ndp_nsol(ND_NEIGHBOR_SOLICIT,        /* type */
                                    0,                          /* code */
                                    0,                          /* checksum */
                                    r,
                                    NULL,                       /* payload */
                                    0,                          /* its size */
                                    l,                          /* libnet handler */
                                    0);                         /* ptag */
   ON_ERROR(t, -1, "libnet_build_icmpv6_ndp_nsol: %s", libnet_geterror(l));
   libnet_toggle_checksum(l, t, LIBNET_ON);
   h += LIBNET_ICMPV6_NDP_NSOL_H;
   
   t = libnet_build_ipv6(0,                                     /* tc */
                         0,                                     /* flow label */
                         h,                                     /* length */
                         IPPROTO_ICMP6,                         /* proto */
                         64,                                    /* hop limit */
                         src,                                   /* source address */
                         dst,                                   /* target address */
                         NULL,                                  /* payload */
                         0,                                     /* its size */
                         l,                                     /* handle */
                         0);                                    /* ptag */
   ON_ERROR(t, -1, "libnet_build_ipv6: %s", libnet_geterror(l));
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write: %s", libnet_geterror(l));

   libnet_clear_packet(l);

   SEND_UNLOCK;

   return c;
}

int send_icmp6_nadv(struct ip_addr *sip, struct ip_addr *tip, struct ip_addr *tgt, u_int8 *macaddr, int router)
{
   libnet_ptag_t t;
   int c, h = 0;
   struct libnet_in6_addr src, dst;
   int flags;
   libnet_t *l;
   
   BUG_IF(GBL_LNET->lnet_IP6 == NULL);
   l = GBL_LNET->lnet_IP6;

   SEND_LOCK;


   memcpy(&src, sip->addr, sizeof(src));
   memcpy(&dst, tip->addr, sizeof(dst));

   t = libnet_build_icmpv6_ndp_opt(ND_OPT_TARGET_LINKADDR,   /* Address type */
                                   macaddr,                  /* MAC address */
                                   MEDIA_ADDR_LEN,           /* Address length */
                                   l,                        /* libnet handle */
                                   0);                       /* ptag */
   ON_ERROR(t, -1, "libnet_build_icmpv6_ndp_lla: %s", libnet_geterror(l));
   h += LIBNET_ICMPV6_NDP_OPT_H + 6;

   flags = ND_NA_FLAG_SOLICITED|ND_NA_FLAG_OVERRIDE;
   if(router)
      flags |= ND_NA_FLAG_ROUTER;
   t = libnet_build_icmpv6_ndp_nadv(ND_NEIGHBOR_ADVERT,  /* type */
                                    0,                   /* code */
                                    0,                   /* checksum */
                                    flags,               /* flags */
                                    src,                 /* address */
                                    NULL,                /* payload */
                                    0,                   /* payload size */
                                    l,                   /* libnet handle */
                                    0);                  /* ptag */
   ON_ERROR(t, -1, "libnet_build_icmpv6_ndp_nadv: %s", libnet_geterror(l));
   libnet_toggle_checksum(l, t, LIBNET_ON);
   h += LIBNET_ICMPV6_NDP_NADV_H;
   
   t = libnet_build_ipv6(0,                  /* tc */
                         0,                  /* flow label */
                         h,                  /* length */
                         IPPROTO_ICMP6,      /* proto */
                         64,                 /* hop limit */
                         src,                /* source address */
                         dst,                /* target address */
                         NULL,               /* payload */
                         0,                  /* its size */
                         l,                  /* handle */
                         0);                 /* ptag */
   ON_ERROR(t, -1, "libnet_build_ipv6: %s", libnet_geterror(l));
   
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write: %s", libnet_geterror(l));

   libnet_clear_packet(l);

   SEND_UNLOCK;

   return c;
}
#endif /* WITH_IPV6 */

/*
 * send a dhcp reply to tmac/tip
 */
int send_dhcp_reply(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int8 *dhcp_hdr, u_int8 *options, size_t optlen)
{
   libnet_ptag_t t;
   int c;
   libnet_t *l;
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == 0);
   l = GBL_IFACE->lnet;
  
   SEND_LOCK;
   
   /* add the dhcp options */
   t = libnet_build_data(
            options,                /* the options */
            optlen,                 /* options len */
            l,                      /* libnet handle */
            0);                     /* libnet ptag */
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(l));
   /* create the dhcp header */
   t = libnet_build_data(
            dhcp_hdr,               /* the header */
            LIBNET_DHCPV4_H,        /* dhcp len */
            l,                      /* libnet handle */
            0);                     /* libnet ptag */
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(l));
   /* create the udp header */
   t = libnet_build_udp(
            67,                                             /* source port */
            68,                                             /* destination port */
            LIBNET_UDP_H + LIBNET_DHCPV4_H + optlen,        /* packet size */
            0,                                              /* checksum */
            NULL,                                           /* payload */
            0,                                              /* payload size */
            l,                                              /* libnet handle */
            0);                                             /* libnet id */
   ON_ERROR(t, -1, "libnet_build_udp: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   t = libnet_build_ipv4(                                                                          
           LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_DHCPV4_H + optlen,  /* length */
           0,                                                        /* TOS */
           htons(EC_MAGIC_16),                                       /* IP ID */
           0,                                                        /* IP Frag */
           64,                                                       /* TTL */
           IPPROTO_UDP,                                              /* protocol */
           0,                                                        /* checksum */
           ip_addr_to_int32(&sip->addr),                             /* source IP */
           ip_addr_to_int32(&tip->addr),                             /* destination IP */
           NULL,                                                     /* payload */
           0,                                                        /* payload size */
           l,                                                        /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
 
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* 
    * send the packet to Layer 2
    * (sending icmp redirect is not permitted at layer 3)
    */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}

/*
 * send a mdns reply
 */
int send_mdns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 anws_rr, u_int16 auth_rr, u_int16 addi_rr)
{
   libnet_ptag_t t;
   int c, proto;
   libnet_t *l;
   proto = ntohs(sip->addr_type);

   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == 0);
   l = GBL_IFACE->lnet;
  
   SEND_LOCK;

   /* create the dns packet */
    t = libnet_build_dnsv4(
             LIBNET_UDP_DNSV4_H,    /* TCP or UDP */
             id,                    /* id */
             0x8400,                /* standard reply, no error */
             0,                     /* num_q */
             anws_rr,               /* num_anws_rr */
             auth_rr,               /* num_auth_rr */
             addi_rr,               /* num_addi_rr */
             data,
             datalen,
             l,                     /* libnet handle */
             0);                    /* libnet id */
   ON_ERROR(t, -1, "libnet_build_dns: %s", libnet_geterror(l));
  
   /* create the udp header */
   t = libnet_build_udp(
            5353,                                           /* source port */
            htons(dport),                                   /* destination port */
            LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen,    /* packet size */
            0,                                              /* checksum */
            NULL,                                           /* payload */
            0,                                              /* payload size */
            l,                                              /* libnet handle */
            0);                                             /* libnet id */
   ON_ERROR(t, -1, "libnet_build_udp: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   switch (proto) {
      case AF_INET: {
         t = libnet_build_ipv4(                                                                          
                 LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen, /* length */
                 0,                                                           /* TOS */
                 htons(EC_MAGIC_16),                                          /* IP ID */
                 0,                                                           /* IP Frag */
                 255,                                                         /* TTL */
                 IPPROTO_UDP,                                                 /* protocol */
                 0,                                                           /* checksum */
                 ip_addr_to_int32(&sip->addr),                                /* source IP */
                 ip_addr_to_int32(&tip->addr),                                /* destination IP */
                 NULL,                                                        /* payload */
                 0,                                                           /* payload size */
                 l,                                                           /* libnet handle */ 
                 0);
         ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
         /* auto calculate the checksum */
         libnet_toggle_checksum(l, t, LIBNET_ON);
   
         break;
      }
#ifdef WITH_IPV6
      case AF_INET6: {
         struct libnet_in6_addr src, dst;
         memcpy(&src, sip->addr, sizeof(src));
         memcpy(&dst, tip->addr, sizeof(dst));
         t = libnet_build_ipv6(
               0,                                                             /* traffic class */
               0,                                                             /* flow label */
               LIBNET_IPV6_H + LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen,   /* length */
               IPPROTO_UDP,                                                   /* next header */
               255,                                                           /* hop limit */
               src,                                                           /* source IP */
               dst,                                                           /* destination IP */
               NULL,                                                          /* payload */
               0,                                                             /* payload size */
               l,                                                             /* libnet handle */
               0);
         ON_ERROR(t, -1, "libnet_build_ipv6: %s", libnet_geterror(l));
         break;
      }
#endif
   };
  
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* send the packet to Layer 2 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * send a dns reply
 */
int send_dns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 anws_rr, u_int16 auth_rr, u_int16 addi_rr)
{
   libnet_ptag_t t;
   int c, proto;
   libnet_t *l;

   proto = ntohs(sip->addr_type);
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == 0);
   l = GBL_IFACE->lnet;
  
   SEND_LOCK;

   /* create the dns packet */
    t = libnet_build_dnsv4(
             LIBNET_UDP_DNSV4_H,    /* TCP or UDP */
             id,                    /* id */
             0x8400,                /* standard reply, no error */
             1,                     /* num_q */
             anws_rr,               /* num_anws_rr */
             auth_rr,               /* num_auth_rr */
             addi_rr,               /* num_addi_rr */
             data,
             datalen,
             l,                     /* libnet handle */
             0);                    /* libnet id */
   ON_ERROR(t, -1, "libnet_build_dns: %s", libnet_geterror(l));
   /* create the udp header */
   t = libnet_build_udp(
            53,                                             /* source port */
            htons(dport),                                   /* destination port */
            LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen,    /* packet size */
            0,                                              /* checksum */
            NULL,                                           /* payload */
            0,                                              /* payload size */
            l,                                              /* libnet handle */
            0);                                             /* libnet id */
   ON_ERROR(t, -1, "libnet_build_udp: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   switch (proto) {
      case AF_INET: {
         t = libnet_build_ipv4(                                                                          
                 LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen, /* length */
                 0,                                                           /* TOS */
                 htons(EC_MAGIC_16),                                          /* IP ID */
                 0,                                                           /* IP Frag */
                 64,                                                          /* TTL */
                 IPPROTO_UDP,                                                 /* protocol */
                 0,                                                           /* checksum */
                 ip_addr_to_int32(&sip->addr),                                /* source IP */
                 ip_addr_to_int32(&tip->addr),                                /* destination IP */
                 NULL,                                                        /* payload */
                 0,                                                           /* payload size */
                 l,                                                           /* libnet handle */ 
                 0);
         ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
         /* auto calculate the checksum */
         libnet_toggle_checksum(l, t, LIBNET_ON);
   
         break;
      }
#ifdef WITH_IPV6
      case AF_INET6: {
         struct libnet_in6_addr src, dst;
         memcpy(&src, sip->addr, sizeof(src));
         memcpy(&dst, tip->addr, sizeof(dst));
         t = libnet_build_ipv6(
               0,                                                             /* traffic class */
               0,                                                             /* flow label */
               LIBNET_IPV6_H + LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen,   /* length */
               IPPROTO_UDP,                                                   /* next header */
               255,                                                           /* hop limit */
               src,                                                           /* source IP */
               dst,                                                           /* destination IP */
               NULL,                                                          /* payload */
               0,                                                             /* payload size */
               l,                                                             /* libnet handle */
               0);
         ON_ERROR(t, -1, "libnet_build_ipv6: %s", libnet_geterror(l));
         break;
      }
#endif
   };
  
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* send the packet to Layer 2 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * send an udp packet
 */
int send_udp(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 sport, u_int16 dport, u_int8 *payload, size_t length)
{
	libnet_ptag_t t;
	libnet_t *l;

	int proto;
	int c;

	proto = ntohs(sip->addr_type);

	l = GBL_IFACE->lnet;
	BUG_IF(GBL_IFACE->lnet == 0);

	SEND_LOCK;


/*
 * libnet_build_udp(uint16_t sp, uint16_t dp, uint16_t len, uint16_t sum,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
*/
	t = libnet_build_udp(
		htons(sport),
		htons(dport),
		LIBNET_UDP_H +  length,
		0,
		payload,
		length,
		l,
		0);
	ON_ERROR(t, -1, "libnet_build_udp: %s", libnet_geterror(l));

	/* auto calculate checksum */
	libnet_toggle_checksum(l, t, LIBNET_ON);

	/* create IP header */
	switch(proto) {
		case AF_INET: {
			t = libnet_build_ipv4(
				LIBNET_IPV4_H + LIBNET_UDP_H + length, /* length */
				0,				/* TOS */
				htons(EC_MAGIC_16),		/* IP ID */
				0,				/* IP FRAG */
				64,				/* TTL */
				IPPROTO_UDP,			/* protocol */
				0,				/* checksum */
				ip_addr_to_int32(&sip->addr),	/* source IP */
				ip_addr_to_int32(&tip->addr),	/* destination IP */
				NULL,				
				0,				/* payload size */
				l,
				0);

			libnet_toggle_checksum(l, t, LIBNET_ON);

			break;
		}	

		case AF_INET6: {
			struct libnet_in6_addr src, dst;
			memcpy(&src, sip->addr, sizeof(src));
			memcpy(&dst, tip->addr, sizeof(dst));
			t = libnet_build_ipv6(
				0,		/* tc */
				0,		/* flow label */
				LIBNET_UDP_H + length, /* length */
				IPPROTO_UDP,		/* protocol */
				255,			/* hop limit */
				src,			/* source */
				dst,			/* destination */
				NULL,			/* payload */
				0, 			/* its length */
				l,			/* handle */
				0);			/* ptag */
			break;
		}
	};

	ON_ERROR(t, -1, "libnet_build_ipvX: %s", libnet_geterror(l));

   	/* add the media header */
   	t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP, l);
   	if (t == -1)
      		FATAL_ERROR("Interface not suitable for layer2 sending");

	/* send the packet to Layer 3 */

	c = libnet_write(l);
	ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

	/* clear the block */
	libnet_clear_packet(l);
	SEND_UNLOCK;

	return c;
}
/*
 * send a tcp packet
 */
int send_tcp(struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags, u_int8 *payload, size_t length)
{
   libnet_ptag_t t;
   libnet_t *l;
   int proto;
   int c;
 
   proto = ntohs(sip->addr_type);
   l = (proto == AF_INET) ? GBL_LNET->lnet_IP4 : GBL_LNET->lnet_IP6;   
   /* if not lnet warn the developer ;) */
   BUG_IF(l == NULL);
  
   SEND_LOCK;

   
    t = libnet_build_tcp(
        ntohs(sport),            /* source port */
        ntohs(dport),            /* destination port */
        ntohl(seq),              /* sequence number */
        ntohl(ack),              /* acknowledgement num */
        flags,                   /* control flags */
        32767,                   /* window size */
        0,                       /* checksum */
        0,                       /* urgent pointer */
        LIBNET_TCP_H + length,   /* TCP packet size */
        payload,                 /* payload */
        length,                  /* payload size */
        l,                       /* libnet handle */
        0);                      /* libnet id */
   ON_ERROR(t, -1, "libnet_build_tcp: %s", libnet_geterror(l));
   
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   switch(proto) {
      case AF_INET: {
         t = libnet_build_ipv4(                                                                          
                 LIBNET_IPV4_H + LIBNET_TCP_H,       /* length */
                 0,                                  /* TOS */
                 htons(EC_MAGIC_16),                 /* IP ID */
                 0,                                  /* IP Frag */
                 64,                                 /* TTL */
                 IPPROTO_TCP,                        /* protocol */
                 0,                                  /* checksum */
                 ip_addr_to_int32(&sip->addr),       /* source IP */
                 ip_addr_to_int32(&tip->addr),       /* destination IP */
                 NULL,                               /* payload */
                 0,                                  /* payload size */
                 l,                                  /* libnet handle */ 
                 0);
         libnet_toggle_checksum(l, t, LIBNET_ON);
         break;
      }
      case AF_INET6: {
         struct libnet_in6_addr src, dst;
         memcpy(&src, sip->addr, sizeof(src));
         memcpy(&dst, tip->addr, sizeof(dst));
         t = libnet_build_ipv6(
                  0,                                 /* tc */
                  0,                                 /* flow label */
                  LIBNET_TCP_H,                      /* payload length */
                  IPPROTO_TCP,                       /* protocol */
                  255,                               /* hop limit */
                  src,                               /* source address */
                  dst,                               /* destination address */
                  NULL,                              /* payload */
                  0,                                 /* its length */
                  l,                                 /* handle */
                  0);                                /* ptag */
         break;
      }
   };
   ON_ERROR(t, -1, "libnet_build_ipvX: %s", libnet_geterror(l));
  
   /* send the packet to Layer 3 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}

/*
 * similar to send_tcp but the user can specify the destination mac addresses
 */
int send_tcp_ether(u_int8 *dmac, struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags)
{
   libnet_ptag_t t;
   int c;
   libnet_t *l;

   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_IFACE->lnet == 0);
   l = GBL_IFACE->lnet;
  
   SEND_LOCK;
   
    t = libnet_build_tcp(
        ntohs(sport),            /* source port */
        ntohs(dport),            /* destination port */
        ntohl(seq),              /* sequence number */
        ntohl(ack),              /* acknowledgement num */
        flags,                   /* control flags */
        32767,                   /* window size */
        0,                       /* checksum */
        0,                       /* urgent pointer */
        LIBNET_TCP_H,            /* TCP packet size */
	NULL,                    /* payload */
        0,                       /* payload size */
        l,                       /* libnet handle */
        0);                      /* libnet id */
   ON_ERROR(t, -1, "libnet_build_tcp: %s", libnet_geterror(l));
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
  
   /* create the IP header */
   t = libnet_build_ipv4(                                                                          
           LIBNET_IPV4_H + LIBNET_TCP_H,       /* length */
           0,                                  /* TOS */
           htons(EC_MAGIC_16),                 /* IP ID */
           0,                                  /* IP Frag */
           64,                                 /* TTL */
           IPPROTO_TCP,                        /* protocol */
           0,                                  /* checksum */
           ip_addr_to_int32(&sip->addr),       /* source IP */
           ip_addr_to_int32(&tip->addr),       /* destination IP */
           NULL,                               /* payload */
           0,                                  /* payload size */
           l,                                  /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(l));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(l, t, LIBNET_ON);
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, dmac, ETHERTYPE_IP, l);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
 
   /* send the packet to Layer 3 */
   c = libnet_write(l);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(l));

   /* clear the pblock */
   libnet_clear_packet(l);

   SEND_UNLOCK;
   
   return c;
}


/* EOF */

// vim:ts=3:expandtab

