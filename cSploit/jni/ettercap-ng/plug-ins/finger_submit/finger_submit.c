/*
    finger_submit -- ettercap plugin -- submit a fingerprint to ettercap website

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
#include <ec_fingerprint.h>

#include <stdlib.h>
#include <string.h>

/* globals */


/* protos */
int plugin_load(void *);
static int finger_submit_init(void *);
static int finger_submit_fini(void *);


/* plugin operations */

struct plugin_ops finger_submit_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "finger_submit",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Submit a fingerprint to ettercap's website",  
   /* the plugin version. */
   .version =           "1.0",   
   /* activation function */
   .init =              &finger_submit_init,
   /* deactivation function */                     
   .fini =              &finger_submit_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &finger_submit_ops);
}

/******************* STANDARD FUNCTIONS *******************/

static int finger_submit_init(void *dummy) 
{
   char finger[FINGER_LEN + 1];
   char os[OS_LEN + 1];
   
   /* don't display messages while operating */
   GBL_OPTIONS->quiet = 1;
 
   memset(finger, 0, sizeof(finger));
   memset(os, 0, sizeof(finger));
   
   /* get the user input */
   ui_input("Fingerprint      ('quit' to exit) : ", finger, sizeof(finger), NULL);
   
   /* exit on user request */
   if (!strcasecmp(finger, "quit") || !strcmp(finger, ""))
      return PLUGIN_FINISHED;
   
   ui_input("Operating System ('quit' to exit) : ", os, sizeof(os), NULL);

   /* exit on user request */
   if (!strcasecmp(os, "quit") || !strcmp(os, ""))
      return PLUGIN_FINISHED;
   
   USER_MSG("\n");

   /* send the fingerprint */
   fingerprint_submit(finger, os);

   /* flush all the messages */
   ui_msg_flush(MSG_ALL);
   
   return PLUGIN_FINISHED;
}


static int finger_submit_fini(void *dummy) 
{
   return PLUGIN_FINISHED;
}


/* EOF */

// vim:ts=3:expandtab
 
