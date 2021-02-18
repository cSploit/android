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
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include "netdefs.h"
#include "logger.h"
#include "prober.h"
#include "ifinfo.h"
#include "host.h"

#include "scanner.h"

struct private_network {
  uint32_t subnet;
  uint8_t  prefix_sz;
};

static struct private_network private_networks[] =  { 
  { 0xC0A80000, 16 }, // 192.168.0.0/16
  { 0xAC100000, 12 }, // 172.16.0.0/12
  { 0x0A000000, 8 }  // 10.0.0.0/8
};

/**
 * @brief perform a full scan sending a NBSTAT request to all possible hosts
 *
 * does not scan hosts on local subnet
 */
void full_scan() {
  uint32_t i, n, broadcast, ip;
  useconds_t delay;
  
  if(prober_info.nbns_sockfd == -1) return;
  
  i = PRIVATE_NETWORKS_HOSTS;
  
  for(n=0;n<32 && !((ifinfo.ip_mask >> n) & 1); n++);
  
  i-= pow(2, n) - 2;
  
  delay = (FULL_SCAN_MS * 1000) / i;
  
  print( DEBUG, "delay is %u us", delay);
  
  for(n=0;n<3 && hosts.control.active;n++) {
    
    i = private_networks[n].subnet + 1;
    broadcast = i | (0xFFFFFF >> private_networks[n].prefix_sz);
    
    while(i<broadcast && hosts.control.active) {
      
      ip = htonl(i);
      
      if((ip & ifinfo.ip_mask) != ifinfo.ip_subnet) {
        begin_nbns_lookup(ip);
        usleep(delay);
      } else {
        // skip local network
        i = ifinfo.ip_broadcast;
      }
      
      i++;
    }
  }
}

/**
 * @brief perform a quick scan sending ARP requests to all hosts of our subnet
 */
void local_scan() {
  uint32_t i, broadcast;
  useconds_t delay;
  
  if(prober_info.nbns_sockfd == -1) return;
  
  i = ntohl(ifinfo.ip_addr & ifinfo.ip_mask) + 1;
  broadcast = ntohl(ifinfo.ip_broadcast);
  
  delay =  (LOCAL_SCAN_MS * 1000) / (broadcast - i);
  
  print( DEBUG, "delay is %u us", delay);
  
  for(;i<broadcast && hosts.control.active;i++) {
    
    begin_arp_lookup(htonl(i));
    
    usleep(delay);
  }
}
