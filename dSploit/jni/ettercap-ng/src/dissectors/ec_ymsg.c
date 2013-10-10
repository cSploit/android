/*
    ettercap -- dissector Yahoo Messenger -- TCP 5050

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

    $Id: ec_ymsg.c,v 1.6 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

/* globals */


/* protos */

FUNC_DECODER(dissector_ymsg);
void ymsg_init(void);
void decode_pwd(char *pwd, char *outpwd);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ymsg_init(void)
{
   dissect_add("ymsg", APP_LAYER_TCP, 5050, dissector_ymsg);
}

FUNC_DECODER(dissector_ymsg)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   u_char *q;
   u_int32 field_len;
   
   /* Empty or not a yahoo messenger packet */
   if (PACKET->DATA.len == 0 || memcmp(ptr, "YMSG", 4))
      return NULL;

   DEBUG_MSG("ymsg --> TCP dissector_ymsg");

   /* standard ymesg separator */
   if ( !(ptr = memmem(ptr, PACKET->DATA.len, "\xC0\x80", 2)) )  
      return NULL;
      
   /* Login is ASCII 0 */
   if (*(ptr-1) == '0' && FROM_CLIENT("ymsg", PACKET)) {
      /* Skip the separator and reach the end*/
      ptr += 2; 
      for (q=ptr; *q != 0xc0 && q < end; q++);
      if (q >= end) 
         return NULL;
      
      /* Calculate the user len (no int overflow) */
      field_len = q - ptr;
      SAFE_CALLOC(PACKET->DISSECTOR.user, field_len + 1, sizeof(char));
      memcpy(PACKET->DISSECTOR.user, ptr, field_len);
      
      /* Skip the separator */
      ptr = q + 2; 
      if (*ptr != '6') { 
         SAFE_FREE(PACKET->DISSECTOR.user);
         return NULL;  
      }
      
      /* Login is ASCII 6 */
      ptr += 3; /* skip the separator and the "6" */
      for (q=ptr; *q != 0xc0 && q < end; q++);
      if (q >= end) { 
         SAFE_FREE(PACKET->DISSECTOR.user);
         return NULL;  
      } 

      /* Calculate the pass len (no int overflow) */
      field_len = q - ptr;
      SAFE_CALLOC(PACKET->DISSECTOR.pass, field_len + 1, sizeof(char));
      memcpy(PACKET->DISSECTOR.pass, ptr, field_len);
         
      PACKET->DISSECTOR.info = strdup("The pass is in MD5 format ( _2s43d5f is the salt )");
      
      DISSECT_MSG("YMSG : %s:%d -> USER: %s  HASH: %s  - %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                                ntohs(PACKET->L4.dst), 
                                                                PACKET->DISSECTOR.user,
                                                                PACKET->DISSECTOR.pass,
                                                                PACKET->DISSECTOR.info);
   } else if (*(ptr-1) == '1')  { /* Message is ASCII 1 */
      u_char *from=NULL, *to=NULL, *message=NULL, *temp_disp_data;

      /* Skip the separator and reach the end*/
      ptr += 2; 
      for (q=ptr; *q != 0xc0 && q < end; q++);
      if (q >= end) 
         return NULL;

      field_len = q - ptr;
      SAFE_CALLOC(from, field_len + 1, sizeof(char));
      memcpy(from, ptr, field_len);

      /* Skip the two separators and the ASCII 5 */
      ptr = q + 5; 
      for (q=ptr; *q != 0xc0 && q < end; q++);
      if (q >= end) {
         SAFE_FREE(from);
         return NULL;
      }
      
      field_len = q - ptr;
      SAFE_CALLOC(to, field_len + 1, sizeof(char));
      memcpy(to, ptr, field_len);
      
      /* Skip the two separators and the ASCII 14 */
      ptr = q + 6; 
      for (q=ptr; *q != 0xc0 && q < end; q++);
      if (q >= end) {
         SAFE_FREE(from);
         SAFE_FREE(to);
         return NULL;
      }

      field_len = q - ptr;
      SAFE_CALLOC(message, field_len + 1, sizeof(char));
      memcpy(message, ptr, field_len);
      
      /* Update disp_data in the packet object (128 byte will be enough for ****YAHOOO.... string */
      temp_disp_data = (u_char *)realloc(PACKET->DATA.disp_data, strlen(from) + strlen(to) + strlen(message) + 128);
      if (temp_disp_data != NULL) {
         PACKET->DATA.disp_data = temp_disp_data;
         sprintf(PACKET->DATA.disp_data, "*** Yahoo Message ***\n From: %s\n To: %s\n\n Message: %s\n", from, to, message); 		  	       
         PACKET->DATA.disp_len = strlen(PACKET->DATA.disp_data);
      }
            
      SAFE_FREE(from);
      SAFE_FREE(to);
      SAFE_FREE(message);
   }

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

