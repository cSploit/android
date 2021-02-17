/*
    ettercap -- dissector MySQL -- TCP 3306

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

/* protos */

FUNC_DECODER(dissector_mysql);
void mysql_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init mysql_init(void)
{
   dissect_add("mysql", APP_LAYER_TCP, 3306, dissector_mysql);
}

#ifdef DEBUG
static void print_hex(unsigned char *str, int len)
{
   int i;
   for (i = 0; i < len; ++i)
      printf("%02x", str[i]);
      printf("\n");
}
#endif

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

FUNC_DECODER(dissector_mysql)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   int old_mysql = 0;
   unsigned char input[20];
   unsigned char output[41];
   int has_password = 0;

   /* Skip ACK packets */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_mysql));

   /* Packets coming from the server */
   if (FROM_SERVER("mysql", PACKET)) {

      /* if the session does not exist... */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
         u_int index = 5;

         /* Check magic numbers and catch the banner */
         if (!memcmp(ptr + 1, "\x00\x00\x00\x0a\x33\x2e", 6)) {
            DEBUG_MSG("\tdissector_mysql BANNER");
            PACKET->DISSECTOR.banner = strdup("MySQL v3.xx.xx");
            old_mysql = 1;
         } else if (!memcmp(ptr + 1, "\x00\x00\x00\x0a\x34\x2e", 6)) {
            DEBUG_MSG("\tdissector_mysql BANNER");
            PACKET->DISSECTOR.banner = strdup("MySQL v4.xx.xx");
            old_mysql = 1;
         } else if (PACKET->DATA.len - 4 == ptr[0] && ptr[3] == 0 && ptr[4] == 10) {
            /* check Packet Length + Packet Number + Protocol */
            DEBUG_MSG("\tdissector_mysql BANNER");
            PACKET->DISSECTOR.banner = strdup("MySQL v5.xx.xx");
         }
         else {
            SAFE_FREE(ident);
            return NULL;
         }

         if(old_mysql) {
            /* Search for 000 padding */
            while( (ptr + index) < end && ((ptr[index] != 0) && (ptr[index - 1] != 0) && (ptr[index - 2] != 0)) )
               index++;
         }

         /* create the new session */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_mysql));
         s->flag = 0;

         if(old_mysql) {
            /* Save the seed */
            s->data = strdup((const char*)ptr + index + 1);
            s->flag = 1;
         }
         else {
            int length;
            unsigned char seed[20];
            ptr += 5; /* skip over Packet Length +  Packet Number + protocol_version*/
            length = strlen((const char*)ptr);
            ptr += (length +  1 + 4); /* skip over Version + Thread ID */
            s->data = malloc(41);
            memcpy(seed, ptr, 8);
            ptr += (8 + 1 + 2 + 1 + 2 + 13);
            memcpy(seed + 8, ptr, 12);
            hex_encode(seed, 20, output);
            output[40] = 0;
            memcpy(s->data, output ,41);
         }
         /* save the session */
         session_put(s);
      } 
   } else { /* Packets coming from the client */
      /* Only if we catched the connection from the beginning */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         if(!s->flag) {
            DEBUG_MSG("\tDissector_mysq DUMP ENCRYPTED");
            /* Reach and save the username */
            ptr += 36;
            PACKET->DISSECTOR.user = strdup((const char*)ptr);
            /* Reach the encrypted password */
            ptr += (strlen(PACKET->DISSECTOR.user) + 1 + 1);
            if(ptr < end) {
               memcpy(input, ptr, 20);
               hex_encode(input, 20, output);
               output[40] = 0;
               has_password = 1;
               int length = strlen(s->data) + 256;
               SAFE_CALLOC(PACKET->DISSECTOR.pass, length, sizeof(char));
               snprintf(PACKET->DISSECTOR.pass, length, "Seed:%s Encrypted:%s", (char *)s->data, output);
            }
            else {
               PACKET->DISSECTOR.pass = strdup("No Password!!!");
            }
            DISSECT_MSG("MYSQL : %s:%d -> USER:%s %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                            ntohs(PACKET->L4.dst),
                            PACKET->DISSECTOR.user,
                            PACKET->DISSECTOR.pass);
            if(has_password) {
               /* output format for JtR */
               DISSECT_MSG("%s*%d*%s:$mysqlna$%s*%s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                               ntohs(PACKET->L4.dst),
                               PACKET->DISSECTOR.user,
                               (char*)s->data, output);
            }
            dissect_wipe_session(PACKET, DISSECT_CODE(dissector_mysql));
         }
         else {
            DEBUG_MSG("\tDissector_mysql DUMP ENCRYPTED");
            /* Reach the username */
            ptr += 9;
            /* save the username */
            PACKET->DISSECTOR.user = strdup((const char*)ptr);
            /* Reach the encrypted password */
            ptr += strlen(PACKET->DISSECTOR.user) + 1;
            if (ptr < end && strlen((const char*)ptr) != 0) {
               int length = strlen(s->data) + strlen((const char*)ptr) + 128;
               SAFE_CALLOC(PACKET->DISSECTOR.pass, length, sizeof(char));
               snprintf(PACKET->DISSECTOR.pass, length, "Seed:%s Encrypted:%s", (char *)s->data, ptr);
            } else {
               PACKET->DISSECTOR.pass = strdup("No Password!!!");
            }
            
            DISSECT_MSG("MYSQL : %s:%d -> USER:%s %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                            ntohs(PACKET->L4.dst),
                            PACKET->DISSECTOR.user,
                            PACKET->DISSECTOR.pass);
            dissect_wipe_session(PACKET, DISSECT_CODE(dissector_mysql));
         }
      }
   }
   SAFE_FREE(ident);
   return NULL;
}

/* EOF */

// vim:ts=3:expandtab

