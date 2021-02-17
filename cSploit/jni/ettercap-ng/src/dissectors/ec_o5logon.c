/*
    ettercap -- dissector for Oracle O5LOGON protocol -- TCP 1521

    Copyright (C) Dhiru Kholia (dhiru at openwall.com)

    Tested with Oracle 11gR1 and 11gR2 64-bit server and Linux +
    Windows SQL*Plus clients.

    It does work with Nmap generated packets though the code is hacky.

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

struct o5logon_status {
   u_char status;
   u_char user[129];
};

#define WAIT_RESPONSE   1
#define WAIT_RESULT   2

/* protos */

FUNC_DECODER(dissector_o5logon);
void o5logon_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init o5logon_init(void)
{
   dissect_add("o5logon", APP_LAYER_TCP, 1521, dissector_o5logon);
}

FUNC_DECODER(dissector_o5logon)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct o5logon_status *conn_status;

   //suppress unused warning
   (void)end;

   if (FROM_CLIENT("o5logon", PACKET)) {

      /* Interesting packets have len >= 4 */
      if (PACKET->DATA.len < 13)
         return NULL;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_o5logon));
      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         unsigned char *skp  = memmem(ptr, PACKET->DATA.len, "AUTH_SESSKEY", 12);
         unsigned char *sp = memmem(ptr, PACKET->DATA.len, "AUTH_TERMINAL", 13);
         if (sp && !skp) {
            /* create the new session */
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_o5logon));

            /* remember the state (used later) */
            SAFE_CALLOC(s->data, 1, sizeof(struct o5logon_status));

            conn_status = (struct o5logon_status *) s->data;
            conn_status->status = WAIT_RESPONSE;

            /* find username */
            unsigned char *last = sp - 6;
            while(last > ptr) {
              if(*last == 0xff || *last == 0x01) {
                 break;
              }
              last--;
            }
            int length = *(last+1);
            if (length < sizeof(conn_status->user))
            {
               strncpy((char*)conn_status->user, (char*)last + 2, length);
               conn_status->user[length] = 0;
            }

            /* save the session */
            session_put(s);
         }
      }
   } else {
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_o5logon));

      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         conn_status = (struct o5logon_status *) s->data;
         unsigned char *skp  = NULL;
         unsigned char *saltp = NULL;
         unsigned char *res = NULL;
         if (PACKET->DATA.len > 16) {
            skp  = memmem(ptr, PACKET->DATA.len, "AUTH_SESSKEY", 12);
            saltp = memmem(ptr, PACKET->DATA.len, "AUTH_VFR_DATA", 13);
            res = memmem(ptr, PACKET->DATA.len, "invalid username", 16);
         }


         if (conn_status->status == WAIT_RESPONSE && skp && saltp) {
            unsigned char sk[97];
            unsigned char salt[21];
            unsigned char *p = skp + 17;
            if(*p == '@') {
               /* Nmap generated packets? */
               strncpy((char*)sk, (char*)p + 1, 64);
               strncpy((char*)sk + 64, (char*)p + 66, 32);
            }
            else {
               strncpy((char*)sk, (char*)skp + 17, 96);
            }
            sk[96] = 0;
            strncpy((char*)salt, (char*)saltp + 18, 20);
            salt[20] = 0;
            DISSECT_MSG("%s-%s-%d:$o5logon$%s*%s\n", conn_status->user, ip_addr_ntoa(&PACKET->L3.src, tmp), ntohs(PACKET->L4.src), sk, salt);
            conn_status->status = WAIT_RESULT;
         }
         else if (conn_status->status == WAIT_RESULT && res) {
            DISSECT_MSG("Login to %s-%d as %s failed!\n", ip_addr_ntoa(&PACKET->L3.src, tmp), ntohs(PACKET->L4.src), conn_status->user) ;
            dissect_wipe_session(PACKET, DISSECT_CODE(dissector_o5logon));
         }
      }
   }

   SAFE_FREE(ident);
   return NULL;
}

// vim:ts=3:expandtab
