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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <csploit/logger.h>

#include "netdefs.h"
#include "sniffer.h"
#include "ifinfo.h"
#include "host.h"
#include "nbns.h"
#include "prober.h"
#include "event.h"
#include "scanner.h"

struct prober_data prober_info;
  
struct prober_host_copy {
  uint32_t ip;
  uint8_t mac[ETH_ALEN];
  time_t timeout;
};

int init_prober() {
#ifdef HAVE_LIBPCAP
  char err_buff[PCAP_ERRBUF_SIZE];
  
#else
  struct sockaddr_ll sll;
  int err;
  socklen_t errlen;
  
#endif
  
  prober_info.nbns_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  
  if(prober_info.nbns_sockfd == -1) {
    print( ERROR, "socket: %s", strerror(errno));
    return -1;
  }
  
#ifdef HAVE_LIBPCAP

  *err_buff = '\0';

  prober_info.handle = pcap_open_live(ifinfo.name, ifinfo.mtu + ETH_HLEN, 0, 1000, err_buff);
  
  if(!(prober_info.handle)) {
    print( ERROR, "pcap_open_live: %s", err_buff);
    close(prober_info.nbns_sockfd);
    return -1;
  }
  
  if(*err_buff) {
    print( WARNING, "pcap_open_live: %s", err_buff);
  }
  
#else
  
  prober_info.arp_sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  
  if(prober_info.arp_sockfd == -1) {
    print( ERROR, "socket: %s", strerror(errno));
    close(prober_info.nbns_sockfd);
    return -1;
  }
  
  memset(&sll, 0, sizeof(sll));
  
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ARP);
  sll.sll_ifindex = ifinfo.index;
  
  if(bind(prober_info.arp_sockfd, (struct sockaddr *) &sll, sizeof(sll)) == -1) {
    print( ERROR, "bind: %s", strerror(errno));
    close(prober_info.nbns_sockfd);
    close(prober_info.arp_sockfd);
    return -1;
  }
  
  errlen = sizeof(err);
  
  if (getsockopt(prober_info.arp_sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
    print( ERROR, "getsockopt: %s", strerror(errno));
    close(prober_info.nbns_sockfd);
    close(prober_info.arp_sockfd);
    return -1;
  }
  
  if(err > 0) {
    print( ERROR, "bind: %s", strerror(err));
    close(prober_info.nbns_sockfd);
    close(prober_info.arp_sockfd);
    return -1;
  }
  
#endif
  
  // build the ethernet header
  
  memcpy(prober_info.arp_request.eh.ether_shost, ifinfo.eth_addr, ETH_ALEN);
  memset(prober_info.arp_request.eh.ether_dhost, 0xFF, ETH_ALEN);
  prober_info.arp_request.eh.ether_type = htons(ETH_P_ARP);
  
  // build arp header
  
  prober_info.arp_request.ah.ar_hrd = htons(ARPHRD_ETHER);
  prober_info.arp_request.ah.ar_pro = htons(ETH_P_IP);
  prober_info.arp_request.ah.ar_hln = ETH_ALEN;
  prober_info.arp_request.ah.ar_pln = 4;
  prober_info.arp_request.ah.ar_op  = htons(ARPOP_REQUEST);
  
  // build arp message constants
  
  memcpy(prober_info.arp_request.arp_sha, ifinfo.eth_addr, ETH_ALEN);
  memcpy(prober_info.arp_request.arp_spa, &(ifinfo.ip_addr), 4);
  memset(prober_info.arp_request.arp_tha, 0x00, ETH_ALEN);
  
  return 0;
}

void begin_nbns_lookup(uint32_t ip) {
  struct sockaddr_in addr;
  
  if(prober_info.nbns_sockfd == -1) return;
  
  memset(&addr, 0, sizeof(addr));
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(137);
  
  addr.sin_addr.s_addr = ip;
    
  if(sendto(prober_info.nbns_sockfd, nbns_nbstat_request, NBNS_NBSTATREQ_LEN, 0, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    print( ERROR, "sendto(%u.%u.%u.%u): %s",
           ((uint8_t *) &ip)[0], ((uint8_t *) &ip)[1], ((uint8_t *) &ip)[2], ((uint8_t *) &ip)[3],
           strerror(errno));
  }
}

static void send_arp_probe(uint32_t ip, uint8_t *mac) {
#ifndef HAVE_LIBPCAP
  struct sockaddr_ll sll;
#endif
  
  if(mac) {
    memcpy(prober_info.arp_request.eh.ether_dhost, mac, ETH_ALEN);
  } else {
    memset(prober_info.arp_request.eh.ether_dhost, 0xFF, ETH_ALEN);
  }
  
  memcpy(&(prober_info.arp_request.arp_tpa), &ip, 4);
  
#ifdef HAVE_LIBPCAP
  if(pcap_inject(prober_info.handle, &(prober_info.arp_request), sizeof(struct arp_packet)) == -1) {
    print( WARNING, "pcap_inject: %s", pcap_geterr(prober_info.handle));
  }
#else
  memset(&sll, 0, sizeof(sll));
  
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = ifinfo.index;
  sll.sll_protocol = htons(ETH_P_ARP);
  memset(&(sll.sll_addr), 0xFF, ETH_ALEN);
  
  if(sendto(prober_info.arp_sockfd, &(prober_info.arp_request), sizeof(struct arp_packet),
              0, (struct sockaddr *) &sll, sizeof(sll)) == -1) {
    print( WARNING, "sendto: %s", strerror(errno));
  }
  
#endif
}

void begin_arp_lookup(uint32_t ip) {
  pthread_mutex_lock(&(prober_info.control.mutex));
  send_arp_probe(ip, NULL);
  pthread_mutex_unlock(&(prober_info.control.mutex));
}

void *prober(void *arg) {
  uint32_t i, size;
  useconds_t delay;
  struct prober_host_copy *tmp;
  struct event *e;
  struct host *h;
  
  local_scan();
  
  delay = 0;
  
  pthread_mutex_lock(&(hosts.control.mutex));
  
  while(hosts.control.active) {
    
    tmp = calloc(sizeof(struct prober_host_copy), hosts.size);
  
    if(!tmp) {
      // wait for some change
      pthread_cond_wait(&(hosts.control.cond), &(hosts.control.mutex));
      continue; // retry
    }
    
    size = hosts.size;
    for(i=0;i<size;i++) {
      tmp[i].ip = hosts.array[i]->ip;
      tmp[i].timeout = hosts.array[i]->timeout;
      memcpy(tmp[i].mac, hosts.array[i]->mac, ETH_ALEN);
    }
    
    pthread_mutex_unlock(&(hosts.control.mutex));
    
    if(size) {
      delay = (LOCAL_SCAN_MS * 1000) / size;
    }
    
    // check for hosts.control.active without locking
    
    for(i=0;i<size && hosts.control.active;i++) {
      
      if(time(NULL) < tmp[i].timeout) {
        
        pthread_mutex_lock(&(prober_info.control.mutex));
        send_arp_probe(tmp[i].ip, tmp[i].mac);
        pthread_mutex_unlock(&(prober_info.control.mutex));
        
      } else {
        pthread_mutex_lock(&(hosts.control.mutex));
        h = get_host(tmp[i].ip);
        if(h) del_host(h);
        pthread_mutex_unlock(&(hosts.control.mutex));
        
        if(h) free_host(h);
        
        e = malloc(sizeof(struct event));
        
        if(e) {
          e->type = MAC_LOST;
          e->ip = tmp[i].ip;
          
          add_event(e);
        } else {
          print( ERROR, "malloc: %s\n", strerror(errno));
        }
      }
      
      usleep(delay);
    }
    
    free(tmp);
    
    pthread_mutex_lock(&(hosts.control.mutex));
  }
  
  pthread_mutex_unlock(&(hosts.control.mutex));
  
  close(prober_info.nbns_sockfd);
  prober_info.nbns_sockfd = -1;
  
#ifdef HAVE_LIBPCAP
  pcap_close(prober_info.handle);
#endif
  
  print( DEBUG, "quitting");
  
  return NULL;
}

void stop_prober() {
  control_deactivate(&(hosts.control));
  shutdown( prober_info.nbns_sockfd, SHUT_WR);
}
