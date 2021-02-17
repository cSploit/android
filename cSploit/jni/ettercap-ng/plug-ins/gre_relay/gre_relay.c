/*
    gre_relay -- ettercap plugin -- Tunnel broker for redirected GRE tunnels

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
#include <ec_packet.h>
#include <ec_hook.h>
#include <ec_send.h>

struct ip_header {
#ifndef WORDS_BIGENDIAN
   u_int8   ihl:4;
   u_int8   version:4;
#else 
   u_int8   version:4;
   u_int8   ihl:4;
#endif
   u_int8   tos;
   u_int16  tot_len;
   u_int16  id;
   u_int16  frag_off;
#define IP_DF 0x4000
#define IP_MF 0x2000
#define IP_FRAG 0x1fff
   u_int8   ttl;
   u_int8   protocol;
   u_int16  csum;
   u_int32  saddr;
   u_int32  daddr;
/*The options start here. */
};

/* globals */
struct in_addr fake_ip;

/* protos */
int plugin_load(void *);
static int gre_relay_init(void *);
static int gre_relay_fini(void *);

static void parse_gre(struct packet_object *po);
static void parse_arp(struct packet_object *po);

/* plugin operations */
struct plugin_ops gre_relay_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "gre_relay",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Tunnel broker for redirected GRE tunnels",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &gre_relay_init,
   /* deactivation function */                     
   .fini =              &gre_relay_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &gre_relay_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int gre_relay_init(void *dummy) 
{
   char tmp[MAX_ASCII_ADDR_LEN];

   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("gre_relay: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   /* don't display messages while operating */
   GBL_OPTIONS->quiet = 1;

   memset(tmp, 0, sizeof(tmp));
   
   ui_input("Unused IP address: ", tmp, sizeof(tmp), NULL);
   if (!inet_aton(tmp, &fake_ip)) {
      INSTANT_USER_MSG("gre_relay: Bad IP address\n");
      return PLUGIN_FINISHED;
   }

   USER_MSG("gre_relay: plugin running...\n");
   
   hook_add(HOOK_PACKET_GRE, &parse_gre);
   hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);

   return PLUGIN_RUNNING;      
}


static int gre_relay_fini(void *dummy) 
{
   USER_MSG("gre_relay: plugin terminated...\n");

   hook_del(HOOK_PACKET_GRE, &parse_gre);
   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);

   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Send back GRE packets */
static void parse_gre(struct packet_object *po)
{
   struct ip_header *iph;
      
   /* Chek if this is a packet for our fake host */
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 

   if ( (iph = (struct ip_header *)po->L3.header) == NULL)
      return;
      
   if ( iph->daddr != fake_ip.s_addr )
      return;
      
   /* Switch source and dest IP address */
   iph->daddr = iph->saddr;
   iph->saddr = fake_ip.s_addr;
   
   /* Increase ttl */
   iph->ttl = 128;

   po->flags |= PO_MODIFIED;
}


/* Reply to requests for our fake host */
static void parse_arp(struct packet_object *po)
{
   struct ip_addr sa;
   
   ip_addr_init(&sa, AF_INET, (u_char *)&(fake_ip.s_addr));
   if (!ip_addr_cmp(&sa, &po->L3.dst))
      send_arp(ARPOP_REPLY, &sa, GBL_IFACE->mac, &po->L3.src, po->L2.src);
}

/* EOF */

// vim:ts=3:expandtab
 
