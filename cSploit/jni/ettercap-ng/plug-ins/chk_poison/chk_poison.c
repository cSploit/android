/*
    chk_poison -- ettercap plugin -- Check if the poisoning had success

    it sends a spoofed icmp packets to each victim with the address 
    of any other target and listen for "forwardable" replies.

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

#include <pthread.h>
#include <time.h>

struct poison_list {
   struct ip_addr ip[2];
   char poison_success[2];
   SLIST_ENTRY(poison_list) next;
};

/* globals */
SLIST_HEAD(, poison_list) poison_table;

/* mutexes */
static pthread_mutex_t poison_mutex = PTHREAD_MUTEX_INITIALIZER;
#define POISON_LOCK     do{ pthread_mutex_lock(&poison_mutex); } while(0)
#define POISON_UNLOCK   do{ pthread_mutex_unlock(&poison_mutex); } while(0)

/* protos */
int plugin_load(void *);
static int chk_poison_init(void *);
static int chk_poison_fini(void *);
static void parse_icmp(struct packet_object *po);

/* plugin operations */

struct plugin_ops chk_poison_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "chk_poison",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Check if the poisoning had success",  
   /* the plugin version. */ 
   .version =           "1.1",   
   /* activation function */
   .init =              &chk_poison_init,
   /* deactivation function */                     
   .fini =              &chk_poison_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &chk_poison_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int chk_poison_init(void *dummy) 
{
   
   char tmp1[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   struct hosts_list *g1, *g2;
   struct poison_list *p;
   char poison_any, poison_full;
   u_char i;

#if !defined(OS_WINDOWS)
   struct timespec tm;
   tm.tv_sec = GBL_CONF->arp_storm_delay;
   tm.tv_nsec = 0;
#endif
     
   /* don't show packets while operating */
   GBL_OPTIONS->quiet = 1;
      
   if (LIST_EMPTY(&arp_group_one) || LIST_EMPTY(&arp_group_two)) {
      INSTANT_USER_MSG("chk_poison: You have to run this plugin during a poisoning session.\n\n"); 
      return PLUGIN_FINISHED;
   }
   
   /* Create a list with all poisoning targets */
   LIST_FOREACH(g1, &arp_group_one, next) {
      LIST_FOREACH(g2, &arp_group_two, next) {
         /* equal ip must be skipped, you cant poison itself */
         if (!ip_addr_cmp(&g1->ip, &g2->ip))
            continue;

         /* create the element and insert it in the list */
         SAFE_CALLOC(p, 1, sizeof(struct poison_list));

         memcpy(&(p->ip[0]), &g1->ip, sizeof(struct ip_addr));
         memcpy(&(p->ip[1]), &g2->ip, sizeof(struct ip_addr));

         SLIST_INSERT_HEAD(&poison_table, p, next);
      }
   }

   /* Add the hook to collect ICMP replies from the victim */
   hook_add(HOOK_PACKET_ICMP, &parse_icmp);

   INSTANT_USER_MSG("chk_poison: Checking poisoning status...\n");

   /* Send spoofed ICMP echo request to each victim */
   SLIST_FOREACH(p, &poison_table, next) {
      for (i = 0; i <= 1; i++) {
         send_L3_icmp_echo(&(p->ip[i]), &(p->ip[!i]));   
#if !defined(OS_WINDOWS)
         nanosleep(&tm, NULL);
#else
         usleep(GBL_CONF->arp_storm_delay);
#endif
      }
   }
         
   /* wait for the response */
   sleep(1);

   /* remove the hook */
   hook_del(HOOK_PACKET_ICMP, &parse_icmp);

   /* To check if all was poisoned, or no one */
   poison_any = 0;
   poison_full = 1;
   
   /* We'll parse the list twice to avoid too long results printing */
   SLIST_FOREACH(p, &poison_table, next) {
      for (i = 0; i <= 1; i++)
         if (p->poison_success[i])
            poison_any = 1;
         else
            poison_full = 0; 
   }

   /* Order does matter :) */
   if (!poison_any) 
      INSTANT_USER_MSG("chk_poison: No poisoning at all :(\n");   
   else if (poison_full) 
      INSTANT_USER_MSG("chk_poison: Poisoning process successful!\n");
   else 
      SLIST_FOREACH(p, &poison_table, next) {
         for (i=0; i<=1; i++)
            if (!p->poison_success[i])
               INSTANT_USER_MSG("chk_poison: No poisoning between %s -> %s\n", ip_addr_ntoa(&(p->ip[i]), tmp1), ip_addr_ntoa(&(p->ip[!i]), tmp2) );
      }
   
   POISON_LOCK;          
   /* delete the poisoning list */
   while (!SLIST_EMPTY(&poison_table)) {
      p = SLIST_FIRST(&poison_table);
      SLIST_REMOVE_HEAD(&poison_table, next);
      SAFE_FREE(p);
   }
   POISON_UNLOCK;
         
   return PLUGIN_FINISHED;
}


static int chk_poison_fini(void *dummy) 
{
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Check if it's the reply to our bougs request */
static void parse_icmp(struct packet_object *po)
{
   struct poison_list *p;
   
   /* If the packet is not forwardable we haven't received it
    * because of the poisoning 
    */
   if (!(po->flags & PO_FORWARDABLE))
      return; 

   /* Check if it's in the poisoning list. If so this poisoning 
    * is successfull.
    */
    POISON_LOCK;
    SLIST_FOREACH(p, &poison_table, next) {
       if (!ip_addr_cmp(&(po->L3.src), &(p->ip[0])) && !ip_addr_cmp(&(po->L3.dst), &(p->ip[1])))
          p->poison_success[0] = 1;

       if (!ip_addr_cmp(&(po->L3.src), &(p->ip[1])) && !ip_addr_cmp(&(po->L3.dst), &(p->ip[0])))
          p->poison_success[1] = 1;
   }	  
   POISON_UNLOCK;
}


/* EOF */

// vim:ts=3:expandtab
 
