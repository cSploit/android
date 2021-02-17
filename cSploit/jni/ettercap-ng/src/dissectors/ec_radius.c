/*
    ettercap -- dissector RADIUS -- UDP 1645 1646

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
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |     Code      |   Identifier  |            Length             |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   | . . . . . . . . . . 16-Bytes Authenticator. . . . . . . . . . |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   | . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . |
 *   | . . . . . . . . . . . . . Attributes  . . . . . . . . . . . . |
 *   | . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *   RFC 2865
 *
 * The password is first
 * padded at the end with nulls to a multiple of 16 octets.  A one-
 * way MD5 hash is calculated over a stream of octets consisting of
 * the shared secret followed by the Request Authenticator.  This
 * value is XORed with the first 16 octet segment of the password and
 * placed in the first 16 octets of the String field of the User-
 * Password Attribute.
 * 
 * If the password is longer than 16 characters, a second one-way MD5
 * hash is calculated over a stream of octets consisting of the
 * shared secret followed by the result of the first xor.  That hash
 * is XORed with the second 16 octet segment of the password and
 * placed in the second 16 octets of the String field of the User-
 * Password Attribute.
 * 
 * If necessary, this operation is repeated, with each xor result
 * being used along with the shared secret to generate the next hash
 * to xor the next segment of the password, to no more than 128
 * characters.
 */

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

/* globals */

struct radius_header {
   u_int8  code;              /* type of the packet */
      #define RADIUS_ACCESS_REQUEST    0x1
      #define RADIUS_ACCESS_ACCEPT     0x2
      #define RADIUS_ACCESS_REJECT     0x3
      #define RADIUS_ACCOUNT_REQUEST   0x4
      #define RADIUS_ACCOUNT_RESPONSE  0x5
   u_int8  id;                /* identifier */
   u_int16 length;            /* packet length */
   u_int8  auth[16];          /* authenticator */
};

#define RADIUS_HEADER_LEN   0x14  /* 12 bytes */

#define RADIUS_ATTR_USER_NAME          0x01
#define RADIUS_ATTR_PASSWORD           0x02

/* proto */

FUNC_DECODER(dissector_radius);
void radius_init(void);
static u_char * radius_get_attribute(u_int8 attr, u_int16 *attr_len, u_char *begin, u_char *end);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init radius_init(void)
{
   dissect_add("radius", APP_LAYER_UDP, 1645, dissector_radius);
   dissect_add("radius", APP_LAYER_UDP, 1646, dissector_radius);
}

FUNC_DECODER(dissector_radius)
{
   DECLARE_REAL_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct radius_header *radius;
   u_char *attributes;
   char *attr;
   u_int16 attr_len;
   char user[0xff+1];
   char pass[0xff+1];
   char auth[0xff];
   size_t i;
   
   DEBUG_MSG("RADIUS --> UDP dissector_radius");

   /* parse the packet as a radius header */
   radius = (struct radius_header *)ptr;

   /* get the pointer to the attributes list */
   attributes = (u_char *)(radius + 1);
   
   /* we are interested only in ACCESS REQUESTS */
   if (radius->code != RADIUS_ACCESS_REQUEST)
      return NULL;

   /* search for the username attribute */
   attr = (char*)radius_get_attribute(RADIUS_ATTR_USER_NAME, &attr_len, attributes, end);
  
   /* if the attribute is not found, the packet is not interesting */
   if (attr == NULL)
      return NULL;

   /* 
    * max attr_len is 0xff 
    * copy the paramenter into the buffer and null terminate it
    */
   memset(user, 0, sizeof(user));
   strncpy(user, attr, attr_len);
   
   /* search for the password attribute */
   attr = (char*)radius_get_attribute(RADIUS_ATTR_PASSWORD, &attr_len, attributes, end);
  
   /* if the attribute is not found, the packet is not interesting */
   if (attr == NULL)
      return NULL;

   /* 
    * max attr_len is 0xff 
    * copy the paramenter into the buffer and null terminate it
    */
   memset(pass, 0, sizeof(pass));
   strncpy(pass, attr, attr_len);
   
   for (i = 0; i < 16; i++)
      snprintf(auth + i*2, 3, "%02X", radius->auth[i]);

  
   SAFE_CALLOC(PACKET->DISSECTOR.pass, attr_len * 2 + 1, sizeof(char));
   
   /* save the info */
   PACKET->DISSECTOR.user = strdup(user);
   
   for (i = 0; i < attr_len; i++)
      snprintf(PACKET->DISSECTOR.pass + i*2, 3, "%02X", pass[i]);
   
   PACKET->DISSECTOR.info = strdup(auth);
  
   /* display the message */
   DISSECT_MSG("RADIUS : %s:%d -> USER: %s  PASS: %s AUTH: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);

   
   return NULL;
}


/* 
 * find a radius attribute thru the list
 */
static u_char * radius_get_attribute(u_int8 attr, u_int16 *attr_len, u_char *begin, u_char *end)
{
   /* sanity check */
   if (begin == NULL || end == NULL)
      return NULL;

   if (begin > end)
      return NULL;

   DEBUG_MSG("radius_get_attribute: [%d]", attr);
   
   /* stop when the attribute list ends */
   while (begin < end) {

      /* get the len of the attribute and subtract the header len */
      *attr_len = *(begin + 1) - 2;
     
      /* we have found our attribute */
      if (*begin == attr) {
         /* return the pointer to the attribute value */
         return begin + 2;
      }

      /* move to the next attribute */
      if (*(begin + 1) > 0)
         begin += *(begin + 1);
      else
         return NULL;
   }
  
   /* not found */
   return NULL;
}

/* EOF */

// vim:ts=3:expandtab

