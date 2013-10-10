/*
    ettercap -- iface and capture functions

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

    $Id: ec_capture.c,v 1.55 2004/07/13 16:24:51 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_threads.h>
#include <ec_capture.h>
#include <ec_ui.h>
#include <ec_inet.h>

#include <pcap.h>
#include <libnet.h>

#if defined(OS_BSD_OPEN) || defined(OS_LINUX)
   /* LINUX does not care about timeout */
   /* OPENBSD needs 0, but it sucks up all the CPU */
   #define PCAP_TIMEOUT 0
#elif defined(OS_SOLARIS)
   /* SOLARIS needs > 1 */
   #define PCAP_TIMEOUT 10
#else
   /* FREEBSD needs 1 */
   /* MACOSX  needs 1 */
   #define PCAP_TIMEOUT 10
#endif

/* globals */

static SLIST_HEAD (, align_entry) aligners_table;

struct align_entry {
   int dlt;
   FUNC_ALIGNER_PTR(aligner);
   SLIST_ENTRY (align_entry) next;
};

/* protos */

void capture_init(void);
void capture_close(void);
EC_THREAD_FUNC(capture);
EC_THREAD_FUNC(capture_bridge);

void get_hw_info(void);
void capture_getifs(void);
int is_pcap_file(char *file, char *errbuf);

static void set_alignment(int dlt);
void add_aligner(int dlt, int (*aligner)(void));

/*******************************************/

/* 
 * return the name of the interface.
 * used to deal with unicode under Windows
 */
static char *iface_name(const char *s)
{
   char *buf;
   
#if defined(OS_WINDOWS) || defined(OS_CYGWIN)
   size_t len = wcslen ((const wchar_t*)s);
   
   if (!s[1]) {   /* should probably use IsTextUnicode() ... */
      SAFE_CALLOC(buf, len + 1, sizeof(char));
      
      sprintf (buf, "%S", s);
      DEBUG_MSG("iface_name: '%S', is_unicode '%s'", s, buf);
      return buf;
   }
#endif

   SAFE_STRDUP(buf, s);
   return buf;
}

/*
 * set up the pcap to capture from the specified interface
 * set up even the first dissector by looking at DLT_*
 */

void capture_init(void)
{
   pcap_t *pd;
   pcap_t *pb = NULL; /* for the bridge */
   pcap_dumper_t *pdump;
   bpf_u_int32 net, mask;
   struct bpf_program bpf;
   char pcap_errbuf[PCAP_ERRBUF_SIZE];
   
   /*
    * if the user didn't specified the interface,
    * we have to found one...
    */
   if (!GBL_OPTIONS->read && GBL_OPTIONS->iface == NULL) {
      char *ifa = pcap_lookupdev(pcap_errbuf);
      ON_ERROR(ifa, NULL, "No suitable interface found...");
      
      GBL_OPTIONS->iface = iface_name(ifa);
   }
  
   if (GBL_OPTIONS->iface)
      DEBUG_MSG("capture_init %s", GBL_OPTIONS->iface);
   else
      DEBUG_MSG("capture_init (no interface)");
      
              
   if (GBL_SNIFF->type == SM_BRIDGED) {
      if (!strcmp(GBL_OPTIONS->iface, GBL_OPTIONS->iface_bridge))
         FATAL_ERROR("Bridging iface must be different from %s", GBL_OPTIONS->iface);
      USER_MSG("Bridging %s and %s...\n\n", GBL_OPTIONS->iface, GBL_OPTIONS->iface_bridge);
   } else if (GBL_OPTIONS->read) {
      USER_MSG("Reading from %s... ", GBL_OPTIONS->pcapfile_in);
   } else
      USER_MSG("Listening on %s... ", GBL_OPTIONS->iface);
   
   /* set the snaplen to maximum */
   GBL_PCAP->snaplen = UINT16_MAX;
   
   /* open the interface from GBL_OPTIONS (user specified) */
   if (GBL_OPTIONS->read)
      pd = pcap_open_offline(GBL_OPTIONS->pcapfile_in, pcap_errbuf);
   else
      pd = pcap_open_live(GBL_OPTIONS->iface, GBL_PCAP->snaplen, GBL_PCAP->promisc, 
                   PCAP_TIMEOUT, pcap_errbuf);
   
   ON_ERROR(pd, NULL, "pcap_open: %s", pcap_errbuf);

   /* 
    * update to the reap assigned snapshot.
    * this may be different reading from files
    */
   DEBUG_MSG("requested snapshot: %d assigned: %d", GBL_PCAP->snaplen, pcap_snapshot(pd));
   GBL_PCAP->snaplen = pcap_snapshot(pd);
  
   /* get the file size */
   if (GBL_OPTIONS->read) {
      struct stat st;
      fstat(fileno(pcap_file(pd)), &st);
      GBL_PCAP->dump_size = st.st_size;
   }

   /* set the pcap filters */
   if (GBL_PCAP->filter != NULL && strcmp(GBL_PCAP->filter, "")) {

      DEBUG_MSG("pcap_filter: %s", GBL_PCAP->filter);
   
      if (pcap_lookupnet(GBL_OPTIONS->iface, &net, &mask, pcap_errbuf) == -1)
         ERROR_MSG("%s", pcap_errbuf);

      if (pcap_compile(pd, &bpf, GBL_PCAP->filter, 1, mask) < 0)
         ERROR_MSG("%s", pcap_errbuf);
            
      if (pcap_setfilter(pd, &bpf) == -1)
         ERROR_MSG("pcap_setfilter");

      pcap_freecode(&bpf);
   }
   
   /* if in bridged sniffing, we have to open even the other iface */
   if (GBL_SNIFF->type == SM_BRIDGED) {
      pb = pcap_open_live(GBL_OPTIONS->iface_bridge, GBL_PCAP->snaplen, GBL_PCAP->promisc, 
                   PCAP_TIMEOUT, pcap_errbuf);
   
      ON_ERROR(pb, NULL, "%s", pcap_errbuf);
   
      /* set the pcap filters */
      if (GBL_PCAP->filter != NULL) {
   
         if (pcap_lookupnet(GBL_OPTIONS->iface_bridge, &net, &mask, pcap_errbuf) == -1)
            ERROR_MSG("%s", pcap_errbuf);

         if (pcap_compile(pb, &bpf, GBL_PCAP->filter, 1, mask) < 0)
            ERROR_MSG("%s", pcap_errbuf);
            
         if (pcap_setfilter(pb, &bpf) == -1)
            ERROR_MSG("pcap_setfilter");

         pcap_freecode(&bpf);
      }
   }


   /* open the dump file */
   if (GBL_OPTIONS->write) {
      DEBUG_MSG("pcapfile_out: %s", GBL_OPTIONS->pcapfile_out);
      pdump = pcap_dump_open(pd, GBL_OPTIONS->pcapfile_out);
      ON_ERROR(pdump, NULL, "%s", pcap_geterr(pd));
      GBL_PCAP->dump = pdump;               
   }
   
   /* set the right dlt type for the iface */
   GBL_PCAP->dlt = pcap_datalink(pd);
     
   DEBUG_MSG("capture_init: %s [%d]", pcap_datalink_val_to_description(GBL_PCAP->dlt), GBL_PCAP->dlt);
   USER_MSG("(%s)\n\n", pcap_datalink_val_to_description(GBL_PCAP->dlt));
 
   /* check that the bridge type is the same as the main iface */
   if (GBL_SNIFF->type == SM_BRIDGED && pcap_datalink(pb) != GBL_PCAP->dlt)
      FATAL_ERROR("You can NOT bridge two different type of interfaces !");
   
   /* check if we support this media */
   if (get_decoder(LINK_LAYER, GBL_PCAP->dlt) == NULL) {
      if (GBL_OPTIONS->read)
         FATAL_ERROR("Dump file not supported (%s)", pcap_datalink_val_to_description(GBL_PCAP->dlt));
      else
         FATAL_ERROR("Inteface \"%s\" not supported (%s)", GBL_OPTIONS->iface, pcap_datalink_val_to_description(GBL_PCAP->dlt));
   }
   
   /* set the alignment for the buffer */
   set_alignment(GBL_PCAP->dlt);
   
   /* allocate the buffer for the packets (UINT16_MAX) */
   SAFE_CALLOC(GBL_PCAP->buffer, UINT16_MAX + GBL_PCAP->align, sizeof(char));
  
   /* set the global descriptor for both the iface and the bridge */
   GBL_PCAP->pcap = pd;               
   if (GBL_SNIFF->type == SM_BRIDGED)
      GBL_PCAP->pcap_bridge = pb;
 
   /* on exit clean up the structures */
   atexit(capture_close);
   
}


void capture_close(void)
{
   pcap_close(GBL_PCAP->pcap);
   if (GBL_OPTIONS->write)
      pcap_dump_close(GBL_PCAP->dump);
   
   if (GBL_SNIFF->type == SM_BRIDGED)
      pcap_close(GBL_PCAP->pcap_bridge);
   
   DEBUG_MSG("ATEXIT: capture_closed");
}

/*
 * start capturing packets
 */

EC_THREAD_FUNC(capture)
{
   int ret;
   
   /* init the thread and wait for start up */
   ec_thread_init();
   
   DEBUG_MSG("neverending loop (capture)");

   /* wipe the stats */
   stats_wipe();
   
   /* 
    * infinite loop 
    * dispatch packets to ec_decode
    */
   ret = pcap_loop(GBL_PCAP->pcap, -1, ec_decode, EC_THREAD_PARAM);
   ON_ERROR(ret, -1, "Error while capturing: %s", pcap_geterr(GBL_PCAP->pcap));
   
   return NULL;
}


EC_THREAD_FUNC(capture_bridge)
{
   /* init the thread and wait for start up */
   ec_thread_init();
   
   DEBUG_MSG("neverending loop (capture_bridge)");
   
   /* wipe the stats */
   stats_wipe();
   
   /* 
    * infinite loop 
    * dispatch packets to ec_decode
    */
   pcap_loop(GBL_PCAP->pcap_bridge, -1, ec_decode, EC_THREAD_PARAM);

   return NULL;
}

/*
 * get the list of all network interfaces
 */
void capture_getifs(void)
{
   pcap_if_t *dev, *pdev, *ndev;
   char pcap_errbuf[PCAP_ERRBUF_SIZE];
   
   DEBUG_MSG("capture_getifs");
  
   /* retrieve the list */
   if (pcap_findalldevs((pcap_if_t **)&GBL_PCAP->ifs, pcap_errbuf) == -1)
      ERROR_MSG("%s", pcap_errbuf);

   /* analize the list and remove unwanted entries */
   for (pdev = dev = (pcap_if_t *)GBL_PCAP->ifs; dev != NULL; dev = ndev) {
      
      /* the next entry in the list */
      ndev = dev->next;
      
      /* set the description for the local loopback */
      if (dev->flags & PCAP_IF_LOOPBACK) {
         SAFE_FREE(dev->description);
         dev->description = strdup("Local Loopback");
      }
     
      /* fill the empty descriptions */
      if (dev->description == NULL)
         dev->description = dev->name;

      /* remove the pseudo device 'any' */
      if (!strcmp(dev->name, "any")) {
         /* check if it is the first in the list */
         if (dev == GBL_PCAP->ifs)
            GBL_PCAP->ifs = ndev;
         else
            pdev->next = ndev;

         SAFE_FREE(dev->name);
         SAFE_FREE(dev->description);
         SAFE_FREE(dev);

         continue;
      }
     
      /* remember the previous device for the next loop */
      pdev = dev;
      
      DEBUG_MSG("capture_getifs: [%s] %s", dev->name, dev->description);
   }

   /* do we have to print the list ? */
   if (GBL_OPTIONS->iflist) {
     
      /* we are before ui_init(), can use printf */
      fprintf(stdout, "List of available Network Interfaces:\n\n");
      
      for (dev = (pcap_if_t *)GBL_PCAP->ifs; dev != NULL; dev = dev->next)
         fprintf(stdout, " %s  \t%s\n", dev->name, dev->description);

      fprintf(stdout, "\n\n");

      clean_exit(0);
   }
                   
}

/* 
 * retrieve the IP and the MAC address of the hardware
 * used to sniff (primary iface or bridge)
 */

void get_hw_info(void)
{
   u_long ip;
   struct libnet_ether_addr *ea;
   bpf_u_int32 network, netmask;
   char pcap_errbuf[PCAP_ERRBUF_SIZE];
 
   /* dont touch the interface reading from file */
   if (!GBL_LNET->lnet || GBL_OPTIONS->read) {
      DEBUG_MSG("get_hw_info: skipping... (not initialized)");
      return;
   }
   
   DEBUG_MSG("get_hw_info");
  
   /* get the ip address */
   ip = libnet_get_ipaddr4(GBL_LNET->lnet);
   
   /* if ip is equal to -1 there was an error */
   if (ip != (u_long)~0) {

      /* the interface has an ip address */
      if (ip != 0)
         GBL_IFACE->configured = 1;
      
      /* save the ip address */
      ip_addr_init(&GBL_IFACE->ip, AF_INET, (char *)&ip);
      
      if (pcap_lookupnet(GBL_OPTIONS->iface, &network, &netmask, pcap_errbuf) == -1)
         ERROR_MSG("%s", pcap_errbuf);
      
      ip_addr_init(&GBL_IFACE->network, AF_INET, (char *)&network);

      /* the user has specified a different netmask, use it */
      if (GBL_OPTIONS->netmask) {
         struct in_addr net;
         /* sanity check */
         if (inet_aton(GBL_OPTIONS->netmask, &net) == 0)
            FATAL_ERROR("Invalid netmask %s", GBL_OPTIONS->netmask);

         ip_addr_init(&GBL_IFACE->netmask, AF_INET, (char *)&net);
      } else
         ip_addr_init(&GBL_IFACE->netmask, AF_INET, (char *)&netmask);
      
   } else
      DEBUG_MSG("get_hw_info: NO IP on %s", GBL_OPTIONS->iface);
   
   /* get the mac address */
   ea = libnet_get_hwaddr(GBL_LNET->lnet);

   if (ea != NULL)
      memcpy(GBL_IFACE->mac, ea->ether_addr_octet, MEDIA_ADDR_LEN);
   else
      DEBUG_MSG("get_hw_info: NO MAC for %s", GBL_OPTIONS->iface);

   /* get the MTU */
   GBL_IFACE->mtu = get_iface_mtu(GBL_OPTIONS->iface);
   
   /* check the mtu */
   if (GBL_IFACE->mtu > INT16_MAX)
      FATAL_ERROR("MTU too large");

   USER_MSG("%6s ->\t%s  ",  GBL_OPTIONS->iface,
            mac_addr_ntoa(GBL_IFACE->mac, pcap_errbuf));
   USER_MSG("%16s  ", ip_addr_ntoa(&GBL_IFACE->ip, pcap_errbuf));
   USER_MSG("%16s\n\n", ip_addr_ntoa(&GBL_IFACE->netmask, pcap_errbuf) );
   
   /* if not in bridged sniffing, return */
   if (GBL_SNIFF->type != SM_BRIDGED)
      return;
   
   ip = libnet_get_ipaddr4(GBL_LNET->lnet_bridge);

   /* if ip is equal to -1 there was an error */
   if (ip != (u_long)~0) {
      ip_addr_init(&GBL_BRIDGE->ip, AF_INET, (char *)&ip);
      
      if (pcap_lookupnet(GBL_OPTIONS->iface_bridge, &network, &netmask, pcap_errbuf) == -1)
         ERROR_MSG("%s", pcap_errbuf);
      
      ip_addr_init(&GBL_BRIDGE->network, AF_INET, (char *)&network);
      ip_addr_init(&GBL_BRIDGE->netmask, AF_INET, (char *)&netmask);
      
   } else
      DEBUG_MSG("get_hw_info: NO IP on %s", GBL_OPTIONS->iface_bridge);
   
   ea = libnet_get_hwaddr(GBL_LNET->lnet_bridge);

   if (ea != NULL)
      memcpy(GBL_BRIDGE->mac, ea->ether_addr_octet, MEDIA_ADDR_LEN);
   else
      DEBUG_MSG("get_hw_info: NO MAC for %s", GBL_OPTIONS->iface_bridge);
   
   /* get the MTU */
   GBL_BRIDGE->mtu = get_iface_mtu(GBL_OPTIONS->iface_bridge);
   
   if (GBL_BRIDGE->mtu > INT16_MAX)
      FATAL_ERROR("MTU too large");

   USER_MSG("%6s ->\t%s  ",  GBL_OPTIONS->iface_bridge,
            mac_addr_ntoa(GBL_BRIDGE->mac, pcap_errbuf));
   USER_MSG("%16s  ", ip_addr_ntoa(&GBL_BRIDGE->ip, pcap_errbuf));
   USER_MSG("%16s\n\n", ip_addr_ntoa(&GBL_BRIDGE->netmask, pcap_errbuf) );

   /* some sanity checks */
   if (GBL_BRIDGE->mtu != GBL_IFACE->mtu)
      FATAL_ERROR("The two interfaces must have the same MTU.");

   if (!memcmp(GBL_BRIDGE->mac, GBL_IFACE->mac, MEDIA_ADDR_LEN))
      FATAL_ERROR("The two bridged interfaces must be phisically different");
}

/*
 * check if the given file is a pcap file
 */
int is_pcap_file(char *file, char *errbuf)
{
   pcap_t *pd;
   
   pd = pcap_open_offline(file, errbuf);
   if (pd == NULL)
      return -EINVALID;

   pcap_close(pd);
   
   return ESUCCESS;
}

/*
 * set the alignment for the buffer 
 */
static void set_alignment(int dlt)
{
   struct align_entry *e;

   SLIST_FOREACH (e, &aligners_table, next)
      if (e->dlt == dlt) {
         GBL_PCAP->align = e->aligner();
         return;
      }

   /* not found */
   BUG("Don't know how to align this media header");
}

/*
 * add a alignment function to the table 
 */
void add_aligner(int dlt, FUNC_ALIGNER_PTR(aligner))
{
   struct align_entry *e;

   SAFE_CALLOC(e, 1, sizeof(struct align_entry));
   
   e->dlt = dlt;
   e->aligner = aligner;

   SLIST_INSERT_HEAD(&aligners_table, e, next); 
}

/* EOF */

// vim:ts=3:expandtab

