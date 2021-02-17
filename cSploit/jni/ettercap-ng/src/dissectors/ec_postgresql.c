/*
    ettercap -- dissector for PostgreSQL authentication traffic -- TCP 5432

    Copyright (C) Dhiru Kholia (dhiru at openwall.com)

    Tested with,

    1. PostgreSQL 9.2.1 64-bit server on Arch Linux
    2. PostgreSQL 9.2.1 64-bit server on Windows 2008 R2 SP1

    Special thanks to RhodiumToad from #postgresql

    Reference: http://www.postgresql.org/docs/9.2/static/protocol.html

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

struct postgresql_status {
   u_char status;
   u_char user[65];
   u_char type;
   u_char password[65];
   u_char hash[33];
   u_char salt[9];
   u_char database[65];
};

#define WAIT_AUTH       1
#define WAIT_RESPONSE   2
#define WAIT_RESULT     3
#define MD5             1
#define CT              2

/* protos */

FUNC_DECODER(dissector_postgresql);
void postgresql_init(void);

/************************************************/

#define GET_ULONG_BE(n,b,i)                             \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)    ] << 24 )        \
        | ( (unsigned long) (b)[(i) + 1] << 16 )        \
        | ( (unsigned long) (b)[(i) + 2] <<  8 )        \
        | ( (unsigned long) (b)[(i) + 3]       );       \
}

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

void __init postgresql_init(void)
{
   dissect_add("postgresql", APP_LAYER_TCP, 5432, dissector_postgresql);
}

FUNC_DECODER(dissector_postgresql)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct postgresql_status *conn_status;

   if (FROM_CLIENT("postgresql", PACKET)) {
      if (PACKET->DATA.len < 4)
         return NULL;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_postgresql));

      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         /* search for user and database strings, look for StartupMessage  */
         unsigned char *u = memmem(ptr, PACKET->DATA.len, "user", 4);
         unsigned char *d = memmem(ptr, PACKET->DATA.len, "database", 8);
         if (!memcmp(ptr + 4, "\x00\x03\x00\x00", 4) && u && d) {
            /* create the new session */
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_postgresql));

            /* remember the state (used later) */
            SAFE_CALLOC(s->data, 1, sizeof(struct postgresql_status));

            conn_status = (struct postgresql_status *) s->data;
            conn_status->status = WAIT_AUTH;

            /* user is always null-terminated */
            strncpy((char*)conn_status->user, (char*)(u + 5), 65);
            conn_status->user[64] = 0;

            /* database is always null-terminated */
            strncpy((char*)conn_status->database, (char*)(d + 9), 65);
            conn_status->database[64] = 0;

            /* save the session */
            session_put(s);
         }
      } else {
         conn_status = (struct postgresql_status *) s->data;
         if (conn_status->status == WAIT_RESPONSE) {

            /* check for PasswordMessage packet */
            if (ptr[0] == 'p' && conn_status->type == MD5) {
               DEBUG_MSG("\tDissector_postgresql RESPONSE type is MD5");
               if(memcmp(ptr + 1, "\x00\x00\x00\x28", 4)) {
                  DEBUG_MSG("\tDissector_postgresql BUG, expected length is 40");
                  return NULL;
               }
               if (PACKET->DATA.len < 40) {
                  DEBUG_MSG("\tDissector_postgresql BUG, expected length is 40");
                  return NULL;
               }
               memcpy(conn_status->hash, ptr + 5 + 3, 32);
               conn_status->hash[32] = 0;
               DISSECT_MSG("%s:$postgres$%s*%s*%s:%s:%d\n", conn_status->user, conn_status->user, conn_status->salt, conn_status->hash, ip_addr_ntoa(&PACKET->L3.dst, tmp), ntohs(PACKET->L4.dst));
               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_postgresql));
            }
            else if (ptr[0] == 'p' && conn_status->type == CT) {
               int length;
               DEBUG_MSG("\tDissector_postgresql RESPONSE type is clear-text!");
               GET_ULONG_BE(length, ptr, 1);
               strncpy((char*)conn_status->password, (char*)(ptr + 5), length - 4);
               conn_status->password[length - 4] = 0;
               DISSECT_MSG("PostgreSQL credentials:%s-%d:%s:%s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp), ntohs(PACKET->L4.dst), conn_status->user, conn_status->password);
               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_postgresql));
            }
         }
      }
   } else { /* Packets coming from the server */
      if (PACKET->DATA.len < 9)
         return NULL;
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_postgresql));

      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         conn_status = (struct postgresql_status *) s->data;
         if (conn_status->status == WAIT_AUTH &&
               ptr[0] == 'R' && !memcmp(ptr + 1, "\x00\x00\x00\x0c", 4)  &&
               !memcmp(ptr + 5, "\x00\x00\x00\x05", 4)) {

            conn_status->status = WAIT_RESPONSE;

            conn_status->type = MD5;
            DEBUG_MSG("\tDissector_postgresql AUTH type is MD5");
            hex_encode(ptr + 9, 4, conn_status->salt); /* save salt */
         }
         else if (conn_status->status == WAIT_AUTH &&
               ptr[0] == 'R' && !memcmp(ptr + 1, "\x00\x00\x00\x08", 4)  &&
               !memcmp(ptr + 5, "\x00\x00\x00\x03", 4)) {
            conn_status->status = WAIT_RESPONSE;
            conn_status->type = CT;
            DEBUG_MSG("\tDissector_postgresql AUTH type is clear-text!");
         }
      }
   }

   SAFE_FREE(ident);
   return NULL;
}

// vim:ts=3:expandtab
