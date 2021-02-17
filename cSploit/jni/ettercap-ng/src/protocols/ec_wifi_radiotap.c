/*
    ettercap -- RadioTap header for WiFi packets

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

struct radiotap_header {
   u_int8  version;
   u_int8  pad;
   u_int16 len;
   u_int32 present_flags;
      #define RADIO_PRESENT_TSFT    0x01
      #define RADIO_PRESENT_FLAGS   0x02
      #define RADIO_PRESENT_RATE    0x04
      #define RADIO_PRESENT_CHANNEL 0x08
};

#define RADIO_FLAGS_FCS  0x10

/* protos */

FUNC_DECODER(decode_radiotap);
FUNC_ALIGNER(align_radiotap);
void radiotap_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init radiotap_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_RADIO, decode_radiotap);
   add_aligner(IL_TYPE_RADIO, align_radiotap);
}


FUNC_DECODER(decode_radiotap)
{
   FUNC_DECODER_PTR(next_decoder);
   struct radiotap_header *radio;
   u_char *rh;
   u_int8 flags = 0;

   /* get the header */
   radio = (struct radiotap_header *)DECODE_DATA;
   rh = (u_char *)(radio + 1);

   /* get the length of the header */
   DECODED_LEN = radio->len;

   /*
	* scan for the presence of the information
	* we are lucky since the FLAGS we are searching is the second field
	* and we don't have to scan for all of them
	*/
   if ((radio->present_flags & RADIO_PRESENT_TSFT)) {
	  /* the TSFT is 1 byte */
	  rh += 1;
   }

   if ((radio->present_flags & RADIO_PRESENT_FLAGS)) {
	  /* read the flags (1 byte) */
	  flags = *rh;
   }

   /* mark the packet, since we have an FCS at the end */
   if ((flags & RADIO_FLAGS_FCS))
	  PACKET->L2.flags |= PO_L2_FCS;

   next_decoder = get_decoder(LINK_LAYER, IL_TYPE_WIFI);
   EXECUTE_DECODER(next_decoder);

   return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_radiotap)
{
   /* already aligned */
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

