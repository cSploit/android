/*
    search_promisc -- ettercap plugin -- Search promisc NICs in the LAN

    It sends malformed arp reqeusts and waits for replies.

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

#include <pthread.h>
#include <time.h>

/* globals */
LIST_HEAD(, hosts_list) promisc_table;
LIST_HEAD(, hosts_list) collected_table;

/* mutexes */
static pthread_mutex_t promisc_mutex = PTHREAD_MUTEX_INITIALIZER;
#define PROMISC_LOCK     do{ pthread_mutex_lock(&promisc_mutex); } while(0)
#define PROMISC_UNLOCK   do{ pthread_mutex_unlock(&promisc_mutex); } while(0)

/* protos */
int plugin_load(void *);
static int search_promisc_init(void *);
static int search_promisc_fini(void *);
static void parse_arp(struct packet_object *po);

/* plugin operations */

struct plugin_ops search_promisc_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "search_promisc",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Search promisc NICs in the LAN",  
   /* the plugin version. */ 
   .version =           "1.2",   
   /* activation function */
   .init =              &search_promisc_init,
   /* deactivation function */                     
   .fini =              &search_promisc_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &search_promisc_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int search_promisc_init(void *dummy) 
{
   
   char tmp[MAX_ASCII_ADDR_LEN];
   struct hosts_list *h;
   u_char bogus_mac[2][6]={"\xfd\xfd\x00\x00\x00\x00", "\xff\xff\x00\x00\x00\x00"};
   char messages[2][48]={"\nLess probably sniffing NICs:\n", "\nMost probably sniffing NICs:\n"};
   u_char i;
 
#if !defined(OS_WINDOWS) 
   struct timespec tm;
   tm.tv_sec = GBL_CONF->arp_storm_delay;
   tm.tv_nsec = 0; 
#endif
   /* don't show packets while operating */
   GBL_OPTIONS->quiet = 1;
      
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("search_promisc: plugin doesn't work in UNOFFENSIVE mode.\n\n");
      return PLUGIN_FINISHED;
   }

   if (LIST_EMPTY(&GBL_HOSTLIST)) {
      INSTANT_USER_MSG("search_promisc: You have to build host-list to run this plugin.\n\n"); 
      return PLUGIN_FINISHED;
   }

   INSTANT_USER_MSG("search_promisc: Searching promisc NICs...\n");
   
   /* We have to perform same operations twice :) */   
   for (i=0; i<=1; i++) {
      /* Add the hook to collect ARP replies from the targets */
      hook_add(HOOK_PACKET_ARP_RP, &parse_arp);

      /* Send malformed ARP requests to each target. 
       * First and second time we'll use different 
       * dest mac addresses
       */
      LIST_FOREACH(h, &GBL_HOSTLIST, next) {
         send_arp(ARPOP_REQUEST, &GBL_IFACE->ip, GBL_IFACE->mac, &h->ip, bogus_mac[i]);   
#if !defined(OS_WINDOWS)
         nanosleep(&tm, NULL);
#else
         usleep(GBL_CONF->arp_storm_delay*1000);
#endif
      }
      
      /* Wait for responses */
      sleep(1);
      
      /* Remove the hook */
      hook_del(HOOK_PACKET_ARP_RP, &parse_arp);

      /* Print results */
      INSTANT_USER_MSG(messages[i]);
      if(LIST_EMPTY(&promisc_table))
         INSTANT_USER_MSG("- NONE \n");
      else 
         LIST_FOREACH(h, &promisc_table, next) 
            INSTANT_USER_MSG("- %s\n",ip_addr_ntoa(&h->ip, tmp));
         

      PROMISC_LOCK;          
      /* Delete the list */
      while (!LIST_EMPTY(&promisc_table)) {
         h = LIST_FIRST(&promisc_table);
         LIST_REMOVE(h, next);
         SAFE_FREE(h);
      }
      PROMISC_UNLOCK;
   }

   PROMISC_LOCK;          
   /* Delete the list */
   while (!LIST_EMPTY(&collected_table)) {
      h = LIST_FIRST(&collected_table);
      LIST_REMOVE(h, next);
      SAFE_FREE(h);
   }
   PROMISC_UNLOCK;
     
   return PLUGIN_FINISHED;
}


static int search_promisc_fini(void *dummy) 
{
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Parse the reply to our bougs requests */
static void parse_arp(struct packet_object *po)
{
   struct hosts_list *h;

   /* We'll parse only replies for us */
   if (memcmp(po->L2.dst, GBL_IFACE->mac, MEDIA_ADDR_LEN))
      return;
   
   PROMISC_LOCK;
   /* Check if it's already in the list */
   LIST_FOREACH(h, &collected_table, next) 
      if (!ip_addr_cmp(&(po->L3.src), &h->ip)) {
         PROMISC_UNLOCK;
         return;
      }
       
   /* create the element and insert it in the two lists */
   SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
   memcpy(&h->ip, &(po->L3.src), sizeof(struct ip_addr));
   LIST_INSERT_HEAD(&promisc_table, h, next);

   SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
   memcpy(&h->ip, &(po->L3.src), sizeof(struct ip_addr));
   LIST_INSERT_HEAD(&collected_table, h, next);

   PROMISC_UNLOCK;
}


/* EOF */

// vim:ts=3:expandtab
 
