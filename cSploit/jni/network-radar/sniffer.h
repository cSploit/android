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
#ifndef SNIFFER_H
#define SNIFFER_H

#ifdef HAVE_LIBPCAP
# include <pcap.h>
#endif
#include <pthread.h>

#include <csploit/control.h>

extern struct sniff_data {
  data_control control;
  pthread_t tid;
  char *buffer;
  int bufflen;
#ifdef HAVE_LIBPCAP
  pcap_t *handle;
#else
  int sockfd;
#endif
} sniffer_info;

int start_sniff(void);
void stop_sniff(void);

#endif
