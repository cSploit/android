/*
    ettercap -- dissector MSN -- TCP 1863

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

    $Id: ec_msn.c,v 1.7 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */

/* protos */

FUNC_DECODER(dissector_msn);
void msn_init(void);
void decode_pwd(char *pwd, char *outpwd);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init msn_init(void)
{
   dissect_add("msn", APP_LAYER_TCP, 1863, dissector_msn);
}

FUNC_DECODER(dissector_msn)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct ec_session *s = NULL;
   void *ident = NULL;
   char *token, *tok;

   /* don't complain about unused var */
   (void)end;
   
   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   DEBUG_MSG("MSN --> TCP dissector_msn");
   
   /* message coming from the client */
   if (FROM_CLIENT("msn", PACKET)) {

      /* FIRST STEP: the client sends the login */

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_msn));
      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      
         /* search the login */
         ptr = strstr(ptr, "MD5 I ");
         
         /* not found */
         if (ptr == NULL)
            goto bad;
   
         DEBUG_MSG("\tDissector_msn - LOGIN ");
         
         /* create the new session */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_msn));
	 
         /* save the login */
         s->data = strdup(ptr + strlen("MD5 I "));
	    
         /* tuncate at the \r */
         if ( (ptr = strchr(s->data,'\r')) != NULL )
            *ptr = '\0';

         session_put(s);
      } else {
      
      /* THIRD STEP: the client sends the MD5 password */
         
         dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_msn));
         /* if the session does not exist... */
         if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         
            /* search the login */
            ptr = strstr(ptr, "MD5 S ");
            
            /* not found */
            if (ptr == NULL)
               goto bad;
   
            DEBUG_MSG("\tDissector_msn - PASS ");
            
            /* save the challenge after the login */
            SAFE_REALLOC(s->data, strlen(s->data) + strlen(ptr) + 2);
            sprintf(s->data + strlen(s->data), " %s", ptr + strlen("MD5 S "));
            
            /* tuncate at the \r */
            if ( (ptr = strchr(s->data,'\r')) != NULL )
               *ptr = '\0';
            
            /* save the proper value (splitting the string)
             * it contains:
             * "user challenge password"
             */
            if ((token = ec_strtok(s->data, " ", &tok))) {
               PACKET->DISSECTOR.user = strdup(token);
               if ((token = ec_strtok(NULL, " ", &tok))) {
                  PACKET->DISSECTOR.info = strdup(token);
                  if ((token = ec_strtok(NULL, " ", &tok))) {
                     PACKET->DISSECTOR.pass = strdup(token);

                     /* display the message */
                     DISSECT_MSG("MSN : %s:%d -> USER: %s  MD5 PASS: %s  CHALLENGE: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                                                           ntohs(PACKET->L4.dst), 
                                                                                           PACKET->DISSECTOR.user,
                                                                                           PACKET->DISSECTOR.pass,
                                                                                           PACKET->DISSECTOR.info);
                  }
               }									   
            }
            /* wipe the session */
            dissect_wipe_session(PACKET, DISSECT_CODE(dissector_msn));
         }
      }
      
   } else {
      /* the message is from the server */
      
      /* SECOND STEP: the server sends the challenge */
      
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_msn));
      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
      
         /* search the login */
         ptr = strstr(ptr, "MD5 S ");
         
         /* not found */
         if (ptr == NULL)
            goto bad;
   
         DEBUG_MSG("\tDissector_msn - CHALLENGE ");
         
         /* save the challenge after the login */
         SAFE_REALLOC(s->data, strlen(s->data) + strlen(ptr) + 2);
         sprintf(s->data + strlen(s->data), " %s", ptr + strlen("MD5 S "));
         
         /* tuncate at the \r */
         if ( (ptr = strchr(s->data,'\r')) != NULL )
            *ptr = '\0';
         
      }
   }
   
bad:
   SAFE_FREE(ident);
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

