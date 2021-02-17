/*
    ettercap -- dissector VNC -- TCP 5900 5901 5902 5903

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

*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */

struct vnc_status {
   u_char status;
   u_char challenge[16];
   u_char response[16];
   u_char banner[16];
};

#define WAIT_AUTH       1
#define WAIT_CHALLENGE  2
#define WAIT_RESPONSE   3
#define WAIT_RESULT     4
#define NO_AUTH         5
#define LOGIN_OK        6
#define LOGIN_FAILED    7
#define LOGIN_TOOMANY   8


/* protos */

FUNC_DECODER(dissector_vnc);
void vnc_init(void);

/************************************************/

static char itoa16[16] =  "0123456789abcdef";

static inline void hex_encode(unsigned char *str, int len, unsigned char *out)
{
   int i;
   for (i = 0; i < len; ++i) {
      out[0] = itoa16[str[i]>>4];
      out[1] = itoa16[str[i]&0xF];
      out += 2;
   }
}

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init vnc_init(void)
{
   dissect_add("vnc", APP_LAYER_TCP, 5900, dissector_vnc);
   dissect_add("vnc", APP_LAYER_TCP, 5901, dissector_vnc);
   dissect_add("vnc", APP_LAYER_TCP, 5902, dissector_vnc);
   dissect_add("vnc", APP_LAYER_TCP, 5903, dissector_vnc);
}

FUNC_DECODER(dissector_vnc)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct vnc_status *conn_status;

   /* Packets coming from the server */
   if (FROM_SERVER("vnc", PACKET)) {

      /* Interesting packets have len >= 4 */
      if (PACKET->DATA.len < 4)
         return NULL;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_vnc));

      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {

         /* This is the first packet from the server (protocol version) */
         if (!strncmp((const char*)ptr, "RFB ", 4)) {

            DEBUG_MSG("\tdissector_vnc BANNER");

            /* Catch the banner that is like "RFB xxx.yyy" */
            PACKET->DISSECTOR.banner = strdup((const char*)ptr);
            /* remove the \n */
            if ( (ptr = (u_char*)strchr(PACKET->DISSECTOR.banner, '\n')) != NULL )
               *ptr = '\0';

            /* create the new session */
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_vnc));

            /* remember the state (used later) */
            SAFE_CALLOC(s->data, 1, sizeof(struct vnc_status));

            conn_status = (struct vnc_status *) s->data;
            conn_status->status = WAIT_AUTH;
            strncpy((char*)conn_status->banner, PACKET->DISSECTOR.banner, 16);

            /* save the session */
            session_put(s);
         }
      } else { /* The session exists */

         conn_status = (struct vnc_status *) s->data;

         /* If we are waiting for auth scheme by the server */
         if (conn_status->status == WAIT_AUTH) {

            /* Authentication scheme requested by the server:
             *  0 - Connection Failed
             *  1 - No Authentication
             *  2 - VNC Authentication
             */

            DEBUG_MSG("\tDissector_vnc AUTH");

            /* No authentication required!!! */
            if (!memcmp(ptr, "\x00\x00\x00\x01", 4)) {

               /*
                * Free authentication will be notified on
                * next client's packet
                */
               if(!strstr((const char*)conn_status->banner, "008")) { /* TightVNC hack */
                    conn_status->status = NO_AUTH;
               }
            } else if (!memcmp(ptr, "\x00\x00\x00\x00", 4)) { /* Connection Failed */
               if(!strstr((const char*)conn_status->banner, "008")) { /* TightVNC hack */
                  /*...so destroy the session*/
                  dissect_wipe_session(PACKET, DISSECT_CODE(dissector_vnc));
                  SAFE_FREE(ident);
                  return NULL;
               }

            } else if (!memcmp(ptr, "\x00\x00\x00\x02", 4)) { /* VNC Auth required */
               /* Skip Authentication type code (if the challenge is in the same packet) */
               ptr+=4;

               /* ...and waits for the challenge */
               conn_status->status = WAIT_CHALLENGE;
            }
            else { /* search for challenge packet */
               char buffer[17];
               if(PACKET->DATA.len >= 16) {
                  memcpy(buffer, ptr, 16);
                  buf[16] = 0;
                  if(!strstr(buffer, "VNCAUTH_") && PACKET->DATA.len == 16) {
                     /* Saves the server challenge (16byte) in the session data */
                     conn_status->status = WAIT_RESPONSE;
                     memcpy(conn_status->challenge, ptr, 16);
                  }
               }
            }
         }

         /* We are waiting for the server challenge */
         if ((conn_status->status == WAIT_CHALLENGE) && (ptr < end)) {
            char buffer[17];
            if(PACKET->DATA.len >= 16) {
               memcpy(buffer, ptr, 16);
               buf[16] = 0;
               /* Saves the server challenge (16byte) in the session data */
               if(!strstr(buffer, "VNCAUTH_") && PACKET->DATA.len == 16) {
                  DEBUG_MSG("\tDissector_vnc CHALLENGE");
                  conn_status->status = WAIT_RESPONSE;
                  memcpy(conn_status->challenge, ptr, 16);
               }
            }
         } else if (conn_status->status == WAIT_RESULT) { /* We are waiting for login result */

            /* Login Result
             *  0 - Ok
             *  1 - Failed
             *  2 - Too many attempts
             */
            DEBUG_MSG("\tDissector_vnc RESULT");

            if (!memcmp(ptr, "\x00\x00\x00\x00", 4))
               conn_status->status = LOGIN_OK;
            else if (!memcmp(ptr, "\x00\x00\x00\x01", 4))
               conn_status->status = LOGIN_FAILED;
            else if (!memcmp(ptr, "\x00\x00\x00\x02", 4))
               conn_status->status = LOGIN_TOOMANY;
         }
      }
   } else { /* Packets coming from the client */

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_vnc));

      /* Only if we catched the connection from the beginning */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {

         /* We have to catch even ACKs to dump
            LOGIN FAILURE information on a client's packet */
         conn_status = (struct vnc_status *) s->data;

         /* If there is no auth! */
         if (conn_status->status == NO_AUTH) {

            DEBUG_MSG("\tDissector_vnc NOAUTH");

            PACKET->DISSECTOR.user = strdup("VNC");
            PACKET->DISSECTOR.pass = strdup("No Password!!!");

            DISSECT_MSG("VNC : %s:%d -> No authentication required\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                                    ntohs(PACKET->L4.dst));
            dissect_wipe_session(PACKET, DISSECT_CODE(dissector_vnc));
         } else    /* If we have catched server result */

            if (conn_status->status >= LOGIN_OK) {
               u_char *str_ptr, index;

               DEBUG_MSG("\tDissector_vnc DUMP ENCRYPTED");

               PACKET->DISSECTOR.user = strdup("VNC");
               SAFE_CALLOC(PACKET->DISSECTOR.pass, 256, sizeof(char));

               /* Dump Challenge and Response */
               snprintf(PACKET->DISSECTOR.pass, 10, "Challenge:");
               str_ptr = (u_char*)PACKET->DISSECTOR.pass + strlen(PACKET->DISSECTOR.pass);

               for (index = 0; index < 16; index++)
                  snprintf((char*)str_ptr + (index * 2), 3, "%.2x", conn_status->challenge[index]);

               strcat((char*)str_ptr, " Response:");
               str_ptr =(u_char*) PACKET->DISSECTOR.pass + strlen(PACKET->DISSECTOR.pass);

               for (index = 0; index < 16; index++)
                  snprintf((char*)str_ptr + (index * 2), 3, "%.2x", conn_status->response[index]);

               if (conn_status->status > LOGIN_OK) {
                  PACKET->DISSECTOR.failed = 1;
                  DISSECT_MSG("VNC : %s:%d -> %s (Login Failed)\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                                 ntohs(PACKET->L4.dst),
                                                                 PACKET->DISSECTOR.pass);
               } else {
                  DISSECT_MSG("VNC : %s:%d -> %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                  ntohs(PACKET->L4.dst),
                                                  PACKET->DISSECTOR.pass);
               }

               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_vnc));
         } else { /* If we are waiting for client response (don't care ACKs) */
            if (conn_status->status == WAIT_RESPONSE && PACKET->DATA.len >= 16) {
               unsigned char challenge[33];
               unsigned char response[33];
               hex_encode(conn_status->challenge, 16, challenge);
               challenge[32] = 0;
               hex_encode(ptr, 16, response);
               response[32] = 0;
               DEBUG_MSG("\tDissector_vnc RESPONSE");
               /* Saves the client response (16byte) in the session data */
               conn_status->status = WAIT_RESULT;
               memcpy(conn_status->response, ptr, 16);
               /* even possibly wrong passwords can be useful */
               DISSECT_MSG("%s-%d:$vnc$*%s*%s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp), ntohs(PACKET->L4.dst), challenge, response);
            }
         }
      }
   }

   SAFE_FREE(ident);
   return NULL;
}

/* EOF */

// vim:ts=3:expandtab

