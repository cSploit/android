/*
    ettercap -- dissector FTP -- TCP 21

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

    $Id: ec_ftp.c,v 1.17 2004/01/21 20:20:06 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>


/* protos */

FUNC_DECODER(dissector_ftp);
void ftp_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ftp_init(void)
{
   dissect_add("ftp", APP_LAYER_TCP, 21, dissector_ftp);
}

FUNC_DECODER(dissector_ftp)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* the connection is starting... create the session */
   CREATE_SESSION_ON_SYN_ACK("ftp", s, dissector_ftp);
   
   /* check if it is the first packet sent by the server */
   IF_FIRST_PACKET_FROM_SERVER("ftp", s, ident, dissector_ftp) {
            
      DEBUG_MSG("\tdissector_ftp BANNER");
      /*
       * get the banner 
       * ptr + 4 to skip the initial 220 response
       */
      if (!strncmp(ptr, "220", 3)) {
         PACKET->DISSECTOR.banner = strdup(ptr + 4);
         
         /* remove the \r\n */
         if ( (ptr = strchr(PACKET->DISSECTOR.banner, '\r')) != NULL )
            *ptr = '\0';
      }
     
   } ENDIF_FIRST_PACKET_FROM_SERVER(s, ident)

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   /* skip messages coming from the server */
   if (FROM_SERVER("ftp", PACKET))
      return NULL;
   
   DEBUG_MSG("FTP --> TCP dissector_ftp");
 
   /* skip the whitespaces at the beginning */
   while(*ptr == ' ' && ptr != end) ptr++;
 
   /* reached the end */
   if (ptr == end) 
      return NULL;
   
   /* harvest the username */
   if ( !strncasecmp(ptr, "USER ", 5) ) {

      DEBUG_MSG("\tDissector_FTP USER");
      
      /* create the session */
      dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_ftp));
      
      ptr += 5;

      /* if not null, free it */
      SAFE_FREE(s->data);

      /* fill the session data */
      s->data = strdup(ptr);
      s->data_len = strlen(ptr);
      
      if ( (ptr = strchr(s->data,'\r')) != NULL )
         *ptr = '\0';
      
      /* save the session */
      session_put(s);
      
      return NULL;
   }

   /* harvest the password */
   if ( !strncasecmp(ptr, "PASS ", 5) ) {

      DEBUG_MSG("\tDissector_FTP PASS");
      
      ptr += 5;
      
      /* create an ident to retrieve the session */
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_ftp));
      
      /* retrieve the session and delete it */
      if (session_get_and_del(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         SAFE_FREE(ident);
         return NULL;
      }
      
      /* check that the user was sent before the pass */
      if (s->data == NULL) {
         SAFE_FREE(ident);
         return NULL;
      }
      
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(s->data);
      
      PACKET->DISSECTOR.pass = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      /* free the session */
      session_free(s);
      SAFE_FREE(ident);

      DISSECT_MSG("FTP : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass);

      
      return NULL;
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

