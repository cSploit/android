/*
    ettercap -- decoder module

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

#include <ec.h>
#include <ec_decode.h>
#include <ec_dispatcher.h>
#include <ec_threads.h>
#include <ec_ui.h>
#include <ec_packet.h>
#include <ec_hash.h>
#include <ec_hook.h>
#include <ec_filter.h>
#include <ec_inject.h>

#include <pcap.h>
#include <libnet.h>
#include <pthread.h>

/* globals */

static struct dec_entry *protocols_table;
static unsigned protocols_num;
static bool table_sorted = false;

struct dec_entry {
   u_int32 type;
   u_int8 level;
   bool active;
   FUNC_DECODER_PTR(decoder);
};

/* protos */

void __init data_init(void);
FUNC_DECODER(decode_data);

void ec_decode(u_char *param, const struct pcap_pkthdr *pkthdr, const u_char *pkt);
void add_decoder(u_int8 level, u_int32 type, FUNC_DECODER_PTR(decoder));
static void sort_decoders(void);
static int cmp_decoders(const void *va, const void *vb);
static struct dec_entry* find_entry(u_int8 level, u_int32 type);
void del_decoder(u_int8 level, u_int32 type);
void *get_decoder(u_int8 level, u_int32 type);
void **get_decoders(u_int8 level, u_int32 type);

/* mutexes */

static pthread_mutex_t decoders_mutex = PTHREAD_MUTEX_INITIALIZER;
#define DECODERS_LOCK     do{ pthread_mutex_lock(&decoders_mutex); } while(0)
#define DECODERS_UNLOCK   do{ pthread_mutex_unlock(&decoders_mutex); } while(0)

static pthread_mutex_t dump_mutex = PTHREAD_MUTEX_INITIALIZER;
#define DUMP_LOCK     do{ pthread_mutex_lock(&dump_mutex); } while(0)
#define DUMP_UNLOCK   do{ pthread_mutex_unlock(&dump_mutex); } while(0)

/*******************************************/


void ec_decode(u_char *param, const struct pcap_pkthdr *pkthdr, const u_char *pkt)
{
   FUNC_DECODER_PTR(packet_decoder);
   struct packet_object po;
   bpf_u_int32 len;
   u_char *data;
   bpf_u_int32 datalen;

   CANCELLATION_POINT();

   /* start the timer for the stats */
   stats_half_start(&GBL_STATS->bh);
  
   /* XXX -- remove this */
#if 0
   if (!GBL_OPTIONS->quiet)
      USER_MSG("CAPTURED: 0x%04x bytes form %s\n", pkthdr->caplen, iface->name );
#endif
   
   if (GBL_OPTIONS->read)
      /* update the offset pointer */
      GBL_PCAP->dump_off = ftell(pcap_file(GBL_IFACE->pcap));
   else {
      /* update the statistics */
      stats_update();
   }
   
   /* 
    * dump packet to file if specified on command line 
    * it dumps all the packets disregarding the filter
    *
    * do not perform the operation if we are reading from another
    * filedump. See below where the file is dumped when reading
    * form other files (useful for decription).
    */
   if (GBL_OPTIONS->write && !GBL_OPTIONS->read) {
      /* 
       * we need to lock this because in SM_BRIDGED the
       * packets are dumped in the log file by two threads
       */
      DUMP_LOCK;
      pcap_dump((u_char *)param, pkthdr, pkt);
      DUMP_UNLOCK;
   }
 
   /* bad packet */
   if (pkthdr->caplen > UINT16_MAX) {
      USER_MSG("Bad packet detected, skipping...\n");
      return;
   }
   
   /* 
    * copy the packet in a "safe" buffer 
    * we don't want other packets after the end of the packet (as in BPF)
    *
    * also keep the buffer aligned !
    * the alignment is set by the media decoder.
    */
   memcpy(GBL_PCAP->buffer + GBL_PCAP->align, pkt, pkthdr->caplen);
   
   /* extract data and datalen from pcap packet */
   data = (u_char *)GBL_PCAP->buffer + GBL_PCAP->align;
   datalen = pkthdr->caplen;

   /* 
    * deal with trucated packets:
    * if someone has created a pcap file with the snaplen
    * too small we have to skip the packet (is not interesting for us)
    */
   if (GBL_PCAP->snaplen <= datalen) {
      USER_MSG("Truncated packet detected, skipping...\n");
      return;
   }
   
   /* alloc the packet object structure to be passet through decoders */
   packet_create_object(&po, data, datalen);

   /* Be sure to NULL terminate our data buffer */
   *(data + datalen) = 0;
   
   /* set the po timestamp */
   memcpy(&po.ts, &pkthdr->ts, sizeof(struct timeval));
   /* set the interface where the packet was captured */
   if (GBL_OPTIONS->iface && !strcmp(GBL_IFACE->name, GBL_OPTIONS->iface))
      po.flags |= PO_FROMIFACE;
   else if (GBL_OPTIONS->iface_bridge && !strcmp(GBL_IFACE->name, GBL_OPTIONS->iface_bridge))
      po.flags |= PO_FROMBRIDGE;

   /* HOOK POINT: RECEIVED */ 
   hook_point(HOOK_RECEIVED, &po);
   
   /* 
    * by default the packet should not be processed by ettercap.
    * if the sniffing filter matches it, the flag will be reset.
    */
   po.flags |= PO_IGNORE;
  
   /* 
    * start the analysis through the decoders stack 
    *
    * if the packet can be handled it will reach the top of the stack
    * where the decoder_data will add it to the top_half queue,
    * else the packet will not be handled but it should be forwarded
    *
    * after this fuction the packet is completed (all flags set)
    */
   packet_decoder = get_decoder(LINK_LAYER, GBL_PCAP->dlt);
   BUG_IF(packet_decoder == NULL);
   packet_decoder(data, datalen, &len, &po);
  
   /* special case for bridged sniffing */
   if (GBL_SNIFF->type == SM_BRIDGED) {
      EXECUTE(GBL_SNIFF->check_forwarded, &po);
      EXECUTE(GBL_SNIFF->set_forwardable, &po);
   }
   
   /* XXX - BIG WARNING !!
    *
    * if the packet was filtered by the filtering engine
    * the state of the packet_object is inconsistent !
    * the fields in the structure may not reflect the real
    * packet fields...
    */
   
   /* use the sniffing method funcion to forward the packet */
   if ( (po.flags & PO_FORWARDABLE) && !(po.flags & PO_FORWARDED) ) {
      /* HOOK POINT: PRE_FORWARD */ 
      hook_point(HOOK_PRE_FORWARD, &po);
      EXECUTE(GBL_SNIFF->forward, &po);
   }


   /* 
    * dump packets to a file from another file.
    * this is useful when decrypting packets or applying filters
    * on pcapfile and we want to save the result in a file
    */
   if (GBL_OPTIONS->write && GBL_OPTIONS->read) {
      DUMP_LOCK;
      /* reuse the original pcap header, but with the modified packet */
      pcap_dump((u_char *)param, pkthdr, po.packet);
      DUMP_UNLOCK;
   }
   
   /* 
    * if it is the last packet set the flag 
    * and send the packet to the top half.
    * we have to do this because the last packet 
    * might be dropped by the filter.
    */
   if (GBL_OPTIONS->read && GBL_PCAP->dump_size == GBL_PCAP->dump_off) {
      po.flags |= PO_EOF;
      top_half_queue_add(&po);
   }
   
   /* free the structure */
   packet_destroy_object(&po);
   
   /* calculate the stats */
   stats_half_end(&GBL_STATS->bh, pkthdr->caplen);
   
   CANCELLATION_POINT();

   return;
}

/* register the data decoder */
void __init data_init(void)
{
   add_decoder(APP_LAYER, PL_DEFAULT, decode_data);
}

/* 
 * if the packet reach the top of the stack (it can be handled),
 * this decoder is invoked
 */

FUNC_DECODER(decode_data)
{
   int proto = 0;
   FUNC_DECODER_PTR(app_decoder);
      
   CANCELLATION_POINT();

   /* this packet must not be passet to dissectors */
   if ( po->flags & PO_DONT_DISSECT)
      return NULL;
   
   /* reset the flag PO_IGNORE if the packet should be processed */
   EXECUTE(GBL_SNIFF->interesting, po);

   /* HOOK POINT: HANDLED */ 
   hook_point(HOOK_HANDLED, po);

   /* 
    * the display engine has stated that this
    * packet should not be processed by us.
    */
   if ( po->flags & PO_IGNORE )
      return NULL;

   /* 
    * run the APP_LAYER decoders 
    *
    * we should run the decoder on both the tcp/udp ports
    * since we may be interested in both client and server traffic.
    */
   switch(po->L4.proto) {
      case NL_TYPE_TCP:
         proto = APP_LAYER_TCP;
         break;

      case NL_TYPE_UDP:
         proto = APP_LAYER_UDP;
         break;
   }

   if(proto) {
      app_decoder = get_decoder(proto, ntohs(po->L4.src));
      EXECUTE_DECODER(app_decoder);

   /*
    * This check prevents from running a decoder twice
    */
      if(po->L4.src != po->L4.dst) {
         app_decoder = get_decoder(proto, ntohs(po->L4.dst));
         EXECUTE_DECODER(app_decoder);
      }
   }

   /* HOOK POINT: DECODED (the po structure is filled) */ 
   hook_point(HOOK_DECODED, po);

   /*
    * here we can filter the content of the packet.
    * the injection is done elsewhere.
    */
   filter_packet(po);

   /* If the modified packet exceeds the MTU split it into inject buffer */
   inject_split_data(po);
   
   /* HOOK POINT: FILTER */ 
   hook_point(HOOK_FILTER, po);
   
   /* 
    * add the packet to the queue and return.
    * we must be fast here !
    */
   top_half_queue_add(po);     

   CANCELLATION_POINT();
  
   return NULL;
}
      

/*
 * add a decoder to the decoders table 
 */
void add_decoder(u_int8 level, u_int32 type, FUNC_DECODER_PTR(decoder))
{
   struct dec_entry *e;
   bool found = false;

   DECODERS_LOCK;

   /* in case this is the first call and no tables are allocated */
   if(protocols_table == NULL) {
      /* the numbers are kindly provided by /usr/bin/grep and /usr/bin/wc */
      SAFE_CALLOC(protocols_table, protocols_num = 19 + 52, sizeof(struct dec_entry));
   }

   e = &protocols_table[protocols_num];

   /* searching for an empty entry */
   while(e --> protocols_table) 
      if(e->level == 0 && e->type == 0 && e->decoder == NULL) {
         /* We got it! */
         found = true;
         break;
      }
   /*
    * wc and grep are a part of The Conspiracy!
    * but we can allocate a little bit more, cant we?
    * and no kidding plz, its a serious code from now on
    */
   if(!found) {
      SAFE_REALLOC(protocols_table, ++protocols_num * sizeof(struct dec_entry));
      e = &protocols_table[protocols_num - 1];
   }

   /* We win, they lose, ha-ha! */
   e->level = level;
   e->type = type;
   e->decoder = decoder;
   e->active = true;

   table_sorted = false;

   DECODERS_UNLOCK;
   
   return;
}

/*
 * this is to be called in the initialization end
 * after all of the protocols are added
 */
static void sort_decoders(void)
{
   qsort(protocols_table, protocols_num, sizeof(struct dec_entry), cmp_decoders);
   table_sorted = true;
}

static int cmp_decoders(const void *va, const void *vb)
{
   const struct dec_entry *da = va, *db = vb;

   /* Here is the tricky part:
    * the array is sorted by level, then by the port number.
    */

   if (da->level != db->level)
   {
      return da->level - db->level;
   }

   return da->type - db->type;
}

static struct dec_entry* find_entry(u_int8 level, u_int32 type)
{
   struct dec_entry *e, fake;

   fake.level = level;
   fake.type = type;

   DECODERS_LOCK;

   if(!table_sorted) 
      sort_decoders();

   e = bsearch(&fake, protocols_table, protocols_num, sizeof(struct dec_entry), cmp_decoders);

   DECODERS_UNLOCK;
   
   return e;
}


/*
 * get a decoder from the decoders table 
 */

void * get_decoder(u_int8 level, u_int32 type)
{
   struct dec_entry *e;

   if((e = find_entry(level, type)))
      if(e->active)
         return e->decoder;
   
   return NULL;
}

/*
 * remove a decoder(s) from the decoders table
 */

void del_decoder(u_int8 level, u_int32 type)
{
   struct dec_entry *e;

   if((e = find_entry(level, type))) {
      DECODERS_LOCK;
      if (e != &protocols_table[protocols_num-1])
      {
          /* Replace this entry with the last one */
          memcpy(e, &protocols_table[protocols_num-1], sizeof(struct dec_entry));
      }
      /* Resize the array */
      SAFE_REALLOC(protocols_table, --protocols_num*sizeof(struct dec_entry));
      /* And mark as unsorted */
      table_sorted = false;

      DECODERS_UNLOCK;
   }
   
   return;
}

/* EOF */

// vim:ts=3:expandtab

