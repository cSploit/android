/*
    remote_browser -- ettercap plugin -- send to the browser the sniffed websites

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
#include <ec_file.h>
#include <ec_hook.h>
#include <ec_inet.h>

#include <stdlib.h>
#include <string.h>

/* protos */

int plugin_load(void *);
static int remote_browser_init(void *);
static int remote_browser_fini(void *);
static void remote_browser(struct packet_object *po);
static int good_page(char *str);

/* plugin operations */

struct plugin_ops remote_browser_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "remote_browser",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Sends visited URLs to the browser",  
   /* the plugin version. */ 
   .version =           "1.2",   
   /* activation function */
   .init =              &remote_browser_init,
   /* deactivation function */                     
   .fini =              &remote_browser_fini,
};

/**********************************************************/

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   return plugin_register(handle, &remote_browser_ops);
}

/*********************************************************/

static int remote_browser_init(void *dummy) 
{
   /* 
    * add the hook in the dissector.
    */
   hook_add(HOOK_PROTO_HTTP, &remote_browser);
   
   return PLUGIN_RUNNING;
}


static int remote_browser_fini(void *dummy) 
{
   /* remove the hook */
   hook_del(HOOK_PROTO_HTTP, &remote_browser);

   return PLUGIN_FINISHED;
}

/* 
 * parse the packet and send the fake reply
 */
static void remote_browser(struct packet_object *po)
{
   char *tmp, *p, *q;
   char *url, *host;
   char *command;
   char **param = NULL;
   int i = 0, k = 0;
   
   /* the client is making a request */
   if (po->DATA.disp_len != 0 && strstr((const char*)po->DATA.disp_data, "GET")) {
      /* I'm the sender, opening a browser with a request coming by me will trigger a loop in this function! */
      if(ip_addr_is_ours(&po->L3.src) == EFOUND || ip_addr_is_ours(&po->L3.src) == EBRIDGE)
         return;

      /* I'm not the sender, I can safely open the browser, the GET triggered by it shouldn't cause bad effects */
      tmp = strdup((const char*)po->DATA.disp_data);

      /* get the Host: directive */
      host = strstr(tmp, "Host: ");
      if (host != NULL) {
         host = host + 6; // 6 is like strlen("Host: ");
         if ((p = strstr(host, "\r\n")) != NULL)
            *p = 0;
      } else
         goto bad;
      
      /* null terminate the request before the HTTP/x.x */
      p = strstr(tmp, " HTTP");
      if (p != NULL)
         *p = 0;
      else
         goto bad;
     
      /* get the requested url */
     url = tmp + 4; // 4 is like strlen("GET ");
      
      /* parse only pages, not images or other amenities */
      if (!good_page(url))
         goto bad;
           
      /* fill the command */
      command = strdup(GBL_CONF->remote_browser);
      str_replace(&command, "%host", host);
      str_replace(&command, "%url", url);
      
      USER_MSG("REMOTE COMMAND: %s\n", command);
      
      /* split the string in the parameter array */
      for (p = ec_strtok(command, " ", &q); p != NULL; p = ec_strtok(NULL, " ", &q)) {
         /* allocate the array */
         SAFE_REALLOC(param, (i + 1) * sizeof(char *));
                        
         /* copy the tokens in the array */
         param[i++] = strdup(p);
      }
   
      /* NULL terminate the array */
      SAFE_REALLOC(param, (i + 1) * sizeof(char *));
      param[i] = NULL;
      /* execute the script */ 
      if (fork() == 0) {
         /* chrome won't start as root, changing UID in order to prevent this and for more security in the browser context */
         /* the following line has been commented since some Penetration Testing distros can run only as root */
         /*setuid(1000);*/
         u_int uid, gid;
         /* XXX should we drop privileges under windows? */
         DEBUG_MSG("drop_privs: getuid(%d) \n", getuid());

         /* are we root ? */
         if (getuid() == 0)
         {
            gid = uid = 1000;
            DEBUG_MSG("drop_privs: setuid(%d) setgid(%d)\n", uid, gid);

            /* drop to a good uid/gid ;) */
            if ( setgid(gid) < 0 )
               DEBUG_MSG("setgid() FAILED\n");
            if ( setuid(uid) < 0 )
               DEBUG_MSG("setuid() FAILED\n");
            DEBUG_MSG("privs: UID: %d %d  GID: %d %d\n", (int)getuid(), (int)geteuid(), (int)getgid(), (int)getegid() );
            DEBUG_MSG("Privileges dropped to UID %d GID %d...\n\n", (int)getuid(), (int)getgid() );
         /* "nobody" cannot open a browser */
         } else if(getuid() == 65535)
            WARN_MSG("your ec_gid and ec_uid in etter.conf file are set to nobody (65535), you probably cannot open a new browser\n");

         execvp(param[0], param);
         WARN_MSG("Cannot launch the default browser (command: %s), please edit your etter.conf file and put a valid value in remote_browser field\n", GBL_CONF->remote_browser);
         _exit(EINVALID);
      }
         
      //to free the char **param
      for(k= 0; k < i; ++k)
    	  SAFE_FREE(param[k]);
      SAFE_FREE(param);

      SAFE_FREE(command);
bad:
      SAFE_FREE(tmp);
   }
   
}

/*
 * return true if the requested URL has to be passed to
 * the REMOTE_COMMAND
 */
static int good_page(char *str)
{
   int i;
   char *suffixes[] = { ".htm", ".html", ".shtml", ".phtml", ".dhtml",
                        ".php", ".asp", ".pl", ".py", ".jsp", NULL};
   
   /* special case if we are requesting the root */
   if (!strcmp(str, "/"))
      return 1;

   /* we are requesting a directory */
   if (str[strlen(str)-1] == '/')
      return 1;
         
   /* search a valid suffix */
   for (i = 0; suffixes[i]; i++) {
      if (strcasestr(str, suffixes[i])) {
         printf("suff %s\n", suffixes[i]);
         return 1;
      }
   }

   return 0;
}

/* EOF */

// vim:ts=3:expandtab

