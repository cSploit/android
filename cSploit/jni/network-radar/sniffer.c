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

#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <unistd.h>

#include <csploit/logger.h>

#include "netdefs.h"
#include "host.h"
#include "event.h"
#include "nbns.h"
#include "sniffer.h"
#include "resolver.h"
#include "prober.h"
#include "ifinfo.h"
#include "analyzer.h"

struct sniff_data sniffer_info;

/**
 * @brief the sniffer thread
 */
void *sniffer(void *arg) {
#ifdef PROFILE
  int enq_res = 0;
  unsigned long int total_pkts = 0;
  unsigned long int rejected_pkts = 0;
  unsigned long int oom_pkts = 0;
# ifdef BUG81370
  unsigned long int bad_pkts = 0;
# endif
#endif /* PROFILE */
#ifdef HAVE_LIBPCAP
  struct pcap_pkthdr pkthdr;
  char *bytes, *buffcopy;
  
  while((bytes = (char *) pcap_next(sniffer_info.handle, &pkthdr))) {
# ifdef PROFILE
    total_pkts++;
    
    buffcopy = malloc(pkthdr.caplen);
    if(buffcopy) {
      memcpy(buffcopy, bytes, pkthdr.caplen);
      enq_res = enqueue_packet(buffcopy, pkthdr.len, pkthdr.caplen);
      if(enq_res) {
        free(buffcopy);
        if(enq_res == -2)
          oom_pkts++;
        else
          rejected_pkts++;
      }
    } else {
      oom_pkts++;
    }
# else
    buffcopy = malloc(pkthdr.caplen);
    
    if(buffcopy) {
      memcpy(buffcopy, bytes, pkthdr.caplen);
      if(enqueue_packet(buffcopy, pkthdr.len, pkthdr.caplen)) {
        free(buffcopy);
      }
    }
# endif /* PROFILE */
  }
  
  pcap_close(sniffer_info.handle);
  
#else /* HAVE_LIBPCAP */
  int pktlen, caplen;
  struct sockaddr_ll from;
  char *buffcopy;
  socklen_t fromlen;
# ifdef BUG81370
  struct ether_header ethhdr;
# endif

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif
  
  fromlen = sizeof(from);
  memset(&from, 0, fromlen);
  
  /* i decided to not lock the sniffer_info.control.mutex
   * for check sniffer_info.control.active to save time.
   * sniffer_info.control.active is initially set to 1,
   * and changes only one time to 0.
   */
  
  while(sniffer_info.control.active) {
    
    pktlen = recvfrom( sniffer_info.sockfd, sniffer_info.buffer,
                       sniffer_info.bufflen, MSG_TRUNC,
                       (struct sockaddr *) &from, &fromlen);
    
    if(pktlen == -1) {
      if(errno == EINTR) {
        continue;
      } else if(errno != EBADF || (sniffer_info.control.active)) {
        print( ERROR, "recvfrom: %s", strerror(errno));
      }
      break;
    } else if(pktlen == 0) { // socket has been shutted down
      break;
    } else if(pktlen < ETH_HLEN) {
      continue;
    }
    
    caplen = MIN(pktlen, sniffer_info.bufflen);
    
# ifdef PROFILE
    total_pkts++;
# endif
    
# ifdef BUG81370
    memcpy(&ethhdr, sniffer_info.buffer, ETH_HLEN);
    if(memcmp(ethhdr.ether_shost, from.sll_addr, ETH_ALEN)) {
#  ifdef PROFILE
      bad_pkts++;
#  endif
      continue;
    }
# endif
    
    buffcopy = malloc(caplen);
    

    if(!buffcopy) {
# ifdef PROFILE
      oom_pkts++;
# endif
      continue;
    }
    
    memcpy(buffcopy, sniffer_info.buffer, caplen);
    
# ifdef BUG81370
    memcpy(buffcopy, &ethhdr, ETH_HLEN);
# endif

# ifdef PROFILE
    enq_res = enqueue_packet(buffcopy, pktlen, caplen);
    if(enq_res) {
      free(buffcopy);
      if(enq_res == -2) {
        oom_pkts++;
      } else {
        rejected_pkts++;
      }
    }
# else
    if(enqueue_packet(buffcopy, pktlen, caplen)) {
      free(buffcopy);
    }
# endif
    
  }

#endif /* HAVE_LIBPCAP */

#ifdef PROFILE
# ifdef BUG81370
  print( DEBUG, "sniffed %lu packets, %lu corrupted, %lu lost for OOM, %lu rejected",
          total_pkts, bad_pkts, oom_pkts, rejected_pkts);
# else
  print( DEBUG, "packets: %lu sniffed, %lu lost for OOM, %lu rejected",
          total_pkts, oom_pkts, rejected_pkts);
# endif /* BUG81370 */
#endif /* PROFILE */

  print( DEBUG, "quitting");

  free(sniffer_info.buffer);

  return NULL;
}

/**
 * @brief start sniffing
 * @returns 0 on success, -1 on error.
 */
int start_sniff() {
#ifdef HAVE_LIBPCAP
  char errbuff[PCAP_ERRBUF_SIZE];
  char filter_str[81];
  struct bpf_program filter;
  
  *errbuff = '\0';

  sniffer_info.handle = pcap_open_live(ifinfo.name, ifinfo.mtu + ETH_HLEN, 0, 0, errbuff);
  
  if(!sniffer_info.handle) {
    print( ERROR, "pcap_open_live: %s", errbuff);
    return -1;
  }
  
  if(*errbuff) {
    print( WARNING, "pcap_open_live: %s", errbuff);
  }
  
  snprintf(filter_str, 81,
    "((( arp or rarp ) and ether src not %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx ) or ( udp and port 137 ))",
    ifinfo.eth_addr[0], ifinfo.eth_addr[1], ifinfo.eth_addr[2],
    ifinfo.eth_addr[3], ifinfo.eth_addr[4], ifinfo.eth_addr[5]);
  
  if(pcap_compile(sniffer_info.handle, &filter, filter_str, 1, (bpf_u_int32) ifinfo.ip_mask)) {
    print( ERROR, "pcap_compile: %s", pcap_geterr(sniffer_info.handle));
    print( DEBUG, "filter: '%s'", filter_str);
    goto error;
  }
  
  if(pcap_setfilter(sniffer_info.handle, &filter)) {
    print( ERROR, "pcap_setfilter: %s", pcap_geterr(sniffer_info.handle));
    goto error;
  }
  
#else /* HAVE_LIBPCAP */
  struct sockaddr_ll  sll;
  int     err;
  socklen_t   errlen = sizeof(err);
  
  /* pcap_open_live for Linux */
  memset(&sll, 0, sizeof(sll));
  sll.sll_family    = AF_PACKET;
  sll.sll_ifindex   = ifinfo.index;
  sll.sll_protocol  = htons(ETH_P_ALL);
  
  sniffer_info.sockfd = socket(PF_PACKET, SOCK_RAW, sll.sll_protocol);
  
  if(sniffer_info.sockfd == -1) {
    print( ERROR, "socket: %s", strerror(errno));
    return -1;
  }

  if (bind(sniffer_info.sockfd, (struct sockaddr *) &sll, sizeof(sll)) == -1) {
    print( ERROR, "bind: %s\n", strerror(errno));
    goto error;
  }

  /* Any pending errors, e.g., network is down? */

  if (getsockopt(sniffer_info.sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
    print( ERROR, "getsockopt: %s\n", strerror(errno));
    goto error;
  }

  if (err > 0) {
    print( ERROR, "bind: %s\n", strerror(err));
    goto error;
  }
  
  /* pcap_open_live ends */

#endif /* HAVE_LIBPCAP */
  
  sniffer_info.bufflen = ifinfo.mtu + ETH_HLEN;
  
  sniffer_info.buffer = malloc(sniffer_info.bufflen);
  
  if(!(sniffer_info.buffer)) {
    print( ERROR, "malloc: %s", strerror(errno));
    goto error;
  }
  
  if(init_hosts()) {
    print( ERROR, "init_hosts: %s", strerror(errno));
    goto hosts_error;
  }
  
  if(pthread_create(&(sniffer_info.tid), NULL, sniffer, NULL)) {
    print( ERROR, "pthread_create: %s", strerror(errno));
    goto pthread_error;
  }
  
  return 0;
  
  pthread_error:
  sniffer_info.tid = 0;
  free(hosts.array);
  
  hosts_error:
  free(sniffer_info.buffer);
  
  error:
#ifdef HAVE_LIBPCAP
  pcap_close(sniffer_info.handle);
#else
  close(sniffer_info.sockfd);
#endif
  
  return -1;
}

/**
 * @brief stop the sniffer thread
 */
void stop_sniff() {
  pthread_t tid;

  pthread_mutex_lock(&(sniffer_info.control.mutex));
  tid = sniffer_info.tid;
  sniffer_info.tid = 0;
  sniffer_info.control.active = 0;
  pthread_mutex_unlock(&(sniffer_info.control.mutex));
  
  if(!tid)
    return;
  
#ifdef HAVE_LIBPCAP
  pcap_breakloop(sniffer_info.handle);
#else
  close(sniffer_info.sockfd);
#endif
  pthread_join(tid, NULL);
}
