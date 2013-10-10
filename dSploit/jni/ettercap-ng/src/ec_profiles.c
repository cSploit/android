/*
    ettercap -- host profiling module

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

    $Id: ec_profiles.c,v 1.43 2004/12/21 20:27:15 alor Exp $
*/

#include <ec.h>
#include <ec_threads.h>
#include <ec_passive.h>
#include <ec_profiles.h>
#include <ec_packet.h>
#include <ec_hook.h>
#include <ec_scan.h>
#include <ec_log.h>

#define ONLY_REMOTE_PROFILES  3
#define ONLY_LOCAL_PROFILES   2

/* protos */

void __init profiles_init(void);

static void profile_purge(int flag);
void profile_purge_local(void);
void profile_purge_remote(void);
void profile_purge_all(void);
int profile_convert_to_hostlist(void);
int profile_dump_to_file(char *filename);

void profile_parse(struct packet_object *po);
static int profile_add_host(struct packet_object *po);
static int profile_add_user(struct packet_object *po);
static void update_info(struct host_profile *h, struct packet_object *po);
static void update_port_list(struct host_profile *h, struct packet_object *po);
static void set_gateway(u_char *L2_addr);

void * profile_print(int mode, void *list, char **desc, size_t len);

/* global mutex on interface */

static pthread_mutex_t profile_mutex = PTHREAD_MUTEX_INITIALIZER;
#define PROFILE_LOCK     do { pthread_mutex_lock(&profile_mutex); } while(0)
#define PROFILE_UNLOCK   do { pthread_mutex_unlock(&profile_mutex); } while(0)

/************************************************/
  
/*
 * add the hook function
 */
void __init profiles_init(void)
{
   /* add the hook for the ARP packets */
   hook_add(HOOK_PACKET_ARP, &profile_parse);
   
   /* add the hook for ICMP packets */
   hook_add(HOOK_PACKET_ICMP, &profile_parse);
   
   /* add the hook for DHCP packets */
   hook_add(HOOK_PROTO_DHCP_PROFILE, &profile_parse);
         
   /* receive all the top half packets */
   hook_add(HOOK_DISPATCHER, &profile_parse);
}


/*
 * decides if the packet has to be added
 * to the profiles
 */
void profile_parse(struct packet_object *po)
{

   /* we don't want profiles in memory. */
   if (!GBL_CONF->store_profiles) {
      return;
   }
   
   /* 
    * skip packet sent (spoofed) by us
    * else we will get duplicated hosts with our mac address
    * this is necessary because check_forwarded() is executed
    * in ec_ip.c, but here we are getting even arp packets...
    */
   EXECUTE(GBL_SNIFF->check_forwarded, po);
   if (po->flags & PO_FORWARDED)
      return;

   /*
    * recheck if the packet is compliant with the visualization filters.
    * we need to redo the test, because here we are hooked to ARP and ICMP
    * packets that are before the test in ec_decode.c
    */
   po->flags |= PO_IGNORE;
   EXECUTE(GBL_SNIFF->interesting, po);
   if ( po->flags & PO_IGNORE )
       return;
   
   /*
    * call the add function only if the packet
    * is interesting...
    *    - ARP packets
    *    - ICMP packets
    *    - DNS packets that contain GW information (they use fake icmp po)
    */
   if ( po->L3.proto == htons(LL_TYPE_ARP) ||                     /* arp packets */
        po->L4.proto == NL_TYPE_ICMP                              /* icmp packets */
      )
      profile_add_host(po);
      
   /*  
    * we don't want to log conversations, only
    * open ports, OSes etc etc ;)
    */
   if ( is_open_port(po->L4.proto, po->L4.src, po->L4.flags) ||   /* src port is open */
        strcmp(po->PASSIVE.fingerprint, "") ||                    /* collected fingerprint */  
        po->DISSECTOR.banner                                      /* banner */
      )
      profile_add_host(po);

   /* 
    * usernames and passwords are to be bound to 
    * destination host, not source.
    * do it here, search for the right host and add
    * the username to the right port.
    */
   if ( po->DISSECTOR.user ||                               /* user */
        po->DISSECTOR.pass ||                               /* pass */
        po->DISSECTOR.info                                  /* info */
      )
      profile_add_user(po);
   
   return;
}


/* 
 * add the infos to the profiles tables
 * return the number of hosts added (1 if added, 0 if updated)
 */
static int profile_add_host(struct packet_object *po)
{
   struct host_profile *h;
   struct host_profile *c;
   struct host_profile *last = NULL;
   
   /* 
    * do not store profiles for hosts with ip == 0.0.0.0
    * they are hosts requesting for a dhcp/bootp reply.
    * they will get an ip address soon and we are interested
    * only in the latter.
    */
   if (ip_addr_is_zero(&po->L3.src))
      return 0;
   
   /* 
    * if the type is FP_HOST_NONLOCAL 
    * search for the GW and mark it
    */
   if (po->PASSIVE.flags & FP_HOST_NONLOCAL) {
      set_gateway(po->L2.src);
      /* the mac address of non local should not be saved */
      memset(&po->L2.src, 0, MEDIA_ADDR_LEN);
   }

   PROFILE_LOCK;

   /* search if it already exists */
   TAILQ_FOREACH(h, &GBL_PROFILES, next) {
      /* an host is identified by the mac and the ip address */
      /* if the mac address is null also update it since it could
       * be captured as a DHCP packet specifying the GW 
       */
      if ((!memcmp(h->L2_addr, po->L2.src, MEDIA_ADDR_LEN) ||
           !memcmp(po->L2.src, "\x00\x00\x00\x00\x00\x00", MEDIA_ADDR_LEN) ) &&
          !ip_addr_cmp(&h->L3_addr, &po->L3.src) ) {

         update_info(h, po);
         /* the host was already in the list
          * return 0 host added */
         PROFILE_UNLOCK;
         return 0;
      }
   }
  
   PROFILE_UNLOCK;
  
   DEBUG_MSG("profile_add_host %x", ip_addr_to_int32(&po->L3.src.addr));
   
   /* 
    * the host was not found, create a new entry 
    * before the creation check if it has to be stored...
    */
   
   /* this is a local and we want only remote */
   if ((po->PASSIVE.flags & FP_HOST_LOCAL) && (GBL_CONF->store_profiles == ONLY_REMOTE_PROFILES))
      return 0;
   
   /* this is remote and we want only local */
   if ((po->PASSIVE.flags & FP_HOST_NONLOCAL) && (GBL_CONF->store_profiles == ONLY_LOCAL_PROFILES))
      return 0;
   
   /* create the new host */
   SAFE_CALLOC(h, 1, sizeof(struct host_profile));
   
   PROFILE_LOCK;
   
   /* fill the structure with the collected infos */
   update_info(h, po);
   
   /* search the right point to inser it (ordered ascending) */
   TAILQ_FOREACH(c, &GBL_PROFILES, next) {
      if ( ip_addr_cmp(&c->L3_addr, &h->L3_addr) > 0 )
         break;
      last = c;
   }
   
   if (TAILQ_FIRST(&GBL_PROFILES) == NULL) 
      TAILQ_INSERT_HEAD(&GBL_PROFILES, h, next);
   else if (c != NULL) 
      TAILQ_INSERT_BEFORE(c, h, next);
   else 
      TAILQ_INSERT_AFTER(&GBL_PROFILES, last, h, next);

   PROFILE_UNLOCK;
   
   DEBUG_MSG("profile_add_host: ADDED");
   
   return 1;   
}

/* set the info in a host profile */

static void update_info(struct host_profile *h, struct packet_object *po)
{
   
   /* if it is marked as the gateway or unkown, don't update */
   if ( !(h->type & FP_GATEWAY) && !(h->type & FP_UNKNOWN) )
      h->type = po->PASSIVE.flags;
   
   /* update the mac address only if local or unknown */
   if (h->type & FP_HOST_LOCAL || h->type == FP_UNKNOWN)
      memcpy(h->L2_addr, po->L2.src, MEDIA_ADDR_LEN);
   
   /* the ip address */
   memcpy(&h->L3_addr, &po->L3.src, sizeof(struct ip_addr));

   /* the distance in HOP */
   if (po->L3.ttl > 1)
      h->distance = TTL_PREDICTOR(po->L3.ttl) - po->L3.ttl + 1;
   else
      h->distance = po->L3.ttl;
      
   /* get the hostname */
   host_iptoa(&po->L3.src, h->hostname);
   
   /* 
    * update the fingerprint only if there isn't a previous one
    * or if the previous fingerprint was an ACK
    * fingerprint. SYN fingers are more reliable
    */
   if (po->PASSIVE.fingerprint[FINGER_TCPFLAG] != '\0' &&
        (h->fingerprint[FINGER_TCPFLAG] == '\0' || 
         h->fingerprint[FINGER_TCPFLAG] == 'A') )
      memcpy(h->fingerprint, po->PASSIVE.fingerprint, FINGER_LEN);

   /* add the open port */
   update_port_list(h, po);
}


/* 
 * search the host with this L2_addr
 * and mark it as the GW
 */

static void set_gateway(u_char *L2_addr)
{
   struct host_profile *h;

   /* skip null mac addresses */
   if (!memcmp(L2_addr, "\x00\x00\x00\x00\x00\x00", MEDIA_ADDR_LEN))
      return;
   
   PROFILE_LOCK;

   TAILQ_FOREACH(h, &GBL_PROFILES, next) {
      if (!memcmp(h->L2_addr, L2_addr, MEDIA_ADDR_LEN) ) {
         h->type |= FP_GATEWAY; 
         PROFILE_UNLOCK;
         return;
      }
   }
   
   PROFILE_UNLOCK;
}

/* 
 * update the list of open ports
 */
   
static void update_port_list(struct host_profile *h, struct packet_object *po)
{
   struct open_port *o;
   struct open_port *p;
   struct open_port *last = NULL;

   /* search for an existing port */
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      if (o->L4_proto == po->L4.proto && o->L4_addr == po->L4.src) {
         /* set the banner for the port */
         if (o->banner == NULL && po->DISSECTOR.banner)
            o->banner = strdup(po->DISSECTOR.banner);

         /* already logged */
         return;
      }
   }
  
   /* skip this port, the packet was logged for
    * another reason, not the open port */
   if ( !is_open_port(po->L4.proto, po->L4.src, po->L4.flags) )
      return;

   DEBUG_MSG("update_port_list");
   
   /* create a new entry */
   SAFE_CALLOC(o, 1, sizeof(struct open_port));
   
   o->L4_proto = po->L4.proto;
   o->L4_addr = po->L4.src;
   
   /* search the right point to inser it (ordered ascending) */
   LIST_FOREACH(p, &(h->open_ports_head), next) {
      if ( ntohs(p->L4_addr) > ntohs(o->L4_addr) )
         break;
      last = p;
   }

   /* insert in the right position */
   if (LIST_FIRST(&(h->open_ports_head)) == NULL) 
      LIST_INSERT_HEAD(&(h->open_ports_head), o, next);
   else if (p != NULL) 
      LIST_INSERT_BEFORE(p, o, next);
   else 
      LIST_INSERT_AFTER(last, o, next);
   
}

/* 
 * update the users list
 */

static int profile_add_user(struct packet_object *po)
{
   struct host_profile *h;
   struct open_port *o = NULL;
   struct active_user *u;
   struct active_user *a;
   struct active_user *last = NULL;
   int found = 0;

   /* no info to update */
   if (po->DISSECTOR.user == NULL || po->DISSECTOR.pass == NULL)
      return 0;
  
   DEBUG_MSG("profile_add_user");
  
   PROFILE_LOCK; 
   
   /* search the right port on the right host */
   TAILQ_FOREACH(h, &GBL_PROFILES, next) {
      
      /* right host */
      if ( !ip_addr_cmp(&h->L3_addr, &po->L3.dst) ) {
      
         LIST_FOREACH(o, &(h->open_ports_head), next) {
            /* right port and proto */
            if (o->L4_proto == po->L4.proto && o->L4_addr == po->L4.dst) {
               found = 1;
               break;
            }
         }
      }
      /* if already found, exit the loop */
      if (found) 
         break;
   }
   
   /* 
    * the port was not found... possible ?
    * yes, but extremely rarely.
    * don't worry, we have lost this for now, 
    * but the next time it will be captured.
    */
   if (!found || o == NULL) {
      PROFILE_UNLOCK;
      return 0;
   }
   
   /* search if the user was already logged */ 
   LIST_FOREACH(u, &(o->users_list_head), next) {
      if (!strcmp(u->user, po->DISSECTOR.user) && 
          !strcmp(u->pass, po->DISSECTOR.pass) &&
          !ip_addr_cmp(&u->client, &po->L3.src)) {
         PROFILE_UNLOCK;
         return 0;
      }
   }
   
   SAFE_CALLOC(u, 1, sizeof(struct active_user));
   
   /* if there are infos copy it, else skip */
   if (po->DISSECTOR.user && po->DISSECTOR.pass) {
      u->user = strdup(po->DISSECTOR.user);
      u->pass = strdup(po->DISSECTOR.pass);
      u->failed = po->DISSECTOR.failed;
      /* save the source of the connection */
      memcpy(&u->client, &po->L3.src, sizeof(struct ip_addr));
   } else {
      SAFE_FREE(u);
      PROFILE_UNLOCK;
      return 0;
   }
  
   if (po->DISSECTOR.info)
      u->info = strdup(po->DISSECTOR.info);
  
   /* search the right point to inser it (ordered alphabetically) */
   LIST_FOREACH(a, &(o->users_list_head), next) {
      if ( strcmp(a->user, u->user) > 0 )
         break;
      last = a;
   }
   
   /* insert in the right position */
   if (LIST_FIRST(&(o->users_list_head)) == NULL) 
      LIST_INSERT_HEAD(&(o->users_list_head), u, next);
   else if (a != NULL) 
      LIST_INSERT_BEFORE(a, u, next);
   else 
      LIST_INSERT_AFTER(last, u, next);
   
   PROFILE_UNLOCK;
   
   return 1;
}

/*
 * purge local hosts from the list
 */
void profile_purge_local(void)
{
   DEBUG_MSG("profile_purge_local");
   profile_purge(FP_HOST_LOCAL);
   return;
}

/*
 * purge local hosts from the list
 */
void profile_purge_remote(void)
{
   DEBUG_MSG("profile_purge_remote");
   profile_purge(FP_HOST_NONLOCAL);
   return;
}

/*
 * purge all the host list 
 */
void profile_purge_all(void)
{
   DEBUG_MSG("profile_purge_all");
   profile_purge( FP_HOST_LOCAL | FP_HOST_NONLOCAL );
   return;
}

/*
 * do the actual elimination 
 */
static void profile_purge(int flags)
{
   struct host_profile *h, *tmp_h = NULL;
   struct open_port *o, *tmp_o = NULL;
   struct active_user *u, *tmp_u = NULL;
   
   PROFILE_LOCK;

   TAILQ_FOREACH_SAFE(h, &GBL_PROFILES, next, tmp_h) {

      /* the host matches the flags */
      if (h->type & flags) {
         /* free all the alloc'd ports */
         LIST_FOREACH_SAFE(o, &(h->open_ports_head), next, tmp_o) {
            
            SAFE_FREE(o->banner);
            
            LIST_FOREACH_SAFE(u, &(o->users_list_head), next, tmp_u) {
               /* free the current infos */
               SAFE_FREE(u->user);
               SAFE_FREE(u->pass);
               SAFE_FREE(u->info);
               /* remove from the list */
               LIST_REMOVE(u, next);
               SAFE_FREE(u);
            }
            LIST_REMOVE(o, next);
            SAFE_FREE(o);
         }
         TAILQ_REMOVE(&GBL_PROFILES, h, next);
         SAFE_FREE(h);
      }
   }
   
   PROFILE_UNLOCK;
}

/*
 * convert the LOCAL profiles into the hosts list 
 * (created by the initial scan)
 * this is useful to start in silent mode and collect
 * the list in passive mode.
 */

int profile_convert_to_hostlist(void)
{
   struct host_profile *h;
   int count = 0;

   /* first, delete the hosts list */
   del_hosts_list();

   /* now parse the profile list and create the hosts list */
   PROFILE_LOCK;

   TAILQ_FOREACH(h, &GBL_PROFILES, next) {
      /* add only local hosts */
      if (h->type & FP_HOST_LOCAL) {
         /* the actual add */
         add_host(&h->L3_addr, h->L2_addr, h->hostname);
         count++;
      }
   }

   PROFILE_UNLOCK;
   
   return count;
}


/*
 * fill the desc and return the next/prev element
 */
void * profile_print(int mode, void *list, char **desc, size_t len)
{
   struct host_profile *h = (struct host_profile *)list;
   struct host_profile *hl;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* NULL is used to retrieve the first element */
   if (list == NULL)
      return TAILQ_FIRST(&GBL_PROFILES);

   /* the caller wants the description */
   if (desc != NULL) {
      struct open_port *o;
      struct active_user *u;
      int found = 0;
         
      /* search at least one account */
      LIST_FOREACH(o, &(h->open_ports_head), next) {
         LIST_FOREACH(u, &(o->users_list_head), next) {
            found = 1;
         }
      }
      
      ip_addr_ntoa(&h->L3_addr, tmp);
      snprintf(*desc, len, "%c %15s   %s", (found) ? '*' : ' ', 
                                           tmp, 
                                           (h->hostname) ? h->hostname : "" );
   }
  
   /* return the next/prev/current to the caller */
   switch (mode) {
      case -1:
         return TAILQ_PREV(h, gbl_ptail, next);
         break;
      case +1:
         return TAILQ_NEXT(h, next);
         break;
      case 0:
         /* if exists in the list, return it */
         TAILQ_FOREACH(hl, &GBL_PROFILES, next) {
            if (hl == h)
               return h;
         }
         /* else, return NULL */
         return NULL;
         break;
      default:
         return list;
         break;
   }
         
   return NULL;
}

/*
 * dump the whole profile list into an eci file
 */
int profile_dump_to_file(char *filename)
{
   struct log_fd fd;
   char eci[strlen(filename)+5];
   struct host_profile *h;
   struct open_port *o;
   struct active_user *u;
   struct packet_object po;
  
   DEBUG_MSG("profile_dump_to_file: %s", filename);

   /* append the extension */
   sprintf(eci, "%s.eci", filename);
   
   if (GBL_OPTIONS->compress)
      fd.type = LOG_COMPRESSED;
   else
      fd.type = LOG_UNCOMPRESSED;
        
   /* open the file for dumping */
   if (log_open(&fd, eci) != ESUCCESS)
      return -EFATAL;

   /* this is an INFO file */
   log_write_header(&fd, LOG_INFO);

   /* now parse the profile list and dump to the file */
   PROFILE_LOCK;

   TAILQ_FOREACH(h, &GBL_PROFILES, next) {
      
      memset(&po, 0, sizeof(struct packet_object));
      
      /* create the po for logging */
      memcpy(&po.L2.src, h->L2_addr, MEDIA_ADDR_LEN);
      memcpy(&po.L3.src, &h->L3_addr, sizeof(struct ip_addr));

      /* fake the distance by subtracting it from a power of 2 */
      po.L3.ttl = 128 - h->distance + 1;
      po.PASSIVE.flags = h->type;
      memcpy(&po.PASSIVE.fingerprint, h->fingerprint, FINGER_LEN);
      
      /* log for each host */
      log_write_info_arp_icmp(&fd, &po);
      
      /* log the info. needed to record the fingerprint.
       * the above function will not log it 
       */
      log_write_info(&fd, &po);
      
      LIST_FOREACH(o, &(h->open_ports_head), next) {
         
         memcpy(&po.L2.src, h->L2_addr, MEDIA_ADDR_LEN);
         memcpy(&po.L3.src, &h->L3_addr, sizeof(struct ip_addr));
         memset(&po.PASSIVE.fingerprint, 0, FINGER_LEN);
         
         po.L4.src = o->L4_addr;
         /* put the fake syn+ack to impersonate an open port */
         po.L4.flags = TH_SYN | TH_ACK;
         po.L4.proto = o->L4_proto;
         
         /* log the packet for the open port */
         log_write_info(&fd, &po);
        
         po.DISSECTOR.banner = o->banner;
         
         /* log for the banner */
         if (o->banner)
            log_write_info(&fd, &po);
      
         LIST_FOREACH(u, &(o->users_list_head), next) {

            memcpy(&po.L3.dst, &h->L3_addr, sizeof(struct ip_addr));
            /* the source addr is the client address */
            memcpy(&po.L3.src, &u->client, sizeof(struct ip_addr));
        
            /* to exclude the open port check */
            po.L4.flags = TH_PSH;
            po.L4.dst = o->L4_addr;
            po.L4.src = 0;
            
            po.DISSECTOR.user = u->user;
            po.DISSECTOR.pass = u->pass;
            po.DISSECTOR.info = u->info;
            po.DISSECTOR.failed = u->failed;

            /* log for each account:
             * the host must be on the dest
             */
            log_write_info(&fd, &po);
         }
      }
   }

   PROFILE_UNLOCK;
   
   /* close the file */
   log_close(&fd);

   return ESUCCESS;
}

/* EOF */

// vim:ts=3:expandtab

