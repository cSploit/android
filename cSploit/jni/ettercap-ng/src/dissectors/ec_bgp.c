/*
    ettercap -- dissector BGP 4 -- TCP 179

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

/*
 *
 *       BPG version 4     RFC 1771
 *
 *        0                   1                   2                   3
 *        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    0  |                                                               |
 *       +                                                               +
 *    4  |                                                               |
 *       +                             Marker                            +
 *    8  |                                                               |
 *       +                                                               +
 *   12  |                                                               |
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   16  |          Length               |      Type     |    Version    |
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   20  |     My Autonomous System      |           Hold Time           |
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   24  |                         BGP Identifier                        |
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   28  | Opt Parm Len  |                                               |
 *       +-+-+-+-+-+-+-+-+       Optional Parameters                     |
 *   32  |                                                               |
 *       |                                                               |
 *       ~                                                               ~
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *        0                   1
 *        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...
 *       |  Parm. Type   | Parm. Length  |  Parameter Value (variable)
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...
 *
 *
 *         a) Authentication Information (Parameter Type 1):
 *
 *            This optional parameter may be used to authenticate a BGP
 *            peer. The Parameter Value field contains a 1-octet
 *            Authentication Code followed by a variable length
 *            Authentication Data.
 *
 *                 0 1 2 3 4 5 6 7 8
 *                +-+-+-+-+-+-+-+-+
 *                |  Auth. Code   |
 *                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                |                                                     |
 *                |              Authentication Data                    |
 *                |                                                     |
 *                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *               Authentication Code:
 *
 *                  This 1-octet unsigned integer indicates the
 *                  authentication mechanism being used.  Whenever an
 *                  authentication mechanism is specified for use within
 *                  BGP, three things must be included in the
 *                  specification:
 *
 *                  - the value of the Authentication Code which indicates
 *                  use of the mechanism,
 *                  - the form and meaning of the Authentication Data, and
 *                  - the algorithm for computing values of Marker fields.
 *
 *                  Note that a separate authentication mechanism may be
 *                  used in establishing the transport level connection.
 *
 *               Authentication Data:
 *
 *                  The form and meaning of this field is a variable-
 *                  length field depend on the Authentication Code.
 *
 */
 
#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>


/* protos */

FUNC_DECODER(dissector_bgp);
void bgp_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init bgp_init(void)
{
   dissect_add("bgp", APP_LAYER_TCP, 179, dissector_bgp);
}

FUNC_DECODER(dissector_bgp)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   u_char *parameters;
   u_char param_length;
   u_int32 i;
   u_char BGP_MARKER[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

   /* don't complain about unused var */
   (void)end;
   
   /* skip packets that don't have enough data */
   if (PACKET->DATA.len < 30)
      return NULL;

   /* not the right version (4) */
   if ( ptr[19] != 4 ) 
      return 0;                  
   
   /* not an OPEN message */
   if ( ptr[18] != 1 ) 
      return 0;
                     
   /* BGP marker has to be FFFFFF... */
   if ( memcmp(ptr, BGP_MARKER, 16) ) 
      return 0;
   
   /* no optional parameter */
   if ( (param_length = ptr[28]) == 0 ) 
      return 0; 

   /* skip to parameters */
   parameters = ptr + 29;

   if ((ptr + param_length) > end)
      return 0;

   DEBUG_MSG("BGP --> TCP dissector_bgp");

   /* move through the param list */
   for ( i = 0; i <= param_length; i += (parameters[i + 1] + 2) ) {

      /* the parameter is an authentication type (1) */
      if (parameters[i] == 1) {
         char *str_ptr;
         u_int32 j;
         u_int32 length = parameters[i + 1];
        
         DEBUG_MSG("\tDissector_BGP 4 AUTH");
         
         PACKET->DISSECTOR.user = strdup("BGP4");
         SAFE_CALLOC(PACKET->DISSECTOR.pass, length * 3 + 10, sizeof(char));
         SAFE_CALLOC(PACKET->DISSECTOR.info, 32, sizeof(char));

         /* Get authentication type */
         snprintf(PACKET->DISSECTOR.info, 32, "AUTH TYPE [0x%02x]", parameters[i+2]);
         
         /* Get authentication data */
         if (length > 1) {
            snprintf(PACKET->DISSECTOR.pass, 4, "Hex(");
            str_ptr = PACKET->DISSECTOR.pass + strlen(PACKET->DISSECTOR.pass);

            for (j = 0; j < (length-1); j++) {
               //u_int32 k = j+3;
               u_char *temp = parameters + i + j + 3;
               size_t temp_len = strlen((char *)temp) + 2;
               snprintf(str_ptr + (j * 3), temp_len, " %.2x", *temp);
            }
         
            strcat(str_ptr, " )");
         }	 
         
         DISSECT_MSG("BGP : %s:%d -> %s  %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                             ntohs(PACKET->L4.dst), 
                                             PACKET->DISSECTOR.info,
                                             PACKET->DISSECTOR.pass);
   
         return NULL;
      }
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

