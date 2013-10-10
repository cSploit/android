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

    $Id: ec_send.c,v 1.60 2004/12/03 09:10:30 alor Exp $
*/

#include <ec.h>

#if defined(OS_DARWIN) || defined(OS_BSD) || defined(__APPLE__)
   #include <net/bpf.h>
   #include <sys/ioctl.h>
#endif

#include <ec_packet.h>
#include <ec_send.h>

#include <pthread.h>
#include <pcap.h>
#include <libnet.h>


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

void send_init(void);
static void send_close(void);
int send_to_L3(struct packet_object *po);
int send_to_L2(struct packet_object *po);
int send_to_bridge(struct packet_object *po);

void capture_only_incoming(pcap_t *p, libnet_t *l);

void add_builder(u_int8 dlt, FUNC_BUILDER_PTR(builder));
libnet_ptag_t ec_build_link_layer(u_int8 dlt, u_int8 *dst, u_int16 proto);

int send_arp(u_char type, struct ip_addr *sip, u_int8 *smac, struct ip_addr *tip, u_int8 *tmac);
int send_L2_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac);
int send_L3_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip);
int send_icmp_redir(u_char type, struct ip_addr *sip, struct ip_addr *gw, struct packet_object *po);
int send_dhcp_reply(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int8 *dhcp_hdr, u_int8 *options, size_t optlen);
int send_dns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 addi_rr);
int send_tcp(struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags);
int send_tcp_ether(u_int8 *dmac, struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags);
int send_L3_icmp_unreach(struct packet_object *po);

static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
#define SEND_LOCK     do{ pthread_mutex_lock(&send_mutex); } while(0)
#define SEND_UNLOCK   do{ pthread_mutex_unlock(&send_mutex); } while(0)

/*******************************************/

/*
 * set up the lnet struct to have a socket to send packets
 */

void send_init(void)
{
   libnet_t *l;
   libnet_t *l3;
   libnet_t *lb;
   char lnet_errbuf[LIBNET_ERRBUF_SIZE];
 
   /* check when to not initialize libnet */
   if (GBL_OPTIONS->read) {
      DEBUG_MSG("send_init: skipping... (reading offline)");
      return;
   }

   /* don't send packet on loopback */
   if (!strcasecmp(GBL_OPTIONS->iface, "lo")) {
      DEBUG_MSG("send_init: using loopback (activating unoffensive mode)");
      GBL_OPTIONS->unoffensive = 1;
   }

   /* in wireless monitor mode we cannot send packets */
   if (GBL_PCAP->dlt == DLT_IEEE802_11) {
      DEBUG_MSG("send_init: skipping... (using wireless in monitor mode)");
      GBL_OPTIONS->unoffensive = 1;
      return;
   }
   
   DEBUG_MSG("send_init %s", GBL_OPTIONS->iface);
   
   /* open the socket at layer 3 */
   l3 = libnet_init(LIBNET_RAW4_ADV, GBL_OPTIONS->iface, lnet_errbuf);               
   ON_ERROR(l3, NULL, "libnet_init(LIBNET_RAW4_ADV) failed: %s", lnet_errbuf);
   
   /* open the socket at layer 2 ( GBL_OPTIONS->iface doesn't matter ) */
   l = libnet_init(LIBNET_LINK_ADV, GBL_OPTIONS->iface, lnet_errbuf);               
   ON_ERROR(l, NULL, "libnet_init(LIBNET_LINK_ADV) failed: %s", lnet_errbuf);
   
   if (GBL_SNIFF->type == SM_BRIDGED) {
      /* open the socket on the other iface for bridging */
      lb = libnet_init(LIBNET_LINK_ADV, GBL_OPTIONS->iface_bridge, lnet_errbuf);               
      ON_ERROR(lb, NULL, "libnet_init() failed: %s", lnet_errbuf);
      GBL_LNET->lnet_bridge = lb;
   }
   
   GBL_LNET->lnet_L3 = l3;               
   GBL_LNET->lnet = l;               
      
   atexit(send_close);
}


static void send_close(void)
{
   libnet_destroy(GBL_LNET->lnet);
   libnet_destroy(GBL_LNET->lnet_L3);

   if (GBL_SNIFF->type == SM_BRIDGED) 
      libnet_destroy(GBL_LNET->lnet_bridge);
   
   DEBUG_MSG("ATEXIT: send_closed");
}

/*
 * send the packet at layer 3
 * the eth header will be handled by the kernel
 */

int send_to_L3(struct packet_object *po)
{
   libnet_ptag_t t;
   char tmp[MAX_ASCII_ADDR_LEN];
   int c;

   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
   
   SEND_LOCK;
   
   t = libnet_build_data(po->fwd_packet, po->fwd_len, GBL_LNET->lnet_L3, 0);
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(GBL_LNET->lnet_L3));
   
   c = libnet_write(GBL_LNET->lnet_L3);
   //ON_ERROR(c, -1, "libnet_write %d (%d): %s", po->fwd_len, c, libnet_geterror(GBL_LNET->lnet_L3));
   if (c == -1)
      USER_MSG("SEND L3 ERROR: %d byte packet (%04x:%02x) destined to %s was not forwarded (%s)\n", 
            po->fwd_len, ntohs(po->L3.proto), po->L4.proto, ip_addr_ntoa(&po->L3.dst, tmp), 
            libnet_geterror(GBL_LNET->lnet_L3));
   
   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet_L3);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * send the packet at layer 2
 * this can be used to send ARP messages
 */

int send_to_L2(struct packet_object *po)
{
   libnet_ptag_t t;
   int c;
   
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
   
   SEND_LOCK;
   
   t = libnet_build_data( po->packet, po->len, GBL_LNET->lnet, 0);
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(GBL_LNET->lnet));
   
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write %d (%d): %s", po->len, c, libnet_geterror(GBL_LNET->lnet));
   
   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * send the packet to the bridge
 */

int send_to_bridge(struct packet_object *po)
{
   libnet_ptag_t t;
   int c;
  
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
   
   SEND_LOCK;

   t = libnet_build_data( po->packet, po->len, GBL_LNET->lnet_bridge, 0);
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(GBL_LNET->lnet_bridge));
   
   c = libnet_write(GBL_LNET->lnet_bridge);
   ON_ERROR(c, -1, "libnet_write %d (%d): %s", po->len, c, libnet_geterror(GBL_LNET->lnet_bridge));
   
   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet_bridge);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * we MUST not sniff packets sent by us at link layer.
 * expecially usefull in bridged sniffing.
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
      
#if defined(OS_DARWIN) || defined(__APPLE__)
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
libnet_ptag_t ec_build_link_layer(u_int8 dlt, u_int8 *dst, u_int16 proto)
{
   struct build_entry *e;

   SLIST_FOREACH (e, &builders_table, next) {
      if (e->dlt == dlt) {
         return e->builder(dst, proto);
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
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
   
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
           GBL_LNET->lnet,          /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_arp: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* MEDIA uses ff:ff:ff:ff:ff:ff broadcast */
   if (type == ARPOP_REQUEST && tmac == ARP_BROADCAST)
      tmac = MEDIA_BROADCAST;
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_ARP);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* send the packet */
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet));
   
   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);

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
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet_L3 == 0);
   
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
           GBL_LNET->lnet_L3,       /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_echo: %s", libnet_geterror(GBL_LNET->lnet_L3));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet_L3, t, LIBNET_ON);
  
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
           GBL_LNET->lnet_L3,                          /* libnet handle */                             
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet_L3));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet_L3, t, LIBNET_ON);
 
   /* send the packet to Layer 3 */
   c = libnet_write(GBL_LNET->lnet_L3);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet_L3));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet_L3);

   SEND_UNLOCK;
   
   return c;
}


/*
 * helper function to send out an ICMP ECHO packet at layer 3
 */
int send_L3_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip)
{
   libnet_ptag_t t;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet_L3 == 0);
   
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
           GBL_LNET->lnet_L3,       /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_echo: %s", libnet_geterror(GBL_LNET->lnet_L3));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet_L3, t, LIBNET_ON);
  
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
           GBL_LNET->lnet_L3,                          /* libnet handle */                             
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet_L3));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet_L3, t, LIBNET_ON);
 
   /* send the packet to Layer 3 */
   c = libnet_write(GBL_LNET->lnet_L3);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet_L3));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet_L3);

   SEND_UNLOCK;
   
   return c;
}

/*
 * helper function to send out an ICMP ECHO packet at layer 2
 */
int send_L2_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac)
{
   libnet_ptag_t t;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
   
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
           GBL_LNET->lnet,          /* libnet handle */
           0);                      /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_echo: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
  
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
           GBL_LNET->lnet,                             /* libnet handle */                             
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");

   /* send the packet to Layer 2 */
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet));
 
   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);

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
   struct libnet_ipv4_hdr *ip;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
  
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
            GBL_LNET->lnet,
            0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* create the ICMP header */
   t = libnet_build_icmpv4_redirect(
           ICMP_REDIRECT,                       /* type */
           type,                                /* code */
           0,                                   /* checksum */
           ip_addr_to_int32(&gw->addr),         /* gateway ip */
           NULL,                                /* payload */
           0,                                   /* payload len */
           GBL_LNET->lnet,                      /* libnet handle */
           0);                                  /* pblock id */
   ON_ERROR(t, -1, "libnet_build_icmpv4_redirect: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
  
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
           GBL_LNET->lnet,                               /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
 
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, po->L2.src, ETHERTYPE_IP);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
  
   /* 
    * send the packet to Layer 2
    * (sending icmp redirect is not permitted at layer 3)
    */
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);

   SEND_UNLOCK;
   
   return c;
}

/*
 * send a dhcp reply to tmac/tip
 */
int send_dhcp_reply(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int8 *dhcp_hdr, u_int8 *options, size_t optlen)
{
   libnet_ptag_t t;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
  
   SEND_LOCK;
   
   /* add the dhcp options */
   t = libnet_build_data(
            options,                /* the options */
            optlen,                 /* options len */
            GBL_LNET->lnet,         /* libnet handle */
            0);                     /* libnet ptag */
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* create the dhcp header */
   t = libnet_build_data(
            dhcp_hdr,               /* the header */
            LIBNET_DHCPV4_H,        /* dhcp len */
            GBL_LNET->lnet,         /* libnet handle */
            0);                     /* libnet ptag */
   ON_ERROR(t, -1, "libnet_build_data: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* create the udp header */
   t = libnet_build_udp(
            67,                                             /* source port */
            68,                                             /* destination port */
            LIBNET_UDP_H + LIBNET_DHCPV4_H + optlen,        /* packet size */
            0,                                              /* checksum */
            NULL,                                           /* payload */
            0,                                              /* payload size */
            GBL_LNET->lnet,                                 /* libnet handle */
            0);                                             /* libnet id */
   ON_ERROR(t, -1, "libnet_build_udp: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
  
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
           GBL_LNET->lnet,                                           /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
 
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* 
    * send the packet to Layer 2
    * (sending icmp redirect is not permitted at layer 3)
    */
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);

   SEND_UNLOCK;
   
   return c;
}

/*
 * send a dns reply
 */
int send_dns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 addi_rr)
{
   libnet_ptag_t t;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
  
   SEND_LOCK;

   /* create the dns packet */
    t = libnet_build_dnsv4(
             LIBNET_UDP_DNSV4_H,    /* TCP or UDP */
             id,                    /* id */
             0x8400,                /* standard reply, no error */
             1,                     /* num_q */
             1,                     /* num_anws_rr */
             0,                     /* num_auth_rr */
             addi_rr,               /* num_addi_rr */
             data,
             datalen,
             GBL_LNET->lnet,        /* libnet handle */
             0);                    /* libnet id */
   ON_ERROR(t, -1, "libnet_build_dns: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* create the udp header */
   t = libnet_build_udp(
            53,                                             /* source port */
            htons(dport),                                   /* destination port */
            LIBNET_UDP_H + LIBNET_UDP_DNSV4_H + datalen,    /* packet size */
            0,                                              /* checksum */
            NULL,                                           /* payload */
            0,                                              /* payload size */
            GBL_LNET->lnet,                                 /* libnet handle */
            0);                                             /* libnet id */
   ON_ERROR(t, -1, "libnet_build_udp: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
  
   /* create the IP header */
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
           GBL_LNET->lnet,                                              /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, tmac, ETHERTYPE_IP);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
   
   /* send the packet to Layer 2 */
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);
   
   SEND_UNLOCK;
   
   return c;
}

/*
 * send a tcp packet
 */
int send_tcp(struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags)
{
   libnet_ptag_t t;
   int c;
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet_L3 == 0);
  
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
        GBL_LNET->lnet_L3,       /* libnet handle */
        0);                                        /* libnet id */
   ON_ERROR(t, -1, "libnet_build_tcp: %s", libnet_geterror(GBL_LNET->lnet_L3));
   
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet_L3, t, LIBNET_ON);
  
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
           GBL_LNET->lnet_L3,                  /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet_L3));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet_L3, t, LIBNET_ON);
 
   /* send the packet to Layer 3 */
   c = libnet_write(GBL_LNET->lnet_L3);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet_L3));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet_L3);

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
 
   /* if not lnet warn the developer ;) */
   BUG_IF(GBL_LNET->lnet == 0);
  
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
        GBL_LNET->lnet,          /* libnet handle */
        0);                                        /* libnet id */
   ON_ERROR(t, -1, "libnet_build_tcp: %s", libnet_geterror(GBL_LNET->lnet));
   
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
  
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
           GBL_LNET->lnet,                     /* libnet handle */ 
           0);
   ON_ERROR(t, -1, "libnet_build_ipv4: %s", libnet_geterror(GBL_LNET->lnet));
  
   /* auto calculate the checksum */
   libnet_toggle_checksum(GBL_LNET->lnet, t, LIBNET_ON);
   
   /* add the media header */
   t = ec_build_link_layer(GBL_PCAP->dlt, dmac, ETHERTYPE_IP);
   if (t == -1)
      FATAL_ERROR("Interface not suitable for layer2 sending");
 
   /* send the packet to Layer 3 */
   c = libnet_write(GBL_LNET->lnet);
   ON_ERROR(c, -1, "libnet_write (%d): %s", c, libnet_geterror(GBL_LNET->lnet));

   /* clear the pblock */
   libnet_clear_packet(GBL_LNET->lnet);

   SEND_UNLOCK;
   
   return c;
}


/* EOF */

// vim:ts=3:expandtab

