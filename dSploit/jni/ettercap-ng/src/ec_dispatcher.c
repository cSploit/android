/*
    ettercap -- top half and dispatching module

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

    $Id: ec_dispatcher.c,v 1.36 2004/07/23 07:25:27 alor Exp $
*/

#include <ec.h>
#include <ec_threads.h>
#include <ec_hook.h>
#include <ec_stats.h>


/* this is the PO queue from bottom to top half */
struct po_queue_entry {
   struct packet_object *po;
   STAILQ_ENTRY(po_queue_entry) next;
};

static STAILQ_HEAD(, po_queue_entry) po_queue = STAILQ_HEAD_INITIALIZER(po_queue);

/* global mutex on interface */

static pthread_mutex_t po_mutex = PTHREAD_MUTEX_INITIALIZER;
#define PO_QUEUE_LOCK     do{ pthread_mutex_lock(&po_mutex); }while(0)
#define PO_QUEUE_UNLOCK   do{ pthread_mutex_unlock(&po_mutex); }while(0)

/* proto */

void top_half_queue_add(struct packet_object *po);
EC_THREAD_FUNC(top_half);

/*******************************************/

/*
 * top half function
 * it is the dispatcher for the various methods
 * which need to process packet objects 
 * created by the bottom_half (capture).
 * it read the queue created by top_half_queue_add()
 * and deliver the po to all the registered functions
 */

EC_THREAD_FUNC(top_half)
{
   struct po_queue_entry *e;
   u_int pck_len;
   
   /* initialize the thread */
   ec_thread_init();
   
   DEBUG_MSG("top_half activated !");

   /* 
    * we don't want profiles in memory.
    * remove the hooks and return
    */
   if (!GBL_CONF->store_profiles) {
      DEBUG_MSG("top_half: profile collection disabled");
      hook_del(HOOK_PACKET_ARP, &profile_parse);
      hook_del(HOOK_PACKET_ICMP, &profile_parse);
      hook_del(HOOK_PROTO_DHCP_PROFILE, &profile_parse);
      hook_del(HOOK_DISPATCHER, &profile_parse);
   }
   
   LOOP { 
     
      CANCELLATION_POINT();
      
      /* the queue is updated by other thread, lock it */
      PO_QUEUE_LOCK;
      
      /* get the first element */
      e = STAILQ_FIRST(&po_queue);

      /* the queue is empty, nothing to do... */
      if (e == NULL) {
         PO_QUEUE_UNLOCK;
         
         usleep(1000);
         continue;
      }
  
      /* start the counter for the TopHalf */
      stats_half_start(&GBL_STATS->th);
       
      /* remove the packet form the queue */
      STAILQ_REMOVE_HEAD(&po_queue, e, next);
     
      /* update the queue stats */
      stats_queue_del();
      
      /* 
       * we have extracted the element, unlock the queue 
       *
       * the bottom half MUST be very fast and it cannot
       * wait on the top half lock.
       */
      PO_QUEUE_UNLOCK;
      
      /* 
       * check if it is the last packet of a file...
       * and exit if we are in text only or demonize mode
       */
      if (e->po->flags & PO_EOF) {
         DEBUG_MSG("End of dump file...");
         USER_MSG("\nEnd of dump file...\n");
         if ((GBL_UI->type == UI_TEXT || GBL_UI->type == UI_DAEMONIZE) && GBL_CONF->close_on_eof)
            clean_exit(0);
         else {
            SAFE_FREE(e);
            continue;
         }
      }
      
      /* HOOK_POINT: DISPATCHER */
      hook_point(HOOK_DISPATCHER, e->po);

      /* save the len before the free() */
      pck_len = e->po->DATA.disp_len;
      
      /* destroy the duplicate packet object */
      packet_destroy_object(e->po);
      SAFE_FREE(e->po);
      SAFE_FREE(e);
      
      /* start the counter for the TopHalf */
      stats_half_end(&GBL_STATS->th, pck_len);
   } 

   return NULL;
}

/* 
 * add a packet to the top half queue.
 * this fuction is called by the bottom half thread
 */

void top_half_queue_add(struct packet_object *po)
{
   struct po_queue_entry *e;

   SAFE_CALLOC(e, 1, sizeof(struct po_queue_entry));
   
   e->po = packet_dup(po, PO_DUP_NONE);
   
   PO_QUEUE_LOCK;
   
   /* add the message to the queue */
   STAILQ_INSERT_TAIL(&po_queue, e, next);
   
   /* update the stats */
   stats_queue_add();
   
   PO_QUEUE_UNLOCK;
}


/* EOF */

// vim:ts=3:expandtab

