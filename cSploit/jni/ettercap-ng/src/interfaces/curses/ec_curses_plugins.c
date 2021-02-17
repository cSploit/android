/*
    ettercap -- curses GUI

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

#include <ec.h>
#include <wdg.h>
#include <ec_curses.h>
#include <ec_plugins.h>

#define MAX_DESC_LEN 75

/* proto */

static void curses_plugin_mgmt(void);
static void curses_plugin_load(void);
static void curses_load_plugin(char *path, char *file);
static void curses_wdg_plugin(char active, struct plugin_ops *ops);
static void curses_refresh_plug_array(char active, struct plugin_ops *ops);
static void curses_plug_destroy(void);
static void curses_select_plugin(void *plugin);
static void curses_create_plug_array(void);
static void curses_plugin_help(void *dummy);

/* globals */

static wdg_t *wdg_plugin;
static struct wdg_list *wdg_plugin_elements;
static size_t nplug;

struct wdg_menu menu_plugins[] = { {"Plugins",            'P',       "",    NULL},
                                   {"Manage the plugins", CTRL('P'), "C-p", curses_plugin_mgmt},
                                   {"Load a plugin...",   0,         "",    curses_plugin_load},
                                   {NULL, 0, NULL, NULL},
                                 };

/*******************************************/

/*
 * display the file open dialog
 */
static void curses_plugin_load(void)
{
   wdg_t *fop;
   
   DEBUG_MSG("curses_plugin_load");
   
   wdg_create_object(&fop, WDG_FILE, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   
   wdg_set_title(fop, "Select a plugin...", WDG_ALIGN_LEFT);
   wdg_set_color(fop, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(fop, WDG_COLOR_WINDOW, EC_COLOR_MENU);
   wdg_set_color(fop, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(fop, WDG_COLOR_TITLE, EC_COLOR_TITLE);

   wdg_file_set_callback(fop, curses_load_plugin);
   
   wdg_draw_object(fop);
   
   wdg_set_focus(fop);
}

static void curses_load_plugin(char *path, char *file)
{
   int ret;

   DEBUG_MSG("curses_load_plugin %s/%s", path, file);

   /* load the plugin */
   ret = plugin_load_single(path, file);

   /* check the return code */
   switch (ret) {
      case ESUCCESS:
         curses_message("Plugin loaded successfully");
         break;
      case -EDUPLICATE:
         ui_error("plugin %s already loaded...", file);
         break;
      case -EVERSION:
         ui_error("plugin %s was compiled for a different ettercap version...", file);
         break;
      case -EINVALID:
      default:
         ui_error("Cannot load the plugin...\nthe file may be an invalid plugin\nor you don't have the permission to open it");
         break;
   }
}

/*
 * plugin management
 */
static void curses_plugin_mgmt(void)
{
   DEBUG_MSG("curses_plugin_mgmt");
   
   /* create the array for the list widget */
   curses_create_plug_array();
   
   /* if the object already exist, set the focus to it */
   if (wdg_plugin) {
      /* set the new array */
      wdg_list_set_elements(wdg_plugin, wdg_plugin_elements);
      return;
   }
   
   wdg_create_object(&wdg_plugin, WDG_LIST, WDG_OBJ_WANT_FOCUS);
   
   wdg_set_size(wdg_plugin, 1, 2, -1, SYSMSG_WIN_SIZE - 1);
   wdg_set_title(wdg_plugin, "Select a plugin...", WDG_ALIGN_LEFT);
   wdg_set_color(wdg_plugin, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_plugin, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_plugin, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(wdg_plugin, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_plugin, WDG_COLOR_TITLE, EC_COLOR_TITLE);

  
   /* set the elements */
   wdg_list_set_elements(wdg_plugin, wdg_plugin_elements);
   
   /* add the destroy callback */
   wdg_add_destroy_key(wdg_plugin, CTRL('Q'), curses_plug_destroy);
  
   /* add the callback */
   wdg_list_select_callback(wdg_plugin, curses_select_plugin);
   wdg_list_add_callback(wdg_plugin, ' ', curses_plugin_help);
     
   wdg_draw_object(wdg_plugin);
   
   wdg_set_focus(wdg_plugin);
}

static void curses_plug_destroy(void)
{
   wdg_plugin = NULL;
}

static void curses_plugin_help(void *dummy)
{
   char help[] = "HELP: shortcut list:\n\n"
                 "  ENTER - activate/deactivate a plugin";

   curses_message(help);
}


/*
 * create the array for the widget.
 * erase any previously alloc'd array 
 */
static void curses_create_plug_array(void)
{
   int res, i = 0;
   
   DEBUG_MSG("curses_create_plug_array");
   
   /* free the array (if alloc'ed) */
   while (wdg_plugin_elements && wdg_plugin_elements[i].desc != NULL) {
      SAFE_FREE(wdg_plugin_elements[i].desc);
      i++;
   }
   SAFE_FREE(wdg_plugin_elements);
   nplug = 0;
   
   /* go thru the list of plugins */
   res = plugin_list_walk(PLP_MIN, PLP_MAX, &curses_wdg_plugin);
   if (res == -ENOTFOUND) { 
      SAFE_CALLOC(wdg_plugin_elements, 1, sizeof(struct wdg_list));
      wdg_plugin_elements->desc = "No plugin found !";
   }
}

/*
 * refresh the array (the array must be pre-alloc'd)
 */
static void curses_refresh_plug_array(char active, struct plugin_ops *ops)
{
   /* refresh the description */ 
   snprintf(wdg_plugin_elements[nplug].desc, MAX_DESC_LEN, "[%d] %15s %4s  %s", active, ops->name, ops->version, ops->info);  
   
   nplug++;
}

/*
 * callback function for displaying the plugin list 
 */
static void curses_wdg_plugin(char active, struct plugin_ops *ops)
{
   /* enlarge the array */ 
   SAFE_REALLOC(wdg_plugin_elements, (nplug + 1) * sizeof(struct wdg_list));

   /* fill the element */
   SAFE_CALLOC(wdg_plugin_elements[nplug].desc, MAX_DESC_LEN + 1, sizeof(char));
   snprintf(wdg_plugin_elements[nplug].desc, MAX_DESC_LEN, "[%d] %15s %4s  %s", active, ops->name, ops->version, ops->info);  
   wdg_plugin_elements[nplug].value = ops->name;
   
   nplug++;
   
   /* null terminate the array */ 
   SAFE_REALLOC(wdg_plugin_elements, (nplug + 1) * sizeof(struct wdg_list));
   wdg_plugin_elements[nplug].desc = NULL;
   wdg_plugin_elements[nplug].value = NULL;
}

/*
 * callback function for a plugin 
 */
static void curses_select_plugin(void *plugin)
{
   /* prevent the selection when the list is empty */
   if (plugin == NULL)
      return;
        
   /* print the message */
   if (plugin_is_activated(plugin) == 0)
      INSTANT_USER_MSG("Activating %s plugin...\n", plugin);
   else
      INSTANT_USER_MSG("Deactivating %s plugin...\n", plugin);
         
   /*
    * pay attention on this !
    * if the plugin init does not return,
    * we are blocked here. So it is encouraged
    * to write plugins which spawn a thread
    * and immediately return
    */
   if (plugin_is_activated(plugin) == 1)
      plugin_fini(plugin);   
   else
      plugin_init(plugin);
        
   /* refres the array for the list widget */
   nplug = 0;
   plugin_list_walk(PLP_MIN, PLP_MAX, &curses_refresh_plug_array);
   
   /* refresh the list */
   wdg_list_refresh(wdg_plugin);
}


/* EOF */

// vim:ts=3:expandtab

