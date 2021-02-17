/*
    reply_arp -- ettercap plugin -- Simple arp responder

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


/* protos */
int plugin_load(void *);
static int reply_arp_init(void *);
static int reply_arp_fini(void *);
static void parse_arp(struct packet_object *po);

/* plugin operations */
struct plugin_ops reply_arp_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "reply_arp",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Simple arp responder",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &reply_arp_init,
   /* deactivation function */                     
   .fini =              &reply_arp_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &reply_arp_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int reply_arp_init(void *dummy) 
{
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("reply_arp: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);

   return PLUGIN_RUNNING;      
}


static int reply_arp_fini(void *dummy) 
{
   USER_MSG("reply_arp: plugin terminated...\n");

   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);

   return PLUGIN_FINISHED;
}

/*********************************************************/


/* Reply to requests for hosts in the target lists */
static void parse_arp(struct packet_object *po)
{
   struct ip_list *i;
   int in_list = 0;

   if (GBL_TARGET1->scan_all || GBL_TARGET2->scan_all)
      in_list = 1;
      
   LIST_FOREACH(i, &GBL_TARGET1->ips, next) {
      if (!ip_addr_cmp(&i->ip, &po->L3.dst)) {
         in_list = 1;
         break;
      }
   }

   LIST_FOREACH(i, &GBL_TARGET2->ips, next) {
      if (!ip_addr_cmp(&i->ip, &po->L3.dst)) {
         in_list = 1;
         break;
      }
   }
   
   if (in_list)
      send_arp(ARPOP_REPLY, &po->L3.dst, GBL_IFACE->mac, &po->L3.src, po->L2.src);
}

/* EOF */

// vim:ts=3:expandtab
 
