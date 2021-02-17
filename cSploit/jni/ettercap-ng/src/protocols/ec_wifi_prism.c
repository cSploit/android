/*
    ettercap -- Prism2 header for WiFi packets 

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

/* protos */

FUNC_DECODER(decode_prism);
FUNC_ALIGNER(align_prism);
void prism_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init prism_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_PRISM, decode_prism);
   add_aligner(IL_TYPE_PRISM, align_prism);
}


FUNC_DECODER(decode_prism)
{
   FUNC_DECODER_PTR(next_decoder);

   /* Simply skip the first 0x90 Bytes (the Prism2 header) and pass
    * the whole packet on to the wifi layer */
   DECODED_LEN = 0x90;
   
   next_decoder =  get_decoder(LINK_LAYER, IL_TYPE_WIFI);
   EXECUTE_DECODER(next_decoder);

   return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_prism)
{
   /* already aligned */
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

