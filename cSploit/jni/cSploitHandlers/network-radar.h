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
#ifndef HANDLERS_NETWORKRADAR_H
#define HANDLERS_NETWORKRADAR_H

enum nrdr_action {
  NRDR_HOST_ADD,  ///< new host found
  NRDR_HOST_EDIT, ///< new info found about an host
  NRDR_HOST_DEL,  ///< host lost
};

struct nrdr_host_info {
  char      nrdr_action;    ///< must be set to ::NRDR_HOST_ADD or ::NRDR_HOST_EDIT
  uint32_t  ip_addr;        ///< IP address of the host
  uint8_t   eth_addr[6];    ///< Ethernet MAC address of the host
  char      name[];         ///< hostname
} __attribute__ ((__packed__));

struct nrdr_host_del_info {
  char      nrdr_action;    ///< must be set to ::NRDR_HOST_DEL
  uint32_t  ip_addr;        ///< IP address of the lost host
};


message *nrdr_output_parser(char *);

#endif
