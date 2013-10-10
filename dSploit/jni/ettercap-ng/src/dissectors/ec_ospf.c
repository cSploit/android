/*
    ettercap -- dissector ospf -- works over IP !

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ec_ospf.c,v 1.5 2003/10/28 22:15:03 alor Exp $
*/

/*
 * RFC: 2328
 * 
 *      0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |   Version #   |     Type      |         Packet length         |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                          Router ID                            |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                           Area ID                             |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |           Checksum            |             AuType            |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                       Authentication                          |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                       Authentication                          |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
*/
 
#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

#define OSPF_AUTH_LEN   8
#define OSPF_AUTH       1
#define OSPF_NO_AUTH    0

struct ospf_hdr {
   u_int8   ver;
   u_int8   type;
   u_int16  len;
   u_int32  rid;
   u_int32  aid;
   u_int16  csum;
   u_int16  auth_type;
   u_int32  auth1;
   u_int32  auth2;
};

/* protos */

FUNC_DECODER(dissector_ospf);
void ospf_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ospf_init(void)
{
   dissect_add("ospf", PROTO_LAYER, NL_TYPE_OSPF, dissector_ospf);
}

/* 
 * the passwords collected by ospf will not be logged
 * in logfile since it is not over TCP or UDP.
 * anyway we can print them in the user message window
 */

FUNC_DECODER(dissector_ospf)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ospf_hdr *ohdr;
   char tmp[MAX_ASCII_ADDR_LEN];
   char pass[12];

   /* don't complain about unused var */
   (void)end;

   /* skip empty packets */
   if (PACKET->DATA.len == 0)
      return NULL;

   DEBUG_MSG("OSPF --> dissector_ospf");
  
   ohdr = (struct ospf_hdr *)ptr;
   
   /* authentication */
   if ( ntohs(ohdr->auth_type) == OSPF_AUTH ) {
      
      DEBUG_MSG("\tDissector_ospf PASS");
      
      /* 
       * we use a local variable since this does 
       * not need to reach the top half
       */
      strncpy(pass, (char *)ohdr->auth1, OSPF_AUTH_LEN);
      
   } 

   /* no authentication */
   if ( ntohs(ohdr->auth_type) == OSPF_NO_AUTH ) {
      
      DEBUG_MSG("\tDissector_ospf NO AUTH");
      
      strcpy(pass, "No Auth");
   }
   
   DISSECT_MSG("OSPF : %s:%d -> AUTH: %s \n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                             ntohs(PACKET->L4.dst), 
                                             pass);

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

