/*
    ettercap -- dissector mountd - RPC

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

    $Id: ec_mountd.c,v 1.3 2004/01/21 20:20:06 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */
typedef struct {
   u_int32 xid;
   u_int32 ver;
   u_char *rem_dir;
} mountd_session;

/* protos */
FUNC_DECODER(dissector_mountd);
void mountd_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init mountd_init(void)
{
   /* This is inserted into the dissectors' chain by the portmap dissector */
}

FUNC_DECODER(dissector_mountd)
{
   DECLARE_DISP_PTR_END(ptr, end);
   u_int32 type, xid, proc, program, version, flen, cred, offs, i;
   mountd_session *pe;
   char *fhandle;
   struct ec_session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* don't complain about unused var */
   (void)end;

   /* skip unuseful packets */
   if (PACKET->DATA.len < 24)
      return NULL;
   
   DEBUG_MSG("mountd --> dissector_mountd");

   /* Offsets differs from TCP to UDP (?) */
   if (PACKET->L4.proto == NL_TYPE_TCP)
      ptr += 4;

   xid  = pntol(ptr);
   type = pntol(ptr + 4);

   /* CALL */
   if (FROM_CLIENT("mountd", PACKET)) {

      program = pntol(ptr + 12);
      version = pntol(ptr + 16);
      proc    = pntol(ptr + 20);

      if (type != 0 || program != 100005 || proc != 1 ) 
         return NULL;

      /* Take remote dir (with some checks) */	
      cred = pntol(ptr + 28);
      if (cred > PACKET->DATA.len)
         return NULL;
      flen  = pntol(ptr + 40 + cred);
      if (flen > 100)
         return NULL;
	 
      dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_mountd));
      SAFE_CALLOC(s->data, 1, sizeof(mountd_session));
      pe = (mountd_session *)s->data;
      pe->xid = xid;
      pe->ver = version;
      SAFE_CALLOC(pe->rem_dir, 1, flen + 1);
      memcpy(pe->rem_dir, ptr + 44 + cred, flen); 
      session_put(s);
      
      return NULL;
   }

   /* REPLY */
   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_mountd));
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
      return NULL;
   }

   SAFE_FREE(ident);
   pe = (mountd_session *)s->data;

   PACKET->DISSECTOR.banner = strdup("mountd");

   /* Unsuccess or not a reply */
   if (!pe || !(pe->rem_dir) || pe->xid != xid || pntol(ptr + 24) != 0 || type != 1) 
      return NULL;

   /* Take the handle */
   if (pe->ver == 3) {
      flen = pntol(ptr + 28);
      if (flen > 64) 
         flen = 64;
      offs = 32;
   } else {
      flen = 32;
      offs = 28;
   }

   SAFE_CALLOC(fhandle, (flen*3) + 10, 1);
   for (i=0; i<flen; i++)
      sprintf(fhandle, "%s%.2x ", fhandle, ptr[i + offs]);
   
   DISSECT_MSG("mountd : Server:%s Handle %s: [%s]\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                                       pe->rem_dir, 
                                                       fhandle);

   SAFE_FREE(pe->rem_dir);
   SAFE_FREE(fhandle);
   dissect_wipe_session(PACKET, DISSECT_CODE(dissector_mountd));
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

