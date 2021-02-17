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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <csploit/logger.h>

#include "ifinfo.h"
#include "netdefs.h"

struct if_info ifinfo;

/**
 * @brief get info about an interface and store them into ::ifinfo .
 * 
 * @param ifname name of the interface
 * @returns 0 on success, -1 on error.
 */
int get_ifinfo(char *ifname) {
  struct ifreq ifr;
  int fd;
  
  memset(&ifinfo, 0, sizeof(ifinfo));
  
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  
  if(fd == -1) {
    print( WARNING, "socket: %s", strerror(errno));
    return -1;
  }
  
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
  ifr.ifr_name[IFNAMSIZ] = '\0';
  
  if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
    print( WARNING, "ioctl: %s", strerror(errno));
    print( ERROR, "unable to get link layer address");
    goto error;
  }
  
  if(ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
    print( ERROR, "for the moment only Ethernet devices are supported");
    goto error;
  }
  
  memcpy(ifinfo.eth_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
  
  memset(&(ifr.ifr_addr), 0, sizeof(ifr.ifr_addr));
  ifr.ifr_addr.sa_family = AF_INET;
  
  if(ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
    print( WARNING, "ioctl: %s", strerror(errno));
    print( ERROR, "unable to get network address");
    goto error;
  }
  
  ifinfo.ip_addr = ((struct sockaddr_in *) &(ifr.ifr_addr))->sin_addr.s_addr;
  
  if(ioctl(fd, SIOCGIFNETMASK, &ifr) == -1) {
    print( WARNING, "ioctl: %s", strerror(errno));
    print( ERROR, "unable to get network mask");
    goto error;
  }
  
  ifinfo.ip_mask = ((struct sockaddr_in *) &(ifr.ifr_addr))->sin_addr.s_addr;
  
  ifinfo.ip_subnet = ifinfo.ip_addr & ifinfo.ip_mask;
  ifinfo.ip_broadcast = ifinfo.ip_addr | ~(ifinfo.ip_mask);
  
  if(ioctl(fd, SIOCGIFMTU, &ifr) == -1) {
    print( WARNING, "ioctl: %s", strerror(errno));
    print( ERROR, "unable to get MTU");
    goto error;
  }
  
  ifinfo.mtu = ifr.ifr_mtu;
  
#ifndef HAVE_LIBPCAP
  
  if(ioctl(fd, SIOCGIFINDEX, &ifr) == -1) {
    print( WARNING, "ioctl: %s", strerror(errno));
    print( ERROR, "unable to get interface index");
    goto error;
  }
  
  ifinfo.index = ifr.ifr_ifindex;
  
#endif
  
  close(fd);
  
  memcpy(ifinfo.name, ifr.ifr_name, IFNAMSIZ);
  
  return 0;
  
  error:
  
  close(fd);
  
  return -1;
}
