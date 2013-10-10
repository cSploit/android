/*
    ettercap -- dissector RIP -- UDP 520

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

    $Id: ec_rip.c,v 1.3 2003/10/28 22:15:04 alor Exp $
*/

/*
 *       RIP version 2      RFC 2453
 *
 *        0                   1                   2                   3
 *        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    0  | command (1)   | version (1)   |      must be zero (2)         |
 *       +---------------+---------------+-------------------------------+
 *    4  | Address Family Identifier (2) |        Route Tag (2)          |
 *       +-------------------------------+-------------------------------+
 *    8  |                         IP Address (4)                        |
 *       +---------------------------------------------------------------+
 *   12  |                         Subnet Mask (4)                       |
 *       +---------------------------------------------------------------+
 *   16  |                         Next Hop (4)                          |
 *       +---------------------------------------------------------------+
 *   20  |                         Metric (4)                            |
 *       +---------------------------------------------------------------+
 *
 *
 *        0                   1                   2                   3 3
 *        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    0  | Command (1)   | Version (1)   |            unused             |
 *       +---------------+---------------+-------------------------------+
 *    4  |             0xFFFF            |    Authentication Type (2)    |
 *       +-------------------------------+-------------------------------+
 *    8  ~                       Authentication (16)                     ~
 *       +---------------------------------------------------------------+
 *
 */
 
#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

struct rip_hdr {
   u_int8   command;
   u_int8   version;
   u_int16  zero;
   u_int16  family;
   u_int16  auth_type;
   u_int8   auth[16];
};

/* protos */

FUNC_DECODER(dissector_rip);
void rip_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init rip_init(void)
{
   dissect_add("rip", APP_LAYER_UDP, 520, dissector_rip);
}

FUNC_DECODER(dissector_rip)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct rip_hdr *rip;

   /* don't complain about unused var */
   (void)end;
   
   /* skip empty packets */
   if (PACKET->DATA.len == 0)
      return NULL;

   DEBUG_MSG("RIP --> UDP dissector_rip");
   
   /* cast the struct */
   rip = (struct rip_hdr *)ptr;
   
   /* switch on the version */
   switch(rip->version) {
      case 2:
         /* address family 0xFF  Tag 2  (AUTH) */
         if ( rip->family == 0xffff && ntohs(rip->auth_type) == 0x0002 ) {
            DEBUG_MSG("\tDissector_RIP version 2 simple AUTH");
            PACKET->DISSECTOR.user = strdup("RIPv2");
            PACKET->DISSECTOR.pass = strdup((char *)rip->auth);
            
            DISSECT_MSG("RIPv2 : %s:%d -> AUTH: %s \n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                   ntohs(PACKET->L4.dst), 
                                                   PACKET->DISSECTOR.pass);
         }
         break;
      case 4:
         /* XXX - TODO RIP v4 */
         break;
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

