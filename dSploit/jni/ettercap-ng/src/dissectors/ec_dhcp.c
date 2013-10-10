/*
    ettercap -- dissector DHCP -- UDP 67

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

    $Id: ec_dhcp.c,v 1.11 2004/05/26 09:13:48 alor Exp $
*/

/*
 * RFC: 2131
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
 *    +---------------+---------------+---------------+---------------+
 *    |                            xid (4)                            |
 *    +-------------------------------+-------------------------------+
 
 *    |           secs (2)            |           flags (2)           |
 *    +-------------------------------+-------------------------------+
 *    |                          ciaddr  (4)                          |
 *    +---------------------------------------------------------------+
 *    |                          yiaddr  (4)                          |
 *    +---------------------------------------------------------------+
 *    |                          siaddr  (4)                          |
 *    +---------------------------------------------------------------+
 *    |                          giaddr  (4)                          |
 *    +---------------------------------------------------------------+
 *    |                          chaddr  (16)                         |
 *    +---------------------------------------------------------------+
 *    |                          sname   (64)                         |
 *    +---------------------------------------------------------------+
 *    |                          file    (128)                        |
 *    +---------------------------------------------------------------+
 *    |                       options  (variable)                     |
 *    +---------------------------------------------------------------+
 */


#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_proto.h>

/* globalse */

struct dhcp_hdr {
   u_int8   op;
      #define BOOTREQUEST  1
      #define BOOTREPLY    2
   u_int8   htype;
   u_int8   hlen;
   u_int8   hops;
   u_int32  id;
   u_int16  secs;
   u_int16  flags;
   u_int32  ciaddr;
   u_int32  yiaddr;
   u_int32  siaddr;
   u_int32  giaddr;
   u_int8   chaddr[16];
   u_int8   sname[64];
   u_int8   file[128];
   u_int32  magic;
};

/* protos */

FUNC_DECODER(dissector_dhcp);
void dhcp_init(void);
u_int8 * get_dhcp_option(u_int8 opt, u_int8 *ptr, u_int8 *end);
void put_dhcp_option(u_int8 opt, u_int8 *value, u_int8 len, u_int8 **ptr);
static void dhcp_add_profile(struct ip_addr *sa, size_t flag);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init dhcp_init(void)
{
   dissect_add("dhcp", APP_LAYER_UDP, 67, dissector_dhcp);
}


FUNC_DECODER(dissector_dhcp)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   struct dhcp_hdr *dhcp;
   u_int8 *options, *opt;

   /* sanity check */
   if (PACKET->DATA.len < sizeof(struct dhcp_hdr))
      return NULL;
         
   DEBUG_MSG("DHCP --> UDP 68  dissector_dhcp");

   /* cast the header and options */
   dhcp = (struct dhcp_hdr *)ptr;
   options = (u_int8 *)(dhcp + 1);

   /* check for the magic cookie */
   if (dhcp->magic != htonl(DHCP_MAGIC_COOKIE))
      return NULL;

   end = ptr + PACKET->DATA.len;
   
   /* search the "message type" option */
   opt = get_dhcp_option(DHCP_OPT_MSG_TYPE, options, end);

   /* option not found */
   if (opt == NULL)
      return NULL;
      
   /* client requests */ 
   if (FROM_CLIENT("dhcp", PACKET)) {
      struct ip_addr client;
      
      /* clients only send request */
      if (dhcp->op != BOOTREQUEST)
         return NULL;
      
      switch (*(opt + 1)) {
         case DHCP_DISCOVER:
            DEBUG_MSG("\tDissector_DHCP DISCOVER");
            
            DISSECT_MSG("DHCP: [%s] DISCOVER \n", mac_addr_ntoa(dhcp->chaddr, tmp)); 
      
            /* HOOK POINT: HOOK_PROTO_DHCP_DISCOVER */
            hook_point(HOOK_PROTO_DHCP_DISCOVER, PACKET);
            
            break;
            
         case DHCP_REQUEST:
            DEBUG_MSG("\tDissector_DHCP REQUEST");
      
            /* requested ip address */
            if ((opt = get_dhcp_option(DHCP_OPT_RQ_ADDR, options, end)) != NULL)
               ip_addr_init(&client, AF_INET, opt + 1);
            else {
               /* search if the client already has the ip address */
               if (dhcp->ciaddr != 0) {
                  ip_addr_init(&client, AF_INET, (char *)&dhcp->ciaddr);
               } else
                  return NULL;
            }

            DISSECT_MSG("DHCP: [%s] REQUEST ", mac_addr_ntoa(dhcp->chaddr, tmp)); 
            DISSECT_MSG("%s\n", ip_addr_ntoa(&client, tmp)); 
      
            /* HOOK POINT: HOOK_PROTO_DHCP_REQUEST */
            hook_point(HOOK_PROTO_DHCP_REQUEST, PACKET);
      
            break;
      }

   /* server replies */ 
   } else {
      struct ip_addr netmask;
      struct ip_addr router;
      struct ip_addr client;
      struct ip_addr dns;
      char domain[64];
      char resp;
      
      /* servers only send replies */
      if (dhcp->op != BOOTREPLY)
         return NULL;

      memset(domain, 0, sizeof(domain));
      memset(&netmask, 0, sizeof(struct ip_addr));
      memset(&router, 0, sizeof(struct ip_addr));
      memset(&client, 0, sizeof(struct ip_addr));
      memset(&dns, 0, sizeof(struct ip_addr));

      resp = *(opt + 1);
      
      switch (resp) {
         case DHCP_ACK:
         case DHCP_OFFER:

            if (resp == DHCP_ACK)
               DEBUG_MSG("\tDissector_DHCP ACK");
            else
               DEBUG_MSG("\tDissector_DHCP OFFER");
   
            /* get the assigned ip */
            ip_addr_init(&client, AF_INET, (char *)&dhcp->yiaddr );
            
            /* netmask */
            if ((opt = get_dhcp_option(DHCP_OPT_NETMASK, options, end)) != NULL)
               ip_addr_init(&netmask, AF_INET, opt + 1);
            
            /* default gateway */
            if ((opt = get_dhcp_option(DHCP_OPT_ROUTER, options, end)) != NULL)
               ip_addr_init(&router, AF_INET, opt + 1);
            
            /* dns server */
            if ((opt = get_dhcp_option(DHCP_OPT_DNS, options, end)) != NULL)
               ip_addr_init(&dns, AF_INET, opt + 1);
            
            DISSECT_MSG("DHCP: [%s] %s : ", ip_addr_ntoa(&PACKET->L3.src, tmp), (resp == DHCP_ACK) ? "ACK" : "OFFER"); 
            DISSECT_MSG("%s ", ip_addr_ntoa(&client, tmp)); 
            DISSECT_MSG("%s ", ip_addr_ntoa(&netmask, tmp)); 
            DISSECT_MSG("GW %s ", ip_addr_ntoa(&router, tmp)); 
            if (!ip_addr_is_zero(&dns)) {
               DISSECT_MSG("DNS %s ", ip_addr_ntoa(&dns, tmp)); 
            }

            /* dns domain */
            if ((opt = get_dhcp_option(DHCP_OPT_DOMAIN, options, end)) != NULL) {
                  strncpy(domain, opt + 1, MIN(*opt, sizeof(domain)) );
            
               DISSECT_MSG("\"%s\"\n", domain);
            } else
               DISSECT_MSG("\n");
            
            /* add the GW and the DNS to hosts' profiles */
            if (!ip_addr_is_zero(&router))
               dhcp_add_profile(&router, FP_GATEWAY | FP_HOST_LOCAL);
            
            if (!ip_addr_is_zero(&dns))
               dhcp_add_profile(&dns, FP_UNKNOWN);
            
            break;
      }
   }
      
   return NULL;
}


/*
 * return the pointer to the named option
 * or NULL if not found
 * ptr will point to the length of the option
 */
u_int8 * get_dhcp_option(u_int8 opt, u_int8 *ptr, u_int8 *end)
{
   do {

      /* we have found our option */
      if (*ptr == opt)
         return ptr + 1;

      /* 
       * move thru options :
       *
       * OPT LEN .. .. .. OPT LEN .. ..
       */
      ptr = ptr + 2 + (*(ptr + 1));

   } while (*ptr != DHCP_OPT_END && ptr < end);
   
   return NULL;
}

/*
 * put an option into the buffer, the ptr will be 
 * move after the options just inserted.
 */
void put_dhcp_option(u_int8 opt, u_int8 *value, u_int8 len, u_int8 **ptr)
{
   u_int8 *p = *ptr;
   
   /* the options type */
   *p++ = opt;
   /* the len of the option */
   *p++ = len;
   /* copy the value */
   memcpy(p, value, len);
   /* move the pointer */
   p += len;

   *ptr = p;
}


/*
 * create a fake packet object to feed the profile_parse function
 */
static void dhcp_add_profile(struct ip_addr *sa, size_t flag)
{
   struct packet_object po;

   DEBUG_MSG("dhcp_add_profile");
   
   /* wipe the object */
   memset(&po, 0, sizeof(struct packet_object));
   
   memcpy(&po.L3.src, sa, sizeof(struct ip_addr));

   /* this is a ludicrious(tm) lie !
    * in order to add the host to the profiles we pretend
    * to have seen an icmp from it 
    */
   po.L4.proto = NL_TYPE_ICMP;
   po.PASSIVE.flags = flag;

   /* HOOK POINT: HOOK_PROTO_DHCP_PROFILE */
   /* used by profile_parse and log_packet */
   hook_point(HOOK_PROTO_DHCP_PROFILE, &po);
}


/* EOF */

// vim:ts=3:expandtab

