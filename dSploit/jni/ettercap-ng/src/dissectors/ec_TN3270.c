/*
    ettercap -- dissector for Mainframe TN3270 z/OS TSO logon protocol

    Copyright (C) Dhiru Kholia (dhiru at openwall.com)

    Based on MFSniffer (https://github.com/mainframed/MFSniffer)

    Created by: Soldier of Fortran (@mainframed767)

    (tested against x3270 and TN3270 X and TN3270 Plus)

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

FUNC_DECODER(dissector_TN3270);
void TN3270_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init TN3270_init(void)
{
   dissect_add("TN3270", APP_LAYER_TCP, 623, dissector_TN3270);
}

/* Function to convert EBCDIC to ASCII */
static unsigned char e2a[256] = {
        0,  1,  2,  3,156,  9,134,127,151,141,142, 11, 12, 13, 14, 15,
        16, 17, 18, 19,157,133,  8,135, 24, 25,146,143, 28, 29, 30, 31,
        128,129,130,131,132, 10, 23, 27,136,137,138,139,140,  5,  6,  7,
        144,145, 22,147,148,149,150,  4,152,153,154,155, 20, 21,158, 26,
        32,160,161,162,163,164,165,166,167,168, 91, 46, 60, 40, 43, 33,
        38,169,170,171,172,173,174,175,176,177, 93, 36, 42, 41, 59, 94,
        45, 47,178,179,180,181,182,183,184,185,124, 44, 37, 95, 62, 63,
        186,187,188,189,190,191,192,193,194, 96, 58, 35, 64, 39, 61, 34,
        195, 97, 98, 99,100,101,102,103,104,105,196,197,198,199,200,201,
        202,106,107,108,109,110,111,112,113,114,203,204,205,206,207,208,
        209,126,115,116,117,118,119,120,121,122,210,211,212,213,214,215,
        216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
        123, 65, 66, 67, 68, 69, 70, 71, 72, 73,232,233,234,235,236,237,
        125, 74, 75, 76, 77, 78, 79, 80, 81, 82,238,239,240,241,242,243,
        92,159, 83, 84, 85, 86, 87, 88, 89, 90,244,245,246,247,248,249,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57,250,251,252,253,254,255
};

static void ebcdic2ascii(unsigned char *str, int n, unsigned char *out)
{
   int i;
   for (i = 0; i < n; ++i)
      out[i] = e2a[str[i]];
}


FUNC_DECODER(dissector_TN3270)
{
   DECLARE_DISP_PTR_END(ptr, end);
   unsigned int i;
   char tmp[MAX_ASCII_ADDR_LEN];

   //suppress unused warning
   (void)end;

   if (FROM_CLIENT("TN3270", PACKET)) {

      if (PACKET->DATA.len > 200 || PACKET->DATA.len < 5) /* is this always valid? */
         return NULL;

      char output[512] = { 0 };
      char username[512] = { 0 };
      char password[512] = { 0 };

      ebcdic2ascii(ptr, PACKET->DATA.len, (unsigned char*)output);

      /* scan packets to find username and password */
      for (i = 0; i < PACKET->DATA.len - 5; i++) {
         /* find username, logons start with 125 193 (215 or 213) 17 64 90 ordinals
          * We relax the check for third byte because it is less error-prone that way */
         if (ptr[i] == 125 && ptr[i+1] == 193 && /* (ptr[i+2] == 215 || ptr[i+2] == 213) && */
                 ptr[i+3] == 17 && ptr[i+4] == 64 && ptr[i+5] == 90) {
                 /* scan for spaces */
                 int j = i + 6;
                 while (j < 512 && output[j] == 32)
                    j++;

		 if (j==511) /* Don't even bother */
			continue;

                 strncpy(username, &output[j], 512);

		 username[511] = 0; /* Boundary */

                 int l = strlen(username);
                 username[l-2] = 0;
                 DISSECT_MSG("%s:%d <= z/OS TSO Username : %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp), ntohs(PACKET->L4.dst), username);
         }
         /* find password */
         if (ptr[i] == 125 && ptr[i+1] == 201 && ptr[i+3] == 17 && ptr[i+4] == 201 && ptr[i+5] == 195) {
                 strncpy(password, &output[i + 6], 512);
		 password[511] = 0; /* Boundary */
                 int l = strlen(password);
                 password[l-2] = 0;
                 DISSECT_MSG("%s:%d <= z/OS TSO Password : %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp), ntohs(PACKET->L4.dst), password);
         }
      }
   }

   return NULL;
}

// vim:ts=3:expandtab
