/*
    ettercap -- dissector ldap -- TCP 389

    Copyright (C) ALoR & NaGA
    
    Additional Copyright for this file: LnZ Lorenzo Porro <lporro@libero.it>

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

    $Id: ec_ldap.c,v 1.3 2004/05/31 08:49:05 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_sslwrap.h>

/* protos */

FUNC_DECODER(dissector_ldap);
void ldap_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ldap_init(void)
{
   dissect_add("ldap", APP_LAYER_TCP, 389, dissector_ldap);
   sslw_dissect_add("ldaps", 636, dissector_ldap, SSL_ENABLED);
}

FUNC_DECODER(dissector_ldap)
{
   DECLARE_DISP_PTR_END(ptr, end);
   u_int16 type, user_len, pass_len;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* unused variable */
   (void)end;

   /* Skip ACK packets */
   if (PACKET->DATA.len == 0)
      return NULL;
    
   /* Only packets coming from the server */
   if (FROM_SERVER("ldap", PACKET) || FROM_SERVER("ldaps", PACKET))
      return NULL;

   /* Message Type */
   type = (u_int16)ptr[5];
   
   if (type != 0x60 && type != 0x00) 
      return 0;

   /* Quite self-explaining :) */
   user_len = (u_int16)ptr[11];
   pass_len = (u_int16)ptr[13 + user_len];

   if (user_len == 0) {
      PACKET->DISSECTOR.user = strdup("[Anonymous Bind]");
      PACKET->DISSECTOR.pass = strdup("");
      DISSECT_MSG("LDAP : %s:%d -> Anonymous Bind\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                      ntohs(PACKET->L4.dst));
      return NULL;
   }

   /* user_len and pass_len are u_int16, so no int overflow :) */
   SAFE_CALLOC(PACKET->DISSECTOR.user, user_len + 1, sizeof(char));
   SAFE_CALLOC(PACKET->DISSECTOR.pass, pass_len + 1, sizeof(char));

   memcpy(PACKET->DISSECTOR.user, &ptr[12], user_len);
   memcpy(PACKET->DISSECTOR.pass, &ptr[14] + user_len, pass_len);

   DISSECT_MSG("LDAP : %s:%d -> USER: %s   PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                        ntohs(PACKET->L4.dst),
                                                        PACKET->DISSECTOR.user,
                                                        PACKET->DISSECTOR.pass);
   
   return NULL;
}      

/* EOF */

// vim:ts=3:expandtab

