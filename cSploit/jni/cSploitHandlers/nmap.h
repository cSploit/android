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
 * 
 * 
 */
#ifndef HANDLERS_NMAP_H
#define HANDLERS_NMAP_H

#include <arpa/inet.h>

enum nmap_action {
  HOP,
  PORT,
  SERVICE,
  OS
};

enum nmap_proto {
  TCP,
  UDP,
  ICMP,
  IGMP,
  UNKNOWN
};

struct nmap_hop_info {
  char            nmap_action;    ///< must be set to ::HOP
  uint16_t        hop;            ///< the hop number
  uint32_t        usec;           ///< useconds for reach this address
  in_addr_t       address;        ///< the address
};

struct nmap_port_info {
  char            nmap_action;    ///< must be set to ::PORT
  char            proto;          ///< ::nmap_proto used by this port
  uint16_t        port;           ///< the discovered port
};

struct nmap_service_info {
  char            nmap_action;    ///< must be set to ::SERVICE
  char            proto;          ///< ::nmap_proto used by this port
  uint16_t        port;           ///< the port number
  /**
   * @brief array of null-terminated strings
   * service[0] is the service name
   * service[1] is the verison (can be empty)
   */
  char            service[];
};

struct nmap_os_info {
  char            nmap_action;    ///< must be set to ::OS
  uint8_t         accuracy;       ///< accuracy of this result
  /**
   * @brief array of null-terminated strings
   * os[0] is the OS
   * os[1] is the device type
   */
  char            os[];
};

message *nmap_output_parser(char *);

#endif
