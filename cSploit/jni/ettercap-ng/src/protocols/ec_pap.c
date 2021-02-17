/*
    ettercap -- PPP PAP decoder module

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

struct ppp_pap_header
{
   u_int8   version;
   u_int8   session;
   u_int16  id;
   u_int16  len;
   u_int16  proto;      /* this is actually part of the PPP header */
};

/* protos */

FUNC_DECODER(decode_ppp_pap);
void ppp_pap_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ppp_pap_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_PAP, decode_ppp_pap);
}


FUNC_DECODER(decode_ppp_pap)
{
   FUNC_DECODER_PTR(next_decoder);
   struct ppp_pap_header *ppp_pap;

   DECODED_LEN = sizeof(struct ppp_pap_header);

   ppp_pap = (struct ppp_pap_header *)DECODE_DATA;

   /* HOOK POINT : HOOK_PACKET_PPP_PAP */
   hook_point(HOOK_PACKET_PPP_PAP, po);

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

