/*
    ettercap -- dissector TELNET -- TCP 23

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

    $Id: ec_telnet.c,v 1.20 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_sslwrap.h>

/* protos */

FUNC_DECODER(dissector_telnet);
void telnet_init(void);
static void skip_telnet_command(u_char **ptr, u_char *end);
static void convert_zeros(u_char *ptr, u_char *end);
static int match_login_regex(char *ptr);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init telnet_init(void)
{
   dissect_add("telnet", APP_LAYER_TCP, 23, dissector_telnet);
   sslw_dissect_add("telnets", 992, dissector_telnet, SSL_ENABLED);
}

/*
 * telnet sends characters one per packet,
 * so we have to make sessions to collect 
 * the string among the packet stram.
 *
 * the telnet collector collects user and pass only if
 * a session is present.
 * the session is created looking at the server response
 * and searching for "login:", "failed" ecc... so we will
 * collect even failed logins.
 */

FUNC_DECODER(dissector_telnet)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* the connection is starting... create the session */
   CREATE_SESSION_ON_SYN_ACK("telnet", s, dissector_telnet);
   CREATE_SESSION_ON_SYN_ACK("telnets", s, dissector_telnet);

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   /* skip the telnet commands, we are interested only in readable data */
   skip_telnet_command(&ptr, end);
   
   /* the packet was made only by commands, skip it */
   if (ptr >= end)
      return NULL;

   DEBUG_MSG("TELNET --> TCP dissector_telnet");

   /* search if there are 0x00 char and covert them to spaces.
    * some OS (BSDI) send zeroes into the packet, so we cant use
    * string functions 
    */
   convert_zeros(ptr, end);
      
   /* create an ident to retrieve the session */
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_telnet));
   
   /* is the message from the server or the client ? */
   if (FROM_SERVER("telnet", PACKET) || FROM_SERVER("telnets", PACKET)) {
      
      /* start the collecting process when a "reserved" word is seen */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         if (match_login_regex(ptr)) {
            DEBUG_MSG("\tdissector_telnet - BEGIN");
         
            /* create the session to begin the collection */
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_telnet));
            /* use this value to remember to not collect the banner again */
            s->data = "\xe7\x7e";
            session_put(s);

            return NULL;
         }
      }
   } else {
      
      /* retrieve the session */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {

         /* sanity check */
         if (s->data == NULL)
            return NULL;
         
         /* if the collecting process has to be initiated */
         if (!strcmp(s->data, "\xe7\x7e")) {
         
            /* the characters are not printable, skip them */
            if (!isprint((int)*ptr)) {
               SAFE_FREE(ident);
               return NULL;
            }
            
            DEBUG_MSG("\tdissector_telnet - FIRST CHAR");

            /* save the first packet */
            s->data = strdup(ptr);
         
         /* collect the subsequent packets */
         } else {
            size_t i;
            u_char *p;
            u_char str[strlen(s->data) + PACKET->DATA.disp_len + 2];

            memset(str, 0, sizeof(str));

            /* concat the char to the previous one */
            sprintf(str, "%s%s", (char *)s->data, ptr);
            
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
            p = s->data = strdup(str);
            
            /* terminate the string at \n */
            if ((p = strchr(s->data, '\n')) != NULL)
               *p = '\0';
            
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
                  
                  
                  /* display the message */
                  DISSECT_MSG("TELNET : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                       ntohs(PACKET->L4.dst), 
                                       PACKET->DISSECTOR.user,
                                       PACKET->DISSECTOR.pass);

                  /* delete the session to stop the collection */
                  dissect_wipe_session(PACKET, DISSECT_CODE(dissector_telnet));
               }
               SAFE_FREE(ident);
               return NULL;
            }
         }
      }
   }
  
   /* delete the ident */
   SAFE_FREE(ident);

   /* check if it is the first readable packet sent by the server */
   IF_FIRST_PACKET_FROM_SERVER_SSL("telnet", "telnets", s, ident, dissector_telnet) {
      size_t i;
      u_char *q;
   
      DEBUG_MSG("\tdissector_telnet BANNER");
      
      /* get the banner */
      SAFE_CALLOC(PACKET->DISSECTOR.banner, PACKET->DATA.len + 1, sizeof(char));
      memcpy(PACKET->DISSECTOR.banner, ptr, end - ptr );

      q = PACKET->DISSECTOR.banner;
      for (i = 0; i < PACKET->DATA.len; i++) {
         /* replace \r\n with spaces */ 
         if (q[i] == '\r' || q[i] == '\n')
            q[i] = ' ';
         /* replace command with end of string */
         if (q[i] == 0xff)
            q[i] = '\0';
      }

      /* XXX - ugly hack !!
       * it is for servers that send a "\r\0\r\n" before the banner (BSDI)
       * returning here, means that we don't erase the SYN+ACK session
       * so the next packet will fall here again.
       */
      if (strlen(PACKET->DISSECTOR.banner) < 5) {
         SAFE_FREE(PACKET->DISSECTOR.banner);
         SAFE_FREE(ident);
         return NULL;
      }

      /* 
       * some OS (e.g. windows and ipso) send the "login:" in the
       * same packet as the banner...
       */
      if (match_login_regex(ptr)) {
         DEBUG_MSG("\tdissector_telnet - BEGIN");
         
         /* create the session to begin the collection */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_telnet));
         /* use this value to remember to not collect the banner again */
         s->data = "\xe7\x7e";
         session_put(s);

         return NULL;
      }

   } ENDIF_FIRST_PACKET_FROM_SERVER(s, ident);           
      
   return NULL;
}

/*
 * move the pointer ptr while it is a telnet command.
 */
static void skip_telnet_command(u_char **ptr, u_char *end)
{
   while(**ptr == 0xff && *ptr < end) {
      /* sub option 0xff 0xfa ... ... 0xff 0xf0 */
      if (*(*ptr + 1) == 0xfa) {
         *ptr += 1;
         /* search the sub-option end (0xff 0xf0) */
         do {
            *ptr += 1;
         } while(**ptr != 0xff && *ptr < end);
         /* skip the sub-option end */
         *ptr += 2;
      } else {
      /* normal option 0xff 0xXX 0xXX */
         *ptr += 3;
      }
   }
}

/* 
 * convert 0x00 char into spaces (0x20) so we can
 * use str*() functions on the buffer...
 */
static void convert_zeros(u_char *ptr, u_char *end)
{
   /* 
    * walk the entire buffer, but skip the last
    * char, if it is 0x00 it is actually the string
    * terminator
    */
   while(ptr < end - 1) {
      if (*ptr == 0x00) {
         DEBUG_MSG("\tdissector_telnet ZERO converted");
         /* convert the char to a space */
         *ptr = ' ';
      }
      ptr++;
   }
}

/* 
 * serach the strings which can identify failed login...
 * return 1 on succes, 0 on failure
 */
static int match_login_regex(char *ptr)
{
   char *words[] = {"incorrect", "failed", "failure", NULL };
   int i = 0;
  
   /* 
    * "login:" is a special case, we have to take care
    * of messages from the server, they can contain login:
    * even if it is not the login prompt
    */
   if ((strcasestr(ptr, "login:") || strcasestr(ptr, "username:") )
         && !strcasestr(ptr, "last") && !strcasestr(ptr, "from"))
      return 1;
   
   /* search for keywords */ 
   do {
      if (strcasestr(ptr, words[i]))
         return 1;
   } while (words[++i] != NULL);
   
   return 0;
}

/* EOF */

// vim:ts=3:expandtab

