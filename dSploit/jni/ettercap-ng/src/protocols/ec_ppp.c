/*
    ettercap -- PPP decoder module

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

    $Id: ec_ppp.c,v 1.11 2004/02/01 16:48:51 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

#ifdef HAVE_OPENSSL
   #include <openssl/sha.h>
#endif

/* globals */
struct ppp_header {
   u_char  address;
   u_char  control;
   u_int16 proto;
};

struct ppp_lcp_header {
   u_char  code;
   u_char  ident;
   u_int16 length;
};

struct ppp_chap_challenge {
   u_char size;
   union {
      u_char challenge_v1[8];
      u_char challenge_v2[16];
      struct {
         u_char lanman[24];
         u_char nt[24];
         u_char flag;
      } response_v1;
      struct {
         u_char peer_challenge[16];
         u_char reserved[8];
         u_char nt[24];
         u_char flag;
      } response_v2;
   } value;
};

#define PPP_PROTO_IP		0x0021
#define PPP_PROTO_CHAP		0xc223
#define PPP_PROTO_PAP		0xc023
#define PPP_PROTO_LCP		0xc021
#define PPP_PROTO_ECP		0x8053
#define PPP_PROTO_CCP		0x80fd		
#define PPP_PROTO_IPCP		0x8021

#define PPP_CHAP_CODE_CHALLENGE	1
#define PPP_CHAP_CODE_RESPONSE	2

/* protos */

FUNC_DECODER(decode_ppp);
void ppp_init(void);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ppp_init(void)
{
   dissect_add("ppp", NET_LAYER, LL_TYPE_PPP, decode_ppp);
}


FUNC_DECODER(decode_ppp)
{
   FUNC_DECODER_PTR(next_decoder);
   struct ppp_header *ppph;
   struct ppp_lcp_header *lcph;
   struct ppp_chap_challenge *chapch;
   u_int16 proto;
   u_int32 i;
   u_char user[128], dummy[3], auth_len, temp[128], *pap_auth;
   static u_char  version=0, schallenge[512];
#ifdef HAVE_OPENSSL
   u_char digest[SHA_DIGEST_LENGTH];
   SHA_CTX ctx;
#endif


   ppph = (struct ppp_header *)DECODE_DATA;

   /* Set the L4 header for the hook */
   PACKET->L4.header = (u_char *)DECODE_DATA;
         
   /* HOOK POINT: HOOK_PACKET_PPP */
   hook_point(HOOK_PACKET_PPP, PACKET);

   /* We have to guess if header compression was negotiated */
   /* XXX - && or || ??? */
   if (ppph->address != 0xff && ppph->control != 0x3) {
      proto = *((u_char *)ppph);
      
      if (proto != PPP_PROTO_IP) {
         proto = ntohs(*((u_int16 *)ppph));
         DECODED_LEN = 2;
      } else 
         DECODED_LEN = 1;
	    
   } else {
      proto = ntohs(ppph->proto);
      DECODED_LEN = sizeof(ppph);
	    
      if (proto != PPP_PROTO_IP && proto != PPP_PROTO_CHAP && 
          proto != PPP_PROTO_PAP && proto != PPP_PROTO_LCP &&
	       proto != PPP_PROTO_ECP && proto != PPP_PROTO_CCP &&
	       proto != PPP_PROTO_IPCP) {   
         proto = *((u_char *)ppph + 2);
         DECODED_LEN = 3;
      }	    
   }

   /* Set the L4 header to LCP for subsequent hooks */
   PACKET->L4.header = (u_char *)(DECODE_DATA + DECODED_LEN);

   /* XXX - Add standard CHAP parsing */

   /* XXX - IPv6 over PPP? */
   if (proto == PPP_PROTO_IP) {
      /* If the payload is an IP packet... */
      /* ...get the next decoder */
      next_decoder =  get_decoder(NET_LAYER, LL_TYPE_IP);
      EXECUTE_DECODER(next_decoder);
      
   } else if (proto == PPP_PROTO_CHAP) { 
      /* Parse MSCHAP auth schemes */
      lcph = (struct ppp_lcp_header *)(DECODE_DATA + DECODED_LEN);
      chapch = (struct ppp_chap_challenge *)(lcph + 1);
      
      switch (lcph->code) {
         case PPP_CHAP_CODE_CHALLENGE:

            if (chapch->size == 8) {
               schallenge[0]=0;
               version = 1;
               for (i=0; i<8; i++) {
                  sprintf(dummy, "%02X", chapch->value.challenge_v1[i]);
                  strcat(schallenge, dummy);
               }	    
            } else if (chapch->size == 16) {
               version = 2;
               memcpy (schallenge, chapch->value.challenge_v2, chapch->size);
            }
               else version = 0;
            break;
	
         case PPP_CHAP_CODE_RESPONSE: 
	    
            if (version != 1 && version !=2) 
               break;			
               
            i = ntohs(lcph->length) - 5 - chapch->size;
            if (i > sizeof(user)-2) 
               i = sizeof(user)-2;
		
            memcpy(user, (u_char *)lcph + 5 + chapch->size, i);
            user[i] = '\0';	
		
            /* Check if it's from PPP or PPTP */
            if (!ip_addr_null(&PACKET->L3.dst) && !ip_addr_null(&PACKET->L3.src)) {
               DISSECT_MSG("\n\nTunnel PPTP: %s -> ", ip_addr_ntoa(&PACKET->L3.src, temp)); 
               DISSECT_MSG("%s\n", ip_addr_ntoa(&PACKET->L3.dst, temp));
            }
		
            DISSECT_MSG("PPP : MS-CHAP Password:   %s:\"\":\"\":", user);
		
            if (version == 1) {        
               for (i = 0; i < 24; i++) 
                  DISSECT_MSG("%02X", chapch->value.response_v1.lanman[i]);
               DISSECT_MSG(":");
			
               for (i = 0; i < 24; i++) 
                  DISSECT_MSG("%02X", chapch->value.response_v1.nt[i]);
               DISSECT_MSG(":%s\n\n",schallenge);
		  	
            } else if (version == 2) {
#ifdef HAVE_OPENSSL 
               u_char *p;

               if ((p = strchr(user, '\\')) == NULL)
                  p = user;
               else 
                  p++;
			
               SHA1_Init(&ctx);
               SHA1_Update(&ctx, chapch->value.response_v2.peer_challenge, 16);
               SHA1_Update(&ctx, schallenge, 16);
               SHA1_Update(&ctx, p, strlen(p));
               SHA1_Final(digest, &ctx);
			
               DISSECT_MSG("000000000000000000000000000000000000000000000000:");
			
               for (i = 0; i < 24; i++) 
                  DISSECT_MSG("%02X",chapch->value.response_v2.nt[i]);
               DISSECT_MSG(":");

               for (i = 0; i < 8; i++) 
                  DISSECT_MSG("%02X", digest[i]);
               DISSECT_MSG("\n\n");			
#endif
            }
            version = 0;
         break;
      }
   } else if (proto == PPP_PROTO_PAP) {

      /* Parse PAP auth scheme */      
      lcph = (struct ppp_lcp_header *)(DECODE_DATA + DECODED_LEN);
      pap_auth = (char *)(lcph + 1);

      if (lcph->code == 1) {

         /* Check if it's from PPP or PPTP */
         if (!ip_addr_null(&PACKET->L3.dst) && !ip_addr_null(&PACKET->L3.src)) {
            DISSECT_MSG("\n\nTunnel PPTP: %s -> ", ip_addr_ntoa(&PACKET->L3.src, temp)); 
            DISSECT_MSG("%s\n", ip_addr_ntoa(&PACKET->L3.dst, temp));
         }
      
         DISSECT_MSG("PPP : PAP User: ");
	
         auth_len = *pap_auth;
         if (auth_len > sizeof(temp)-2) 
            auth_len = sizeof(temp)-2;
         pap_auth++;
         memcpy(temp, pap_auth, auth_len);
         temp[auth_len] = 0;
         DISSECT_MSG("%s\n",temp);
	       
         pap_auth += auth_len;
         auth_len = *pap_auth;
         pap_auth++;
         if (auth_len > sizeof(temp)-2) 
            auth_len = sizeof(temp)-2;
         memcpy(temp, pap_auth, auth_len);
         temp[auth_len] = 0;
         DISSECT_MSG("PPP : PAP Pass: %s\n\n", temp);   
      }
   } else if (proto == PPP_PROTO_LCP) { 
      /* HOOK POINT: HOOK_PACKET_LCP */
      hook_point(HOOK_PACKET_LCP, PACKET);
   } else if (proto == PPP_PROTO_ECP || proto == PPP_PROTO_CCP) { 
      /* HOOK POINT: HOOK_PACKET_ECP */
      hook_point(HOOK_PACKET_ECP, PACKET);
   } else if (proto == PPP_PROTO_IPCP) { 
      /* HOOK POINT: HOOK_PACKET_IPCP */
      hook_point(HOOK_PACKET_IPCP, PACKET);
   }

   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

