/*
    find_conn -- ettercap plugin -- Search connections on a switched LAN

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

/* protos */
int plugin_load(void *);
static int find_conn_init(void *);
static int find_conn_fini(void *);

static void parse_arp(struct packet_object *po);

/* plugin operations */

struct plugin_ops find_conn_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "find_conn",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Search connections on a switched LAN",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &find_conn_init,
   /* deactivation function */                     
   .fini =              &find_conn_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &find_conn_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int find_conn_init(void *dummy) 
{
   USER_MSG("find_conn: plugin running...\n");
   
   hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);
   return PLUGIN_RUNNING;   
}


static int find_conn_fini(void *dummy) 
{
   USER_MSG("find_conn: plugin terminated...\n");

   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Parse the arp request */
static void parse_arp(struct packet_object *po)
{
   char tmp1[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   
   USER_MSG("find_conn: Probable connection attempt %s -> %s\n", ip_addr_ntoa(&po->L3.src, tmp1), ip_addr_ntoa(&po->L3.dst, tmp2));
}

/* EOF */

// vim:ts=3:expandtab
 
