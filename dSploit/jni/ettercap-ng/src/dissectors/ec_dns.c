/*
    ettercap -- dissector DNS -- UDP 53

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

    $Id: ec_dns.c,v 1.8 2003/11/22 13:57:11 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_resolv.h>

/*
 *                                     1  1  1  1  1  1
 *       0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                      ID                       |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    QDCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    ANCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    NSCOUNT                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    ARCOUNT                    |
 *    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */


struct dns_header {
   u_int16 id;                /* DNS packet ID */
#ifdef WORDS_BIGENDIAN
   u_char  qr: 1;             /* response flag */
   u_char  opcode: 4;         /* purpose of message */
   u_char  aa: 1;             /* authoritative answer */
   u_char  tc: 1;             /* truncated message */
   u_char  rd: 1;             /* recursion desired */
   u_char  ra: 1;             /* recursion available */
   u_char  unused: 1;         /* unused bits */
   u_char  ad: 1;             /* authentic data from named */
   u_char  cd: 1;             /* checking disabled by resolver */
   u_char  rcode: 4;          /* response code */
#else /* WORDS_LITTLEENDIAN */
   u_char  rd: 1;             /* recursion desired */
   u_char  tc: 1;             /* truncated message */
   u_char  aa: 1;             /* authoritative answer */
   u_char  opcode: 4;         /* purpose of message */
   u_char  qr: 1;             /* response flag */
   u_char  rcode: 4;          /* response code */
   u_char  cd: 1;             /* checking disabled by resolver */
   u_char  ad: 1;             /* authentic data from named */
   u_char  unused: 1;         /* unused bits */
   u_char  ra: 1;             /* recursion available */
#endif
   u_int16 num_q;             /* Number of questions */
   u_int16 num_answer;        /* Number of answer resource records */
   u_int16 num_auth;          /* Number of authority resource records */
   u_int16 num_res;           /* Number of additional resource records */
};


#define DNS_HEADER_LEN   0xc  /* 12 bytes */

/* protos */

FUNC_DECODER(dissector_dns);
void dns_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init dns_init(void)
{
   dissect_add("dns", APP_LAYER_UDP, 53, dissector_dns);
}


FUNC_DECODER(dissector_dns)
{
   struct dns_header *dns;
   u_char *data, *end;
   char name[NS_MAXDNAME], alias[NS_MAXDNAME];
   int name_len, i;
   u_char *q;
   int16 class, type, a_len;
   int32 ttl;

   DEBUG_MSG("DNS --> UDP 53  dissector_dns");
   
   dns = (struct dns_header *)po->DATA.data;
   data = (u_char *)(dns + 1);
   end = (u_char *)dns + po->DATA.len;
   
   /* initialize the name */
   memset(name, 0, sizeof(name));
   memset(alias, 0, sizeof(alias));
   
   /* extract the name from the packet */
   name_len = dn_expand((u_char *)dns, end, data, name, sizeof(name));
   
   /* thre was an error */
   if (name_len < 0)
      return NULL;

   q = data + name_len;
  
   /* get the type and class */
   NS_GET16(type, q);
   NS_GET16(class, q);

   /* handle only internet class */
   if (class != ns_c_in)
      return NULL;

   /* HOOK POINT: HOOK_PROTO_DNS */
   hook_point(HOOK_PROTO_DNS, PACKET);
   
   /* this is a DNS answer */
   if (dns->qr && dns->rcode == ns_r_noerror && htons(dns->num_answer) > 0) {
  
      for (i = 0; i <= ntohs(dns->num_answer); i++) {
         
         /* 
          * decode the answer 
          * keep the name separated from aliases...
          */
         if (i == 0)
            name_len = dn_expand((u_char *)dns, end, q, name, sizeof(name));
         else 
            name_len = dn_expand((u_char *)dns, end, q, alias, sizeof(alias));
         
         /* thre was an error */
         if (name_len < 0)
            return NULL;

         /* update the pointer */
         q += name_len;

         NS_GET16(type, q);
         NS_GET16(class, q);
         NS_GET32(ttl, q);
         NS_GET16(a_len, q);
         
         /* only internet class */
         if (class != ns_c_in)
            return NULL;
        
         /* alias */
         if (type == ns_t_cname || type == ns_t_ptr) {
            name_len = dn_expand((u_char *)dns, end, q, alias, sizeof(alias));
            q += a_len;
         } 
         
         /* name to ip */
         if (type == ns_t_a) {
            int32 addr;
            struct ip_addr ip;
            char aip[MAX_ASCII_ADDR_LEN];
              
            /* get the address */
            NS_GET32(addr, q);
            /* convert to network order */
            addr = htonl(addr);
            ip_addr_init(&ip, AF_INET, (char *)&addr);
           
            /* insert the answer in the resolv cache */
            resolv_cache_insert(&ip, name);

            /* display the user message */
            ip_addr_ntoa(&ip, aip);
            
            //DISSECT_MSG("DNS: %s ->> %s ->> %s\n", name, alias, aip);
            DEBUG_MSG("DNS: %s ->> %s ->> %s\n", name, alias, aip);
         }
      }
      
   }
      
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

