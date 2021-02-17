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
#include <ec_format.h>
#include <ec_parser.h>
#include <ec_encryption.h>

/* proto */

static void toggle_resolve(void);
static void curses_show_stats(void);
static void curses_stop_stats(void);
static void refresh_stats(void);
static void curses_vis_method(void);
static void curses_set_method(void);
static void curses_vis_regex(void);
static void curses_set_regex(void);
static void curses_wifi_key(void);
static void curses_set_wifikey(void);
extern void curses_show_profiles(void);
extern void curses_show_connections(void);

/* globals */

static char tag_resolve[] = " ";
static wdg_t *wdg_stats;
#define VLEN 8
static char vmethod[VLEN];
#define RLEN 50
static char vregex[RLEN];
#define WLEN 70
static char wkey[WLEN];

struct wdg_menu menu_view[] = { {"View",                 'V', "",  NULL},
                                {"Connections",          'C', "C", curses_show_connections},
                                {"Profiles",             'O', "O", curses_show_profiles},
                                {"Statistics",           's', "s", curses_show_stats},
                                {"-",                     0,  "",  NULL},
                                {"Resolve IP addresses",  0, tag_resolve,   toggle_resolve},
                                {"Visualization method...", 'v', "v", curses_vis_method},
                                {"Visualization regex...", 'R', "R", curses_vis_regex},
                                {"-",                     0,  "",  NULL},
                                {"Set the WiFi key...",   'w', "w", curses_wifi_key},
                                {NULL, 0, NULL, NULL},
                              };


/*******************************************/


static void toggle_resolve(void)
{
   if (GBL_OPTIONS->resolve) {
      tag_resolve[0] = ' ';
      GBL_OPTIONS->resolve = 0;
   } else {
      tag_resolve[0] = '*';
      GBL_OPTIONS->resolve = 1;
   }
}

/*
 * display the statistics windows
 */
static void curses_show_stats(void)
{
   DEBUG_MSG("curses_show_stats");

   /* if the object already exist, set the focus to it */
   if (wdg_stats) {
      wdg_set_focus(wdg_stats);
      return;
   }
   
   wdg_create_object(&wdg_stats, WDG_WINDOW, WDG_OBJ_WANT_FOCUS);
   
   wdg_set_title(wdg_stats, "Statistics:", WDG_ALIGN_LEFT);
   wdg_set_size(wdg_stats, 1, 2, 70, 21);
   wdg_set_color(wdg_stats, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_stats, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_stats, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(wdg_stats, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_stats, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_draw_object(wdg_stats);
 
   wdg_set_focus(wdg_stats);
  
   /* display the stats */
   refresh_stats(); 

   /* add the callback on idle to refresh the stats */
   wdg_add_idle_callback(refresh_stats);

   /* add the destroy callback */
   wdg_add_destroy_key(wdg_stats, CTRL('Q'), curses_stop_stats);
}

static void curses_stop_stats(void)
{
   DEBUG_MSG("curses_stop_stats");
   wdg_del_idle_callback(refresh_stats);

   /* the object does not exist anymore */
   wdg_stats = NULL;
}

static void refresh_stats(void)
{
   /* if not focused don't refresh it */
   if (!(wdg_stats->flags & WDG_OBJ_FOCUSED))
      return;
   
   wdg_window_print(wdg_stats, 1, 1, "Received packets    : %8lld", GBL_STATS->ps_recv);
   wdg_window_print(wdg_stats, 1, 2, "Dropped packets     : %8lld  %.2f %% ", GBL_STATS->ps_drop, 
          (GBL_STATS->ps_recv) ? (float)GBL_STATS->ps_drop * 100 / GBL_STATS->ps_recv : 0 );
   wdg_window_print(wdg_stats, 1, 3, "Forwarded packets   : %8lld  bytes: %8lld ", GBL_STATS->ps_sent, GBL_STATS->bs_sent);
  
   wdg_window_print(wdg_stats, 1, 5, "Current queue len   : %d/%d ", GBL_STATS->queue_curr, GBL_STATS->queue_max);
   wdg_window_print(wdg_stats, 1, 6, "Sampling rate       : %d ", GBL_CONF->sampling_rate);
   
   wdg_window_print(wdg_stats, 1, 8, "Bottom Half received packet : pck: %8lld  bytes: %8lld", 
         GBL_STATS->bh.pck_recv, GBL_STATS->bh.pck_size);
   wdg_window_print(wdg_stats, 1, 9, "Top Half received packet    : pck: %8lld  bytes: %8lld", 
         GBL_STATS->th.pck_recv, GBL_STATS->th.pck_size);
   wdg_window_print(wdg_stats, 1, 10, "Interesting packets         : %.2f %% ",
         (GBL_STATS->bh.pck_recv) ? (float)GBL_STATS->th.pck_recv * 100 / GBL_STATS->bh.pck_recv : 0 );

   wdg_window_print(wdg_stats, 1, 12, "Bottom Half packet rate : worst: %8d  adv: %8d p/s", 
         GBL_STATS->bh.rate_worst, GBL_STATS->bh.rate_adv);
   wdg_window_print(wdg_stats, 1, 13, "Top Half packet rate    : worst: %8d  adv: %8d p/s", 
         GBL_STATS->th.rate_worst, GBL_STATS->th.rate_adv);
   
   wdg_window_print(wdg_stats, 1, 14, "Bottom Half thruoutput  : worst: %8d  adv: %8d b/s", 
         GBL_STATS->bh.thru_worst, GBL_STATS->bh.thru_adv);
   wdg_window_print(wdg_stats, 1, 15, "Top Half thruoutput     : worst: %8d  adv: %8d b/s", 
         GBL_STATS->th.thru_worst, GBL_STATS->th.thru_adv);
}

/*
 * change the visualization method 
 */
static void curses_vis_method(void)
{
   DEBUG_MSG("curses_vis_method");

   curses_input("Visualization method :", vmethod, VLEN, curses_set_method);
}

static void curses_set_method(void)
{
   set_format(vmethod);
}

/*
 * change the visualization regex 
 */
static void curses_vis_regex(void)
{
   DEBUG_MSG("curses_vis_regex");

   curses_input("Visualization regex :", vregex, RLEN, curses_set_regex);
}

static void curses_set_regex(void)
{
   set_regex(vregex);
}

/*
 * change the WiFi key for wifi
 */
static void curses_wifi_key(void)
{
   DEBUG_MSG("curses_wifi_key");

   curses_input("WiFi key :", wkey, WLEN, curses_set_wifikey);
}

static void curses_set_wifikey(void)
{
   wifi_key_prepare(wkey);
}


/* EOF */

// vim:ts=3:expandtab

