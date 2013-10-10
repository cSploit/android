/*
    ettercap -- dissector CVS -- TCP 2401

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

    $Id: ec_cvs.c,v 1.2 2003/10/28 22:15:03 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */

/* stolen from CVS scramble.c */
static u_char cvs_shifts[] = {
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	114,120, 53, 79, 96,109, 72,108, 70, 64, 76, 67,116, 74, 68, 87,
	111, 52, 75,119, 49, 34, 82, 81, 95, 65,112, 86,118,110,122,105,
	 41, 57, 83, 43, 46,102, 40, 89, 38,103, 45, 50, 42,123, 91, 35,
	125, 55, 54, 66,124,126, 59, 47, 92, 71,115, 78, 88,107,106, 56,
	 36,121,117,104,101,100, 69, 73, 99, 63, 94, 93, 39, 37, 61, 48,
	 58,113, 32, 90, 44, 98, 60, 51, 33, 97, 62, 77, 84, 80, 85,223,
	225,216,187,166,229,189,222,188,141,249,148,200,184,136,248,190,
	199,170,181,204,138,232,218,183,255,234,220,247,213,203,226,193,
	174,172,228,252,217,201,131,230,197,211,145,238,161,179,160,212,
	207,221,254,173,202,146,224,151,140,196,205,130,135,133,143,246,
	192,159,244,239,185,168,215,144,139,165,180,157,147,186,214,176,
	227,231,219,169,175,156,206,198,129,164,150,210,154,177,134,127,
	182,128,158,208,162,132,167,209,149,241,153,251,237,236,171,195,
	243,233,253,240,194,250,191,155,142,137,245,235,163,242,178,152
};

#define CVS_LOGIN "BEGIN VERIFICATION REQUEST"

/* protos */

FUNC_DECODER(dissector_cvs);
void cvs_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init cvs_init(void)
{
   dissect_add("cvs", APP_LAYER_TCP, 2401, dissector_cvs);
}

FUNC_DECODER(dissector_cvs)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   char *p;
   size_t i;

   /* don't complain about unused var */
   (void)end;

   /* skip messages coming from the server */
   if (FROM_SERVER("cvs", PACKET))
      return NULL;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   /* not a login packet */
   if ( strncmp(ptr, CVS_LOGIN, strlen(CVS_LOGIN)) )
      return NULL;
   
   DEBUG_MSG("CVS --> TCP dissector_cvs");
   
   /* move over the cvsroot path */
   ptr += strlen(CVS_LOGIN) + 1;

   /* go until \n */
   while(*ptr != '\n' && ptr != end) ptr++;
   if (ptr == end) return NULL;

   PACKET->DISSECTOR.user = strdup(++ptr);
   
   /* cut the username on \n */
   if ( (p = strchr(PACKET->DISSECTOR.user, '\n')) != NULL )
      *p = '\0';
   
   /* go until \n */
   while(*ptr != '\n' && ptr != end) ptr++;
   if (ptr == end) return NULL;
 
   /* unsupported scramble method */
   if (*(++ptr) != 'A')
      return NULL;
   
   PACKET->DISSECTOR.pass = strdup(ptr);
   
   /* cut the username on \n */
   if ( (p = strchr(PACKET->DISSECTOR.pass, '\n')) != NULL )
      *p = '\0';
  
   /* no password */
   if (strlen(PACKET->DISSECTOR.pass) == 1 && PACKET->DISSECTOR.pass[0] == 'A') {
      SAFE_FREE(PACKET->DISSECTOR.pass);
      PACKET->DISSECTOR.pass = strdup("(empty)");
   } else {
   
      p = PACKET->DISSECTOR.pass;
   
      /* descramble the password */
      for (i = 1; i < sizeof(cvs_shifts) - 1 && p[i]; i++)                        
         p[i] = cvs_shifts[(size_t)p[i]];

      /* shift it to the left to eliminate the 'A' */
      for (i = 0; p[i]; i++)
         p[i] = p[i + 1];
      
   }
   
   /* final message */
   DISSECT_MSG("CVS : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass);

   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

