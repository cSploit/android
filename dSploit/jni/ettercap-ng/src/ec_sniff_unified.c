/*
    ettercap -- unified sniffing method module

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

    $Id: ec_sniff_unified.c,v 1.25 2004/11/04 10:29:06 alor Exp $
*/

#include <ec.h>
#include <ec_capture.h>
#include <ec_send.h>
#include <ec_threads.h>
#include <ec_inject.h>
#include <ec_conntrack.h>
#include <ec_sslwrap.h>

/* proto */
void start_unified_sniff(void);
void stop_unified_sniff(void);
void forward_unified_sniff(struct packet_object *po);
void unified_check_forwarded(struct packet_object *po);
void unified_set_forwardable(struct packet_object *po);

/*******************************************/

/*
 * creates the threads for capturing 
 */
void start_unified_sniff(void)
{
   DEBUG_MSG("start_unified_sniff");
   
   if (GBL_SNIFF->active == 1) {
      USER_MSG("Unified sniffing already started...\n");
      return;
   }
   
   USER_MSG("Starting Unified sniffing...\n\n");
   
   /* create the timeouter thread */
   if (!GBL_OPTIONS->read) { 
      pthread_t pid;
      
      pid = ec_thread_getpid("timer");
      if (pthread_equal(pid, EC_PTHREAD_NULL))
         ec_thread_new("timer", "conntrack timeouter", &conntrack_timeouter, NULL);
   }

   /* create the thread for packet capture */
   ec_thread_new("capture", "pcap handler and packet decoder", &capture, GBL_OPTIONS->iface);

   /* start ssl_wrapper thread */
   if (!GBL_OPTIONS->read && !GBL_OPTIONS->unoffensive && !GBL_OPTIONS->only_mitm)
      ec_thread_new("sslwrap", "wrapper for ssl connections", &sslw_start, NULL);

   GBL_SNIFF->active = 1;
}


/*
 * kill the capturing threads, but leave untouched the others
 */
void stop_unified_sniff(void)
{
   pthread_t pid;
   
   DEBUG_MSG("stop_unified_sniff");
   
   if (GBL_SNIFF->active == 0) {
      USER_MSG("Unified sniffing is not running...\n");
      return;
   }
  
   /* get the pid and kill it */
   pid = ec_thread_getpid("capture");
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);
   
   pid = ec_thread_getpid("sslwrap");
   if (!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);

   USER_MSG("Unified sniffing was stopped.\n");

   GBL_SNIFF->active = 0;
}


void forward_unified_sniff(struct packet_object *po)
{
   /* if it was not initialized, no packet are forwardable */
   if (!GBL_LNET->lnet)
      return;

   /* if the interface was not configured, no packet are forwardable */
   if (!GBL_IFACE->configured)
      return;
   
   /* if unoffensive is set, don't forward any packet */
   if (GBL_OPTIONS->unoffensive || GBL_OPTIONS->read)
      return;

   /* 
    * forward the packet to Layer 3, the kernel
    * will route them to the correct destination (host or gw)
    */

   /* don't forward dropped packets */
   if ((po->flags & PO_DROPPED) == 0)
      send_to_L3(po);

    /* 
     * if the packet was modified and it exceeded the mtu,
     * we have to inject the exceeded data
     */
    if (po->DATA.inject) 
       inject_buffer(po); 
}

/*
 * check if the packet has been forwarded by us
 * the source mac address is our, but the ip address is different
 */
void unified_check_forwarded(struct packet_object *po) 
{
   /* the interface was not configured, the packets are not forwardable */
   if (!GBL_IFACE->configured)
      return;
   
   /* 
    * dont sniff forwarded packets (equal mac, different ip) 
    * but only if we are on live connections
    */
   if ( GBL_CONF->skip_forwarded && !GBL_OPTIONS->read &&
        !memcmp(GBL_IFACE->mac, po->L2.src, MEDIA_ADDR_LEN) &&
        ip_addr_cmp(&GBL_IFACE->ip, &po->L3.src) ) {
      po->flags |= PO_FORWARDED;
   }
}

/* 
 * if the dest mac address of the packet is
 * the same of GBL_IFACE->mac but the dest ip is
 * not the same as GBL_IFACE->ip, the packet is not
 * for us and we can do mitm on it before forwarding.
 */
void unified_set_forwardable(struct packet_object *po)
{
   /* 
    * if the mac is our, but the ip is not...
    * it has to be forwarded
    */
   if (!memcmp(GBL_IFACE->mac, po->L2.dst, MEDIA_ADDR_LEN) &&
       memcmp(GBL_IFACE->mac, po->L2.src, MEDIA_ADDR_LEN) &&
       ip_addr_cmp(&GBL_IFACE->ip, &po->L3.dst) ) {
      po->flags |= PO_FORWARDABLE;
   }
   
}


/* EOF */

// vim:ts=3:expandtab

