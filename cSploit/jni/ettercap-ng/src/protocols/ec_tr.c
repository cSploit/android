/*
    ettercap -- TOKEN RING decoder module

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
#include <ec_send.h>
#include <ec_capture.h>

/* globals */
struct token_ring_header
{
   u_int8  access_control;
      #define TR_FRAME  0x10
   u_int8  frame_control;
      #define TR_LLC_FRAME  0x40
   u_int8  dha[TR_ADDR_LEN];
   u_int8  sha[TR_ADDR_LEN];
   u_int8  llc_dsap;
   u_int8  llc_ssap;
   u_int8  llc_control;
   u_int8  llc_org_code[3];
   u_int16 proto;
};

/* encapsulated ethernet */
u_int8 TR_ORG_CODE[3] = {0x00, 0x00, 0x00};

/* protos */

FUNC_DECODER(decode_tr);
FUNC_BUILDER(build_tr);
FUNC_ALIGNER(align_tr);
void tr_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init tr_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_TR, decode_tr);
   add_builder(IL_TYPE_TR, build_tr);
   add_aligner(IL_TYPE_TR, align_tr);
}


FUNC_DECODER(decode_tr)
{
   FUNC_DECODER_PTR(next_decoder);
   struct token_ring_header *tr;

   DECODED_LEN = sizeof(struct token_ring_header);
   
   tr = (struct token_ring_header *)DECODE_DATA;

   /* org_code != encapsulated ethernet not yet supported */
   if (memcmp(tr->llc_org_code, TR_ORG_CODE, 3))
      NOT_IMPLEMENTED();
   
   /* fill the packet object with sensitive data */
   PACKET->L2.header = (u_char *)DECODE_DATA;
   PACKET->L2.proto = IL_TYPE_TR;
   PACKET->L2.len = DECODED_LEN;
   
   memcpy(PACKET->L2.src, tr->sha, TR_ADDR_LEN);
   memcpy(PACKET->L2.dst, tr->dha, TR_ADDR_LEN);

   /* HOOK POINT : HOOK_PACKET_tr */
   hook_point(HOOK_PACKET_TR, po);
   
   /* leave the control to the next decoder */   
   next_decoder = get_decoder(NET_LAYER, ntohs(tr->proto));

   EXECUTE_DECODER(next_decoder);
      
   /* token ring header does not care about modification of upper layer */
   
   return NULL;
}

/*
 * function to create a token ring header 
 */
FUNC_BUILDER(build_tr)
{
   return libnet_autobuild_token_ring(
            LIBNET_TOKEN_RING_FRAME,
            LIBNET_TOKEN_RING_LLC_FRAME,  /* LLC - Normal buffer */
            dst,                          /* token ring destination */
            LIBNET_SAP_SNAP,              /* DSAP -> SNAP encap */
            LIBNET_SAP_SNAP,              /* SSAP -> SNAP encap */
            0x03,                         /* Unnumbered info/frame */
            TR_ORG_CODE,                  /* Organization Code */
            proto,                        /* protocol type */
            l);                           /* libnet handle */
   
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_tr)
{
   return (24 - sizeof(struct token_ring_header));
}

/* EOF */

// vim:ts=3:expandtab

