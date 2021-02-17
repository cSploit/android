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
#include <ec_profiles.h>
#include <ec_manuf.h>
#include <ec_services.h>

/* proto */

void curses_show_profiles(void);
static void curses_kill_profiles(void);
static void refresh_profiles(void);
static void curses_profile_detail(void *profile);
static void curses_profiles_local(void *dummy);
static void curses_profiles_remote(void *dummy);
static void curses_profiles_convert(void *dummy);
static void curses_profiles_dump(void *dummy);
static void dump_profiles(void);
static void curses_profiles_help(void *dummy);

/* globals */

static wdg_t *wdg_profiles, *wdg_pro_detail;
static char *logfile;


/*******************************************/

/*
 * the auto-refreshing list of profiles 
 */
void curses_show_profiles(void)
{
   DEBUG_MSG("curses_show_profiles");

   /* if the object already exist, set the focus to it */
   if (wdg_profiles) {
      wdg_set_focus(wdg_profiles);
      return;
   }
   
   wdg_create_object(&wdg_profiles, WDG_DYNLIST, WDG_OBJ_WANT_FOCUS);
   
   wdg_set_title(wdg_profiles, "Collected passive profiles:", WDG_ALIGN_LEFT);
   wdg_set_size(wdg_profiles, 1, 2, -1, SYSMSG_WIN_SIZE - 1);
   wdg_set_color(wdg_profiles, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_profiles, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_profiles, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(wdg_profiles, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_profiles, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_draw_object(wdg_profiles);
 
   wdg_set_focus(wdg_profiles);

   /* set the list print callback */
   wdg_dynlist_print_callback(wdg_profiles, profile_print);
   
   /* set the select callback */
   wdg_dynlist_select_callback(wdg_profiles, curses_profile_detail);
  
   /* add the callback on idle to refresh the profile list */
   wdg_add_idle_callback(refresh_profiles);

   /* add the destroy callback */
   wdg_add_destroy_key(wdg_profiles, CTRL('Q'), curses_kill_profiles);

   wdg_dynlist_add_callback(wdg_profiles, 'l', curses_profiles_local);
   wdg_dynlist_add_callback(wdg_profiles, 'r', curses_profiles_remote);
   wdg_dynlist_add_callback(wdg_profiles, 'c', curses_profiles_convert);
   wdg_dynlist_add_callback(wdg_profiles, 'd', curses_profiles_dump);
   wdg_dynlist_add_callback(wdg_profiles, ' ', curses_profiles_help);
}

static void curses_kill_profiles(void)
{
   DEBUG_MSG("curses_kill_profiles");
   wdg_del_idle_callback(refresh_profiles);

   /* the object does not exist anymore */
   wdg_profiles = NULL;
}

static void curses_profiles_help(void *dummy)
{
   char help[] = "HELP: shortcut list:\n\n"
                 "  ENTER - show the infos about the host\n"
                 "    l   - remove the remote hosts from the list\n"
                 "    r   - remove the local hosts from the list\n"
                 "    c   - convert the profile list into hosts list\n"
                 "    d   - dump the profiles information to a file";

   curses_message(help);
}

static void refresh_profiles(void)
{
   /* if not focused don't refresh it */
   if (!(wdg_profiles->flags & WDG_OBJ_FOCUSED))
      return;
   
   wdg_dynlist_refresh(wdg_profiles);
}

/*
 * display details for a profile
 */
static void curses_profile_detail(void *profile)
{
   struct host_profile *h = (struct host_profile *)profile;
   struct open_port *o;
   struct active_user *u;
   char tmp[MAX_ASCII_ADDR_LEN];
   char os[OS_LEN+1];
  
   DEBUG_MSG("curses_profile_detail");

   /* if the object already exist, set the focus to it */
   if (wdg_pro_detail) {
      wdg_destroy_object(&wdg_pro_detail);
      wdg_pro_detail = NULL;
   }
   
   wdg_create_object(&wdg_pro_detail, WDG_SCROLL, WDG_OBJ_WANT_FOCUS);
   
   wdg_set_title(wdg_pro_detail, "Profile details:", WDG_ALIGN_LEFT);
   wdg_set_size(wdg_pro_detail, 1, 2, -1, SYSMSG_WIN_SIZE - 1);
   wdg_set_color(wdg_pro_detail, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_pro_detail, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_pro_detail, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(wdg_pro_detail, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_pro_detail, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_draw_object(wdg_pro_detail);
 
   wdg_set_focus(wdg_pro_detail);

   wdg_add_destroy_key(wdg_pro_detail, CTRL('Q'), NULL);
   wdg_scroll_set_lines(wdg_pro_detail, 100);

   memset(os, 0, sizeof(os));
   
   wdg_scroll_print(wdg_pro_detail, EC_COLOR, " IP address   : %s \n", ip_addr_ntoa(&h->L3_addr, tmp));
   if (strcmp(h->hostname, ""))
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " Hostname     : %s \n\n", h->hostname);
   else
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, "\n");   
      
   if (h->type & FP_HOST_LOCAL || h->type == FP_UNKNOWN) {
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " MAC address  : %s \n", mac_addr_ntoa(h->L2_addr, tmp));
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " MANUFACTURER : %s \n\n", manuf_search((const char*)h->L2_addr));
   }

   wdg_scroll_print(wdg_pro_detail, EC_COLOR, " DISTANCE     : %d   \n", h->distance);
   if (h->type & FP_GATEWAY)
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " TYPE         : GATEWAY\n\n");
   else if (h->type & FP_HOST_LOCAL)
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " TYPE         : LAN host\n\n");
   else if (h->type & FP_ROUTER)
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " TYPE         : REMOTE ROUTER\n\n");
   else if (h->type & FP_HOST_NONLOCAL)
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " TYPE         : REMOTE host\n\n");
   else if (h->type == FP_UNKNOWN)
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " TYPE         : unknown\n\n");

   if (h->os)
    wdg_scroll_print(wdg_pro_detail, EC_COLOR, " OBSERVED OS     : %s\n\n", h->os);

   wdg_scroll_print(wdg_pro_detail, EC_COLOR, " FINGERPRINT      : %s\n", h->fingerprint);
   if (fingerprint_search((const char*)h->fingerprint, os) == ESUCCESS)
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " OPERATING SYSTEM : %s \n\n", os);
   else {
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " OPERATING SYSTEM : unknown fingerprint (please submit it) \n");
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, " NEAREST ONE IS   : %s \n\n", os);
   }
      
   
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      
      wdg_scroll_print(wdg_pro_detail, EC_COLOR, "   PORT     : %s %d | %s \t[%s]\n", 
                  (o->L4_proto == NL_TYPE_TCP) ? "TCP" : "UDP" , 
                  ntohs(o->L4_addr),
                  service_search(o->L4_addr, o->L4_proto), 
                  (o->banner) ? o->banner : "");
      
      LIST_FOREACH(u, &(o->users_list_head), next) {
        
         if (u->failed)
            wdg_scroll_print(wdg_pro_detail, EC_COLOR, "      ACCOUNT : * %s / %s  (%s)\n", u->user, u->pass, ip_addr_ntoa(&u->client, tmp));
         else
            wdg_scroll_print(wdg_pro_detail, EC_COLOR, "      ACCOUNT : %s / %s  (%s)\n", u->user, u->pass, ip_addr_ntoa(&u->client, tmp));
         if (u->info)
            wdg_scroll_print(wdg_pro_detail, EC_COLOR, "      INFO    : %s\n\n", u->info);
         else
            wdg_scroll_print(wdg_pro_detail, EC_COLOR, "\n");
      }
   }
}

static void curses_profiles_local(void *dummy)
{
   profile_purge_remote();
   wdg_dynlist_reset(wdg_profiles);
   wdg_dynlist_refresh(wdg_profiles);
}

static void curses_profiles_remote(void *dummy)
{
   profile_purge_local();
   wdg_dynlist_reset(wdg_profiles);
   wdg_dynlist_refresh(wdg_profiles);
}

static void curses_profiles_convert(void *dummy)
{
   profile_convert_to_hostlist();
   curses_message("The hosts list was populated with local profiles");
}

static void curses_profiles_dump(void *dummy)
{
   DEBUG_MSG("curses_profiles_dump");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, 50, sizeof(char));

   curses_input("Log File :", logfile, 50, dump_profiles);

}

static void dump_profiles(void)
{
   /* dump the profiles */
   if (profile_dump_to_file(logfile) == ESUCCESS)
      curses_message("Profiles dumped to file");
}

/* EOF */

// vim:ts=3:expandtab

