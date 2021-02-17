/*
    ettercap -- MPLS decoder module

    Copyright (C) The Ettercap Dev Team

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

*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

/* globals */

struct mpls_header
{
   u_int32  shim;
   /*
    * 20 bit for the label
    * 3 bit for priority
    * 1 bit for stack bit (1 is the end of the stack, 0 is in the stack: other mpls headers)
    * 8 bit for time to live
    */
};

/* protos */

FUNC_DECODER(decode_mpls);
void mpls_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init mpls_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_MPLS, decode_mpls);
}


FUNC_DECODER(decode_mpls)
{
   FUNC_DECODER_PTR(next_decoder);
   struct mpls_header *mpls;

   DECODED_LEN = sizeof(struct mpls_header);

   mpls = (struct mpls_header *)DECODE_DATA;

   /* HOOK POINT : HOOK_PACKET_mpls */
   hook_point(HOOK_PACKET_MPLS, po);

   /* check the end of stack bit */
   if (mpls->shim & 0x00010000) {
      /* leave the control to the IP decoder */
      next_decoder = get_decoder(NET_LAYER, LL_TYPE_IP);
   } else {
      /* leave the control to the another MPLS header */
      next_decoder = get_decoder(NET_LAYER, LL_TYPE_MPLS);
   }

   EXECUTE_DECODER(next_decoder);

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

