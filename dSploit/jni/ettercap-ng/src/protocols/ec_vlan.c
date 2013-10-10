/*
    ettercap -- VLAN (802.1q) decoder module

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

    $Id: ec_vlan.c,v 1.2 2004/06/13 18:22:52 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_capture.h>

/* globals */

struct vlan_header
{
   u_int16  vlan;                    /* vlan identifier */
   u_int16  proto;                   /* packet type ID field */
};

/* protos */

FUNC_DECODER(decode_vlan);
void vlan_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init vlan_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_VLAN, decode_vlan);
}


FUNC_DECODER(decode_vlan)
{
   FUNC_DECODER_PTR(next_decoder);
   struct vlan_header *vlan;

   DECODED_LEN = sizeof(struct vlan_header);
   
   vlan = (struct vlan_header *)DECODE_DATA;

   /* HOOK POINT : HOOK_PACKET_VLAN */
   hook_point(HOOK_PACKET_VLAN, po);
   
   /* leave the control to the next decoder */   
   next_decoder = get_decoder(NET_LAYER, ntohs(vlan->proto));

   EXECUTE_DECODER(next_decoder);
      
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

