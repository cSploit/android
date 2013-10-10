/*
    ettercap -- dissector SOCKS5 -- TCP 1080

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

    $Id: ec_socks.c,v 1.7 2004/01/21 20:20:07 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* protos */

FUNC_DECODER(dissector_socks);
void socks_init(void);

#define USER_PASS 0x02
#define NO_AUTH   0x00
/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init socks_init(void)
{
   dissect_add("socks", APP_LAYER_TCP, 1080, dissector_socks);
}

FUNC_DECODER(dissector_socks)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   u_int16 d_len; /* We don't want heap overflows :) */

   /* don't complain about unused var */
   (void)end;

   /* Skip ACK packets */
   if (PACKET->DATA.len == 0)
      return NULL;

   /* Check the version */
   if (ptr[0] != 0x05)
      return NULL;
    
   /* Packets coming from the server */
   if (FROM_SERVER("socks", PACKET)) {
   
      /* Is this an auth-scheme accept message? */
      if (PACKET->DATA.len != 2)
           return NULL;

      DEBUG_MSG("\tdissector_socks BANNER");
      PACKET->DISSECTOR.banner = strdup("socks v5");

      /* If the server didn't accepted user/pass scheme */
      if (ptr[1] != USER_PASS || ptr[1] != NO_AUTH) 
         return NULL;
    
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_socks));
      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         /* create the new session */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_socks));
	 
         if (ptr[1] == NO_AUTH)
            s->data = strdup("NO AUTH");
	    
         session_put(s);
      }
          
   } else { /* Packets coming from the client */
   
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_socks));
      /* Only if the server accepted user/pass scheme */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         
         DEBUG_MSG("\tDissector_socks USER/PASSWORD");

         /* No Auth required */
         if (s->data) {
            PACKET->DISSECTOR.user = strdup("SOCKSv5");
            PACKET->DISSECTOR.pass = strdup("No auth required");
            DISSECT_MSG("SOCKS5 : %s:%d -> No Auth Required\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                             ntohs(PACKET->L4.dst));                             
         } else { /* User/pass */
            
            /* Username len */
            d_len = *(++ptr);
	 
            /* Save the username (take care of the null termination) */
            SAFE_CALLOC(PACKET->DISSECTOR.user, d_len + 1, sizeof(char));
            memcpy(PACKET->DISSECTOR.user, ++ptr, d_len);
	 
            /* Reach the password */
            ptr += d_len;
            d_len = *ptr;
         
            /* Save the password (take care of the null termination) */
            SAFE_CALLOC(PACKET->DISSECTOR.pass, d_len + 1, sizeof(char));
            memcpy(PACKET->DISSECTOR.pass, ++ptr, d_len);
         
            DISSECT_MSG("SOCKS5 : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                         ntohs(PACKET->L4.dst),
                                                         PACKET->DISSECTOR.user,
                                                         PACKET->DISSECTOR.pass);
         }
	 
         dissect_wipe_session(PACKET, DISSECT_CODE(dissector_socks));
      }
   }        
   
   SAFE_FREE(ident);
   return NULL;
}      

/* EOF */

// vim:ts=3:expandtab

