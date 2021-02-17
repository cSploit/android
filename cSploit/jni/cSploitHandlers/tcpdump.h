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
#ifndef HANDLERS_TCPDUMP_H
#define HANDLERS_TCPDUMP_H

#include <arpa/inet.h>

enum tcpdump_action {
  TCPDUMP_PACKET
};

struct tcpdump_packet_info {
  char          tcpdump_action; ///< must be set to ::TCPDUMP_PACKET
  in_addr_t     src;            ///< source address of the packet
  in_addr_t     dst;            ///< destination address of the packet
  uint16_t      len;            ///< length of the packet
};

message *tcpdump_output_parser(char *);

#endif
