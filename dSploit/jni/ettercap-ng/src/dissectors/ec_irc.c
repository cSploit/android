/*
    ettercap -- dissector irc -- TCP 6666 6667 6668 6669

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

    $Id: ec_irc.c,v 1.10 2004/10/04 12:31:21 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_sslwrap.h>

/* protos */

FUNC_DECODER(dissector_irc);
void irc_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init irc_init(void)
{
   dissect_add("irc", APP_LAYER_TCP, 6666, dissector_irc);
   dissect_add("irc", APP_LAYER_TCP, 6667, dissector_irc);
   dissect_add("irc", APP_LAYER_TCP, 6668, dissector_irc);
   dissect_add("irc", APP_LAYER_TCP, 6669, dissector_irc);
   sslw_dissect_add("ircs", 994, dissector_irc, SSL_ENABLED);
}

FUNC_DECODER(dissector_irc)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* unused variable */
   (void)end;

   /* skip messages coming from the server */
   if (FROM_SERVER("irc", PACKET) || FROM_SERVER("ircs", PACKET))
      return NULL;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   DEBUG_MSG("IRC --> TCP dissector_irc");
 
/*
 * authentication method: PASS
 *
 * /PASS password
 * 
 */
   if ( !strncasecmp(ptr, "PASS ", 5) ) {

      DEBUG_MSG("\tDissector_irc PASS");
      
      ptr += 5;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_irc));
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
     
      SAFE_FREE(ident);
      
      PACKET->DISSECTOR.pass = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/PASS password");
      
      DISSECT_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }
   
/*
 * changing a channel key
 *
 * /MODE #channel +k password
 * 
 */
   if ( !strncasecmp(ptr, "MODE ", 5) && match_pattern(ptr + 5, "#* +k *") ) {

      DEBUG_MSG("\tDissector_irc MODE");
      
      ptr += 5;
      
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.user, ' ')) != NULL )
         *ptr = '\0';
     
      /* skip the " +k " */
      PACKET->DISSECTOR.pass = strdup(ptr + 4);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/MODE #channel +k password");
      
      DISSECT_MSG("IRC : %s:%d -> CHANNEL: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * entering in a channel with a key
 *
 * /JOIN #channel password
 * 
 */
   if ( !strncasecmp(ptr, "JOIN ", 5) && match_pattern(ptr + 5, "#* *") ) {

      DEBUG_MSG("\tDissector_irc JOIN");
      
      ptr += 5;
      
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.user, ' ')) != NULL )
         *ptr = '\0';
     
      PACKET->DISSECTOR.pass = strdup(ptr + 1);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/JOIN #channel password");
      
      DISSECT_MSG("IRC : %s:%d -> CHANNEL: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * identifying to the nickserv
 *
 * /msg nickserv identify password
 * 
 */
   if ( !strncasecmp(ptr, "PRIVMSG ", 8) && match_pattern(ptr + 8, "* :identify *\r\n") ) {
      char *pass;

      DEBUG_MSG("\tDissector_irc PRIVMSG");
      
      if (!(pass = strcasestr(ptr, "identify")))
         return NULL;
      
      pass += 9;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_irc));
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
      
      SAFE_FREE(ident);
     
      PACKET->DISSECTOR.pass = strdup(pass);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/msg nickserv identify password");
      
      DISSECT_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }
   
/*
 * identifying to the nickserv
 *
 * /nickserv identify password
 * 
 */
   if ( !strncasecmp(ptr, "NICKSERV ", 9) || !strncasecmp(ptr, "NS ", 3) ) {
      char *pass;

      DEBUG_MSG("\tDissector_irc NICKSERV");
      
      if (!(pass = strcasestr(ptr, "identify")))
         return NULL;
      
      pass += 9;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_irc));
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
      
      SAFE_FREE(ident);
     
      PACKET->DISSECTOR.pass = strdup(pass);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/nickserv identify password");
      
      DISSECT_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * identifying to the nickserv
 *
 * /identify password
 * 
 */
   if ( !strncasecmp(ptr, "IDENTIFY ", 9) ) {
      char *pass;

      DEBUG_MSG("\tDissector_irc IDENTIFY");
      
      if (!(pass = strcasestr(ptr, " ")))
         return NULL;
      
      /* adjust the pointer */
      if (*++pass == ':') 
         pass += 1;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_irc));
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
     
      SAFE_FREE(ident);
     
      PACKET->DISSECTOR.pass = strdup(pass);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/identify password");
      
      DISSECT_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * register the nick in the session
 * list, we need it later when printing 
 * passwords.
 */

   /* user is taking a nick */
   if (!strncasecmp(ptr, "NICK ", 5)) {
      char *p;
      char *user;

      ptr += 5;

      if (*ptr == ':')
         ptr++;
      
      /* delete any previous saved nick */
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_irc));
      /* create the new session */
      dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_irc));
     
      /* save the nick */
      s->data = strdup(ptr);
      if ( (p = strchr(s->data, '\r')) != NULL )
         *p = '\0';
      if ( (p = strchr(s->data, '\n')) != NULL )
         *p = '\0';

      /* print the user info */
      if ((ptr = strcasestr(ptr, "USER "))) {
         user = strdup(ptr + 5);
         if ( (p = strchr(user, '\r')) != NULL )
            *p = '\0';
         if ( (p = strchr(user, '\n')) != NULL )
            *p = '\0';
         
         DISSECT_MSG("IRC : %s:%d -> USER: %s (%s)\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    s->data, 
                                    user);
         SAFE_FREE(user);
      }
      /* save the session */
      session_put(s);

      return NULL;
   }

   /* delete the user */
   if (!strncasecmp(ptr, "QUIT ", 5)) {
      dissect_wipe_session(PACKET, DISSECT_CODE(dissector_irc));

      return NULL;
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

