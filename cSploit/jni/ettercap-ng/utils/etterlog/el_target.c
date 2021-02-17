/*
    etterlog -- target filtering module

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

    $Id: el_target.c,v 1.17 2004/06/25 14:24:30 alor Exp $
*/

#include <el.h>
#include <el_functions.h>

/* proto */

void target_compile(char *target);
static void add_port(void *ports, int n);
static void add_ip(void *digit, int n);
static void expand_range_ip(char *str, void *target);
int cmp_ip_list(struct ip_addr *ip, struct target_env *t);
void add_ip_list(struct ip_addr *ip, struct target_env *t);

int is_target_pck(struct log_header_packet *pck);
int is_target_info(struct host_profile *hst);

int find_user(struct host_profile *hst, char *user);

/*******************************************/


void target_compile(char *target)
{
#define MAX_TOK 3
   char valid[] = "1234567890/.,-;:ABCDEFabcdef";
   char *tok[MAX_TOK];
   char *p;
   int i = 0;

   /* sanity check */ 
   if (strlen(target) != strspn(target, valid))
      FATAL_ERROR("TARGET contains invalid chars !");

   /* TARGET parsing */
   for(p=strsep(&target, "/"); p != NULL; p=strsep(&target, "/")) {
      tok[i++] = strdup(p);
      /* bad parsing */
      if (i > MAX_TOK) break;
   }

   if (i != MAX_TOK)
      FATAL_ERROR("Incorrect number of token (//) in TARGET !!");

   /* reset the target */
   GBL_TARGET->all_mac = 0;
   GBL_TARGET->all_ip = 0;
   GBL_TARGET->all_port = 0;

   /* set the mac address */
   if (!strcmp(tok[0], ""))
      GBL_TARGET->all_mac = 1;
   else if (mac_addr_aton(tok[0], GBL_TARGET->mac) == 0)
      FATAL_ERROR("Incorrect TARGET MAC parsing... (%s)", tok[0]);

   /* parse the IP range */
   if (!strcmp(tok[1], ""))
      GBL_TARGET->all_ip = 1;
   else
      for(p=strsep(&tok[1], ";"); p != NULL; p=strsep(&tok[1], ";"))
         expand_range_ip(p, GBL_TARGET);

   /* 
    * expand the range into the port bitmap array
    * 1<<16 is MAX_PORTS 
    */
   if (!strcmp(tok[2], ""))
      GBL_TARGET->all_port = 1;
   else
      expand_token(tok[2], 1<<16, &add_port, GBL_TARGET->ports);

   /* free the data */
   for(i=0; i < MAX_TOK; i++)
      SAFE_FREE(tok[i]);
                        
}

static void add_port(void *ports, int n)
{
   u_int8 *bitmap = ports;

   if (n > 1<<16)
      FATAL_ERROR("Port outside the range (65535) !!");

   BIT_SET(bitmap, n);
}


/*
 * this structure is used to contain all the possible
 * value of a token.
 * it is used as a digital clock.
 * an impulse is made to the last digit and it increment
 * its value, when it reach the maximum, it reset itself 
 * and gives an impulse to the second to last digit.
 * the impulse is propagated till the first digit so all
 * the values are displayed as in a daytime from 00:00 to 23:59
 */

struct digit {
   int n;
   int cur;
   u_char values[0xff];
};


/* 
 * prepare the set of 4 digit to create an IP address
 */

static void expand_range_ip(char *str, void *target)
{
   struct digit ADDR[4];
   struct ip_addr tmp;
   struct in_addr ipaddr;
   char *addr[4];
   char parsed_ip[16];
   char *p, *q;
   int i = 0, j;
   int permut = 1;
   char *tok;
                     
   memset(&ADDR, 0, sizeof(ADDR));

   p = str;

   /* tokenize the ip into 4 slices */
   while ( (q = ec_strtok(p, ".", &tok)) ) {
      addr[i++] = strdup(q);
      /* reset p for the next strtok */
      if (p != NULL) p = NULL;
      if (i > 4) break;
   }

   if (i != 4)
      FATAL_ERROR("Invalid IP format !!");

   for (i = 0; i < 4; i++) {
      p = addr[i];
      expand_token(addr[i], 255, &add_ip, &ADDR[i]);
   }

   /* count the free permutations */
   for (i = 0; i < 4; i++) 
      permut *= ADDR[i].n;

   /* give the impulses to the last digit */
   for (i = 0; i < permut; i++) {

      sprintf(parsed_ip, "%d.%d.%d.%d",  ADDR[0].values[ADDR[0].cur],
                                         ADDR[1].values[ADDR[1].cur],
                                         ADDR[2].values[ADDR[2].cur],
                                         ADDR[3].values[ADDR[3].cur]);

      if (inet_aton(parsed_ip, &ipaddr) == 0)
         FATAL_ERROR("Invalid IP address (%s)", parsed_ip);

      ip_addr_init(&tmp, AF_INET,(char *)&ipaddr );
      add_ip_list(&tmp, target);
      
      /* give the impulse to the last octet */ 
      ADDR[3].cur++;

      /* adjust the other digits as in a digital clock */
      for (j = 2; j >= 0; j--) {    
         if ( ADDR[j+1].cur >= ADDR[j+1].n  ) {
            ADDR[j].cur++;
            ADDR[j+1].cur = 0;
         }
      }
   } 
  
   for (i = 0; i < 4; i++)
      SAFE_FREE(addr[i]);
     
}

/* fill the digit structure with data */
static void add_ip(void *digit, int n)
{
   struct digit *buf = digit;
   
   buf->n++;
   buf->values[buf->n - 1] = (u_char) n;
}


/*
 * add an IP to the list 
 */

void add_ip_list(struct ip_addr *ip, struct target_env *t)
{
   struct ip_list *e;

   SAFE_CALLOC(e, 1, sizeof(struct ip_list));

   memcpy(&e->ip, ip, sizeof(struct ip_addr));

   /* insert it at the beginning of the list */
   SLIST_INSERT_HEAD (&t->ips, e, next); 

   return;
}

/*
 * return true if the ip is in the list
 */

int cmp_ip_list(struct ip_addr *ip, struct target_env *t)
{
   struct ip_list *e;

   SLIST_FOREACH (e, &t->ips, next)
      if (!ip_addr_cmp(&(e->ip), ip))
         return 1;

   return 0;
}


/*
 * return true if the packet conform to TARGET
 */

int is_target_pck(struct log_header_packet *pck)
{
   int proto = 0;
   int good = 0;
   
   /* 
    * first check the protocol.
    * if it is not the one specified it is 
    * useless to parse the mac, ip and port
    */

    if (!GBL_TARGET->proto || !strcasecmp(GBL_TARGET->proto, "all"))  
       proto = 1;

    if (GBL_TARGET->proto && !strcasecmp(GBL_TARGET->proto, "tcp") 
          && pck->L4_proto == NL_TYPE_TCP)
       proto = 1;
   
    if (GBL_TARGET->proto && !strcasecmp(GBL_TARGET->proto, "udp") 
          && pck->L4_proto == NL_TYPE_UDP)
       proto = 1;
    
    /* the protocol does not match */
    if (!GBL.reverse && proto == 0)
       return 0;
    
   /*
    * we have to check if the packet is complying with the filter
    * specified by the users.
    */
 
   /* it is in the source */
   if ( (GBL_TARGET->all_mac  || !memcmp(GBL_TARGET->mac, pck->L2_src, MEDIA_ADDR_LEN)) &&
        (GBL_TARGET->all_ip   || cmp_ip_list(&pck->L3_src, GBL_TARGET) ) &&
        (GBL_TARGET->all_port || BIT_TEST(GBL_TARGET->ports, ntohs(pck->L4_src))) )
      good = 1;

   /* it is in the dest */
   if ( (GBL_TARGET->all_mac  || !memcmp(GBL_TARGET->mac, pck->L2_dst, MEDIA_ADDR_LEN)) &&
        (GBL_TARGET->all_ip   || cmp_ip_list(&pck->L3_dst, GBL_TARGET)) &&
        (GBL_TARGET->all_port || BIT_TEST(GBL_TARGET->ports, ntohs(pck->L4_dst))) )
      good = 1;   
  
   /* check the reverse option */
   if (GBL.reverse ^ (good && proto) ) 
      return 1;
      
   
   return 0;
}

/*
 * return 1 if the packet conform to TARGET
 */

int is_target_info(struct host_profile *hst)
{
   struct open_port *o;
   int proto = 0;
   int port = 0;
   int host = 0;
   
   /* 
    * first check the protocol.
    * if it is not the one specified it is 
    * useless to parse the mac, ip and port
    */

   if (!GBL_TARGET->proto || !strcasecmp(GBL_TARGET->proto, "all"))  
      proto = 1;
   
   /* all the ports are good */
   if (GBL_TARGET->all_port && proto)
      port = 1;
   else {
      LIST_FOREACH(o, &(hst->open_ports_head), next) {
    
         if (GBL_TARGET->proto && !strcasecmp(GBL_TARGET->proto, "tcp") 
             && o->L4_proto == NL_TYPE_TCP)
            proto = 1;
   
         if (GBL_TARGET->proto && !strcasecmp(GBL_TARGET->proto, "udp") 
             && o->L4_proto == NL_TYPE_UDP)
            proto = 1;

         /* if the port is open, it matches */
         if (proto && (GBL_TARGET->all_port || BIT_TEST(GBL_TARGET->ports, ntohs(o->L4_addr))) ) {
            port = 1;
            break;
         }
      }
   }

   /*
    * we have to check if the packet is complying with the filter
    * specified by the users.
    */
 
   /* it is in the source */
   if ( (GBL_TARGET->all_mac || !memcmp(GBL_TARGET->mac, hst->L2_addr, MEDIA_ADDR_LEN)) &&
        (GBL_TARGET->all_ip  || cmp_ip_list(&hst->L3_addr, GBL_TARGET) ) )
      host = 1;


   /* check the reverse option */
   if (GBL.reverse ^ (host && port) ) 
      return 1;
   else
      return 0;

}


/* 
 * return ESUCCESS if the user 'user' is in the user list
 */

int find_user(struct host_profile *hst, char *user)
{
   struct open_port *o;
   struct active_user *u;
      
   if (user == NULL)
      return ESUCCESS;
   
   LIST_FOREACH(o, &(hst->open_ports_head), next) {
      LIST_FOREACH(u, &(o->users_list_head), next) {
         if (strcasestr(u->user, user))
            return ESUCCESS;
      }
   }
   
   return -ENOTFOUND;
}




/* EOF */

// vim:ts=3:expandtab

