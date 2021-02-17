/*
    WDG -- dynamic list widget

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
#include <stdarg.h>

/* GLOBALS */

struct wdg_dynlist_call {
   int key;
   void (*callback)(void *);
   SLIST_ENTRY(wdg_dynlist_call) next;
};

struct wdg_dynlist {
   WINDOW *win;
   WINDOW *sub;
   void * (*func)(int mode, void *list, char **desc, size_t len);
   void *top;
   void *bottom;
   void *current;
   void (*select_callback)(void *);
   SLIST_HEAD(, wdg_dynlist_call) callbacks;
};

/* PROTOS */

void wdg_create_dynlist(struct wdg_object *wo);

static int wdg_dynlist_destroy(struct wdg_object *wo);
static int wdg_dynlist_resize(struct wdg_object *wo);
static int wdg_dynlist_redraw(struct wdg_object *wo);
static int wdg_dynlist_get_focus(struct wdg_object *wo);
static int wdg_dynlist_lost_focus(struct wdg_object *wo);
static int wdg_dynlist_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_dynlist_border(struct wdg_object *wo);
static void wdg_dynlist_move(struct wdg_object *wo, int key);
static void wdg_dynlist_mouse(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

void wdg_dynlist_refresh(wdg_t *wo);
void wdg_dynlist_print_callback(wdg_t *wo, void * func(int mode, void *list, char **desc, size_t len));
void wdg_dynlist_select_callback(wdg_t *wo, void (*callback)(void *));
void wdg_dynlist_add_callback(wdg_t *wo, int key, void (*callback)(void *));
static int wdg_dynlist_callback(struct wdg_object *wo, int key);
void wdg_dynlist_reset(wdg_t *wo);

/*******************************************/

/* 
 * called to create a window
 */
void wdg_create_dynlist(struct wdg_object *wo)
{
   WDG_DEBUG_MSG("wdg_create_dynlist");
   
   /* set the callbacks */
   wo->destroy = wdg_dynlist_destroy;
   wo->resize = wdg_dynlist_resize;
   wo->redraw = wdg_dynlist_redraw;
   wo->get_focus = wdg_dynlist_get_focus;
   wo->lost_focus = wdg_dynlist_lost_focus;
   wo->get_msg = wdg_dynlist_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_dynlist));
}

/* 
 * called to destroy a window
 */
static int wdg_dynlist_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   struct wdg_dynlist_call *c;
   
   WDG_DEBUG_MSG("wdg_dynlist_destroy (%p)", wo);

   /* erase the window */
   wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
   wbkgd(ww->sub, COLOR_PAIR(wo->screen_color));
   werase(ww->sub);
   werase(ww->win);
   wnoutrefresh(ww->sub);
   wnoutrefresh(ww->win);
   
   /* dealloc the structures */
   delwin(ww->sub);
   delwin(ww->win);
   
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
 * called to resize a window
 */
static int wdg_dynlist_resize(struct wdg_object *wo)
{
   wdg_dynlist_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw a window
 */
static int wdg_dynlist_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   
   WDG_DEBUG_MSG("wdg_dynlist_redraw");
 
   /* the window already exist */
   if (ww->win) {
      /* erase the border */
      wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
      werase(ww->win);
      touchwin(ww->win);
      wnoutrefresh(ww->win);
      
      /* resize the window and draw the new border */
      mvwin(ww->win, y, x);
      wresize(ww->win, l, c);
      wdg_dynlist_border(wo);
      
      /* resize the actual window and touch it */
      mvwin(ww->sub, y + 2, x + 2);
      wresize(ww->sub, l - 4, c - 4);
      /* set the window color */
      wbkgd(ww->sub, COLOR_PAIR(wo->window_color));

   /* the first time we have to allocate the window */
   } else {

      /* create the outher window */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the borders */
      wdg_dynlist_border(wo);

      /* create the inner (actual) window */
      if ((ww->sub = newwin(l - 4, c - 4, y + 2, x + 2)) == NULL)
         return -WDG_EFATAL;
      
      /* set the window color */
      wbkgd(ww->sub, COLOR_PAIR(wo->window_color));
      werase(ww->sub);
      redrawwin(ww->sub);

      /* initialize the pointer */
      wmove(ww->sub, 0, 0);

      scrollok(ww->sub, FALSE);
   }
   
   /* refresh the window */
   redrawwin(ww->sub);
   redrawwin(ww->win);
   wnoutrefresh(ww->win);
   wnoutrefresh(ww->sub);
   
   wo->flags |= WDG_OBJ_VISIBLE;

   return WDG_ESUCCESS;
}

/* 
 * called when the window gets the focus
 */
static int wdg_dynlist_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_dynlist_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the window looses the focus
 */
static int wdg_dynlist_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* redraw the window */
   wdg_dynlist_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the window is focused
 */
static int wdg_dynlist_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);

   /* handle the message */
   switch (key) {
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x)) {
            if (wo->flags & WDG_OBJ_FOCUSED)
               wdg_dynlist_mouse(wo, key, mouse);
            else
               wdg_set_focus(wo);
         } else 
            return -WDG_ENOTHANDLED;
         break;
      
      case KEY_UP:
      case KEY_DOWN:
      case KEY_PPAGE:
      case KEY_NPAGE:
         wdg_dynlist_move(wo, key);
         break;
         
      case KEY_RETURN:
         if (ww->current)
            WDG_EXECUTE(ww->select_callback, ww->current);
         break;
         
      /* message not handled */
      default:
         return wdg_dynlist_callback(wo, key);
         break;
   }
  
   return WDG_ESUCCESS;
}

/*
 * draw the borders and title
 */
static void wdg_dynlist_border(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
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
 * set the callback for displaying the list
 */
void wdg_dynlist_print_callback(wdg_t *wo, void * func(int mode, void *list, char **desc, size_t len))
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   
   WDG_DEBUG_MSG("wdg_dynlist_print_callback %p", func);

   ww->func = func;
}

/*
 * set the select callback
 */
void wdg_dynlist_select_callback(wdg_t *wo, void (*callback)(void *))
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   
   WDG_DEBUG_MSG("wdg_dynlist_select_callback %p", callback);

   ww->select_callback = callback;
}

/*
 * add the callback on key pressed by the user
 */
void wdg_dynlist_add_callback(wdg_t *wo, int key, void (*callback)(void *))
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   struct wdg_dynlist_call *c;

   WDG_SAFE_CALLOC(c, 1, sizeof(struct wdg_dynlist_call));
   
   c->key = key;
   c->callback = callback;

   SLIST_INSERT_HEAD(&ww->callbacks, c, next);
}

/*
 * search for a callback and execute it
 */
static int wdg_dynlist_callback(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   struct wdg_dynlist_call *c;

   SLIST_FOREACH(c, &ww->callbacks, next) {
      if (c->key == key) {
         
         WDG_DEBUG_MSG("wdg_dynlist_callback");
         
         /* execute the callback only if current is not NULL */
         if (ww->current)
            WDG_EXECUTE(c->callback, ww->current);
         
         return WDG_ESUCCESS;
      }
   }

   return -WDG_ENOTHANDLED;
}


/*
 * refresh the list
 */
void wdg_dynlist_refresh(wdg_t *wo)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   size_t l = wdg_get_nlines(wo) - 4;
   size_t c = wdg_get_ncols(wo) - 4;
   size_t i = 0, found = 0;
   void *list, *next;
   char *desc;
  
   /* sanity check */
   if (ww->func == NULL)
      return;
   
   /* erase the window */
   werase(ww->sub);

   /* 
    * update the top (on the first element) in two case:
    *    top is not set
    *    bottom is not set (to update the list over the current element)
    */
   if (ww->top == NULL || ww->bottom == NULL) {
      /* retrieve the first element */
      ww->top = ww->func(0, NULL, NULL, 0);
   }

   /* no elements */
   if (ww->top == NULL)
      return;
   
   WDG_SAFE_CALLOC(desc, 100, sizeof(char));
   
   /* no current item, set it to the first element */
   if (ww->current == NULL)
      ww->current = ww->top;
  
   /* if the top does not exist any more, set it to the first */
   if (ww->func(0, ww->top, NULL, 0) == NULL)
      ww->top = ww->func(0, NULL, NULL, 0);
  
   /* start from the top element */
   list = ww->top;

   /* print all the entry until the bottom of the window */
   while (list) {
      next = ww->func(+1, list, &desc, 99);
      
      /* dont print string longer than the window */
      if (strlen(desc) > c)
         desc[c] = 0;

      /* the current item is selected */
      if (ww->current == list) {
         wattron(ww->sub, A_REVERSE);
         wmove(ww->sub, i, 0);
         whline(ww->sub, ' ', c);
         wprintw(ww->sub, "%s", desc);
         wattroff(ww->sub, A_REVERSE);
         wmove(ww->sub, i+1, 0);
         found = 1;
      } else {
         wprintw(ww->sub, "%s\n", desc);
      }

      /* exit when the bottom is reached */
      if (++i == l) {
         ww->bottom = list;
         break;
      } else {
         /* 
          * set to null, to have the bottom set only if the list 
          * is as long as the window
          */
         ww->bottom = NULL;
      }
      
      /* move the pointer */
      list = next;
   }

   /* if the current element does not exist anymore, set it to 'top' */
   if (!found)
      ww->current = ww->top;
   
   WDG_SAFE_FREE(desc);

   wnoutrefresh(ww->sub);
}

/*
 * move the focus
 */
static void wdg_dynlist_move(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   size_t l = wdg_get_nlines(wo) - 4;
   size_t i = 0;
   void *first, *prev, *next;

   /* retrieve the first element of the list */
   first = ww->func(0, NULL, NULL, 0);
   
   switch (key) {
      case KEY_UP:
         prev = ww->func(-1, ww->current, NULL, 0);
         /* we are on the first element */
         if (ww->current == first)
            return;
        
         /* move up the list if we are on the top */
         if (ww->current == ww->top)
            ww->top = prev;
         
         /* update the current element */
         ww->current = prev;
         
         break;
         
      case KEY_DOWN:
         next = ww->func(+1, ww->current, NULL, 0);
         /* we are on the last element */
         if (next == NULL)
            return;
        
         /* move the top if we are on the bottom (scroll the list) */
         if (ww->current == ww->bottom)
            ww->top = ww->func(+1, ww->top, NULL, 0);

         /* update the current element */
         ww->current = next;
         
         break;

      case KEY_PPAGE:
         
         while (ww->current != first) {
            prev = ww->func(-1, ww->current, NULL, 0);
            /* move up the list if we are on the top */
            if (ww->current == ww->top)
               ww->top = prev;
            /* update the current element */
            ww->current = prev;
            /* move only one page */
            if (++i == l-1)
               break;
         }
         
         break;
         
      case KEY_NPAGE:
         
         while ((next = ww->func(+1, ww->current, NULL, 0)) != NULL) {
            /* move the top if we are on the bottom (scroll the list) */
            if (ww->current == ww->bottom) {
               ww->top = ww->func(+1, ww->top, NULL, 0);
               ww->bottom = ww->func(+1, ww->bottom, NULL, 0);
            }
            /* update the current element */
            ww->current = next;
            /* move only one page */
            if (++i == l-1)
               break;
         }
         
         break;
   }

   wdg_dynlist_refresh(wo);
}

/*
 * move the focus with the mouse
 */
static void wdg_dynlist_mouse(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);
   /* we are for sure within the edges */
   size_t y = wdg_get_begin_y(wo) + 2;
   size_t line, i = 0;
   void *next;
   
   /* calculate which line was selected */
   line = mouse->y - y;

   /* calculate the distance from the top */
   ww->current = ww->top;
  
   while (line != 0 && (next = ww->func(+1, ww->current, NULL, 0)) != NULL) {
      /* update the current element */
      ww->current = next;
      /* move until the selected line */
      if (++i == line)
         break;
   }

   /* if double click, execute the callback */
   if (mouse->event == BUTTON1_DOUBLE_CLICKED)
      if (ww->current)
         WDG_EXECUTE(ww->select_callback, ww->current);

   wdg_dynlist_refresh(wo);
}

/*
 * reset the focus pointer 
 */
void wdg_dynlist_reset(wdg_t *wo)
{
   WDG_WO_EXT(struct wdg_dynlist, ww);

   ww->top = NULL;
   ww->current = NULL;
   ww->bottom = NULL;
   
   wdg_dynlist_refresh(wo);
}

/* EOF */

// vim:ts=3:expandtab

