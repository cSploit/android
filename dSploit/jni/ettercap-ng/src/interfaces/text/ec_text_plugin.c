/*
    ettercap -- text GUI for plugin management

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

    $Id: ec_text_plugin.c,v 1.8 2004/01/10 14:15:26 alor Exp $
*/

#include <ec.h>
#include <ec_ui.h>
#include <ec_threads.h>
#include <ec_plugins.h>

/* proto */

int text_plugin(char *plugin);
static void text_plugin_list(char active, struct plugin_ops *ops);

/*******************************************/


/* the interface */

int text_plugin(char *plugin)
{
   int type;

   DEBUG_MSG("text_plugin: %s", plugin);
   
   /*
    * if the plugin name is "list", print the 
    * plugin list and exit
    */
   if (!strcasecmp(plugin, "list")) {
      /* delete any previous message */
      ui_msg_purge_all();

      INSTANT_USER_MSG("\nAvailable plugins :\n\n");
      type = plugin_list_walk(PLP_MIN, PLP_MAX, &text_plugin_list);
      if (type == -ENOTFOUND) 
         FATAL_MSG("No plugin found !\n");
      
      INSTANT_USER_MSG("\n\n");
      /* 
       * return an error, so the text interface 
       * ends and returns to main
       */
      return -EINVALID;
   }
   

   /* check if the plugin exists */
   if (search_plugin(plugin) != ESUCCESS)
      FATAL_MSG("%s plugin can not be found !", plugin);
   
   
   if (plugin_is_activated(plugin) == 0)
      INSTANT_USER_MSG("Activating %s plugin...\n\n", plugin);
   else
      INSTANT_USER_MSG("Deactivating %s plugin...\n\n", plugin);
  
   /*
    * pay attention on this !
    * if the plugin init does not return,
    * we are blocked here. So it is encouraged
    * to write plugins which spawn a thread
    * and immediately return
    */
   if (plugin_is_activated(plugin) == 1)
      return plugin_fini(plugin);
   else 
      return plugin_init(plugin);
      
}

/*
 * callback function for displaying the plugin list 
 */
static void text_plugin_list(char active, struct plugin_ops *ops)
{
   INSTANT_USER_MSG("[%d] %15s %4s  %s\n", active, 
         ops->name, ops->version, ops->info);  
}

/* EOF */

// vim:ts=3:expandtab

