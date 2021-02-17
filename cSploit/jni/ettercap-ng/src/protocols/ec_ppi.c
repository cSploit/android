/*
    ettercap -- per packet information processor

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

FUNC_DECODER(decode_ppi);
FUNC_ALIGNER(align_ppi);
void radiotap_init(void);

/*******************************************/
void __init ppi_init(void)
{
    add_decoder(LINK_LAYER, IL_TYPE_PPI, decode_ppi);
    add_aligner(IL_TYPE_PPI, align_ppi);
}

FUNC_DECODER(decode_ppi)
{
    FUNC_DECODER_PTR(next_decoder);
    unsigned char* data = DECODE_DATA;

    if (PACKET->len <= 4)
    {
        // not enough data to be useful ppi
        return NULL;
    }

    if (data[0] != 0)
    {
        // unknown version field
        return NULL;
    }

    if (data[1] != 0)
    {
        // TODO handle unaligned data. What would this look like?
        return NULL;
    }

    uint16_t header_len = (data[3] << 8) | (data[2]);
    if (header_len >= PACKET->len)
    {
        // not enough data to be interesting
        return NULL;
    }

    char next_hop = data[4];
    DECODED_LEN = header_len;
    next_decoder =  get_decoder(LINK_LAYER, next_hop);
    EXECUTE_DECODER(next_decoder);
    return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_ppi)
{
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

