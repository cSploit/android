/*
    ettercap -- dissector icq -- TCP 5190

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

    $Id: ec_icq.c,v 1.5 2003/10/29 20:41:07 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */

struct tlv_hdr {
   u_int8 type[2];
      #define TLV_LOGIN "\x00\x01"
      #define TLV_PASS  "\x00\x02"
   u_int8 len[2];
};

struct flap_hdr {
   u_int8 cmd;
   u_int8 chan;
      #define FLAP_CHAN_LOGIN 1
   u_int16 seq;
   u_int16 dlen;
};

struct snac_hdr {
   u_int16 family;
   u_int16 subtype;
   u_int16 flags;
   u_char id[4];
};

/* protos */

FUNC_DECODER(dissector_icq);
void icq_init(void);
void decode_pwd(char *pwd, char *outpwd);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init icq_init(void)
{
   dissect_add("icq", APP_LAYER_TCP, 5190, dissector_icq);
}

FUNC_DECODER(dissector_icq)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct flap_hdr *fhdr;
   struct tlv_hdr *thdr;
   char *user, *pwdtemp;

   /* don't complain about unused var */
   (void)end;

   /* parse only version 7/8 */
   if (ptr[0] != 0x2a || ptr[1] > 4) 
      return NULL;
   
   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   /* skip messages coming from the server */
   if (FROM_SERVER("icq", PACKET))
      return NULL;
  
   DEBUG_MSG("ICQ --> TCP dissector_icq [%d.%d]", ptr[0], ptr[1]);
   
   /* we try to recognize the protocol */
   fhdr = (struct flap_hdr *) ptr;

   /* login sequence */
   if (fhdr->chan == FLAP_CHAN_LOGIN) {

      /* move the pointer */
      ptr += sizeof(struct flap_hdr);
      thdr = (struct tlv_hdr *) ptr;
      
      /* we need server HELLO (0000 0001) */
      if (memcmp(ptr, "\x00\x00\x00\x01", 4) ) 
         return NULL;
      
      /* move the pointer */
      thdr = thdr + 1;

      /* catch the login */
      if (memcmp(thdr->type, TLV_LOGIN, sizeof(thdr->type)))
         return NULL;
      
      DEBUG_MSG("\tDissector_icq - LOGIN ");

      /* point to the user */
      user = (char *)(thdr + 1);

      /* move the pointer */
      thdr = (struct tlv_hdr *) ((char *)thdr + sizeof(struct tlv_hdr) + thdr->len[1]);

      DEBUG_MSG("\tdissector_icq : TLV TYPE [%d]", thdr->type[1]);
            
      /* catch the pass */
      if (memcmp(thdr->type, TLV_PASS, sizeof(thdr->type)))
         return NULL;

      DEBUG_MSG("\tDissector_icq - PASS");
      
      /* use a temp buff to decript the password */
      pwdtemp = strdup((char *)(thdr + 1));

      SAFE_CALLOC(PACKET->DISSECTOR.pass, strlen(pwdtemp) + 1, sizeof(char));

      /* decode the password */
      decode_pwd(pwdtemp, PACKET->DISSECTOR.pass);
      /* save the user */
      PACKET->DISSECTOR.user = strdup(user);

      SAFE_FREE(pwdtemp);
      
      /* move the pointer */
      thdr = (struct tlv_hdr *) ((char *)thdr + sizeof(struct tlv_hdr) + thdr->len[1]);

      PACKET->DISSECTOR.info = strdup((char *)(thdr + 1));
      
      DISSECT_MSG("ICQ : %s:%d -> USER: %s  PASS: %s \n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass);

   } 

   return NULL;
}

/*
 * decode the crypted password 
 */
void decode_pwd(char *pwd, char *outpwd)
{
   size_t x;
   u_char pwd_key[] = {
      0xF3, 0x26, 0x81, 0xC4, 0x39, 0x86, 0xDB, 0x92,
      0x71, 0xA3, 0xB9, 0xE6, 0x53, 0x7A, 0x95, 0x7C
   };
   
   for( x = 0; x < strlen(pwd); x++)
      *(outpwd + x) = pwd[x] ^ pwd_key[x];
   
   return;
}

/* EOF */

// vim:ts=3:expandtab

