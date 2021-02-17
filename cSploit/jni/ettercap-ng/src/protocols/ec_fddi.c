/*
    ettercap -- FDDI decoder module

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
struct fddi_header
{
   u_int8  frame_control;
   u_int8  dha[FDDI_ADDR_LEN];
   u_int8  sha[FDDI_ADDR_LEN];
   u_int8  llc_dsap;
   u_int8  llc_ssap;
   u_int8  llc_control;
   u_int8  llc_org_code[3];
   /*
    * ARGH ! org_core is 3 and it has disaligned the struct !
    * we can rely in on the alignment of the buffer...
    */
   u_int16 proto;
};

/* encapsulated ethernet */
u_int8 FDDI_ORG_CODE[3] = {0x00, 0x00, 0x00};

/* protos */

FUNC_DECODER(decode_fddi);
FUNC_BUILDER(build_fddi);
FUNC_ALIGNER(align_fddi);
void fddi_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init fddi_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_FDDI, decode_fddi);
   add_builder(IL_TYPE_FDDI, build_fddi);
   add_aligner(IL_TYPE_FDDI, align_fddi);
}


FUNC_DECODER(decode_fddi)
{
   FUNC_DECODER_PTR(next_decoder);
   struct fddi_header *fddi;

   DECODED_LEN = sizeof(struct fddi_header);
   
   fddi = (struct fddi_header *)DECODE_DATA;
   
   /* org_code != encapsulated ethernet not yet supported */
   if (memcmp(fddi->llc_org_code, FDDI_ORG_CODE, 3))
      NOT_IMPLEMENTED();

   /* fill the packet object with sensitive data */
   PACKET->L2.header = (u_char *)DECODE_DATA;
   PACKET->L2.proto = IL_TYPE_FDDI;
   PACKET->L2.len = DECODED_LEN;
   
   memcpy(PACKET->L2.src, fddi->sha, FDDI_ADDR_LEN);
   memcpy(PACKET->L2.dst, fddi->dha, FDDI_ADDR_LEN);

   /* HOOK POINT : HOOK_PACKET_fddi */
   hook_point(HOOK_PACKET_FDDI, po);
   
   /* leave the control to the next decoder */   
   next_decoder = get_decoder(NET_LAYER, ntohs(fddi->proto));

   EXECUTE_DECODER(next_decoder);
      
   /* fddi header does not care about modification of upper layer */
   
   return NULL;
}

/*
 * function to create a token ring header 
 */
FUNC_BUILDER(build_fddi)
{
   return libnet_autobuild_fddi(
            LIBNET_FDDI_FC_REQD | 0x04,   /* Asynch LLC - priority 4 */
            dst,                          /* token ring destination */
            LIBNET_SAP_SNAP,              /* DSAP -> SNAP encap */
            LIBNET_SAP_SNAP,              /* SSAP -> SNAP encap */
            0x03,                         /* Unnumbered info/frame */
            FDDI_ORG_CODE,                /* Organization Code */
            proto,                        /* protocol type */
            l);                           /* libnet handle */
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_fddi)
{
   return (24 - sizeof(struct fddi_header));
}

/* EOF */

// vim:ts=3:expandtab

