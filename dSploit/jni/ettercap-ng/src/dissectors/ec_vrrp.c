/*
    ettercap -- dissector vrrp -- works over IP !

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

    $Id: ec_vrrp.c,v 1.3 2003/10/28 22:15:04 alor Exp $
*/

/*
 * RFC: 2338
 * 
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |Version| Type  | Virtual Rtr ID|   Priority    | Count IP Addrs|
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |   Auth Type   |   Adver Int   |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                         IP Address (1)                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                            .                                  |
 *    |                            .                                  |
 *    |                            .                                  |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                         IP Address (n)                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                     Authentication Data (1)                   |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                     Authentication Data (2)                   |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

/* globals */

struct vrrp_hdr {                                                                                
   u_char  ver;         /* Version */
   u_char  id;          /* Virtual Router ID */
   u_char  prio;        /* Router Priority */
   u_char  naddr;       /* # of addresses */
   u_char  auth;        /* Type of Authentication */
   u_char  adv;         /* ADVERTISEMENT Interval */
   u_short csum;        /* Checksum */
};  

#define VRRP_AUTH_NONE          0
#define VRRP_AUTH_SIMPLE        1
#define VRRP_AUTH_AH            2
#define VRRP_AUTH_DATA_LEN      8

/* protos */

FUNC_DECODER(dissector_vrrp);
void vrrp_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init vrrp_init(void)
{
   dissect_add("vrrp", PROTO_LAYER, NL_TYPE_VRRP, dissector_vrrp);
}

/* 
 * the passwords collected by vrrp will not be logged
 * in logfile since it is not over TCP or UDP.
 * anyway we can print them in the user message window
 */

FUNC_DECODER(dissector_vrrp)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct vrrp_hdr *vhdr;
   char *auth;

   /* don't complain about unused var */
   (void)end;

   /* skip empty packets */
   if (PACKET->DATA.len < sizeof(struct vrrp_hdr))
      return NULL;

   DEBUG_MSG("VRRP --> dissector_vrrp");
  
   vhdr = (struct vrrp_hdr *)ptr;

   /* not an authenticated message */
   if (ntohs(vhdr->auth) != VRRP_AUTH_SIMPLE)
      return NULL;

   /* point to the auth */
   auth = ptr + sizeof(struct vrrp_hdr) + (vhdr->naddr * IP_ADDR_LEN);
   
   DISSECT_MSG("VRRP : %s:%d -> AUTH: %s \n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                             ntohs(PACKET->L4.dst), 
                                             auth);

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

