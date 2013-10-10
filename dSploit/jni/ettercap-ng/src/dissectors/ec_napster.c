/*
    ettercap -- dissector napster -- TCP 7777 8888

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

    $Id: ec_napster.c,v 1.6 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

/* globals */

#define NAPSTER_CMD_LOGIN  2

struct nap_hdr {
   u_int16 len;
   u_int16 type;
};

/* protos */

FUNC_DECODER(dissector_napster);
void napster_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init napster_init(void)
{
   dissect_add("napster", APP_LAYER_TCP, 7777, dissector_napster);
   dissect_add("napster", APP_LAYER_TCP, 8888, dissector_napster);
}

FUNC_DECODER(dissector_napster)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct nap_hdr *nap;
   u_char *tbuf, *user, *pass, *client;
   size_t tlen;
   char *tok;

   /* don't complain about unused var */
   (void)end;
   
   /* skip messages coming from the server */
   if (FROM_SERVER("napster", PACKET))
      return NULL;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   /* cast the napster header */
   nap = (struct nap_hdr *)ptr;
   
   DEBUG_MSG("NAPSTER --> TCP dissector_napster (%d)", phtos(&nap->type));

   /* not a login packet */
   if (phtos(&nap->type) != NAPSTER_CMD_LOGIN)
      return NULL;

   /*
    *  format of an napster login:
    *  
    *  <nick> <password> <port> "<client-info>" <link-type> [ <num> ]
    */

   DEBUG_MSG("\tDissector_napster LOGIN\n");
   
   /* move after the header */
   ptr = (char *)(nap + 1);
   
   /* get the len of the command */
   tlen = phtos(&nap->len);

   /* use a temp buffer to operate on */
   SAFE_CALLOC(tbuf, tlen + 1, sizeof(char));
   strlcpy(tbuf, ptr, tlen + 1);   

   /* get the user */
   if ((user = ec_strtok(tbuf, " ", &tok)) == NULL)
      goto bad;
   
   /* get the pass */
   if ((pass = ec_strtok(NULL, " ", &tok)) == NULL)
      goto bad;

   /* skip the port */
   if ((client = ec_strtok(NULL, " ", &tok)) == NULL)
      goto bad;

   /* get the client */
   if ((client = ec_strtok(NULL, "\"", &tok)) == NULL)
      goto bad;
   
   PACKET->DISSECTOR.user = strdup(user);
   PACKET->DISSECTOR.pass = strdup(pass);
   PACKET->DISSECTOR.info = strdup(client);

   DISSECT_MSG("NAPSTER : %s:%d -> USER: %s  PASS: %s  CLIENT: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                 ntohs(PACKET->L4.dst), 
                                 PACKET->DISSECTOR.user,
                                 PACKET->DISSECTOR.pass,
                                 PACKET->DISSECTOR.info);
bad:
   
   SAFE_FREE(tbuf);
  
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

