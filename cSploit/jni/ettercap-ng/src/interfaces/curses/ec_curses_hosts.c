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
#include <ec_scan.h>
#include <ec_threads.h>

/* proto */

static void curses_scan(void);
static void curses_load_hosts(void);
static void load_hosts(char *path, char *file);
static void curses_save_hosts(void);
static void save_hosts(void);
static void curses_host_list(void);
void curses_hosts_update(void);
static void curses_hosts_destroy(void);
static void curses_create_hosts_array(void);
static void curses_delete_host(void *host);
static void curses_host_target1(void *host);
static void curses_host_target2(void *host);
static void curses_hosts_help(void *dummy);

/* globals */

static wdg_t *wdg_hosts;
static struct wdg_list *wdg_hosts_elements;

struct wdg_menu menu_hosts[] = { {"Hosts",             'H',       "",    NULL},
                                 {"Hosts list",        'h',       "h",   curses_host_list},
                                 {"-",                 0,         "",    NULL},
                                 {"Scan for hosts",    CTRL('S'), "C-s", curses_scan},
                                 {"Load from file...", 0,         "",    curses_load_hosts},
                                 {"Save to file...",   0,         "",    curses_save_hosts},
                                 {NULL, 0, NULL, NULL},
                               };

/*******************************************/

/*
 * scan the lan for hosts 
 */
static void curses_scan(void)
{
   /* wipe the current list */
   del_hosts_list();

   /* no target defined...  force a full scan */
   if (GBL_TARGET1->all_ip && GBL_TARGET2->all_ip &&
       GBL_TARGET1->all_ip6 && GBL_TARGET2->all_ip6 &&
      !GBL_TARGET1->scan_all && !GBL_TARGET2->scan_all) {
      GBL_TARGET1->scan_all = 1;
      GBL_TARGET2->scan_all = 1;
   }
   
   /* perform a new scan */
   build_hosts_list();
}

/*
 * display the file open dialog
 */
static void curses_load_hosts(void)
{
   wdg_t *fop;
   
   DEBUG_MSG("curses_load_hosts");
   
   wdg_create_object(&fop, WDG_FILE, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   
   wdg_set_title(fop, "Select an hosts file...", WDG_ALIGN_LEFT);
   wdg_set_color(fop, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(fop, WDG_COLOR_WINDOW, EC_COLOR_MENU);
   wdg_set_color(fop, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(fop, WDG_COLOR_TITLE, EC_COLOR_TITLE);

   wdg_file_set_callback(fop, load_hosts);
   
   wdg_draw_object(fop);
   
   wdg_set_focus(fop);
}

static void load_hosts(char *path, char *file)
{
   char *tmp;
   char current[PATH_MAX];
   
   DEBUG_MSG("load_hosts %s/%s", path, file);
   
   SAFE_CALLOC(tmp, strlen(path)+strlen(file)+2, sizeof(char));

   /* get the current working directory */
   getcwd(current, PATH_MAX);

   /* we are opening a file in the current dir.
    * use the relative path, so we can open files
    * in the current dir even if the complete path
    * is not traversable with ec_uid permissions
    */
   if (!strcmp(current, path))
      sprintf(tmp, "./%s", file);
   else
      sprintf(tmp, "%s/%s", path, file);

   /* wipe the current list */
   del_hosts_list();

   /* load the hosts list */
   scan_load_hosts(tmp);
   
   SAFE_FREE(tmp);
   
   curses_host_list();
}

/*
 * display the write file menu
 */
static void curses_save_hosts(void)
{
#define FILE_LEN  40
   
   DEBUG_MSG("curses_save_hosts");

   SAFE_FREE(GBL_OPTIONS->hostsfile);
   SAFE_CALLOC(GBL_OPTIONS->hostsfile, FILE_LEN, sizeof(char));
   
   curses_input("Output file :", GBL_OPTIONS->hostsfile, FILE_LEN, save_hosts);
}

static void save_hosts(void)
{
   FILE *f;
   
   /* check if the file is writeable */
   f = fopen(GBL_OPTIONS->hostsfile, "w");
   if (f == NULL) {
      ui_error("Cannot write %s", GBL_OPTIONS->hostsfile);
      SAFE_FREE(GBL_OPTIONS->hostsfile);
      return;
   }
 
   /* if ok, delete it */
   fclose(f);
   unlink(GBL_OPTIONS->hostsfile);
   
   scan_save_hosts(GBL_OPTIONS->hostsfile);
}

/*
 * display the host list 
 */
static void curses_host_list(void)
{
   DEBUG_MSG("curses_host_list");
   
   /* if the object already exist, recreate it */
   if (wdg_hosts) {
      wdg_destroy_object(&wdg_hosts);
   }
   
   wdg_create_object(&wdg_hosts, WDG_LIST, WDG_OBJ_WANT_FOCUS);
   
   wdg_set_size(wdg_hosts, 1, 2, -1, SYSMSG_WIN_SIZE - 1);
   wdg_set_title(wdg_hosts, "Hosts list...", WDG_ALIGN_LEFT);
   wdg_set_color(wdg_hosts, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_hosts, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_hosts, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(wdg_hosts, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_hosts, WDG_COLOR_TITLE, EC_COLOR_TITLE);

   /* create the array for the list widget */
   curses_create_hosts_array();
  
   /* set the elements */
   wdg_list_set_elements(wdg_hosts, wdg_hosts_elements);
   
   /* add the destroy callback */
   wdg_add_destroy_key(wdg_hosts, CTRL('Q'), curses_hosts_destroy);
  
   /* add the callbacks */
   wdg_list_add_callback(wdg_hosts, 'd', curses_delete_host);
   wdg_list_add_callback(wdg_hosts, '1', curses_host_target1);
   wdg_list_add_callback(wdg_hosts, '2', curses_host_target2);
   wdg_list_add_callback(wdg_hosts, ' ', curses_hosts_help);
   
   wdg_draw_object(wdg_hosts);
   
   wdg_set_focus(wdg_hosts);
}

static void curses_hosts_destroy(void)
{
   wdg_hosts = NULL;
}

void curses_hosts_update()
{
   if(wdg_hosts)
      curses_host_list();
}

static void curses_hosts_help(void *dummy)
{
   char help[] = "HELP: shortcut list:\n\n"
                 "  d - to delete an host from the list\n"
                 "  1 - to add the host to TARGET1\n"
                 "  2 - to add the host to TARGET2";

   curses_message(help);
}


/*
 * create the array for the widget.
 * erase any previously alloc'd array 
 */
static void curses_create_hosts_array(void)
{
   int i = 0;
   struct hosts_list *hl;
   char tmp[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   char name[MAX_HOSTNAME_LEN];
   size_t nhosts;

#define MAX_DESC_LEN 70
   
   DEBUG_MSG("curses_create_hosts_array");
   
   /* free the array (if alloc'ed) */
   while (wdg_hosts_elements && wdg_hosts_elements[i].desc != NULL) {
      SAFE_FREE(wdg_hosts_elements[i].desc);
      i++;
   }
   SAFE_FREE(wdg_hosts_elements);
   nhosts = 0;
   
   /* walk the hosts list */
   LIST_FOREACH(hl, &GBL_HOSTLIST, next) {
      /* enlarge the array */ 
      SAFE_REALLOC(wdg_hosts_elements, (nhosts + 1) * sizeof(struct wdg_list));

      /* fill the element */
      SAFE_CALLOC(wdg_hosts_elements[nhosts].desc, MAX_DESC_LEN + 1, sizeof(char));
      /* print the description in the array */
      if (hl->hostname) {
         snprintf(wdg_hosts_elements[nhosts].desc, MAX_DESC_LEN, "%-15s  %17s  %s", 
            ip_addr_ntoa(&hl->ip, tmp), mac_addr_ntoa(hl->mac, tmp2), hl->hostname);  
      } else {
         /* resolve the hostname (using the cache) */
         host_iptoa(&hl->ip, name);
         snprintf(wdg_hosts_elements[nhosts].desc, MAX_DESC_LEN, "%-15s  %17s  %s", 
            ip_addr_ntoa(&hl->ip, tmp), mac_addr_ntoa(hl->mac, tmp2), name);  
      }
      
      wdg_hosts_elements[nhosts].value = hl;
   
      nhosts++;
   }
   
   /* null terminate the array */ 
   SAFE_REALLOC(wdg_hosts_elements, (nhosts + 1) * sizeof(struct wdg_list));
   wdg_hosts_elements[nhosts].desc = NULL;
   wdg_hosts_elements[nhosts].value = NULL;
}

/*
 * deletes one host from the list
 */
static void curses_delete_host(void *host)
{
   struct hosts_list *hl;
 
   /* sanity check */
   if (host == NULL)
      return;

   /* cast the parameter */
   hl = (struct hosts_list *)host;

   /* remove the host from the list */
   LIST_REMOVE(hl, next);
   SAFE_FREE(hl->hostname);
   SAFE_FREE(hl);

   /* redraw the window */
   curses_host_list();
}

/*
 * add an host to TARGET 1
 */
static void curses_host_target1(void *host)
{
   struct hosts_list *hl;
   char tmp[MAX_ASCII_ADDR_LEN];
  
   DEBUG_MSG("curses_host_target1");
   
   /* cast the parameter */
   hl = (struct hosts_list *)host;
  
   /* add the ip to the target */
   add_ip_list(&hl->ip, GBL_TARGET1);

   USER_MSG("Host %s added to TARGET1\n", ip_addr_ntoa(&hl->ip, tmp));
}

/*
 * add an host to TARGET 2
 */
static void curses_host_target2(void *host)
{
   struct hosts_list *hl;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   DEBUG_MSG("curses_host_target2");
   
   /* cast the parameter */
   hl = (struct hosts_list *)host;
  
   /* add the ip to the target */
   add_ip_list(&hl->ip, GBL_TARGET2);
   
   USER_MSG("Host %s added to TARGET2\n", ip_addr_ntoa(&hl->ip, tmp));
}

/* EOF */

// vim:ts=3:expandtab

