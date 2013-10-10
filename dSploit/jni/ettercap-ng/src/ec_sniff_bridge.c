/*
    ettercap -- bridged sniffing method module

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

    $Id: ec_sniff_bridge.c,v 1.18 2004/11/04 10:29:06 alor Exp $
*/

#include <ec.h>
#include <ec_capture.h>
#include <ec_send.h>
#include <ec_threads.h>
#include <ec_conntrack.h>

struct origin_mac_table {
   u_int8 mac[MEDIA_ADDR_LEN];
   LIST_ENTRY(origin_mac_table) next;
};

static LIST_HEAD(, origin_mac_table) iface_origin_table;
static LIST_HEAD(, origin_mac_table) bridge_origin_table;

/* proto */
void start_bridge_sniff(void);
void stop_bridge_sniff(void);
void forward_bridge_sniff(struct packet_object *po);
void bridge_check_forwarded(struct packet_object *po);
void bridge_set_forwardable(struct packet_object *po);

/*******************************************/

void start_bridge_sniff(void)
{
   DEBUG_MSG("start_bridge_sniff");
   
   if (GBL_SNIFF->active == 1) {
      USER_MSG("Bridged sniffing already started...\n");
      return;
   }
   
   USER_MSG("Starting Bridged sniffing...\n\n");
   
   /* create the timeouter thread */
   if (!GBL_OPTIONS->read) { 
      pthread_t pid;
      
      pid = ec_thread_getpid("timer");
      if (pthread_equal(pid, EC_PTHREAD_NULL))
         ec_thread_new("timer", "conntrack timeouter", &conntrack_timeouter, NULL);
   }

   /* create the thread for packet capture */
   ec_thread_new("capture", "pcap handler and packet decoder", &capture, GBL_OPTIONS->iface);
   
   /* create the thread for packet capture on the bridged interface */
   ec_thread_new("bridge", "pcap handler and packet decoder", &capture_bridge, GBL_OPTIONS->iface_bridge);

   GBL_SNIFF->active = 1;
}

/*
 * kill the capturing threads, but leave untouched the others
 */
void stop_bridge_sniff(void)
{
   pthread_t pid;
   
   DEBUG_MSG("stop_bridge_sniff");
   
   if (GBL_SNIFF->active == 0) {
      USER_MSG("Bridged sniffing is not running...\n");
      return;
   }
  
   /* get the pid and kill it */
   pid = ec_thread_getpid("capture");
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);
   
   pid = ec_thread_getpid("bridge");
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);

   USER_MSG("Bridged sniffing was stopped.\n");

   GBL_SNIFF->active = 0;
}


void forward_bridge_sniff(struct packet_object *po)
{
   /* don't forward dropped packets */
   if (po->flags & PO_DROPPED)
      return;

   /*
    * If the filters modified the packet len
    * recalculate it (only if some L3 decoder parsed it).
    */
   if (po->fwd_packet)
      po->len = po->L2.len + po->fwd_len;
         
   /* 
    * send the packet to the other interface.
    * the socket was opened during the initialization
    * phase (parameters parsing) by bridge_init()
    */
   if (po->flags & PO_FROMIFACE)
      send_to_bridge(po);
   else if (po->flags & PO_FROMBRIDGE)
      send_to_L2(po);
   
}

/*
 * keep a list of source mac addresses for each interface.
 * each list will contain mac address coming form an host connected
 * on the iface.
 * we can determine if a packet is forwarded or not searching it in
 * the lists.
 */
void bridge_check_forwarded(struct packet_object *po)
{
   struct origin_mac_table *omt;
   u_char tmp[MAX_ASCII_ADDR_LEN];

   /* avoid gcc complaining for unused var */
   (void)tmp;
   
   if (po->flags & PO_FROMIFACE) {
      /* search the mac in the iface table */
      LIST_FOREACH(omt, &iface_origin_table, next)
         if (!memcmp(omt->mac, po->L2.src, MEDIA_ADDR_LEN))
            return;

      /* 
       * now search it in the opposite table
       * if it was registered there, the packet is forwarded
       */
      LIST_FOREACH(omt, &bridge_origin_table, next)
         if (!memcmp(omt->mac, po->L2.src, MEDIA_ADDR_LEN)) {
            po->flags |= PO_FORWARDED;
            return;
         }
   }
         
   if (po->flags & PO_FROMBRIDGE) {
      /* search the mac in the bridge table */
      LIST_FOREACH(omt, &bridge_origin_table, next)
         if (!memcmp(omt->mac, po->L2.src, MEDIA_ADDR_LEN))
            return;
      
      /* 
       * now search it in the opposite table
       * if it was registered there, the packet is forwarded
       */
      LIST_FOREACH(omt, &iface_origin_table, next)
         if (!memcmp(omt->mac, po->L2.src, MEDIA_ADDR_LEN)) {
            po->flags |= PO_FORWARDED;
            return;
         }
   }


   /* allocate a new entry for the newly discovered mac address */
   SAFE_CALLOC(omt, 1, sizeof(struct origin_mac_table));

   memcpy(omt->mac, po->L2.src, MEDIA_ADDR_LEN);

   /* insert the new mac address in the proper list */
   if (po->flags & PO_FROMIFACE) {
      DEBUG_MSG("Added the mac [%s] to IFACE table", mac_addr_ntoa(po->L2.src, tmp));
      LIST_INSERT_HEAD(&iface_origin_table, omt, next);
   }
   
   if (po->flags & PO_FROMBRIDGE) {
      DEBUG_MSG("Added the mac [%s] to BRIDGE table", mac_addr_ntoa(po->L2.src, tmp));
      LIST_INSERT_HEAD(&bridge_origin_table, omt, next);
   }
}

/* 
 * in bridged sniffing all the packet must be forwarded
 * on the other iface
 */
void bridge_set_forwardable(struct packet_object *po)
{
   /* If for us on the iface */
   if ( !memcmp(GBL_IFACE->mac, po->L2.src, MEDIA_ADDR_LEN) || !memcmp(GBL_IFACE->mac, po->L2.dst, MEDIA_ADDR_LEN) )
      return;

   /* If for us on the bridge */
   if ( !memcmp(GBL_BRIDGE->mac, po->L2.src, MEDIA_ADDR_LEN) || !memcmp(GBL_BRIDGE->mac, po->L2.dst, MEDIA_ADDR_LEN) )
      return;

   /* in bridged sniffing all the packet have to be forwarded */      
   po->flags |= PO_FORWARDABLE;
}



/* EOF */

// vim:ts=3:expandtab

