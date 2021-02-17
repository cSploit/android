/*
    ettercap -- ERF (endace) decoder module

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
#include <ec_capture.h>

/* globals */

struct erf_header
{
   u_int32  timestamp1;
   u_int32  timestamp2;
   u_int8   type;
   u_int8   flags;
   u_int16  rlen;
   u_int16  color;
   u_int16  wlen;
};

/* protos */

FUNC_DECODER(decode_erf);
FUNC_ALIGNER(align_erf);
void erf_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init erf_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_ERF, decode_erf);
   add_aligner(IL_TYPE_ERF, align_erf);
}


FUNC_DECODER(decode_erf)
{
   FUNC_DECODER_PTR(next_decoder);
   struct erf_header *erf;

   DECODED_LEN = sizeof(struct erf_header);

   erf = (struct erf_header *)DECODE_DATA;

   /* check presence of extension header */
   if (erf->type & 0x80) {
      // "ERF Extension header not supported"
      return NULL;
   }

   /* HOOK POINT : HOOK_PACKET_ERF */
   hook_point(HOOK_PACKET_ERF, po);

   /* ethernet packets */
   if (erf->type == 0x02) {

      /* remove the padding */
      DECODED_LEN += 2;

      /* leave the control to the ethernet decoder */
      next_decoder = get_decoder(LINK_LAYER, IL_TYPE_ETH);

      EXECUTE_DECODER(next_decoder);
   }

   return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_erf)
{
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

