/*
    arp_cop -- ettercap plugin -- Report suspicious ARP activity

    Copyright (C) ALoR & NaGA
    
    Copyright (C) 2001 for the original plugin :  Paulo Madeira <acelent@hotmail.com>
    
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

/* protos */
int plugin_load(void *);
static int arp_cop_init(void *);
static int arp_cop_fini(void *);

static void parse_arp(struct packet_object *po);
static void arp_init_list(void);

/* globals */
LIST_HEAD(, hosts_list) arp_cop_table;

/* plugin operations */

struct plugin_ops arp_cop_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "arp_cop",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Report suspicious ARP activity",  
   /* the plugin version. */ 
   .version =           "1.1",   
   /* activation function */
   .init =              &arp_cop_init,
   /* deactivation function */                     
   .fini =              &arp_cop_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &arp_cop_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int arp_cop_init(void *dummy) 
{
   USER_MSG("arp_cop: plugin running...\n");

   arp_init_list();
   
   hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);
   hook_add(HOOK_PACKET_ARP_RP, &parse_arp);
   return PLUGIN_RUNNING;   
}


static int arp_cop_fini(void *dummy) 
{
   USER_MSG("arp_cop: plugin terminated...\n");

   /* We don't free the global list for further reuse */
   
   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);
   hook_del(HOOK_PACKET_ARP_RP, &parse_arp);
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Parse the arp packets */
static void parse_arp(struct packet_object *po)
{
   char tmp1[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   char str1[ETH_ASCII_ADDR_LEN];
   char str2[ETH_ASCII_ADDR_LEN];
   char found = 0;
   struct hosts_list *h1, *h2;

   LIST_FOREACH(h1, &arp_cop_table, next) {
      /* The IP address is already in the list */
      if(!ip_addr_cmp(&po->L3.src, &h1->ip)) {
      
         /* This is its normal MAC address */
         if (!memcmp(po->L2.src, h1->mac, MEDIA_ADDR_LEN))
            return;

         /* Someone is spoofing, check if we already know its mac address */    
         LIST_FOREACH(h2, &arp_cop_table, next) {
            if (!memcmp(po->L2.src, h2->mac, MEDIA_ADDR_LEN)) {
               /* don't report my own poisoning */
               if (ip_addr_cmp(&h2->ip, &GBL_IFACE->ip))
                  USER_MSG("arp_cop: (WARNING) %s[%s] pretends to be %s[%s]\n", ip_addr_ntoa(&h2->ip, tmp1), 
                                                                             mac_addr_ntoa(h2->mac, str1), 
                                                                             ip_addr_ntoa(&h1->ip, tmp2),
                                                                             mac_addr_ntoa(h1->mac, str2));
               return;
            }
         }
	 
         /* A new NIC claims an existing IP address */
         USER_MSG("arp_cop: (IP-conflict) [%s] wants to be %s[%s]\n", mac_addr_ntoa(po->L2.src, str1), 
                                                                      ip_addr_ntoa(&h1->ip, tmp1),
                                                                      mac_addr_ntoa(h1->mac, str2));      
         return;
      } 
   }
      
   /* The IP address is not yet in the list */
   LIST_FOREACH(h1, &arp_cop_table, next) {
      if (!memcmp(po->L2.src, h1->mac, MEDIA_ADDR_LEN)) {
         USER_MSG("arp_cop: (IP-change) [%s]  %s -> %s\n", mac_addr_ntoa(h1->mac, str1), 
                                                           ip_addr_ntoa(&h1->ip, tmp1), 
                                                           ip_addr_ntoa(&po->L3.src, tmp2));
         found = 1;
      }
   }
   
   if (!found)
      USER_MSG("arp_cop: (new host) %s[%s]\n", ip_addr_ntoa(&po->L3.src, tmp1), mac_addr_ntoa(po->L2.src, str1));   

   /* Insert the host in th list */
   SAFE_CALLOC(h1, 1, sizeof(struct hosts_list));
   memcpy(&h1->ip, &po->L3.src, sizeof(struct ip_addr));
   memcpy(h1->mac, po->L2.src, MEDIA_ADDR_LEN);
   LIST_INSERT_HEAD(&arp_cop_table, h1, next);
}

static void arp_init_list()
{
   struct hosts_list *h1, *h2;

   /* If we have already run it once */
   if(!LIST_EMPTY(&arp_cop_table))
      return;
      
   /* Fill the arp_cop_table with the initial host list */
   LIST_FOREACH(h1, &GBL_HOSTLIST, next) {
      SAFE_CALLOC(h2, 1, sizeof(struct hosts_list));
      memcpy(&h2->ip, &h1->ip, sizeof(struct ip_addr));
      memcpy(h2->mac, h1->mac, MEDIA_ADDR_LEN);
      LIST_INSERT_HEAD(&arp_cop_table, h2, next);
   }
   
   /* Add our IP address */
   SAFE_CALLOC(h2, 1, sizeof(struct hosts_list));
   memcpy(&h2->ip, &GBL_IFACE->ip, sizeof(struct ip_addr));
   memcpy(h2->mac, GBL_IFACE->mac, MEDIA_ADDR_LEN);
   LIST_INSERT_HEAD(&arp_cop_table, h2, next);
}


/* EOF */

// vim:ts=3:expandtab
 
