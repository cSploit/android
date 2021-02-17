/*
    ettercap -- Linux cooked decoder module

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

*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_capture.h>

/* globals */
#define COOKED_LEN   16
#define PROTO_OFFSET 14
#define SENT_BY_US   4 

/* protos */

FUNC_DECODER(decode_cook);
FUNC_ALIGNER(align_cook);
void cook_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init cook_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_COOK, decode_cook);
   add_aligner(IL_TYPE_COOK, align_cook);
}


FUNC_DECODER(decode_cook)
{
   FUNC_DECODER_PTR(next_decoder);
   u_int16 proto;
   u_int16 pck_type;
   char bogus_mac[6]="\x00\x01\x00\x01\x00\x01";

   DECODED_LEN = COOKED_LEN;
   proto = pntos(DECODE_DATA + PROTO_OFFSET);
   pck_type = pntos(DECODE_DATA);

   PACKET->L2.header = (u_char *)DECODE_DATA;
   PACKET->L2.proto = IL_TYPE_COOK;
   PACKET->L2.len = DECODED_LEN;

   /* By default L2.src and L2.dst are NULL, so are equal to our
    * "mac address". According to packet type we set bogus source
    * or dest to help other decoders to guess if the packet is for us
    * (check_forwarded, set_forwardable_flag and so on)
    */
   if (pck_type != SENT_BY_US)
      memcpy(PACKET->L2.src, bogus_mac, ETH_ADDR_LEN);
   else
      memcpy(PACKET->L2.dst, bogus_mac, ETH_ADDR_LEN);
   
   next_decoder =  get_decoder(NET_LAYER, proto);
   EXECUTE_DECODER(next_decoder);

   return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_cook)
{
   /* already aligned */
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

