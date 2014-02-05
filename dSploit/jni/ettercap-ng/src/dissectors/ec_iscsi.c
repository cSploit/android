/*
    ettercap -- dissector for iSCSI CHAP authentication -- TCP 3260

    Copyright (C) Dhiru Kholia (dhiru at openwall.com)

    Tested with,

    1. Dell EqualLogic target and Open-iSCSI initiator on CentOS 6.2
    2. LIO Target and Open-iSCSI initiator, both on Arch Linux 2012

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

struct iscsi_status {
   u_char status;
   u_char id; /* CHAP_I */
   u_char challenge[49]; /* CHAP_C can be 24 bytes long (48 bytes hex-encoded)  */
};

#define WAIT_RESPONSE   1

/* protos */

FUNC_DECODER(dissector_iscsi);
void iscsi_init(void);

/************************************************/

/* borrowed from http://dsss.be/w/c:memmem */
static unsigned char *_memmem(unsigned char *haystack, int hlen, char *needle, int nlen)
{
   int i, j = 0;
   if (nlen > hlen)
      return 0;
   switch (nlen) { // we have a few specialized compares for certain needle sizes
   case 0:     // no needle? just give the haystack
      return haystack;
   case 1:     // just use memchr for 1-byte needle
      return memchr(haystack, needle[0], hlen);
   case 2:     // use 16-bit compares for 2-byte needles
      for (i = 0; i < hlen - nlen + 1; i++) {
         if (*(uint16_t *) (haystack + i) == *(uint16_t *) needle) {
            return haystack + i;
         }
      }
      break;
   case 4:     // use 32-bit compares for 4-byte needles
      for (i = 0; i < hlen - nlen + 1; i++) {
         if (*(uint32_t *) (haystack + i) == *(uint32_t *) needle) {
            return haystack + i;
         }
      }
      break;
   default:    // generic compare for any other needle size
      // walk i through the haystack, matching j as long as needle[j] matches haystack[i]
      for (i = 0; i < hlen - nlen + 1; i++) {
         if (haystack[i] == needle[j]) {
            if (j == nlen - 1) { // end of needle and it all matched?  win.
               return haystack + i - j;
            } else { // keep advancing j (and i, implicitly)
               j++;
            }
         } else { // no match, rewind i the length of the failed match (j), and reset j
            i -= j;
            j = 0;
         }
      }
   }
   return NULL;
}

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init iscsi_init(void)
{
   dissect_add("iscsi", APP_LAYER_TCP, 3260, dissector_iscsi);
}

FUNC_DECODER(dissector_iscsi)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct iscsi_status *conn_status;

   //suppress unused warning
   (void)end;

   /* Packets coming from the server */
   if (FROM_SERVER("iscsi", PACKET)) {

      /* Interesting packets have len >= 4 */
      if (PACKET->DATA.len < 4)
         return NULL;

      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_iscsi));

      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         /* search for CHAP ID and Message Challenge */
         char *i = (char*)_memmem(ptr, PACKET->DATA.len, "CHAP_I", 6);
         char *c = (char*)_memmem(ptr, PACKET->DATA.len, "CHAP_C", 6);
         if (i && c) {
            /* create the new session */
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_iscsi));

            /* remember the state (used later) */
            SAFE_CALLOC(s->data, 1, sizeof(struct iscsi_status));

            conn_status = (struct iscsi_status *) s->data;
            conn_status->status = WAIT_RESPONSE;
            conn_status->id = (u_char)atoi(i + 7);

            /* CHAP_C is always null-terminated */
            strncpy((char*)conn_status->challenge, c + 9, 48);
            conn_status->challenge[48] = 0;

            /* save the session */
            session_put(s);
         }
      }
   } else { /* Packets coming from the client */
      dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_iscsi));

      /* Only if we catched the connection from the beginning */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         conn_status = (struct iscsi_status *) s->data;
         char *n = NULL;
         char *r = NULL;
         if (PACKET->DATA.len > 5) {
            n = (char*)_memmem(ptr, PACKET->DATA.len, "CHAP_N", 6);
            r = (char*)_memmem(ptr, PACKET->DATA.len, "CHAP_R", 6);
         }

         if (conn_status->status == WAIT_RESPONSE && n && r) {
            char user[65];
            char response[33];
            DEBUG_MSG("\tDissector_iscsi RESPONSE");
            /* CHAP_R and CHAP_N are always null-terminated */
            /* Even possibly wrong passwords can be useful,
             * don't look for login result */
            strncpy(response, r + 9, 32);
            response[32] = 0;
            strncpy(user, n + 7, 64);
            user[64] = 0;
            DISSECT_MSG("%s-%s-%d:$chap$%d*%s*%s\n", user, ip_addr_ntoa(&PACKET->L3.dst, tmp),
                            ntohs(PACKET->L4.dst), conn_status->id, conn_status->challenge, response);
            dissect_wipe_session(PACKET, DISSECT_CODE(dissector_iscsi));
         }
      }
   }

   SAFE_FREE(ident);
   return NULL;
}

// vim:ts=3:expandtab
