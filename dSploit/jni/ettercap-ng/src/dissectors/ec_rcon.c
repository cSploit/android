/*
    ettercap -- dissector RCON -- UDP 27015 27960

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

    $Id: ec_rcon.c,v 1.3 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

/* protos */

FUNC_DECODER(dissector_rcon);
void rcon_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init rcon_init(void)
{
   dissect_add("rcon", APP_LAYER_UDP, 27015, dissector_rcon); /* half life */
   dissect_add("rcon", APP_LAYER_UDP, 27960, dissector_rcon); /* quake 3 */
}

FUNC_DECODER(dissector_rcon)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];

   /* skip messages coming from the server */
   if (FROM_SERVER("rcon", PACKET))
      return NULL;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   DEBUG_MSG("RCON --> UDP dissector_rcon");

   /*
    *  format of an rcon-command:
    *  
    *  0xFF 0xFF 0xFF 0xFF "RCON authkey command"
    */

   /* not a good packet */
   if (memcmp(ptr, "\xff\xff\xff\xff", 4))
      return NULL;

   ptr += 4;
   
   if ( !strncasecmp(ptr, "rcon", 4)  ) {
      
      u_char *q;
      
      DEBUG_MSG("\tDissector_rcon RCON command\n");
      
      ptr += 4;

      /* skip the whitespaces at the beginning */
      while(*ptr == ' ' && ptr != end) ptr++;

      /* reached the end */
      if (ptr == end) return NULL;

      q = ptr;
      
      /* move after the authkey */
      while(*q != ' ' && q != end) q++;
      
      /* reached the end */
      if (q == end) return NULL;

      PACKET->DISSECTOR.user = strdup("RCON");

      SAFE_CALLOC(PACKET->DISSECTOR.pass, q - ptr + 1, sizeof(char));
      strlcpy(PACKET->DISSECTOR.pass, ptr, q - ptr + 1);

      SAFE_CALLOC(PACKET->DISSECTOR.info, strlen(q) + 1, sizeof(char));
      sprintf(PACKET->DISSECTOR.info, "%s", q);

      DISSECT_MSG("RCON : %s:%d -> AUTHKEY: %s  COMMAND: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
   }
  
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

