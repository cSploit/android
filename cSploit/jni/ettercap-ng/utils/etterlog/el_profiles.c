/*
    etterlog -- host profiling module

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

    $Id: el_profiles.c,v 1.17 2004/02/16 20:21:56 alor Exp $
*/

#include <el.h>
#include <ec_log.h>
#include <ec_profiles.h>

/* globals */

TAILQ_HEAD(, host_profile) hosts_list_head = TAILQ_HEAD_INITIALIZER(hosts_list_head);

/* protos */

void *get_host_list_ptr(void);
int profile_add_info(struct log_header_info *inf, struct dissector_info *buf);
static void update_info(struct host_profile *h, struct log_header_info *inf, struct dissector_info *buf);
static void update_port_list(struct host_profile *h, struct log_header_info *inf, struct dissector_info *buf);
static void update_user_list(struct open_port *o, struct log_header_info *inf, struct dissector_info *buf);
static void set_gateway(u_char *L2_addr);

/************************************************/

/*
 * return the pointer to the host list
 */

void *get_host_list_ptr(void)
{
   return &hosts_list_head;
}

/* 
 * creates or updates the host list
 * return the number of hosts added (1 if added, 0 if updated)
 */

int profile_add_info(struct log_header_info *inf, struct dissector_info *buf)
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
   if (ip_addr_is_zero(&inf->L3_addr))
      return 0;
   
   /* 
    * if the type is FP_HOST_NONLOCAL 
    * search for the GW and mark it
    */
   if (inf->type & FP_HOST_NONLOCAL) {
      set_gateway(inf->L2_addr);
      /* the mac address of non local should not be saved */
      memset(inf->L2_addr, 0, MEDIA_ADDR_LEN);
   }

   /* search if it already exists */
   TAILQ_FOREACH(h, &hosts_list_head, next) {
      /* an host is identified by the mac and the ip address */
      /* if the mac address is null also update it since it could
       * be captured as a DHCP packet specifying the GW 
       */
      if ((!memcmp(h->L2_addr, inf->L2_addr, MEDIA_ADDR_LEN) ||
           !memcmp(inf->L2_addr, "\x00\x00\x00\x00\x00\x00", MEDIA_ADDR_LEN) ) &&
          !ip_addr_cmp(&h->L3_addr, &inf->L3_addr) ) {

         update_info(h, inf, buf);
         /* the host was already in the list
          * return 0 host added */
         return 0;
      }
   }
  
   /* the host was not found, create a new entry */
   SAFE_CALLOC(h, 1, sizeof(struct host_profile));
 
   /* update the host info */
   update_info(h, inf, buf);
   
   /* search the right point to inser it (ordered ascending) */
   TAILQ_FOREACH(c, &hosts_list_head, next) {
      if ( ip_addr_cmp(&c->L3_addr, &h->L3_addr) > 0 )
         break;
      last = c;
   }
   
   if (TAILQ_FIRST(&hosts_list_head) == NULL) 
      TAILQ_INSERT_HEAD(&hosts_list_head, h, next);
   else if (c != NULL) 
      TAILQ_INSERT_BEFORE(c, h, next);
   else 
      TAILQ_INSERT_AFTER(&hosts_list_head, last, h, next);

   return 1;   
}

/* set the info in a host profile */

static void update_info(struct host_profile *h, struct log_header_info *inf, struct dissector_info *buf)
{
   /* update the type only if not previously saved */ 
   if (h->type == 0)
      h->type = inf->type;
   
   /* update the mac address only if local or unknown 
    * and only if it is not null
    */
   if (h->type & FP_HOST_LOCAL || h->type == FP_UNKNOWN)
      if (memcmp(inf->L2_addr, "\x00\x00\x00\x00\x00\x00", MEDIA_ADDR_LEN))
         memcpy(h->L2_addr, inf->L2_addr, MEDIA_ADDR_LEN);
   
   /* the ip address */
   memcpy(&h->L3_addr, &inf->L3_addr, sizeof(struct ip_addr));

   /* the distance in HOP */
   if (h->distance == 0)
      h->distance = inf->distance;

   /* copy the hostname */
   strncpy(h->hostname, inf->hostname, MAX_HOSTNAME_LEN);
   
   /* 
    * update the fingerprint only if there isn't a previous one
    * or if the previous fingerprint was an ACK
    * fingerprint. SYN fingers are more reliable
    */
   if (inf->fingerprint[FINGER_TCPFLAG] != '\0' &&
        (h->fingerprint[FINGER_TCPFLAG] == '\0' || 
         h->fingerprint[FINGER_TCPFLAG] == 'A') )
      memcpy(h->fingerprint, inf->fingerprint, FINGER_LEN);

   /* add the open port */
   update_port_list(h, inf, buf);
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
      
   TAILQ_FOREACH(h, &hosts_list_head, next) {
      if (!memcmp(h->L2_addr, L2_addr, MEDIA_ADDR_LEN) ) {
         h->type |= FP_GATEWAY; 
         return;
      }
   }
}

/* 
 * update the list of open ports
 * and add the user and pass infos
 */
   
static void update_port_list(struct host_profile *h, struct log_header_info *inf, struct dissector_info *buf)
{
   struct open_port *o;
   struct open_port *p;
   struct open_port *last = NULL;

   /* search for an existing port */
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      if (o->L4_proto == inf->L4_proto && o->L4_addr == inf->L4_addr) {
         /* set the banner for the port */
         if (o->banner == NULL && buf->banner)
            o->banner = strdup(buf->banner);
         /* update the user info */
         update_user_list(o, inf, buf);
         return;
      }
   }
   
   /* skip this port, the packet was logged for
    * another reason, not the open port */
   if (inf->L4_addr == 0)
      return;

   /* create a new entry */
   SAFE_CALLOC(o, 1, sizeof(struct open_port)); 

   o->L4_proto = inf->L4_proto;
   o->L4_addr = inf->L4_addr;
   
   /* add user and pass */
   update_user_list(o, inf, buf);

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


static void update_user_list(struct open_port *o, struct log_header_info *inf, struct dissector_info *buf)
{
   struct active_user *u;
   struct active_user *a;
   struct active_user *last = NULL;

   /* no info to update */
   if (buf->user == NULL || buf->pass == NULL)
      return;
   
   /* search for an existing user and pass */
   LIST_FOREACH(u, &(o->users_list_head), next) {
      if (!strcmp(u->user, buf->user) && 
          !strcmp(u->pass, buf->pass) &&
          !ip_addr_cmp(&u->client, &inf->client) ) {
         return;
      }
   }
  
   SAFE_CALLOC(u, 1, sizeof(struct active_user));

   /* if there are infos copy it, else skip */
   if (buf->user && buf->pass) {
      u->user = strdup(buf->user);
      u->pass = strdup(buf->pass);
      u->failed = inf->failed;
      memcpy(&u->client, &inf->client, sizeof(struct ip_addr));
   } else {
      SAFE_FREE(u);
      return;
   }
  
   if (buf->info)
      u->info = strdup(buf->info);
  
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
     
}



/* EOF */

// vim:ts=3:expandtab

