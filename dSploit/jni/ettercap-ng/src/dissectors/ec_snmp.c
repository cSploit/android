/*
    ettercap -- dissector snmp -- UDP 161

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

    $Id: ec_snmp.c,v 1.10 2003/10/28 22:15:04 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_resolv.h>

#define ASN1_INTEGER    2                                                                           
#define ASN1_STRING     4                                                                           
#define ASN1_SEQUENCE   16 

#define SNMP_VERSION_1  0
#define SNMP_VERSION_2c 1
#define SNMP_VERSION_2u 2
#define SNMP_VERSION_3  3

#define MAX_COMMUNITY_LEN 128

/* protos */

FUNC_DECODER(dissector_snmp);
void snmp_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init snmp_init(void)
{
   dissect_add("snmp", APP_LAYER_UDP, 161, dissector_snmp);
}

FUNC_DECODER(dissector_snmp)
{
   DECLARE_DISP_PTR_END(ptr, end);
   size_t clen = 0;
   char tmp[MAX_ASCII_ADDR_LEN];
   u_int32 version, n;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   DEBUG_MSG("SNMP --> UDP dissector_snmp");

   /* get the version */
   while (*ptr++ != ASN1_INTEGER && ptr < end);
   
   /* reached the end */
   if (ptr >= end) return NULL;
      
   /* move to the len */
   ptr += *ptr;

   /* get the version */
   version = *ptr++;

   /* convert the code to the real version */
   if (version++ == 3)
      version = 2;
   else if (version > 3)
      version = 3;
   
   /* move till the community name len */
   while(*ptr++ != ASN1_STRING && ptr < end);
   
   /* reached the end */
   if (ptr >= end) return NULL;

   /* get the community name lenght */
   n = *ptr;
   
   if (n >= 128) {
      n &= ~128;
      ptr += n;
      
      switch(*ptr) {
         case 1:
            clen = *ptr;
            break;
         case 2:
            NS_GET16(clen, ptr);
            break;
         case 3:
            ptr--;
            NS_GET32(clen, ptr);
            clen &= 0xfff;
            break;
         case 4:
            NS_GET32(clen, ptr);
            break;
      }
   } else
      clen = *ptr;

   /* update the pointer */
   ptr++;

   /* Avoid bof */
   if (clen > MAX_COMMUNITY_LEN)
      return NULL;
         
   SAFE_CALLOC(PACKET->DISSECTOR.user, clen + 2, sizeof(char));

   /* fill the structure */
   snprintf(PACKET->DISSECTOR.user, clen + 1, "%s", ptr);
   PACKET->DISSECTOR.pass = strdup(" ");
   PACKET->DISSECTOR.info = strdup("SNMP v ");
   /* put the number in the string */
   PACKET->DISSECTOR.info[6] = version + 48;
   
   DISSECT_MSG("SNMP : %s:%d -> COMMUNITY: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                 ntohs(PACKET->L4.dst), 
                                 PACKET->DISSECTOR.user,
                                 PACKET->DISSECTOR.info);
   

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

