/*
    find_ip -- ettercap plugin -- Search an unused IP address in the subnet

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


/* protos */
int plugin_load(void *);
static int find_ip_init(void *);
static int find_ip_fini(void *);
static int in_list(struct ip_addr *scanip);
static struct ip_addr *search_netmask(void);
static struct ip_addr *search_targets(void);

/* plugin operations */

struct plugin_ops find_ip_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "find_ip",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Search an unused IP address in the subnet",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &find_ip_init,
   /* deactivation function */                     
   .fini =              &find_ip_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &find_ip_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int find_ip_init(void *dummy) 
{
   
   char tmp[MAX_ASCII_ADDR_LEN];
   struct ip_addr *e;
   
   /* don't show packets while operating */
   GBL_OPTIONS->quiet = 1;
      
   if (LIST_EMPTY(&GBL_HOSTLIST)) {
      INSTANT_USER_MSG("find_ip: You have to build host-list to run this plugin.\n\n"); 
      return PLUGIN_FINISHED;
   }

   INSTANT_USER_MSG("find_ip: Searching an unused IP address...\n");

   /* If one of the targets is // search in the whole subnet */
   if (GBL_TARGET1->scan_all || GBL_TARGET2->scan_all)
      e = search_netmask();
   else
      e = search_targets();
     
   if (e == NULL) 
      INSTANT_USER_MSG("find_ip: No free IP address in this range :(\n");
   else
      INSTANT_USER_MSG("find_ip: %s seems to be unused\n", ip_addr_ntoa(e, tmp));
         
   return PLUGIN_FINISHED;
}


static int find_ip_fini(void *dummy) 
{
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Check if the IP is in the host list */
static int in_list(struct ip_addr *scanip) 
{
   struct hosts_list *h;

   LIST_FOREACH(h, &GBL_HOSTLIST, next) {
      if (!ip_addr_cmp(scanip, &h->ip)) {
         return 1;
      }
   }
   return 0;
}


/* Find first free IP address in the netmask */
static struct ip_addr *search_netmask(void)
{
   u_int32 netmask, current, myip;
   int nhosts, i;
   static struct ip_addr scanip;

   netmask = ip_addr_to_int32(&GBL_IFACE->netmask.addr);
   myip = ip_addr_to_int32(&GBL_IFACE->ip.addr);
   
   /* the number of hosts in this netmask */
   nhosts = ntohl(~netmask);
  
   /* scan the netmask */
   for (i = 1; i <= nhosts; i++) {
      /* calculate the ip */
      current = (myip & netmask) | htonl(i);
      ip_addr_init(&scanip, AF_INET, (u_char *)&current);
      if (!in_list(&scanip))
         return(&scanip);
   }
   
   return NULL;
}


/* Find first free IP address in the user range */
static struct ip_addr *search_targets(void)
{
   struct ip_list *i;
  
   LIST_FOREACH(i, &GBL_TARGET1->ips, next) {
      if (!in_list(&i->ip))
         return(&i->ip);
   }

   LIST_FOREACH(i, &GBL_TARGET2->ips, next) {
      if (!in_list(&i->ip))
         return(&i->ip);
   }
   
   return NULL;
}

/* EOF */

// vim:ts=3:expandtab
 
