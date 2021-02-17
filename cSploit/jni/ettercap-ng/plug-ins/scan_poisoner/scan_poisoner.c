/*
    scan_poisoner -- ettercap plugin -- Actively search other poisoners

    It checks the hosts list, searching for eqaul mac addresses. 
    It also sends icmp packets to see if any ip-mac association has
    changed.
    
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
#include <time.h>

/* globals */
char flag_strange;

/* protos */
int plugin_load(void *);
static int scan_poisoner_init(void *);
static int scan_poisoner_fini(void *);
static void parse_icmp(struct packet_object *po);

/* plugin operations */

struct plugin_ops scan_poisoner_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "scan_poisoner",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Actively search other poisoners",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &scan_poisoner_init,
   /* deactivation function */                     
   .fini =              &scan_poisoner_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &scan_poisoner_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int scan_poisoner_init(void *dummy) 
{
   
   char tmp1[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   struct hosts_list *h1, *h2;
   
#if !defined(OS_WINDOWS)  
   struct timespec tm;
   tm.tv_sec = GBL_CONF->arp_storm_delay;
   tm.tv_nsec = 0; 
#endif

   /* don't show packets while operating */
   GBL_OPTIONS->quiet = 1;
      
   if (LIST_EMPTY(&GBL_HOSTLIST)) {
      INSTANT_USER_MSG("scan_poisoner: You have to build host-list to run this plugin.\n\n"); 
      return PLUGIN_FINISHED;
   }

   INSTANT_USER_MSG("scan_poisoner: Checking hosts list...\n");
   flag_strange = 0;

   /* Compares mac address of each host with any other */   
   LIST_FOREACH(h1, &GBL_HOSTLIST, next) 
      for(h2=LIST_NEXT(h1,next); h2!=LIST_END(&GBL_HOSTLIST); h2=LIST_NEXT(h2,next)) 
         if (!memcmp(h1->mac, h2->mac, MEDIA_ADDR_LEN)) {
            flag_strange = 1;
            INSTANT_USER_MSG("scan_poisoner: - %s and %s have same MAC address\n", ip_addr_ntoa(&h1->ip, tmp1), ip_addr_ntoa(&h2->ip, tmp2));
         }

   if (!flag_strange)
      INSTANT_USER_MSG("scan_poisoner: - Nothing strange\n");
   flag_strange=0;

   /* Can't continue in unoffensive */
   if (GBL_OPTIONS->unoffensive || GBL_OPTIONS->read) {
      INSTANT_USER_MSG("\nscan_poisoner: Can't make active test in UNOFFENSIVE mode.\n\n");
      return PLUGIN_FINISHED;
   }

   INSTANT_USER_MSG("\nscan_poisoner: Actively searching poisoners...\n");
   
   /* Add the hook to collect ICMP replies from the victim */
   hook_add(HOOK_PACKET_ICMP, &parse_icmp);

   /* Send ICMP echo request to each target */
   LIST_FOREACH(h1, &GBL_HOSTLIST, next) {
      send_L3_icmp_echo(&GBL_IFACE->ip, &h1->ip);   
#if !defined(OS_WINDOWS)
      nanosleep(&tm, NULL);
#else
      usleep(GBL_CONF->arp_storm_delay);
#endif
   }
         
   /* wait for the response */

#if !defined(OS_WINDOWS)
   sleep(1);
#else
   usleep(1000000);
#endif

   /* remove the hook */
   hook_del(HOOK_PACKET_ICMP, &parse_icmp);

   /* We don't need mutex on it :) */
   if (!flag_strange)
      INSTANT_USER_MSG("scan_poisoner: - Nothing strange\n");
     
   return PLUGIN_FINISHED;
}


static int scan_poisoner_fini(void *dummy) 
{
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Check icmp replies */
static void parse_icmp(struct packet_object *po)
{
   struct hosts_list *h1, *h2;
   char poisoner[MAX_ASCII_ADDR_LEN];
   char tmp[MAX_ASCII_ADDR_LEN];

   /* If the poisoner is not in the hosts list */
   sprintf(poisoner, "UNKNOWN");
   
   /* Check if the reply contains the correct ip/mac association */
   LIST_FOREACH(h1, &GBL_HOSTLIST, next) {
      if (!ip_addr_cmp(&(po->L3.src), &h1->ip) && memcmp(po->L2.src, h1->mac, MEDIA_ADDR_LEN)) {
         flag_strange = 1;
         /* Check if the mac address of the poisoner is in the hosts list */
         LIST_FOREACH(h2, &GBL_HOSTLIST, next) 
            if (!memcmp(po->L2.src, h2->mac, MEDIA_ADDR_LEN))
               ip_addr_ntoa(&h2->ip, poisoner);
		
         INSTANT_USER_MSG("scan_poisoner: - %s is replying for %s\n", poisoner, ip_addr_ntoa(&h1->ip, tmp)); 
      }
   }	  
}


/* EOF */

// vim:ts=3:expandtab
 
