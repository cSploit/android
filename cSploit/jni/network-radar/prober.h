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
#ifndef PROBER_H
#define PROBER_H

#ifdef HAVE_LIBPCAP
#include <pcap.h>
#endif

#include "netdefs.h"

#include <csploit/control.h>

int init_prober();

void *prober(void *);
void begin_nbns_lookup(uint32_t);
void begin_arp_lookup(uint32_t);
void stop_prober(void);

extern struct prober_data {
  data_control control;
  pthread_t tid;
  int nbns_sockfd;
#ifdef HAVE_LIBPCAP
  pcap_t *handle;
#else
  int arp_sockfd;
#endif
  struct arp_packet arp_request;
} prober_info;

#endif
