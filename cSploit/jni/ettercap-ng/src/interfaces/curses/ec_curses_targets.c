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

/* proto */

static void toggle_reverse(void);
static void curses_select_protocol(void);
static void set_protocol(void);
static void wipe_targets(void);
static void curses_select_targets(void);
static void set_targets(void);
static void curses_current_targets(void);
static void curses_destroy_tsel(void);
static void curses_create_targets_array(void);
static void curses_delete_target1(void *);
static void curses_delete_target2(void *);
static void curses_add_target1(void *);
static void curses_add_target2(void *);
static void add_target1(void);
static void add_target2(void);
static void curses_target_help(void);

/* globals */

static wdg_t *wdg_comp;
static wdg_t *wdg_t1, *wdg_t2;
static struct wdg_list *wdg_t1_elm, *wdg_t2_elm;
static char thost[MAX_ASCII_ADDR_LEN];
static char tag_reverse[] = " ";

struct wdg_menu menu_targets[] = { {"Targets",          'T',       "",    NULL},
                                   {"Current Targets",  't',       "t",   curses_current_targets},
                                   {"Select TARGET(s)", CTRL('T'), "C-t", curses_select_targets},
                                   {"-",                0,         "",    NULL},
                                   {"Protocol...",      'p',       "p",    curses_select_protocol},
                                   {"Reverse matching", 0,   tag_reverse, toggle_reverse},
                                   {"-",                0,         "",    NULL},
                                   {"Wipe targets",     'W',       "W",    wipe_targets},
                                   {NULL, 0, NULL, NULL},
                                 };


/*******************************************/

static void toggle_reverse(void)
{
   if (GBL_OPTIONS->reversed) {
      tag_reverse[0] = ' ';
      GBL_OPTIONS->reversed = 0;
   } else {
      tag_reverse[0] = '*';
      GBL_OPTIONS->reversed = 1;
   }
}

/*
 * wipe the targets struct setting both T1 and T2 to ANY/ANY/ANY
 */
static void wipe_targets(void)
{
   DEBUG_MSG("wipe_targets");
   
   reset_display_filter(GBL_TARGET1);
   reset_display_filter(GBL_TARGET2);

   /* display the message */
   curses_message("TARGETS were reset to ANY/ANY/ANY");

   /* if the 'current targets' window is displayed, refresh it */
   if (wdg_comp)
      curses_current_targets();
}

/*
 * display the protocol dialog
 */
static void curses_select_protocol(void)
{
   DEBUG_MSG("curses_select_protocol");

   /* this will contain 'all', 'tcp' or 'udp' */
   if (!GBL_OPTIONS->proto) {
      SAFE_CALLOC(GBL_OPTIONS->proto, 4, sizeof(char));
      strncpy(GBL_OPTIONS->proto, "all", 3);
   }

   curses_input("Protocol :", GBL_OPTIONS->proto, 3, set_protocol);
}

static void set_protocol(void)
{
   if (strcasecmp(GBL_OPTIONS->proto, "all") &&
       strcasecmp(GBL_OPTIONS->proto, "tcp") &&
       strcasecmp(GBL_OPTIONS->proto, "udp")) {
      ui_error("Invalid protocol");
      SAFE_FREE(GBL_OPTIONS->proto);
   }
}

/*
 * display the TARGET(s) dialog
 */
static void curses_select_targets(void)
{
   wdg_t *in;
   
#define TARGET_LEN 50
   
   DEBUG_MSG("curses_select_target1");

   /* make sure we have enough space */
   SAFE_REALLOC(GBL_OPTIONS->target1, TARGET_LEN * sizeof(char));
   SAFE_REALLOC(GBL_OPTIONS->target2, TARGET_LEN * sizeof(char));
   
   wdg_create_object(&in, WDG_INPUT, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   wdg_set_color(in, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(in, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(in, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(in, WDG_COLOR_TITLE, EC_COLOR_MENU);
   wdg_input_size(in, strlen("TARGET1 :") + TARGET_LEN, 4);
   wdg_input_add(in, 1, 1, "TARGET1 :", GBL_OPTIONS->target1, TARGET_LEN, 1);
   wdg_input_add(in, 1, 2, "TARGET2 :", GBL_OPTIONS->target2, TARGET_LEN, 1);
   wdg_input_set_callback(in, set_targets);
   
   wdg_draw_object(in);
      
   wdg_set_focus(in);
}

/*
 * set the targets 
 */
static void set_targets(void)
{
   /* delete the previous filters */
   reset_display_filter(GBL_TARGET1);
   reset_display_filter(GBL_TARGET2);

   /* free empty filters */
   if (!strcmp(GBL_OPTIONS->target1, ""))
      SAFE_FREE(GBL_OPTIONS->target1);
   
   /* free empty filters */
   if (!strcmp(GBL_OPTIONS->target2, ""))
      SAFE_FREE(GBL_OPTIONS->target2);
   
   /* compile the filters */
   compile_display_filter();

   /* if the 'current targets' window is displayed, refresh it */
   if (wdg_comp)
      curses_current_targets();
}

/*
 * display the list of current targets
 */
static void curses_current_targets(void)
{
   DEBUG_MSG("curses_current_targets");
   
   /* prepare the arrays for the target lists */
   curses_create_targets_array();
  
   /* if the object already exist, recreate it */
   if (wdg_comp) {
      wdg_destroy_object(&wdg_comp);
   }

   wdg_create_object(&wdg_comp, WDG_COMPOUND, WDG_OBJ_WANT_FOCUS);
   wdg_set_color(wdg_comp, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_comp, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_comp, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_comp, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_set_title(wdg_comp, "Current targets", WDG_ALIGN_LEFT);
   wdg_set_size(wdg_comp, 1, 2, 98, SYSMSG_WIN_SIZE - 1);
   
   wdg_create_object(&wdg_t1, WDG_LIST, 0);
   wdg_set_title(wdg_t1, "Target 1", WDG_ALIGN_LEFT);
   wdg_set_color(wdg_t1, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_set_color(wdg_t1, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_size(wdg_t1, 2, 3, 49, SYSMSG_WIN_SIZE - 2);
   
   wdg_create_object(&wdg_t2, WDG_LIST, 0);
   wdg_set_title(wdg_t2, "Target 2", WDG_ALIGN_LEFT);
   wdg_set_color(wdg_t2, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_set_color(wdg_t2, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_size(wdg_t2, 50, 3, 97, SYSMSG_WIN_SIZE - 2);

   /* link the array to the widgets */
   wdg_list_set_elements(wdg_t1, wdg_t1_elm);
   wdg_list_set_elements(wdg_t2, wdg_t2_elm);

   /* add the callbacks */
   wdg_list_add_callback(wdg_t1, 'd', curses_delete_target1);
   wdg_list_add_callback(wdg_t1, 'a', curses_add_target1);
   wdg_list_add_callback(wdg_t2, 'd', curses_delete_target2);
   wdg_list_add_callback(wdg_t2, 'a', curses_add_target2);
         
   /* link the widget together within the compound */
   wdg_compound_add(wdg_comp, wdg_t1);
   wdg_compound_add(wdg_comp, wdg_t2);
   
   /* add the destroy callback */
   wdg_add_destroy_key(wdg_comp, CTRL('Q'), curses_destroy_tsel);
   wdg_compound_add_callback(wdg_comp, ' ', curses_target_help);
   
   wdg_draw_object(wdg_comp);
   wdg_set_focus(wdg_comp);
   
}

static void curses_destroy_tsel(void)
{
   wdg_comp = NULL;
}

static void curses_target_help(void)
{
   char help[] = "HELP: shortcut list:\n\n"
                 "  ARROWS - switch between panels\n" 
                 "    a    - to add a new host\n"
                 "    d    - to delete an host from the list";

   curses_message(help);
}

/*
 * create the array for the widget.
 * erase any previously alloc'd array 
 */
static void curses_create_targets_array(void)
{
   struct ip_list *il;
   char tmp[MAX_ASCII_ADDR_LEN];
   size_t nhosts = 0;

   DEBUG_MSG("curses_create_targets_array");
   
   /* free the array (if alloc'ed) */
   while (wdg_t1_elm && wdg_t1_elm[nhosts].desc != NULL) {
      SAFE_FREE(wdg_t1_elm[nhosts].desc);
      nhosts++;
   }
   nhosts = 0;
   while (wdg_t2_elm && wdg_t2_elm[nhosts].desc != NULL) {
      SAFE_FREE(wdg_t2_elm[nhosts].desc);
      nhosts++;
   }
   SAFE_FREE(wdg_t1_elm);
   SAFE_FREE(wdg_t2_elm);
   nhosts = 0;
   
   /* XXX - two more loops were added to handle ipv6 targets
    *       since ipv6 targets require a separate list and it is
    *       unreasonable to put both ipv4 and ipv6 at the same list
    *       this code should be completely rewritten.
    *       do it if you have time.
    *                                  the braindamaged one.
    */

   /* walk TARGET 1 */
   LIST_FOREACH(il, &GBL_TARGET1->ips, next) {
      /* enlarge the array */ 
      SAFE_REALLOC(wdg_t1_elm, (nhosts + 1) * sizeof(struct wdg_list));

      /* fill the element */
      SAFE_CALLOC(wdg_t1_elm[nhosts].desc, MAX_ASCII_ADDR_LEN + 1, sizeof(char));
      /* print the description in the array */
      snprintf(wdg_t1_elm[nhosts].desc, MAX_ASCII_ADDR_LEN, "%s", ip_addr_ntoa(&il->ip, tmp));  
      
      wdg_t1_elm[nhosts].value = il;
   
      nhosts++;
   }
   /* same for IPv6 targets */
   LIST_FOREACH(il, &GBL_TARGET1->ip6, next) {
      /* enlarge the array */ 
      SAFE_REALLOC(wdg_t1_elm, (nhosts + 1) * sizeof(struct wdg_list));

      /* fill the element */
      SAFE_CALLOC(wdg_t1_elm[nhosts].desc, MAX_ASCII_ADDR_LEN + 1, sizeof(char));
      /* print the description in the array */
      snprintf(wdg_t1_elm[nhosts].desc, MAX_ASCII_ADDR_LEN, "%s", ip_addr_ntoa(&il->ip, tmp));  
      
      wdg_t1_elm[nhosts].value = il;
   
      nhosts++;
   }
   /* null terminate the array */ 
   SAFE_REALLOC(wdg_t1_elm, (nhosts + 1) * sizeof(struct wdg_list));
   wdg_t1_elm[nhosts].desc = NULL;
   wdg_t1_elm[nhosts].value = NULL;
   
   nhosts = 0;
   /* walk TARGET 2 */
   LIST_FOREACH(il, &GBL_TARGET2->ips, next) {
      /* enlarge the array */ 
      SAFE_REALLOC(wdg_t2_elm, (nhosts + 1) * sizeof(struct wdg_list));

      /* fill the element */
      SAFE_CALLOC(wdg_t2_elm[nhosts].desc, MAX_ASCII_ADDR_LEN + 1, sizeof(char));
      /* print the description in the array */
      snprintf(wdg_t2_elm[nhosts].desc, MAX_ASCII_ADDR_LEN, "%s", ip_addr_ntoa(&il->ip, tmp));  
      
      wdg_t2_elm[nhosts].value = il;
   
      nhosts++;
   }

   LIST_FOREACH(il, &GBL_TARGET2->ip6, next) {
      /* enlarge the array */ 
      SAFE_REALLOC(wdg_t2_elm, (nhosts + 1) * sizeof(struct wdg_list));

      /* fill the element */
      SAFE_CALLOC(wdg_t2_elm[nhosts].desc, MAX_ASCII_ADDR_LEN + 1, sizeof(char));
      /* print the description in the array */
      snprintf(wdg_t2_elm[nhosts].desc, MAX_ASCII_ADDR_LEN, "%s", ip_addr_ntoa(&il->ip, tmp));  
      
      wdg_t2_elm[nhosts].value = il;
   
      nhosts++;
   }
   
   /* null terminate the array */ 
   SAFE_REALLOC(wdg_t2_elm, (nhosts + 1) * sizeof(struct wdg_list));
   wdg_t2_elm[nhosts].desc = NULL;
   wdg_t2_elm[nhosts].value = NULL;
}

/*
 * delete an host from the target list
 */
static void curses_delete_target1(void *host)
{
   struct ip_list *il;

   DEBUG_MSG("curses_delete_target1");
   
   /* cast the parameter */
   il = (struct ip_list *)host;

   /* remove the host from the list */
   del_ip_list(&il->ip, GBL_TARGET1);

   /* redraw the window */
   curses_current_targets();
}

static void curses_delete_target2(void *host)
{
   struct ip_list *il;

   DEBUG_MSG("curses_delete_target2");
   
   /* cast the parameter */
   il = (struct ip_list *)host;

   /* remove the host from the list */
   del_ip_list(&il->ip, GBL_TARGET2);

   /* redraw the window */
   curses_current_targets();
}

/*
 * display the "add host" dialog
 */
static void curses_add_target1(void *entry)
{
   DEBUG_MSG("curses_add_target1");

   curses_input("IP address :", thost, MAX_ASCII_ADDR_LEN, add_target1);
}

static void curses_add_target2(void *entry)
{
   DEBUG_MSG("curses_add_target2");

   curses_input("IP address :", thost, MAX_ASCII_ADDR_LEN, add_target2);
}

static void add_target1(void)
{
   struct ip_addr ip;

   if(ip_addr_pton(thost, &ip) == -EINVALID) { 
      curses_message("Invalid ip address");
      return;
   }
   
   add_ip_list(&ip, GBL_TARGET1);
   
   /* redraw the window */
   curses_current_targets();
}

static void add_target2(void)
{
   struct ip_addr ip;

   if(ip_addr_pton(thost, &ip) == -EINVALID) { 
      curses_message("Invalid ip address");
      return;
   }

   add_ip_list(&ip, GBL_TARGET2);
   
   /* redraw the window */
   curses_current_targets();
   
   /* redraw the window */
   curses_current_targets();
}

/* EOF */

// vim:ts=3:expandtab

