/*
    WDG -- compound widget (can contain other widgets)

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

struct wdg_compound_call {
   int key;
   void (*callback)(void);
   SLIST_ENTRY(wdg_compound_call) next;
};

struct wdg_widget_list {
   struct wdg_object *wdg;
   TAILQ_ENTRY(wdg_widget_list) next;
};

struct wdg_compound {
   WINDOW *win;
   struct wdg_widget_list *focused;
   TAILQ_HEAD(wtail, wdg_widget_list) widgets_list;
   SLIST_HEAD(, wdg_compound_call) callbacks;
};

/* PROTOS */

void wdg_create_compound(struct wdg_object *wo);

static int wdg_compound_destroy(struct wdg_object *wo);
static int wdg_compound_resize(struct wdg_object *wo);
static int wdg_compound_redraw(struct wdg_object *wo);
static int wdg_compound_get_focus(struct wdg_object *wo);
static int wdg_compound_lost_focus(struct wdg_object *wo);
static int wdg_compound_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_compound_border(struct wdg_object *wo);
static void wdg_compound_move(struct wdg_object *wo, int key);
static int wdg_compound_dispatch(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);
void wdg_compound_add(wdg_t *wo, wdg_t *widget);
void wdg_compound_set_focus(wdg_t *wo, wdg_t *widget);
wdg_t * wdg_compound_get_focused(wdg_t *wo);
void wdg_compound_add_callback(wdg_t *wo, int key, void (*callback)(void));

/*******************************************/

/* 
 * called to create a compound
 */
void wdg_create_compound(struct wdg_object *wo)
{
   struct wdg_compound *ww;
   
   WDG_DEBUG_MSG("wdg_create_compound");
   
   /* set the callbacks */
   wo->destroy = wdg_compound_destroy;
   wo->resize = wdg_compound_resize;
   wo->redraw = wdg_compound_redraw;
   wo->get_focus = wdg_compound_get_focus;
   wo->lost_focus = wdg_compound_lost_focus;
   wo->get_msg = wdg_compound_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_compound));

   ww = (struct wdg_compound *)wo->extend;
   
   TAILQ_INIT(&ww->widgets_list);
}

/* 
 * called to destroy a compound
 */
static int wdg_compound_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e, *tmp;
   struct wdg_compound_call *c;
   
   WDG_DEBUG_MSG("wdg_compound_destroy");

   /* erase the window */
   wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
   werase(ww->win);
   wnoutrefresh(ww->win);
   
   /* dealloc the structures */
   delwin(ww->win);

   /* destroy all the contained widgets */
   TAILQ_FOREACH_SAFE(e, &(ww->widgets_list), next, tmp) {
      wdg_destroy_object(&e->wdg);
      WDG_SAFE_FREE(e);
   }
   
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
 * called to resize a compound
 */
static int wdg_compound_resize(struct wdg_object *wo)
{
   wdg_compound_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw a compound (it redraws all the contained widgets)
 */
static int wdg_compound_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e;
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   
   WDG_DEBUG_MSG("wdg_compound_redraw");
 
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
      wdg_compound_border(wo);
      
   /* the first time we have to allocate the window */
   } else {

      /* create the outher window */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the borders */
      wdg_compound_border(wo);

   }
   
   /* refresh the window */
   redrawwin(ww->win);
   wnoutrefresh(ww->win);
   
   wo->flags |= WDG_OBJ_VISIBLE;

   /* redraw all the contained widget */
   TAILQ_FOREACH(e, &(ww->widgets_list), next) {
      wdg_draw_object(e->wdg);
   }

   return WDG_ESUCCESS;
}

/* 
 * called when the compound gets the focus
 */
static int wdg_compound_get_focus(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e;
   
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;
   
   /* set the focus on the focused widget */
   TAILQ_FOREACH(e, &(ww->widgets_list), next) {
      if (e == ww->focused)
         e->wdg->flags |= WDG_OBJ_FOCUSED;
   }

   /* redraw the window */
   wdg_compound_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the compound looses the focus
 */
static int wdg_compound_lost_focus(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e;
   
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* remove the focus from the focused widget */
   TAILQ_FOREACH(e, &(ww->widgets_list), next) {
      if (e == ww->focused)
         e->wdg->flags &= ~WDG_OBJ_FOCUSED;
   }
   
   /* redraw the window */
   wdg_compound_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the compound is focused
 */
static int wdg_compound_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *wl;

   /* handle the message */
   switch (key) {
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x)) {
            wdg_set_focus(wo);
            /* dispatch to the proper widget */
            TAILQ_FOREACH(wl, &ww->widgets_list, next)
               if (wl->wdg->get_msg(wl->wdg, key, mouse) == WDG_ESUCCESS) {
                  /* 
                   * the widget has handled the message,
                   * set to the focused one 
                   */
                  ww->focused = wl;
                  /* 
                   * regain focus to the compound
                   * this is needed because it is always the 
                   * compound that must receive the messages
                   */
                  wdg_set_focus(wo);
               }
         }
         else 
            return -WDG_ENOTHANDLED;
         break;

      /* move the focus */
      case KEY_LEFT:
      case KEY_RIGHT:
         wdg_compound_move(wo, key);
         break;
         
      /* dispatch the message to the focused widget */
      default:
         return wdg_compound_dispatch(wo, key, mouse);
         break;
   }
  
   return WDG_ESUCCESS;
}

/*
 * move the focus thru the contained widgets
 */
static void wdg_compound_move(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_compound, ww);

   if (ww->focused == NULL)
      return;
  

   /* move the focus to the right widget */
   if (key == KEY_LEFT) {
      WDG_DEBUG_MSG("wdg_compound_move: prev");
      
     if (TAILQ_PREV(ww->focused, wtail, next) != NULL) {
        /* remove the focus from the current object */
        ww->focused->wdg->flags &= ~WDG_OBJ_FOCUSED;
        ww->focused = TAILQ_PREV(ww->focused, wtail, next);
        /* give the focus to the new one */
        ww->focused->wdg->flags |= WDG_OBJ_FOCUSED;
     }
   } else if (key == KEY_RIGHT) {
     WDG_DEBUG_MSG("wdg_compound_move: next");
      
     if (TAILQ_NEXT(ww->focused, next) != NULL) {
        /* remove the focus from the current object */
        ww->focused->wdg->flags &= ~WDG_OBJ_FOCUSED;
        ww->focused = TAILQ_NEXT(ww->focused, next);
        /* give the focus to the new one */
        ww->focused->wdg->flags |= WDG_OBJ_FOCUSED;
     }
   }

   /* repaint the object */
   wdg_compound_redraw(wo);
}

/*
 * dispatch the key to the focused widget
 */
static int wdg_compound_dispatch(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_compound_call *c;

   /* first: check if the key is linked to a callback */
   SLIST_FOREACH(c, &ww->callbacks, next) {
      if (c->key == key) {
         
         WDG_DEBUG_MSG("wdg_compound_callback");
         
         /* execute the callback */
         WDG_EXECUTE(c->callback);
         
         return WDG_ESUCCESS;
      }
   }

   /* pass the message to the focused widget */
   return ww->focused->wdg->get_msg(ww->focused->wdg, key, mouse);
}


/*
 * draw the borders and title
 */
static void wdg_compound_border(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_compound, ww);
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
 * add a widget to the compound
 */
void wdg_compound_add(wdg_t *wo, wdg_t *widget)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e;

   WDG_SAFE_CALLOC(e, 1, sizeof(struct wdg_widget_list));

   e->wdg = widget;

   TAILQ_INSERT_TAIL(&(ww->widgets_list), e, next);

   /* set the first focused widget */
   if (ww->focused == NULL)
      ww->focused = e;
}

/*
 * set the focus on a widget contained in the compound
 */
void wdg_compound_set_focus(wdg_t *wo, wdg_t *widget)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e;

   TAILQ_FOREACH(e, &ww->widgets_list, next) {
      
     /* remove the focus from the current object */
     if (e->wdg->flags & WDG_OBJ_FOCUSED)
        ww->focused->wdg->flags &= ~WDG_OBJ_FOCUSED;

     if (e->wdg == widget) {
        /* give the focus to the new one */
        ww->focused->wdg->flags |= WDG_OBJ_FOCUSED;
     }
   }
}

/*
 * returns the current focused widget
 */
wdg_t * wdg_compound_get_focused(wdg_t *wo)
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_widget_list *e;

   /* search and return the focused one */
   TAILQ_FOREACH(e, &ww->widgets_list, next)
     if (e->wdg->flags & WDG_OBJ_FOCUSED)
        return e->wdg;
  
   return NULL;
}

/*
 * add the callback on key presse by the user
 */
void wdg_compound_add_callback(wdg_t *wo, int key, void (*callback)(void))
{
   WDG_WO_EXT(struct wdg_compound, ww);
   struct wdg_compound_call *c;

   WDG_SAFE_CALLOC(c, 1, sizeof(struct wdg_compound_call));
   
   c->key = key;
   c->callback = callback;

   SLIST_INSERT_HEAD(&ww->callbacks, c, next);
}

/* EOF */

// vim:ts=3:expandtab

