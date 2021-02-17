/*
    ettercap -- dissector for MongoDB authenctication protocol -- TCP 1521

    Copyright (C) Dhiru Kholia (dhiru at openwall.com)

    Tested with MongoDB 2.2.1 on Arch Linux (November 2012)

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

struct mongodb_status {
   u_char status;
   u_char username[129];
   u_char salt[17];
};

#define WAIT_RESPONSE   1
#define WAIT_RESULT   2

/* protos */

FUNC_DECODER(dissector_mongodb);
void mongodb_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init mongodb_init(void)
{
   dissect_add("mongodb", APP_LAYER_TCP, 27017, dissector_mongodb);
}

FUNC_DECODER(dissector_mongodb)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct mongodb_status *conn_status;

   //suppress unused warning
   (void)end;

   if (FROM_SERVER("mongodb", PACKET)) {
      if (PACKET->DATA.len < 13)
         return NULL;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_mongodb));
      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         unsigned char *noncep  = memmem(ptr, PACKET->DATA.len, "nonce", 5);
         unsigned char *gnoncep  = memmem(ptr, PACKET->DATA.len, "getnonce\x00", 9);
         unsigned char *keyp  = memmem(ptr, PACKET->DATA.len, "authenticate\x00", 12);
         if (noncep && !gnoncep && !keyp) {
            /* create the new session */
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_mongodb));

            /* remember the state (used later) */
            SAFE_CALLOC(s->data, 1, sizeof(struct mongodb_status));

            conn_status = (struct mongodb_status *) s->data;
            conn_status->status = WAIT_RESPONSE;

            /* find and change nonce  */
            unsigned char *p = noncep;
            p += (5 + 1 + 4); /* skip over "nonce" + '\x00; + length */
            strncpy((char*)conn_status->salt, (char*)p, 16);
            conn_status->salt[16] = 0;
#ifdef MITM
            memset(p, 0, 16);
            PACKET->flags |= PO_MODIFIED;
#endif
            /* save the session */
            session_put(s);
         }
      }
      else {
              if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
                      conn_status = (struct mongodb_status *) s->data;
                      if (PACKET->DATA.len < 16)
                              return NULL;
                      unsigned char *res = memmem(ptr, PACKET->DATA.len, "fails", 5);
                      unsigned char *gres = memmem(ptr, PACKET->DATA.len, "readOnly", 8);
                      if (conn_status->status == WAIT_RESULT && res) {
                              DISSECT_MSG("Login to %s-%d as %s failed!\n", ip_addr_ntoa(&PACKET->L3.src, tmp), ntohs(PACKET->L4.src), conn_status->username);
                              dissect_wipe_session(PACKET, DISSECT_CODE(dissector_mongodb));
                      }
                      else if (gres) {
                              DISSECT_MSG("Login to %s-%d as %s succeeded!\n", ip_addr_ntoa(&PACKET->L3.src, tmp), ntohs(PACKET->L4.src), conn_status->username) ;
                              dissect_wipe_session(PACKET, DISSECT_CODE(dissector_mongodb));
                      }
              }
      }
   } else {
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_mongodb));

      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         conn_status = (struct mongodb_status *) s->data;
         if (PACKET->DATA.len < 16)
                 return NULL;

         unsigned char *noncep  = memmem(ptr, PACKET->DATA.len, "nonce", 5);
         unsigned char *keyp  = memmem(ptr, PACKET->DATA.len, "key\x00", 4);
         unsigned char *usernamep  = memmem(ptr, PACKET->DATA.len, "user\x00", 5);
         if (conn_status->status == WAIT_RESPONSE && noncep && keyp) {
            usernamep += (4 + 1 + 4); /* skip over "user" + '\x00; + length */
            strncpy((char*)conn_status->username, (char*)usernamep, 128);
            conn_status->username[128] = 0;
            unsigned char key[33];
            keyp += (3 + 1 + 4); /* skip over "key" + '\x00; + length */
            strncpy((char*)key, (char*)keyp, 32);
            key[32] = 0;
            DISSECT_MSG("%s-%s-%d:$mongodb$1$%s$%s$%s\n", conn_status->username, ip_addr_ntoa(&PACKET->L3.src, tmp), ntohs(PACKET->L4.src), conn_status->username, conn_status->salt, key);
            conn_status->status = WAIT_RESULT;
         }
      }
   }

   SAFE_FREE(ident);
   return NULL;
}
