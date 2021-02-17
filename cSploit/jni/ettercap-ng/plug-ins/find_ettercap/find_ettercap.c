/*
    find_ettercap -- ettercap plugin -- Try to discover ettercap activity on the lan

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


#include <ec.h>                        /* required for global variables */
#include <ec_plugins.h>                /* required for plugin ops */
#include <ec_file.h>
#include <ec_hook.h>

#include <stdlib.h>
#include <string.h>

#include <libnet.h>

/* protos */

int plugin_load(void *);
static int find_ettercap_init(void *);
static int find_ettercap_fini(void *);
static void parse_ip(struct packet_object *po);
static void parse_icmp(struct packet_object *po);
static void parse_tcp(struct packet_object *po);

/* plugin operations */

struct plugin_ops find_ettercap_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "find_ettercap",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Try to find ettercap activity",  
   /* the plugin version. */ 
   .version =           "2.0",   
   /* activation function */
   .init =              &find_ettercap_init,
   /* deactivation function */                     
   .fini =              &find_ettercap_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &find_ettercap_ops);
}

/*********************************************************/

static int find_ettercap_init(void *dummy) 
{
   /* add the hook in the dissector.  */
   hook_add(HOOK_PACKET_IP, &parse_ip);
   hook_add(HOOK_PACKET_ICMP, &parse_icmp);
   hook_add(HOOK_PACKET_TCP, &parse_tcp);
   
   return PLUGIN_RUNNING;
}


static int find_ettercap_fini(void *dummy) 
{
   /* remove the hook */
   hook_del(HOOK_PACKET_IP, &parse_ip);
   hook_del(HOOK_PACKET_ICMP, &parse_icmp);
   hook_del(HOOK_PACKET_TCP, &parse_tcp);

   return PLUGIN_FINISHED;
}

/* 
 * parse the packet for ettercap traces
 */
static void parse_ip(struct packet_object *po)
{
   struct libnet_ipv4_hdr *ip;
   char tmp[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];

   ip = (struct libnet_ipv4_hdr *)po->L3.header;

   if (ntohs(ip->ip_id) == EC_MAGIC_16)
      USER_MSG("ettercap traces (ip) from %s...\n", ip_addr_ntoa(&po->L3.src, tmp));
   
   if (ntohs(ip->ip_id) == 0xbadc)
      USER_MSG("ettercap plugin (banshee) is killing from %s to %s...\n", ip_addr_ntoa(&po->L3.src, tmp), ip_addr_ntoa(&po->L3.dst, tmp2));
      
}

/* 
 * parse the packet for ettercap traces
 */
static void parse_icmp(struct packet_object *po)
{
   struct libnet_icmpv4_hdr *icmp;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   icmp = (struct libnet_icmpv4_hdr *)po->L4.header;

   if (ntohs(icmp->hun.echo.id) == EC_MAGIC_16 && ntohs(icmp->hun.echo.seq) == EC_MAGIC_16)
      USER_MSG("ettercap traces (icmp) from %s...\n", ip_addr_ntoa(&po->L3.src, tmp));

}

/* 
 * parse the packet for ettercap traces
 */
static void parse_tcp(struct packet_object *po)
{
   struct libnet_tcp_hdr *tcp;
   char tmp[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];

   tcp = (struct libnet_tcp_hdr *)po->L4.header;
  
   switch (ntohl(tcp->th_seq)) {
      case EC_MAGIC_16:
         USER_MSG("ettercap traces (tcp) from %s...\n", ip_addr_ntoa(&po->L3.src, tmp));
         break;
      case 6969:
         USER_MSG("ettercap plugin (shadow) is scanning from %s to %s:%d...\n", ip_addr_ntoa(&po->L3.src, tmp), ip_addr_ntoa(&po->L3.dst, tmp2), ntohs(po->L4.dst));
         break;
      case 0xabadc0de:
         if (ntohl(tcp->th_ack) == 0xabadc0de)
            USER_MSG("ettercap plugin (spectre) is flooding the lan.\n");
         else
            USER_MSG("ettercap plugin (golem) is DOSing from %s to %s...\n", ip_addr_ntoa(&po->L3.src, tmp), ip_addr_ntoa(&po->L3.dst, tmp2));
         break;
   }

   if (ntohs(tcp->th_sport) == EC_MAGIC_16 && (tcp->th_flags & TH_SYN) )
      USER_MSG("ettercap NG plugin (gw_discover) is trying to discover the gateway from %s...\n", ip_addr_ntoa(&po->L3.src, tmp));
   
}

/* EOF */

// vim:ts=3:expandtab

