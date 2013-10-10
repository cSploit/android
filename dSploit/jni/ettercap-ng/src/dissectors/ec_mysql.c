/*
    ettercap -- dissector MySQL -- TCP 3306

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

    $Id: ec_mysql.c,v 1.8 2004/01/21 20:20:06 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* protos */

FUNC_DECODER(dissector_mysql);
void mysql_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init mysql_init(void)
{
   dissect_add("mysql", APP_LAYER_TCP, 3306, dissector_mysql);
}

FUNC_DECODER(dissector_mysql)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];


   /* Skip ACK packets */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_mysql));
 
   /* Packets coming from the server */
   if (FROM_SERVER("mysql", PACKET)) {

      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         u_int index = 5;
    
         /* Check magic numbers and catch the banner */
         if (!memcmp(ptr + 1, "\x00\x00\x00\x0a\x33\x2e", 6)) {
            DEBUG_MSG("\tdissector_mysql BANNER");
            PACKET->DISSECTOR.banner = strdup("MySQL v3.xx.xx");
         } else if (!memcmp(ptr + 1, "\x00\x00\x00\x0a\x34\x2e", 6)) {
            DEBUG_MSG("\tdissector_mysql BANNER");
            PACKET->DISSECTOR.banner = strdup("MySQL v4.xx.xx");
         } else {
            SAFE_FREE(ident);
            return NULL;
         }
    
         /* Search for 000 padding */
         while( (ptr + index) < end && (ptr[index] != ptr[index - 1] != ptr[index - 2] != 0) )
            index++;
          
         /* create the new session */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_mysql));
       
         /* Save the seed */
         s->data = strdup(ptr + index + 1);

         /* save the session */
         session_put(s);
      } 
   } else { /* Packets coming from the client */

      /* Only if we catched the connection from the beginning */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {

         DEBUG_MSG("\tDissector_mysql DUMP ENCRYPTED");

         /* Reach the username */
         ptr += 9;
         /* save the username */
         PACKET->DISSECTOR.user = strdup(ptr);
    
         /* Reach the encrypted password */
         ptr += strlen(PACKET->DISSECTOR.user) + 1;
         if (ptr < end && strlen(ptr) != 0) {
            SAFE_CALLOC(PACKET->DISSECTOR.pass, strlen(s->data) + strlen(ptr) + 128, sizeof(char));
            
            sprintf(PACKET->DISSECTOR.pass, "Seed:%s Encrypted:%s", (char *)s->data, ptr);
         } else {
            PACKET->DISSECTOR.pass = strdup("No Password!!!");
         }
    
         DISSECT_MSG("MYSQL : %s:%d -> USER:%s %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                   ntohs(PACKET->L4.dst),
                                                   PACKET->DISSECTOR.user,
                                                   PACKET->DISSECTOR.pass);
         dissect_wipe_session(PACKET, DISSECT_CODE(dissector_mysql));
      }
   }        
   
   SAFE_FREE(ident);
   return NULL;
}      

/* EOF */

// vim:ts=3:expandtab

