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

#include <string.h>
#include <errno.h>
#include <assert.h>

#include "list.h"
#include "analyzer.h"
#include "netdefs.h"
#include "logger.h"
#include "prober.h"
#include "ifinfo.h"
#include "host.h"
#include "nbns.h"

struct analyzer_data analyze;

struct packet_node {
  node *next;
  int pktlen; ///< size of the packet in bytes
  int caplen; ///< captured bytes of this packet
  char *packet; ///< packet bytes
};

#if defined(PROFILE) && !defined(MAX)
#define MAX(a, b) (a > b ? a : b)
#endif

/**
 * @brief enqueue a packet to the analyzer list
 * @param packet packet bytes .
 * @param pktlen total length of @p packet .
 * @param caplen amount of captured bytes of @p packet .
 * @returns 0 if packets has been enqueued, -1 if not, -2 if memory error occurs.
 */
int enqueue_packet(char *packet, int pktlen, int caplen) {
  struct packet_node *p;
  char analyze_it;
#ifndef HAVE_LIBPCAP
  struct ether_header *eth;
  struct ether_arp *arp;
  struct iphdr *ip;
  struct udphdr *udp;
#endif
  
  p = malloc(sizeof(struct packet_node));
  
  if(!p) {
    print(ERROR, "malloc: %s", strerror(errno));
    return -2;
  }
  
  p->packet = packet;
  p->pktlen = pktlen;
  p->caplen = caplen;
  analyze_it = 0;

#ifndef HAVE_LIBPCAP
  // do what pcap-filter does
  
  eth = (struct ether_header *) p->packet;
  
  // the following is the same as this pacp filter:
  // ( arp and ether src not $interface_ethernet_address ) or (udp and port 137)
  
  if(eth->ether_type == htons(ETH_P_IP)) {
    ip = (struct iphdr *) (eth+1);
    if(ip->protocol == IPPROTO_UDP) {
      udp = (struct udphdr *) (((uint32_t *)ip) + ip->ihl);
      if(udp->dest == htons(137) || udp->source == htons(137)) {
        analyze_it = 1;
      }
    }
  } else if(eth->ether_type == htons(ETH_P_ARP)) {
    arp = (struct ether_arp *) (eth+1);
    
    if(memcmp(arp->arp_sha, ifinfo.eth_addr, ETH_ALEN)) {
      analyze_it = 1;
    }
  }
  
  if(!analyze_it) {
    free(p);
    return -1;
  }
#endif

  pthread_mutex_lock(&(analyze.control.mutex));
  analyze_it = analyze.control.active;
  if(analyze_it) {
    queue_put(&(analyze.list), (node *) p);
#ifdef PROFILE
    analyze.queue_size += p->caplen;
    analyze.queue_count++;
    analyze.queue_max_count = MAX(analyze.queue_max_count, analyze.queue_count);
    analyze.queue_max_size = MAX( analyze.queue_max_size, analyze.queue_size);
#endif
  }
  pthread_mutex_unlock(&(analyze.control.mutex));
  
  pthread_cond_broadcast(&(analyze.control.cond));
  
  if(!analyze_it) {
    free(p);
    return -1;
  }
  return 0;
}

void analyze_arp(struct ether_arp *arp) {
  uint8_t *s_mac, *d_mac;
  uint32_t s_ip, d_ip;
  
  static const uint8_t arp_hwaddr_any[ETH_ALEN] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  // sanity check
  if(arp->arp_hln != ETH_ALEN || arp->arp_pln != 4) {
    print( WARNING, "corrupted ethernet frame");
    return;
  }
  
  s_mac = (uint8_t *) arp->arp_sha;
  s_ip  = *((uint32_t *) arp->arp_spa);
  d_mac = (uint8_t *) arp->arp_tha;
  d_ip  = *((uint32_t *) arp->arp_tpa);
  
  assert(memcmp(s_mac, ifinfo.eth_addr, ETH_ALEN) != 0);
  
  if(s_ip != INADDR_ANY && memcmp(s_mac, arp_hwaddr_any, ETH_ALEN)) {
    on_host_found(s_mac, s_ip, NULL, 0);
  }
  
  if(d_ip != INADDR_ANY && d_ip != ifinfo.ip_addr) {
    if(memcmp(d_mac, arp_hwaddr_any, ETH_ALEN)) {
      on_host_found(d_mac, d_ip, NULL, 0);
    } else if (!get_host(d_ip)) {
      // we dont have this host and someone want to talk to him
      begin_arp_lookup(d_ip);
    }
  }
}

void analyze_udp(struct ether_header *eth) {
  struct iphdr *ip;
  struct udphdr *udp;
  struct nbnshdr *nb;
  char *nbname;
  uint32_t host_ip;
  uint8_t *host_mac;
  
  ip = (struct iphdr *) (eth+1);
  udp = NULL;
  nb = NULL;
  nbname = NULL;
  
  if(!memcmp(eth->ether_dhost, ifinfo.eth_addr, ETH_ALEN)) {
    // received UDP packet
    
    udp = (struct udphdr *) (((uint32_t *)ip) + ip->ihl);
    nb = (struct nbnshdr *) (((uint8_t *)udp) + sizeof(struct udphdr));
    
    host_mac = eth->ether_shost;
    
    host_ip = ip->saddr;
  } else {
    // sent UDP packet
    
    host_mac = eth->ether_dhost;
    host_ip = ip->daddr;
  }
  
  if(nb) {
    nbname = nbns_get_status_name(nb);
  }
  
  on_host_found(host_mac, host_ip, nbname, HOST_LOOKUP_NBNS);
}

void *analyzer(void *arg) {
  struct packet_node *p;
  struct ether_header *eth;
#ifdef PROFILE
  unsigned long int sz;
  const char *sz_str;
#endif
  
  pthread_mutex_lock(&(analyze.control.mutex));
  
  while(analyze.control.active) {
    
    while(analyze.control.active && !(analyze.list.head)) {
      pthread_cond_wait(&(analyze.control.cond), &(analyze.control.mutex));
    }
    
    p = (struct packet_node *) queue_get(&(analyze.list));
    
    if(!p) continue;
    
#ifdef PROFILE
    analyze.queue_count--;
    analyze.queue_size -= p->caplen;
#endif
    
    pthread_mutex_unlock(&(analyze.control.mutex));
    
    eth = (struct ether_header *) p->packet;
    
    if(eth->ether_type == htons(ETH_P_ARP)) {
      analyze_arp((struct ether_arp *) (eth+1));
    } else {
      analyze_udp(eth);
    }
    
    free(p->packet);
    free(p);
    
    pthread_mutex_lock(&(analyze.control.mutex));
  }
  
  while(analyze.list.head) {
    p = (struct packet_node *) queue_get(&(analyze.list));
    
    free(p->packet);
    free(p);
  }
  
#ifdef PROFILE
  if(analyze.queue_max_size < 1024) {
    sz = analyze.queue_max_size;
    sz_str = "B";
  } else if(analyze.queue_max_size < (1024 * 1024)) {
    sz = (analyze.queue_max_size % 1024);
    sz_str = "KB";
  } else if(analyze.queue_max_size < (1024 * 1024 * 1024)) {
    sz = (analyze.queue_max_size % (1024 * 1024));
    sz_str = "MB";
  } else {
    sz = (analyze.queue_max_size % (1024 * 1024 * 1024));
    sz_str = "GB";
  }
  
  print(  DEBUG, "queue maximum usage: %lu packets, for a total of %lu %s",
          analyze.queue_max_count, sz, sz_str);

#endif
          
  pthread_mutex_unlock(&(analyze.control.mutex));
  
  return NULL;
}

int start_analyzer() {
  if(pthread_create(&(analyze.tid), NULL, analyzer, NULL)) {
    print( ERROR, "pthread_create: %s", strerror(errno));
    return -1;
  }
  return 0;
}

void stop_analyzer() {
  pthread_t tid;
  
  pthread_mutex_lock(&(analyze.control.mutex));
  tid = analyze.tid;
  analyze.tid = 0;
  analyze.control.active = 0;
  pthread_mutex_unlock(&(analyze.control.mutex));
  
  pthread_cond_broadcast(&(analyze.control.cond));
  
  if(!tid) return;
  
  pthread_join(tid, NULL);
}
