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
#include <stdint.h>
#include <errno.h>

#include "nbns.h"

/**
 * @brief header of a NetBIOS status response.
 * 
 * the ::NAME_TRN_ID is omitted
 * 
 * RFC 1002, Paragraph 4.2.18
 */
uint8_t nbns_nbstat_response_header[] = {
  0x84, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00
};

/**
 * @brief static NetBIOS NBSTAT request.
 * 
 * RFC 1002, Paragraph 4.2.17
 */
uint8_t nbns_nbstat_request[NBNS_NBSTATREQ_LEN] = {
    0x82, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x43, 0x4B, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x00, 0x00, 0x21, 0x00, 0x01
};

/**
 * @brief a fast way to extract the NetBIOS name from a NBSTAT response.
 * @param nbpkt the netbios packet to analyze
 * @returns the name on success, NULL on error.
 */
char *nbns_get_status_name(struct nbnshdr *nbpkt) {
  
  uint8_t *p, names, group, node;
  struct nbns_name *node_name;
  struct nbns_node_name_tail *node_name_tail;
  
  p = ((uint8_t *)nbpkt) + 2;
  
  // check header
  
  if(memcmp(p, nbns_nbstat_response_header, 10)) {
    errno = EINVAL;
    return NULL;
  }
  
  p+= 10;
  
  // skip encoded name
  while((*p)) p++;
  p++;
  
  p += sizeof(struct nbns_resource_tail);
  
  // first byte is number of names ( rfc 1002 4.2.18 )
  names = *p;
  p++;
  
  while(names) {
    node_name = (struct nbns_name *) p;
    node_name_tail = (struct nbns_node_name_tail *) (node_name + 1);
    
    group = node_name_tail->flags & NBNS_NODE_NAME_TAIL_GROUP;
    node  = node_name_tail->flags & NBNS_NODE_NAME_TAIL_NODE;
    
    if(!group && node == NBNS_NAME_NODE_B &&
       ( node_name->purpose == NBNS_NAME_PURPOSE_WORKSTATION || node_name->purpose == NBNS_NAME_PURPOSE_FILE)) {      
      
      // find last space from right
      for(names=(NBNS_NAME_SIZE-1); names && node_name->name[names] == ' '; names--);
      
      return strndup((char *) node_name->name, names + 1);
    }
    
    p = (uint8_t *) (node_name_tail+1);
    names--;
  }
  
  errno = ENOENT;
  
  return NULL;
}
