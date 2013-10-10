/*
    ettercap -- dissector SMTP -- TCP 25

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

    $Id: ec_smtp.c,v 1.6 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_sslwrap.h>

/* protos */

FUNC_DECODER(dissector_smtp);
void smtp_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init smtp_init(void)
{
   dissect_add("smtp", APP_LAYER_TCP, 25, dissector_smtp);
   sslw_dissect_add("ssmtp", 465, dissector_smtp, SSL_ENABLED);
}

FUNC_DECODER(dissector_smtp)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* the connection is starting... create the session */
   CREATE_SESSION_ON_SYN_ACK("smtp", s, dissector_smtp);
   CREATE_SESSION_ON_SYN_ACK("ssmtp", s, dissector_smtp);
   
   /* check if it is the first packet sent by the server */
   IF_FIRST_PACKET_FROM_SERVER_SSL("smtp", "ssmtp", s, ident, dissector_smtp) {
          
      DEBUG_MSG("\tdissector_smtp BANNER");
       
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
   
   /* skip messages coming from the server */
   if (FROM_SERVER("smtp", PACKET) || FROM_SERVER("ssmtp", PACKET))
      return NULL;
   
   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
 
   
   DEBUG_MSG("SMTP --> TCP dissector_smtp");

   /* skip the number, move to the command */
   while(*ptr == ' ' && ptr != end) ptr++;
  
   /* reached the end */
   if (ptr == end) return NULL;


/* 
 * AUTH LOGIN
 *
 * digest(user)
 * digest(pass)
 *
 * the digests are in base64
 */
   if ( !strncasecmp(ptr, "AUTH LOGIN", 10) ) {
      
      DEBUG_MSG("\tDissector_smtp AUTH LOGIN");

      /* destroy any previous session */
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_smtp));
      
      /* create the new session */
      dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_smtp));
     
      /* remember the state (used later) */
      s->data = strdup("AUTH");
     
      /* save the session */
      session_put(s);
      
      /* username is in the next packet */
      return NULL;
   }
   
   /* search the session (if it exist) */
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_smtp));
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
      return NULL;
   }

   SAFE_FREE(ident);
   
   /* the session is invalid */
   if (s->data == NULL) {
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_smtp));
      return NULL;
   }
   
   if (!strcmp(s->data, "AUTH")) {
      char *user;
      int i;
     
      DEBUG_MSG("\tDissector_smtp AUTH LOGIN USER");
      
      SAFE_CALLOC(user, strlen(ptr), sizeof(char));
     
      /* username is encoded in base64 */
      i = base64_decode(user, ptr);
     
      SAFE_FREE(s->data);

      /* store the username in the session */
      SAFE_CALLOC(s->data, strlen("AUTH USER ") + i + 1, sizeof(char) );
      
      sprintf(s->data, "AUTH USER %s", user);
      
      SAFE_FREE(user);

      /* pass is in the next packet */
      return NULL;
   }
   
   if (!strncmp(s->data, "AUTH USER", 9)) {
      char *pass;
     
      DEBUG_MSG("\tDissector_smtp AUTH LOGIN PASS");
      
      SAFE_CALLOC(pass, strlen(ptr), sizeof(char));
      
      /* password is encoded in base64 */
      base64_decode(pass, ptr);
     
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(s->data + strlen("AUTH USER "));
      PACKET->DISSECTOR.pass = strdup(pass);
      
      SAFE_FREE(pass);
      /* destroy the session */
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_smtp));
      
      /* print the message */
      DISSECT_MSG("SMTP : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass);
      return NULL;
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

