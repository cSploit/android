/*
    ettercap -- dissector IMAP -- TCP 143 220

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

    $Id: ec_imap.c,v 1.16 2004/06/25 14:24:29 alor Exp $
*/

/*
 * The authentication schema can be found here:
 *
 * ftp://ftp.rfc-editor.org/in-notes/rfc1730.txt
 * ftp://ftp.rfc-editor.org/in-notes/rfc1731.txt
 *
 * we currently support:
 *    - LOGIN
 *    - AUTHENTICATE LOGIN
 */

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_sslwrap.h>

/* protos */

FUNC_DECODER(dissector_imap);
void imap_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init imap_init(void)
{
   dissect_add("imap", APP_LAYER_TCP, 143, dissector_imap);
   dissect_add("imap", APP_LAYER_TCP, 220, dissector_imap);
   sslw_dissect_add("imaps", 993, dissector_imap, SSL_ENABLED);
}

FUNC_DECODER(dissector_imap)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* the connection is starting... create the session */
   CREATE_SESSION_ON_SYN_ACK("imap", s, dissector_imap);
   /* create the session even if we are into an ssl tunnel */
   CREATE_SESSION_ON_SYN_ACK("imaps", s, dissector_imap);
   
   /* check if it is the first packet sent by the server */
   IF_FIRST_PACKET_FROM_SERVER_SSL("smtp", "ssmtp", s, ident, dissector_imap) {
          
      DEBUG_MSG("\tdissector_imap BANNER");
      /*
       * get the banner 
       * "* OK banner"
       */
       
      /* skip the number, go to response */
      while(*ptr != ' ' && ptr != end) ptr++;
      
      /* reached the end */
      if (ptr == end) return NULL;
      
      if (!strncmp(ptr, " OK ", 4)) {
         PACKET->DISSECTOR.banner = strdup(ptr + 3);
      
         /* remove the \r\n */
         if ( (ptr = strchr(PACKET->DISSECTOR.banner, '\r')) != NULL )
            *ptr = '\0';
      }
   } ENDIF_FIRST_PACKET_FROM_SERVER(s, ident)
   
   /* skip messages coming from the server */
   if (FROM_SERVER("imap", PACKET) || FROM_SERVER("imaps", PACKET))
      return NULL;
   
   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
 
   DEBUG_MSG("IMAP --> TCP dissector_imap");
   

   /* skip the number, move to the command */
   while(*ptr != ' ' && ptr != end) ptr++;
  
   /* reached the end */
   if (ptr == end) return NULL;

/*
 * LOGIN authentication:
 *
 * n LOGIN user pass
 */
   if ( !strncasecmp(ptr, " LOGIN ", 7) ) {

      DEBUG_MSG("\tDissector_imap LOGIN");

      ptr += 7;
      
      PACKET->DISSECTOR.user = strdup(ptr);
      
      /* split the string */
      if ( (ptr = strchr(PACKET->DISSECTOR.user, ' ')) != NULL )
         *ptr = '\0';
      
      /* save the second part */
      PACKET->DISSECTOR.pass = strdup(ptr + 1);
      
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      
      /* print the message */
      DISSECT_MSG("IMAP : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass);
      return NULL;
   }

/* 
 * AUTHENTICATE LOGIN
 *
 * digest(user)
 * digest(pass)
 *
 * the digests are in base64
 */
   if ( !strncasecmp(ptr, "AUTHENTICATE LOGIN", 19) ) {
      
      DEBUG_MSG("\tDissector_imap AUTHENTICATE LOGIN");

      /* destroy any previous session */
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_imap));
      
      /* create the new session */
      dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_imap));
     
      /* remember the state (used later) */
      s->data = strdup("AUTH");
     
      /* save the session */
      session_put(s);
      
      /* username is in the next packet */
      return NULL;
   }
   
   /* search the session (if it exist) */
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_imap));
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
      return NULL;
   }

   SAFE_FREE(ident);
   
   /* the session is invalid */
   if (s->data == NULL) {
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_imap));
      return NULL;
   }
   
   if (!strcmp(s->data, "AUTH")) {
      char *user;
      int i;
     
      DEBUG_MSG("\tDissector_imap AUTHENTICATE LOGIN USER");
      
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
     
      DEBUG_MSG("\tDissector_imap AUTHENTICATE LOGIN PASS");
      
      SAFE_CALLOC(pass, strlen(ptr), sizeof(char));
      
      /* password is encoded in base64 */
      base64_decode(pass, ptr);
     
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(s->data + strlen("AUTH USER "));
      PACKET->DISSECTOR.pass = strdup(pass);
      
      SAFE_FREE(pass);
      /* destroy the session */
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_imap));
      
      /* print the message */
      DISSECT_MSG("IMAP : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass);
      return NULL;
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

