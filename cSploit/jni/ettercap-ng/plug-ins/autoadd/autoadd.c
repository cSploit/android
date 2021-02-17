/*
    autoadd -- ettercap plugin -- Report suspicious ARP activity

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
#include <ec_mitm.h>

/* protos */
int plugin_load(void *);
static int autoadd_init(void *);
static int autoadd_fini(void *);

static void parse_arp(struct packet_object *po);
static int add_to_victims(void *group, struct packet_object *po);

/* plugin operations */

struct plugin_ops autoadd_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "autoadd",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Automatically add new victims in the target range",  
   /* the plugin version. */ 
   .version =           "1.2",   
   /* activation function */
   .init =              &autoadd_init,
   /* deactivation function */                     
   .fini =              &autoadd_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &autoadd_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int autoadd_init(void *dummy) 
{
   /* 
    * we'll use arp request to detect active hosts.
    * if an host sends arp rq, it want to communicate,
    * so it is alive
    */
   hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);
   return PLUGIN_RUNNING;   
}


static int autoadd_fini(void *dummy) 
{
   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Parse the arp packets */
static void parse_arp(struct packet_object *po)
{
   char tmp[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   struct ip_list *t;

   /* if arp poisonin is not running, do nothing */
   if (!is_mitm_active("arp"))
      return;

   /* don't add our addresses */
   if (!ip_addr_cmp(&GBL_IFACE->ip, &po->L3.src))
      return;
   if (!memcmp(&GBL_IFACE->mac, &po->L2.src, MEDIA_ADDR_LEN))
      return;
   
   /* search in target 1 */
   if (GBL_TARGET1->all_ip) {
      if (add_to_victims(&arp_group_one, po) == ESUCCESS)
         USER_MSG("autoadd: %s %s added to GROUP1\n", ip_addr_ntoa(&po->L3.src, tmp), mac_addr_ntoa(po->L2.src, tmp2));
   } else {
      LIST_FOREACH(t, &GBL_TARGET1->ips, next) 
         if (!ip_addr_cmp(&t->ip, &po->L3.src)) 
            if (add_to_victims(&arp_group_one, po) == ESUCCESS)
               USER_MSG("autoadd: %s %s added to GROUP1\n", ip_addr_ntoa(&po->L3.src, tmp), mac_addr_ntoa(po->L2.src, tmp2));
   }
   
   /* search in target 2 */
   if (GBL_TARGET2->all_ip) {
      if (add_to_victims(&arp_group_two, po) == ESUCCESS)
         USER_MSG("autoadd: %s %s added to GROUP2\n", ip_addr_ntoa(&po->L3.src, tmp), mac_addr_ntoa(po->L2.src, tmp2));
   } else {
      LIST_FOREACH(t, &GBL_TARGET2->ips, next) 
         if (!ip_addr_cmp(&t->ip, &po->L3.src)) 
            if (add_to_victims(&arp_group_two, po) == ESUCCESS)
               USER_MSG("autoadd: %s %s added to GROUP2\n", ip_addr_ntoa(&po->L3.src, tmp), mac_addr_ntoa(po->L2.src, tmp2));
   }
}

/*
 * add a victim to the right group.
 * the arp poisoning thread will automatically pick it up
 * since this function modifies directy the mitm internal lists
 */
static int add_to_victims(void *group, struct packet_object *po)
{
   char tmp[MAX_ASCII_ADDR_LEN];
   struct hosts_list *h;
   LIST_HEAD(, hosts_list) *head = group;

   (void)tmp;
   
   /* search if it was already inserted in the list */
   LIST_FOREACH(h, head, next)
      if (!ip_addr_cmp(&h->ip, &po->L3.src)) 
         return -ENOTHANDLED;
  
   SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
   
   memcpy(&h->ip, &po->L3.src, sizeof(struct ip_addr));
   memcpy(&h->mac, &po->L2.src, MEDIA_ADDR_LEN);
     
   DEBUG_MSG("autoadd: added %s to arp groups", ip_addr_ntoa(&h->ip, tmp));
   LIST_INSERT_HEAD(head, h, next);
   
   /* add the host even in the hosts list */
   LIST_FOREACH(h, &GBL_HOSTLIST, next)
      if (!ip_addr_cmp(&h->ip, &po->L3.src)) 
         return ESUCCESS;
   
   /* 
    * we need another copy, since the group lists 
    * are freed by the mitm process
    */
   SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
   
   memcpy(&h->ip, &po->L3.src, sizeof(struct ip_addr));
   memcpy(&h->mac, &po->L2.src, MEDIA_ADDR_LEN);
   
   DEBUG_MSG("autoadd: added %s to hosts list", ip_addr_ntoa(&h->ip, tmp));
   LIST_INSERT_HEAD(&GBL_HOSTLIST, h, next);
   
   return ESUCCESS;
}

/* EOF */

// vim:ts=3:expandtab
 
