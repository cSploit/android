/*
    ettercap -- dissector X11 -- TCP 6000, 6001, ...

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

    $Id: ec_x11.c,v 1.9 2004/01/21 20:20:07 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */

struct x11_request {
   u_int8 endianness;
   u_int8 unused8;
   u_int16 major;
   u_int16 minor;
   u_int16 name_len;
   u_int16 data_len;
   u_int16 unused16;
   u_int8 name[18];
   u_int16 null;
   u_int8 data[16];
};

/* protos */

FUNC_DECODER(dissector_x11);
void x11_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init x11_init(void)
{
   dissect_add("x11", APP_LAYER_TCP, 6000, dissector_x11);
   dissect_add("x11", APP_LAYER_TCP, 6001, dissector_x11);
   dissect_add("x11", APP_LAYER_TCP, 6002, dissector_x11);
   dissect_add("x11", APP_LAYER_TCP, 6003, dissector_x11);
}

FUNC_DECODER(dissector_x11)
{
   struct ec_session *s = NULL;
   void *ident;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct x11_request *x11;
   int i;
   
   /* 
    * check if it is the first packet sent by the server i
    * after the session is created (cookie already sent)
    */
   IF_FIRST_PACKET_FROM_SERVER("x11", s, ident, dissector_x11) {
            
      DEBUG_MSG("\tdissector_x11 BANNER");
      /*
       * get the banner 
       * this parsing is very ugly, but is works (at least for me)
       * it should be better checked in the header to find the 
       * banner lenght etc etc...
       */
      PACKET->DISSECTOR.banner = strdup(PACKET->DATA.disp_data + 40);
     
   } ENDIF_FIRST_PACKET_FROM_SERVER(s, ident)
   
   /* skip messages coming from the server */
   if (FROM_SERVER("x11", PACKET))
      return NULL;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
 
   DEBUG_MSG("X11 --> TCP dissector_x11");
   
   x11 = (struct x11_request *)PACKET->DATA.disp_data;

   /* 
    * can somebody test it under big-endian systems
    * and tell me what is the right parsing ?
    */
   if (x11->endianness != 0x6c)
      return NULL;
   
   /* not supported other than MIT-MAGIC-COOKIE-1 */
   if (x11->name_len != 18 || x11->data_len != 16)
      return NULL;
   
   /* check the magic string */
   if (strncmp(x11->name, "MIT-MAGIC-COOKIE-1", x11->name_len))
      return NULL;
       
   DEBUG_MSG("\tDissector_x11 COOKIE");
   
   /* fill the structure */
   PACKET->DISSECTOR.user = strdup("MIT-MAGIC-COOKIE-1");
  
   /* the cookie's lenght is 32, take care of the null char */
   SAFE_CALLOC(PACKET->DISSECTOR.pass, 33, sizeof(char));
      
   for (i = 0; i < 16; i++)                                                                      
      sprintf(PACKET->DISSECTOR.pass + (i * 2), "%.2x", x11->data[i] ); 
   
   /* 
    * create the session to remember to check the
    * banner on the next packet sent by server
    * the check is made by IF_FIRST_PACKET_FROM_SERVER
    */
   dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_x11));
   session_put(s);
   
   /* print the message */
   DISSECT_MSG("X11 : %s:%d -> XAUTH: %s %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                 ntohs(PACKET->L4.dst), 
                                 PACKET->DISSECTOR.user,
                                 PACKET->DISSECTOR.pass);
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

