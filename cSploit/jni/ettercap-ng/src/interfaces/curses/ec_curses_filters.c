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
#include <ec_filter.h>

#define MAX_DESC_LEN 75

/* proto */

static void curses_manage_filters(void);
static void curses_select_filter(void *filter);
static void curses_load_filter(void);
static void load_filter(char *path, char *file);
static void curses_stop_filter(void);

/* globals */

struct wdg_menu menu_filters[] = { {"Filters",          'F',       "",    NULL},
                                   {"Manage filters", CTRL('M'), "C-m", curses_manage_filters},
                                   {"Load a filter...", CTRL('F'), "C-f", curses_load_filter},
                                   {"Stop filtering",   'f',       "f",   curses_stop_filter},
                                   {NULL, 0, NULL, NULL},
                                 };

/*******************************************/

static wdg_t *wdg_filters = NULL;
static struct wdg_list *wdg_filters_elements;
static int n_filters = 0;

static int add_filter_to_list(struct filter_list *f, void *data)
{
   SAFE_REALLOC(wdg_filters_elements, (n_filters + 1) * sizeof(struct wdg_list));
   SAFE_CALLOC(wdg_filters_elements[n_filters].desc, MAX_DESC_LEN + 1, sizeof(char));
   snprintf(wdg_filters_elements[n_filters].desc, MAX_DESC_LEN, "[%c] %s", f->enabled?'X':' ', f->name);
   wdg_filters_elements[n_filters].value = f;
   n_filters++;

   return 1;
}

static void build_filter_list(void)
{
   while (wdg_filters_elements && n_filters > 0) {
      SAFE_FREE(wdg_filters_elements[n_filters-1].desc);
      n_filters--;
   }
   SAFE_FREE(wdg_filters_elements);

   n_filters = 0;
   filter_walk_list( add_filter_to_list, &n_filters );

   SAFE_REALLOC(wdg_filters_elements, (n_filters + 1) * sizeof(struct wdg_list));
   /* 0-terminate the array */
   wdg_filters_elements[n_filters].desc = NULL;
   wdg_filters_elements[n_filters].value = NULL;
}

static void refresh_filter_list(void)
{
   build_filter_list();
   wdg_list_set_elements(wdg_filters, wdg_filters_elements);
   wdg_list_refresh(wdg_filters);
}

static void curses_manage_filters(void)
{
   if (!wdg_filters) {
      wdg_create_object(&wdg_filters, WDG_LIST, WDG_OBJ_WANT_FOCUS);
   }
   wdg_set_size(wdg_filters, 1, 2, -1, SYSMSG_WIN_SIZE - 1);
   wdg_set_title(wdg_filters, "Select a filter...", WDG_ALIGN_LEFT);
   wdg_set_color(wdg_filters, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(wdg_filters, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(wdg_filters, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(wdg_filters, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(wdg_filters, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_list_select_callback(wdg_filters, curses_select_filter);
   wdg_add_destroy_key(wdg_filters, CTRL('Q'), NULL);

   wdg_draw_object(wdg_filters);
   wdg_set_focus(wdg_filters);
   refresh_filter_list();
}

static void curses_select_filter(void *filter)
{
   if (filter == NULL)
      return;
   struct filter_list *f = filter;
   /* toggle the filter */
   f->enabled = ! f->enabled;
   refresh_filter_list();
}

/*
 * display the file open dialog
 */
static void curses_load_filter(void)
{
   wdg_t *fop;
   
   DEBUG_MSG("curses_load_filter");
   
   wdg_create_object(&fop, WDG_FILE, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   
   wdg_set_title(fop, "Select a precompiled filter file...", WDG_ALIGN_LEFT);
   wdg_set_color(fop, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(fop, WDG_COLOR_WINDOW, EC_COLOR_MENU);
   wdg_set_color(fop, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(fop, WDG_COLOR_TITLE, EC_COLOR_TITLE);

   wdg_file_set_callback(fop, load_filter);
   
   wdg_draw_object(fop);
   
   wdg_set_focus(fop);
}

static void load_filter(char *path, char *file)
{
   char *tmp;
   
   DEBUG_MSG("load_filter %s/%s", path, file);
   
   SAFE_CALLOC(tmp, strlen(path)+strlen(file)+2, sizeof(char));

   snprintf(tmp, strlen(path)+strlen(file)+2, "%s/%s", path, file);

   /* 
    * load the filters chain.
    * errors are spawned by the function itself
    */
   filter_load_file(tmp, GBL_FILTERS, 1);
   
   SAFE_FREE(tmp);
}


/*
 * uload the filter chain
 */
static void curses_stop_filter(void)
{
   DEBUG_MSG("curses_stop_filter");

   filter_unload(GBL_FILTERS);
   
   curses_message("Filters were unloaded");
}

/* EOF */

// vim:ts=3:expandtab

