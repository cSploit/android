/*
    ettercap -- dissector RLOGIN -- TCP 512 513

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

    $Id: ec_rlogin.c,v 1.14 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* protos */

FUNC_DECODER(dissector_rlogin);
void rlogin_init(void);
void skip_rlogin_command(u_char **ptr, u_char *end);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init rlogin_init(void)
{
   dissect_add("rlogin", APP_LAYER_TCP, 512, dissector_rlogin);
   dissect_add("rlogin", APP_LAYER_TCP, 513, dissector_rlogin);
}

/*
 * rlogin sends characters one per packet for the password
 * but the login is sent all together on the second packet 
 * sent by the client.
 */

FUNC_DECODER(dissector_rlogin)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* skip messages from the server */
   if (FROM_SERVER("rlogin", PACKET))
      return NULL;
   
   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   DEBUG_MSG("rlogin --> TCP dissector_rlogin");

   /* create an ident to retrieve the session */
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_rlogin));

   /* this is the rlogin handshake */
   if (*ptr == '\0') {
      /* retrieve the session */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_rlogin));
         /* remember the state (used later) */
         s->data = strdup("HANDSHAKE");
         
         /* save the session */
         session_put(s);

         SAFE_FREE(ident);
         return NULL;
      } 
   }
   
   /* the first packet after handshake */
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS && s->data) {
      if (!strcmp(s->data, "HANDSHAKE")) {
         u_char *localuser;
         u_char *remoteuser;

         localuser = ptr;

         /* sanity check */
         if (localuser + strlen(localuser) + 2 < end)
            remoteuser = localuser + strlen(localuser) + 1;
         else {
            /* bad packet, abort the collection process */
            session_del(ident, DISSECT_IDENT_LEN);
            SAFE_FREE(ident);
            return NULL;
         }

         SAFE_FREE(s->data);

         /* make room for the string */
         SAFE_CALLOC(s->data, strlen(localuser) + strlen(remoteuser) + 5, sizeof(char));
         
         sprintf(s->data, "%s (%s)\r", remoteuser, localuser);

         SAFE_FREE(ident);
         return NULL;
      }
   }
   
   /* concat the pass to the collected user */
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS && s->data) {
      size_t i;
      u_char *p;
      u_char str[strlen(s->data) + PACKET->DATA.disp_len + 2];

      memset(str, 0, sizeof(str));
    
      /* concat the char to the previous one */
      snprintf(str, strlen(s->data) + PACKET->DATA.disp_len + 2, "%s%s", (char *)s->data, ptr);
      
      /* parse the string for backspaces and erase as wanted */
      for (p = str, i = 0; i < strlen(str); i++) {
         if (str[i] == '\b' || str[i] == 0x7f) {
            p--;
         } else {
            *p = str[i];
            p++;  
         }
      }
      *p = '\0';
            
      /* save the new string */
      SAFE_FREE(s->data);
      s->data = strdup(str);
      
      /* 
       * the user input is terminated
       * check if it was the password by checking
       * the presence of \r in the string
       * we store "user\rpass\r" and then we split it
       */
      if (strchr(ptr, '\r') || strchr(ptr, '\n')) {
         /* there is the \r and it is not the last char */
         if ( ((ptr = strchr(s->data, '\r')) || (ptr = strchr(s->data, '\n')))
               && ptr != s->data + strlen(s->data) - 1 ) {

            /* fill the structure */
            PACKET->DISSECTOR.user = strdup(s->data);
            if ( (ptr = strchr(PACKET->DISSECTOR.user, '\r')) != NULL )
               *ptr = '\0';
   
            PACKET->DISSECTOR.pass = strdup(ptr + 1);
            if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
               *ptr = '\0';
           
            /* 
             * delete the session to remember that 
             * user and pass was collected
             */
            session_del(ident, DISSECT_IDENT_LEN);
            SAFE_FREE(ident);
            
            /* display the message */
            DISSECT_MSG("RLOGIN : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                 ntohs(PACKET->L4.dst), 
                                 PACKET->DISSECTOR.user,
                                 PACKET->DISSECTOR.pass);
         }
      }
   }

   SAFE_FREE(ident);
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

