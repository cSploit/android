/*
    ettercap -- Raw IP decoder module

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

/* protos */

FUNC_DECODER(decode_rawip);
FUNC_ALIGNER(align_rawip);
void rawip_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init rawip_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_RAWIP, decode_rawip);
   add_aligner(IL_TYPE_RAWIP, align_rawip);
}


FUNC_DECODER(decode_rawip)
{
   FUNC_DECODER_PTR(next_decoder);

   DECODED_LEN = 0;

   /* 
    * there is no L2 header, it is raw ip, 
    * so skip the L2 and invoke directly
    * the L3 dissector
    */
   PACKET->L2.header = (u_char *)DECODE_DATA;
   PACKET->L2.proto = IL_TYPE_RAWIP;
   PACKET->L2.len = DECODED_LEN;

   next_decoder =  get_decoder(NET_LAYER, LL_TYPE_IP);
   EXECUTE_DECODER(next_decoder);

   return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_rawip)
{
   /* already aligned */
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

