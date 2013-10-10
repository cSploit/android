/*
    ettercap -- initial scan to build the hosts list

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

    $Id: ec_scan.c,v 1.43 2004/12/21 20:27:15 alor Exp $
*/

#include <ec.h>
#include <ec_packet.h>
#include <ec_threads.h>
#include <ec_send.h>
#include <ec_decode.h>
#include <ec_resolv.h>
#include <ec_file.h>

#include <pthread.h>
#include <pcap.h>
#include <libnet.h>

/* globals */

/* used to create the random list */
static LIST_HEAD (, ip_list) ip_list_head;
static struct ip_list **rand_array;

/* protos */

void build_hosts_list(void);
void del_hosts_list(void);

static void scan_netmask(pthread_t pid);
static void scan_targets(pthread_t pid);

int scan_load_hosts(char *filename);
int scan_save_hosts(char *filename);

void add_host(struct ip_addr *ip, u_int8 mac[MEDIA_ADDR_LEN], char *name);

static void random_list(struct ip_list *e, int max);

static void get_response(struct packet_object *po);
static EC_THREAD_FUNC(capture_scan);
static EC_THREAD_FUNC(scan_thread);
static void scan_decode(u_char *param, const struct pcap_pkthdr *pkthdr, const u_char *pkt);

/*******************************************/

/*
 * build the initial host list with ARP requests
 */

void build_hosts_list(void)
{
   struct hosts_list *hl;
   int nhosts = 0;

   DEBUG_MSG("build_hosts_list");

   /* don't create the list in bridged mode */
   if (GBL_SNIFF->type == SM_BRIDGED)
      return;

   /*
    * load the list from the file
    * this option automatically enable GBL_OPTIONS->silent
    */
   if (GBL_OPTIONS->load_hosts) {
      scan_load_hosts(GBL_OPTIONS->hostsfile);

      LIST_FOREACH(hl, &GBL_HOSTLIST, next)
         nhosts++;

      INSTANT_USER_MSG("%d hosts added to the hosts list...\n", nhosts);

      return;
   }

   /* in silent mode, the list should not be created */
   if (GBL_OPTIONS->silent)
      return;

   /* it not initialized don't make the list */
   if (GBL_LNET->lnet == NULL)
      return;

   /* no target defined... */
   if (GBL_TARGET1->all_ip && GBL_TARGET2->all_ip &&
       !GBL_TARGET1->scan_all && !GBL_TARGET2->scan_all)
      return;

   /* delete the previous list */
   del_hosts_list();

#ifdef OS_MINGW
	/* FIXME: for some reason under windows it does not work in thread mode
    * to be investigated...
    */
	scan_thread(NULL);
#else
   /* check the type of UI we are running under... */
   if (GBL_UI->type == UI_TEXT || GBL_UI->type == UI_DAEMONIZE)
      /* in text mode and demonized call the function directly */
      scan_thread(NULL);
   else
      /* do the scan in a separate thread */
      ec_thread_new("scan", "scanning thread", &scan_thread, NULL);
#endif
}

/*
 * the thread responsible of the hosts scan
 */
static EC_THREAD_FUNC(scan_thread)
{
   pthread_t pid;
   struct hosts_list *hl;
   int i = 1, ret;
   int nhosts = 0;
   int threadize = 1;

   DEBUG_MSG("scan_thread");

   /* in text mode and demonized this function should NOT be a thread */
   if (GBL_UI->type == UI_TEXT || GBL_UI->type == UI_DAEMONIZE)
      threadize = 0;

#ifdef OS_MINGW
   /* FIXME: for some reason under windows it does not work in thread mode
    * to be investigated...
    */
   threadize = 0;
#endif

   /* if necessary, don't create the thread */
   if (threadize)
      ec_thread_init();

   /*
    * create a simple decode thread, it will call
    * the right HOOK POINT. so we only have to hook to
    * ARP packets.
    */
   hook_add(HOOK_PACKET_ARP_RP, &get_response);
   pid = ec_thread_new("scan_cap", "decoder module while scanning", &capture_scan, NULL);

   /*
    * if at least one target is ANY, scan the whole netmask
    * else scan only the specified targets
    *
    * the pid parameter is used to kill the thread if
    * the user request to stop the scan.
    */
   if (GBL_TARGET1->scan_all || GBL_TARGET2->scan_all)
      scan_netmask(pid);
   else
      scan_targets(pid);

   /*
    * free the temporary array for random computations
    * allocated in random_list()
    */
   SAFE_FREE(rand_array);

   /*
    * wait for some delayed packets...
    * the other thread is listening for ARP pachets
    */
   sleep(1);

   /* destroy the thread and remove the hook function */
   ec_thread_destroy(pid);
   hook_del(HOOK_PACKET_ARP, &get_response);

   /* count the hosts and print the message */
   LIST_FOREACH(hl, &GBL_HOSTLIST, next) {
      char tmp[MAX_ASCII_ADDR_LEN];
      (void)tmp;
      DEBUG_MSG("Host: %s", ip_addr_ntoa(&hl->ip, tmp));
      nhosts++;
   }

   INSTANT_USER_MSG("%d hosts added to the hosts list...\n", nhosts);

   /*
    * resolve the hostnames only if we are scanning
    * the lan. when loading from file, hostnames are
    * already in the file.
    */

   if (!GBL_OPTIONS->load_hosts && GBL_OPTIONS->resolve) {
      char title[50];

      snprintf(title, sizeof(title), "Resolving %d hostnames...", nhosts);

      INSTANT_USER_MSG("%s\n", title);

      LIST_FOREACH(hl, &GBL_HOSTLIST, next) {
         char tmp[MAX_HOSTNAME_LEN];

         host_iptoa(&hl->ip, tmp);
         hl->hostname = strdup(tmp);

         ret = ui_progress(title, i++, nhosts);

         /* user has requested to stop the task */
         if (ret == UI_PROGRESS_INTERRUPTED) {
            INSTANT_USER_MSG("Interrupted by user. Partial results may have been recorded...\n");
            ec_thread_exit();
         }
      }
   }

   /* save the list to the file */
   if (GBL_OPTIONS->save_hosts)
      scan_save_hosts(GBL_OPTIONS->hostsfile);

   /* if necessary, don't create the thread */
   if (threadize)
      ec_thread_exit();

   /* NOT REACHED */
   return NULL;
}


/*
 * delete the hosts list
 */
void del_hosts_list(void)
{
   struct hosts_list *hl, *tmp = NULL;

   LIST_FOREACH_SAFE(hl, &GBL_HOSTLIST, next, tmp) {
      SAFE_FREE(hl->hostname);
      LIST_REMOVE(hl, next);
      SAFE_FREE(hl);
   }
}


/*
 * capture the packets and call the HOOK POINT
 */
static EC_THREAD_FUNC(capture_scan)
{
   DEBUG_MSG("capture_scan");

   ec_thread_init();

   pcap_loop(GBL_PCAP->pcap, -1, scan_decode, EC_THREAD_PARAM);

   return NULL;
}


/*
 * parses the POs and executes the HOOK POINTs
 */
static void scan_decode(u_char *param, const struct pcap_pkthdr *pkthdr, const u_char *pkt)
{
   FUNC_DECODER_PTR(packet_decoder);
   struct packet_object po;
   int len;
   u_char *data;
   int datalen;

   CANCELLATION_POINT();

   /* extract data and datalen from pcap packet */
   data = (u_char *)pkt;
   datalen = pkthdr->caplen;

   /* alloc the packet object structure to be passet through decoders */
   packet_create_object(&po, data, datalen);

   /* set the po timestamp */
   memcpy(&po.ts, &pkthdr->ts, sizeof(struct timeval));

   /*
    * in this special parsing, the packet must be ingored by
    * application layer, leave this un touched.
    */
   po.flags |= PO_DONT_DISSECT;

   /*
    * start the analysis through the decoders stack
    * after this fuction the packet is completed (all flags set)
    */
   packet_decoder = get_decoder(LINK_LAYER, GBL_PCAP->dlt);
   BUG_IF(packet_decoder == NULL);
   packet_decoder(data, datalen, &len, &po);

   /* free the structure */
   packet_destroy_object(&po);

   CANCELLATION_POINT();

   return;
}


/*
 * receives the ARP packets
 */
static void get_response(struct packet_object *po)
{
   struct ip_list *t;

   /* if at least one target is the whole netmask, add the entry */
   if (GBL_TARGET1->scan_all || GBL_TARGET2->scan_all) {
      add_host(&po->L3.src, po->L2.src, NULL);
      return;
   }

   /* else only add arp replies within the targets */

   /* search in target 1 */
   LIST_FOREACH(t, &GBL_TARGET1->ips, next)
      if (!ip_addr_cmp(&t->ip, &po->L3.src)) {
         add_host(&po->L3.src, po->L2.src, NULL);
         return;
      }

   /* search in target 2 */
   LIST_FOREACH(t, &GBL_TARGET2->ips, next)
      if (!ip_addr_cmp(&t->ip, &po->L3.src)) {
         add_host(&po->L3.src, po->L2.src, NULL);
         return;
      }
}


/*
 * scan the netmask to find all hosts
 */
static void scan_netmask(pthread_t pid)
{
   u_int32 netmask, current, myip;
   int nhosts, i, ret;
   struct ip_addr scanip;
   struct ip_list *e, *tmp;
   char title[100];

   netmask = ip_addr_to_int32(&GBL_IFACE->netmask.addr);
   myip = ip_addr_to_int32(&GBL_IFACE->ip.addr);

   /* the number of hosts in this netmask */
   nhosts = ntohl(~netmask);

   DEBUG_MSG("scan_netmask: %d hosts", nhosts);

   INSTANT_USER_MSG("Randomizing %d hosts for scanning...\n", nhosts);

   /* scan the netmask */
   for (i = 1; i <= nhosts; i++) {
      /* calculate the ip */
      current = (myip & netmask) | htonl(i);
      ip_addr_init(&scanip, AF_INET, (char *)&current);

      SAFE_CALLOC(e, 1, sizeof(struct ip_list));

      memcpy(&e->ip, &scanip, sizeof(struct ip_addr));

      /* add to the list randomly */
      random_list(e, i);

   }

   snprintf(title, sizeof(title), "Scanning the whole netmask for %d hosts...", nhosts);
   INSTANT_USER_MSG("%s\n", title);

   i = 1;

   /* send the actual ARP request */
   LIST_FOREACH(e, &ip_list_head, next) {
      /* send the arp request */
      send_arp(ARPOP_REQUEST, &GBL_IFACE->ip, GBL_IFACE->mac, &e->ip, MEDIA_BROADCAST);

      /* update the progress bar */
      ret = ui_progress(title, i++, nhosts);

      /* user has requested to stop the task */
      if (ret == UI_PROGRESS_INTERRUPTED) {
         INSTANT_USER_MSG("Scan interrupted by user. Partial results may have been recorded...\n");
         /* destroy the capture thread and remove the hook function */
         ec_thread_destroy(pid);
         hook_del(HOOK_PACKET_ARP, &get_response);
         /* delete the temporary list */
         LIST_FOREACH_SAFE(e, &ip_list_head, next, tmp) {
            LIST_REMOVE(e, next);
            SAFE_FREE(e);
         }
         /* cancel the scan thread */
         ec_thread_exit();
      }

      /* wait for a delay */
      usleep(GBL_CONF->arp_storm_delay * 1000);
   }

   /* delete the temporary list */
   LIST_FOREACH_SAFE(e, &ip_list_head, next, tmp) {
      LIST_REMOVE(e, next);
      SAFE_FREE(e);
   }
}


/*
 * scan only the target hosts
 */
static void scan_targets(pthread_t pid)
{
   int nhosts = 0, found, n = 1, ret;
   struct ip_list *e, *i, *m, *tmp;
   char title[100];

   DEBUG_MSG("scan_targets: merging targets...");

   /*
    * make an unique list merging the two target
    * and count the number of hosts to be scanned
    */

   /* first get all the target1 ips */
   LIST_FOREACH(i, &GBL_TARGET1->ips, next) {

      SAFE_CALLOC(e, 1, sizeof(struct ip_list));

      memcpy(&e->ip, &i->ip, sizeof(struct ip_addr));

      nhosts++;

      /* add to the list randomly */
      random_list(e, nhosts);
   }

   /* then merge the target2 ips */
   LIST_FOREACH(i, &GBL_TARGET2->ips, next) {

      SAFE_CALLOC(e, 1, sizeof(struct ip_list));

      memcpy(&e->ip, &i->ip, sizeof(struct ip_addr));

      found = 0;

      /* search if it is already in the list */
      LIST_FOREACH(m, &ip_list_head, next)
         if (!ip_addr_cmp(&m->ip, &i->ip)) {
            found = 1;
            SAFE_FREE(e);
            break;
         }

      /* add it */
      if (!found) {
         nhosts++;
         /* add to the list randomly */
         random_list(e, nhosts);
      }
   }

   DEBUG_MSG("scan_targets: %d hosts to be scanned", nhosts);

   /* don't scan if there are no hosts */
   if (nhosts == 0)
      return;

   snprintf(title, sizeof(title), "Scanning for merged targets (%d hosts)...", nhosts);
   INSTANT_USER_MSG("%s\n\n", title);

   /* and now scan the LAN */
   LIST_FOREACH(e, &ip_list_head, next) {
      /* send the arp request */
      send_arp(ARPOP_REQUEST, &GBL_IFACE->ip, GBL_IFACE->mac, &e->ip, MEDIA_BROADCAST);

      /* update the progress bar */
      ret = ui_progress(title, n++, nhosts);

      /* user has requested to stop the task */
      if (ret == UI_PROGRESS_INTERRUPTED) {
         INSTANT_USER_MSG("Scan interrupted by user. Partial results may have been recorded...\n");
         /* destroy the capture thread and remove the hook function */
         ec_thread_destroy(pid);
         hook_del(HOOK_PACKET_ARP, &get_response);
         /* delete the temporary list */
         LIST_FOREACH_SAFE(e, &ip_list_head, next, tmp) {
            LIST_REMOVE(e, next);
            SAFE_FREE(e);
         }
         /* cancel the scan thread */
         ec_thread_exit();
      }

      /* wait for a delay */
      usleep(GBL_CONF->arp_storm_delay * 1000);
   }

   /* delete the temporary list */
   LIST_FOREACH_SAFE(e, &ip_list_head, next, tmp) {
      LIST_REMOVE(e, next);
      SAFE_FREE(e);
   }

}

/*
 * load the hosts list from this file
 */
int scan_load_hosts(char *filename)
{
   FILE *hf;
   int nhosts;
   char ip[16], mac[18], name[128];
   struct in_addr tip;
   struct ip_addr hip;
   u_int8 hmac[MEDIA_ADDR_LEN];

   DEBUG_MSG("scan_load_hosts: %s", filename);

   /* open the file */
   hf = fopen(filename, FOPEN_READ_TEXT);
   if (hf == NULL)
      SEMIFATAL_ERROR("Cannot open %s", filename);

   INSTANT_USER_MSG("Loading hosts list from file %s\n", filename);

   /* XXX - adapt to IPv6 */
   /* read the file */
   for (nhosts = 0; !feof(hf); nhosts++) {

      if (fscanf(hf,"%15s %17s %127s\n", ip, mac, name) != 3 ||
         *ip == '#' || *mac == '#' || *name == '#')
         continue;


      /* convert to network */
      mac_addr_aton(mac, hmac);

      if (inet_aton(ip, &tip) == 0) {
         del_hosts_list();
         SEMIFATAL_ERROR("Bad parsing on line %d", nhosts + 1);
      }

      ip_addr_init(&hip, AF_INET, (char *)&tip);

      /* wipe the null hostname */
      if (!strcmp(name, "-"))
         name[0] = '\0';

      /* add to the list */
      add_host(&hip, hmac, name);
   }

   fclose(hf);

   DEBUG_MSG("scan_load_hosts: loaded %d hosts lines", nhosts);

   return ESUCCESS;
}


/*
 * save the host list to this file
 */
int scan_save_hosts(char *filename)
{
   FILE *hf;
   int nhosts = 0;
   struct hosts_list *hl;
   char tmp[MAX_ASCII_ADDR_LEN];

   DEBUG_MSG("scan_save_hosts: %s", filename);

   /* open the file */
   hf = fopen(filename, FOPEN_WRITE_TEXT);
   if (hf == NULL)
      SEMIFATAL_ERROR("Cannot open %s for writing", filename);

   /* save the list */
   LIST_FOREACH(hl, &GBL_HOSTLIST, next) {
      fprintf(hf, "%s ", ip_addr_ntoa(&hl->ip, tmp));
      fprintf(hf, "%s ", mac_addr_ntoa(hl->mac, tmp));
      if (hl->hostname && *hl->hostname != '\0')
         fprintf(hf, "%s\n", hl->hostname);
      else
         fprintf(hf, "-\n");
      nhosts++;
   }

   /* close the file */
   fclose(hf);

   INSTANT_USER_MSG("%d hosts saved to file %s\n", nhosts, filename);

   return ESUCCESS;
}


/*
 * add an host to the list
 * order the list while inserting the elements
 */
void add_host(struct ip_addr *ip, u_int8 mac[MEDIA_ADDR_LEN], char *name)
{
   struct hosts_list *hl, *h;

   SAFE_CALLOC(h, 1, sizeof(struct hosts_list));

   /* fill the struct */
   memcpy(&h->ip, ip, sizeof(struct ip_addr));
   memcpy(&h->mac, mac, MEDIA_ADDR_LEN);

   if (name)
      h->hostname = strdup(name);

   /* insert in order (ascending) */
   LIST_FOREACH(hl, &GBL_HOSTLIST, next) {

      if (ip_addr_cmp(&h->ip, &hl->ip) == 0) {
         /* the ip was already collected skip it */
         SAFE_FREE(h->hostname);
         SAFE_FREE(h);
         return;
      } else if (ip_addr_cmp(&hl->ip, &h->ip) < 0 && LIST_NEXT(hl, next) != LIST_END(&GBL_HOSTLIST) )
         continue;
      else if (ip_addr_cmp(&h->ip, &hl->ip) > 0) {
         LIST_INSERT_AFTER(hl, h, next);
         break;
      } else {
         LIST_INSERT_BEFORE(hl, h, next);
         break;
      }

   }

   /* the first element */
   if (LIST_FIRST(&GBL_HOSTLIST) == LIST_END(&GBL_HOSTLIST))
      LIST_INSERT_HEAD(&GBL_HOSTLIST, h, next);

}


/*
 * insert the element in the list randomly.
 * 'max' is the number of elements in the list
 */
static void random_list(struct ip_list *e, int max)
{
   int rnd;

   srand(time(NULL));

   /* calculate the position in the list. */
   rnd = rand() % ((max == 1) ? max : max - 1);

   //rnd = 1+(int) ((float)max*rand()/(RAND_MAX+1.0));

   /* allocate the array used to keep track of the pointer
    * to the elements in the list. this array speed up the
    * access method to the list
    */
   SAFE_REALLOC(rand_array, (max + 1) * sizeof(struct ip_addr *));

   /* the first element */
   if (LIST_FIRST(&ip_list_head) == LIST_END(&ip_list_head)) {
      LIST_INSERT_HEAD(&ip_list_head, e, next);
      rand_array[0] = e;
      return;
   }

   /* bound checking */
   rnd = (rnd > 1) ? rnd : 1;

   /* insert the element in the list */
   LIST_INSERT_AFTER(rand_array[rnd - 1], e, next);
   /* and add the pointer in the array */
   rand_array[max - 1] = e;

}

/* EOF */

// vim:ts=3:expandtab

