/*
    WDG -- list widget

    Copyright (C) ALoR

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

#include <wdg.h>

#include <ncurses.h>
#include <menu.h>

#include <stdarg.h>

/* GLOBALS */

struct wdg_list_call {
   int key;
   void (*callback)(void *);
   SLIST_ENTRY(wdg_list_call) next;
};

struct wdg_list_handle {
   MENU *menu;
   WINDOW *mwin;
   WINDOW *win;
   ITEM *current;
   ITEM **items;
   size_t nitems;
   void (*select_callback)(void *);
   SLIST_HEAD(, wdg_list_call) callbacks;
};

/* PROTOS */

void wdg_create_list(struct wdg_object *wo);

static int wdg_list_destroy(struct wdg_object *wo);
static int wdg_list_resize(struct wdg_object *wo);
static int wdg_list_redraw(struct wdg_object *wo);
static int wdg_list_get_focus(struct wdg_object *wo);
static int wdg_list_lost_focus(struct wdg_object *wo);
static int wdg_list_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_list_borders(struct wdg_object *wo);

static int wdg_list_virtualize(int key);
static int wdg_list_driver(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);
static void wdg_list_menu_create(struct wdg_object *wo);
static void wdg_list_menu_destroy(struct wdg_object *wo);

void wdg_list_set_elements(struct wdg_object *wo, struct wdg_list *list);
void wdg_list_add_callback(wdg_t *wo, int key, void (*callback)(void *));
void wdg_list_select_callback(wdg_t *wo, void (*callback)(void *));
static int wdg_list_callback(struct wdg_object *wo, int key);
void wdg_list_refresh(wdg_t *wo);

/*******************************************/

/* 
 * called to create the menu
 */
void wdg_create_list(struct wdg_object *wo)
{
   WDG_DEBUG_MSG("wdg_create_list");
   
   /* set the callbacks */
   wo->destroy = wdg_list_destroy;
   wo->resize = wdg_list_resize;
   wo->redraw = wdg_list_redraw;
   wo->get_focus = wdg_list_get_focus;
   wo->lost_focus = wdg_list_lost_focus;
   wo->get_msg = wdg_list_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_list_handle));
}

/* 
 * called to destroy the menu
 */
static int wdg_list_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   struct wdg_list_call *c;
   int i = 0;
   
   WDG_DEBUG_MSG("wdg_list_destroy");

   /* erase the window */
   wdg_list_menu_destroy(wo);
   
   wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
   werase(ww->win);
   wnoutrefresh(ww->win);
   
   /* dealloc the structures */
   delwin(ww->win);

   /* all the items */
   while (ww->items && ww->items[i] != NULL)
      free_item(ww->items[i++]);

   WDG_SAFE_FREE(ww->items);

   /* free the callback list */
   while (SLIST_FIRST(&ww->callbacks) != NULL) {
      c = SLIST_FIRST(&ww->callbacks);
      SLIST_REMOVE_HEAD(&ww->callbacks, next);
      WDG_SAFE_FREE(c);
   }

   WDG_SAFE_FREE(wo->extend);

   return WDG_ESUCCESS;
}

/* 
 * called to resize the menu
 */
static int wdg_list_resize(struct wdg_object *wo)
{
   wdg_list_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw the menu
 */
static int wdg_list_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   
   WDG_DEBUG_MSG("wdg_list_redraw");
 
   /* the window already exist */
   if (ww->win) {
      /* erase the border */
      wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
      werase(ww->win);
      touchwin(ww->win);
      wnoutrefresh(ww->win);
    
      /* delete the internal menu */
      wdg_list_menu_destroy(wo);
       
      /* resize the window and draw the new border */
      mvwin(ww->win, y, x);
      wresize(ww->win, l, c);
      wdg_list_borders(wo);
     
      /* redraw the menu */
      wdg_list_menu_create(wo);

   /* the first time we have to allocate the window */
   } else {

      /* create the menu window (fixed dimensions) */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the titles */
      wdg_list_borders(wo);
      
      /* draw the menu */
      wdg_list_menu_create(wo);

      /* no scrolling for menu */
      scrollok(ww->win, FALSE);

   }
   
   /* refresh the window */
   touchwin(ww->win);
   wnoutrefresh(ww->win);
   touchwin(ww->mwin);
   wnoutrefresh(ww->mwin);
   
   wo->flags |= WDG_OBJ_VISIBLE;

   return WDG_ESUCCESS;
}

/* 
 * called when the menu gets the focus
 */
static int wdg_list_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_list_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the menu looses the focus
 */
static int wdg_list_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
  
   /* redraw the window */
   wdg_list_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the menu is focused
 */
static int wdg_list_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);

   /* handle the message */
   switch (key) {
         
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x)) {
            wdg_set_focus(wo);
            /* pass the event to the menu */
            wdg_list_driver(wo, key, mouse);
         } else 
            return -WDG_ENOTHANDLED;
         break;

      case KEY_DOWN:
      case KEY_UP:
      case KEY_NPAGE:
      case KEY_PPAGE:
         /* move only if focused */
         if (wo->flags & WDG_OBJ_FOCUSED) {
            /* pass the event to the menu */
            wdg_list_driver(wo, key, mouse);
         } else
            return -WDG_ENOTHANDLED;
         break;

      case KEY_RETURN:
         if (item_userptr(current_item(ww->menu))) 
            WDG_EXECUTE(ww->select_callback, item_userptr(current_item(ww->menu)));
         break;
         
      /* message not handled */
      default:
         return wdg_list_callback(wo, key);
         break;
   }
  
   return WDG_ESUCCESS;
}

/*
 * draw the list borders
 */
static void wdg_list_borders(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   size_t c = wdg_get_ncols(wo);
   
   /* the object was focused */
   if (wo->flags & WDG_OBJ_FOCUSED) {
      wattron(ww->win, A_BOLD);
      wbkgdset(ww->win, COLOR_PAIR(wo->focus_color));
   } else
      wbkgdset(ww->win, COLOR_PAIR(wo->border_color));

   /* draw the borders */
   box(ww->win, 0, 0);
   
   /* set the title color */
   wbkgdset(ww->win, COLOR_PAIR(wo->title_color));
      
   /* there is a title: print it */
   if (wo->title) {
      switch (wo->align) {
         case WDG_ALIGN_LEFT:
            wmove(ww->win, 0, 3);
            break;
         case WDG_ALIGN_CENTER:
            wmove(ww->win, 0, (c - strlen(wo->title)) / 2);
            break;
         case WDG_ALIGN_RIGHT:
            wmove(ww->win, 0, c - strlen(wo->title) - 3);
            break;
      }
      wprintw(ww->win, wo->title);
   }
   
   /* restore the attribute */
   if (wo->flags & WDG_OBJ_FOCUSED)
      wattroff(ww->win, A_BOLD);
  
}

/*
 * set the elements for the list
 */
void wdg_list_set_elements(struct wdg_object *wo, struct wdg_list *list)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   size_t i = 0;
 
   wdg_list_menu_destroy(wo);

   /* forget the curren position, we are creating a new menu */
   ww->current = NULL;
 
   /* free any previously alloc'd item */
   while (ww->items && ww->items[i] != NULL)
      free_item(ww->items[i++]);

   WDG_SAFE_FREE(ww->items);
   i = 0;
   ww->nitems = 0;
   
   /* walk thru the list and set the menu items */
   while (list[i].desc != NULL) {
      /* count the items added */
      ww->nitems++;

      WDG_SAFE_REALLOC(ww->items, ww->nitems * sizeof(ITEM *));
      
      ww->items[i] = new_item(list[i].desc, "");
      set_item_userptr(ww->items[i], list[i].value);
      i++;
   }
         
   /* add the null termination to the array */
   WDG_SAFE_REALLOC(ww->items, (ww->nitems + 1) * sizeof(ITEM *));
   ww->items[ww->nitems] = NULL;

   wdg_list_menu_create(wo);
}


/*
 * stransform keys into menu commands 
 */
static int wdg_list_virtualize(int key)
{
   switch (key) {
      case KEY_NPAGE:
         return (REQ_SCR_DPAGE);
      case KEY_PPAGE:
         return (REQ_SCR_UPAGE);
      case KEY_DOWN:
         return (REQ_NEXT_ITEM);
      case KEY_UP:
         return (REQ_PREV_ITEM);
      default:
         if (key != KEY_MOUSE)
            beep();
         return (key);
   }
}

/*
 * sends command to the active menu 
 */
static int wdg_list_driver(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   int c;
   
   c = menu_driver(ww->menu, wdg_list_virtualize(key) );
   
   /* skip non selectable items */
   if ( !(item_opts(current_item(ww->menu)) & O_SELECTABLE) )
      c = menu_driver(ww->menu, wdg_list_virtualize(key) );

   /* one item has been selected */
   if (c == E_UNKNOWN_COMMAND) {
      if (item_userptr(current_item(ww->menu)))
         WDG_EXECUTE(ww->select_callback, item_userptr(current_item(ww->menu)));
   }
   
   /* tring to go outside edges */
   if (c == E_REQUEST_DENIED)
      return -WDG_ENOTHANDLED;

   wnoutrefresh(ww->mwin);
      
   return WDG_ESUCCESS;
}

/*
 * create the menu
 */
static void wdg_list_menu_create(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   int mrows = 0, mcols = 0;
  
   /* skip the creation if:
    *    - already displayed 
    *    - no items are present
    */
   if (ww->menu || !ww->items || ww->nitems == 0)
      return;

   /* create the menu */
   ww->menu = new_menu(ww->items);

   /* set the dimensions */
   set_menu_format(ww->menu, l - 4, 1);
   set_menu_spacing(ww->menu, 2, 0, 0);

   /* get the geometry to make a window */
   scale_menu(ww->menu, &mrows, &mcols);

   ww->mwin = newwin(mrows + 1, mcols, y + 2, x + 2);
   /* set the color */
   wbkgd(ww->mwin, COLOR_PAIR(wo->window_color));
   keypad(ww->mwin, TRUE);
   
   /* associate with the menu */
   set_menu_win(ww->menu, ww->mwin);
   
   /* the subwin for the menu */
   set_menu_sub(ww->menu, derwin(ww->mwin, mrows + 1, mcols, 2, 2));

   /* menu attributes */
   set_menu_mark(ww->menu, "");
   set_menu_grey(ww->menu, COLOR_PAIR(wo->window_color));
   set_menu_back(ww->menu, COLOR_PAIR(wo->window_color));
   set_menu_fore(ww->menu, COLOR_PAIR(wo->window_color) | A_REVERSE | A_BOLD);
  
   /* repristinate the current position */
   if (ww->current)
      set_current_item(ww->menu, ww->current);
   
   /* display the menu */
   post_menu(ww->menu);

   wnoutrefresh(ww->mwin);
}

/*
 * destroy the menu
 */
static void wdg_list_menu_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);

   /* nothing to clear */
   if (ww->menu == NULL)
      return;
  
   /* remember the current position to be repristinated on menu create */
   ww->current = current_item(ww->menu);
   
   /* hide the menu */
   unpost_menu(ww->menu);
   
   /* erase the menu */
   wbkgd(ww->mwin, COLOR_PAIR(wo->screen_color));
   werase(ww->mwin);
   wnoutrefresh(ww->mwin);

   /* delete the memory */
   free_menu(ww->menu);

   ww->menu = NULL;
}

/*
 * set the select callback
 */
void wdg_list_select_callback(wdg_t *wo, void (*callback)(void *))
{
   WDG_WO_EXT(struct wdg_list_handle, ww);

   ww->select_callback = callback;
}

/*
 * add the user callback(s)
 */
void wdg_list_add_callback(wdg_t *wo, int key, void (*callback)(void *))
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   struct wdg_list_call *c;

   WDG_SAFE_CALLOC(c, 1, sizeof(struct wdg_list_call));
   
   c->key = key;
   c->callback = callback;

   SLIST_INSERT_HEAD(&ww->callbacks, c, next);
}

/*
 * search for a callback and execute it
 */
static int wdg_list_callback(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   struct wdg_list_call *c;

   SLIST_FOREACH(c, &ww->callbacks, next) {
      if (c->key == key) {
         void *value;
         
         WDG_DEBUG_MSG("wdg_list_callback");
         
         /* retrieve the value from the current item */
         value = item_userptr(current_item(ww->menu));
         
         /* execute the callback */
         WDG_EXECUTE(c->callback, value);
         
         return WDG_ESUCCESS;
      }
   }

   return -WDG_ENOTHANDLED;
}

/*
 * force the repaint of the menu 
 */
void wdg_list_refresh(wdg_t *wo)
{
   WDG_WO_EXT(struct wdg_list_handle, ww);
   
   WDG_DEBUG_MSG("wdg_list_refresh");

   /* remember the position */
   ww->current = current_item(ww->menu);

   /* delete the window */
   unpost_menu(ww->menu);

   /* set the old position */
   set_current_item(ww->menu, ww->current);
   /* draw the menu */
   post_menu(ww->menu);

   wnoutrefresh(ww->mwin);
}

/* EOF */

// vim:ts=3:expandtab

