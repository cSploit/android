/*
    WDG -- scroll widget

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

struct wdg_scroll {
   WINDOW *win;
   WINDOW *sub;
   size_t y_scroll;
   size_t y_max;
};

#define WDG_PAD_REFRESH(ww, c, l, x, y) pnoutrefresh(ww->sub, ww->y_scroll + 1, 0, y + 1, x + 1, y + l - 2, x + c - 2)

/* PROTOS */

void wdg_create_scroll(struct wdg_object *wo);

static int wdg_scroll_destroy(struct wdg_object *wo);
static int wdg_scroll_resize(struct wdg_object *wo);
static int wdg_scroll_redraw(struct wdg_object *wo);
static int wdg_scroll_get_focus(struct wdg_object *wo);
static int wdg_scroll_lost_focus(struct wdg_object *wo);
static int wdg_scroll_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_scroll_border(struct wdg_object *wo);

void wdg_scroll_erase(wdg_t *wo);
void wdg_scroll_print(wdg_t *wo, int color, char *fmt, ...);
void wdg_scroll_set_lines(wdg_t *wo, size_t lines);
static void wdg_set_scroll(struct wdg_object *wo, int s);
static void wdg_mouse_scroll(struct wdg_object *wo, int s);

/*******************************************/

/* 
 * called to create a window
 */
void wdg_create_scroll(struct wdg_object *wo)
{
   WDG_DEBUG_MSG("wdg_create_scroll");
   
   /* set the callbacks */
   wo->destroy = wdg_scroll_destroy;
   wo->resize = wdg_scroll_resize;
   wo->redraw = wdg_scroll_redraw;
   wo->get_focus = wdg_scroll_get_focus;
   wo->lost_focus = wdg_scroll_lost_focus;
   wo->get_msg = wdg_scroll_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_scroll));
}

/* 
 * called to destroy a window
 */
static int wdg_scroll_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   
   WDG_DEBUG_MSG("wdg_scroll_destroy");

   /* erase the window */
   wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
   wbkgd(ww->sub, COLOR_PAIR(wo->screen_color));
   werase(ww->sub);
   werase(ww->win);
   wnoutrefresh(ww->win);
   
   /* dealloc the structures */
   delwin(ww->sub);
   delwin(ww->win);

   WDG_SAFE_FREE(wo->extend);

   return WDG_ESUCCESS;
}

/* 
 * called to resize a window
 */
static int wdg_scroll_resize(struct wdg_object *wo)
{
   wdg_scroll_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw a window
 */
static int wdg_scroll_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   
   WDG_DEBUG_MSG("wdg_scroll_redraw");
 
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
      wdg_scroll_border(wo);
      
      /* set the window color */
      wbkgd(ww->sub, COLOR_PAIR(wo->window_color));
      touchwin(ww->sub);
      /* the pad is resized only orizzontally.
       * the vertical dimension can be larger than the screen */
      wdg_scroll_set_lines(wo, ww->y_max);
      /* refresh it */
      WDG_PAD_REFRESH(ww, c, l, x, y);

   /* the first time we have to allocate the window */
   } else {
      
      /* a default value waiting for wdg_scroll_set_lines() */
      ww->y_max = l * 5;

      /* create the outher window */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the borders */
      wdg_scroll_border(wo);
      /* initialize the pointer */
      wdg_set_scroll(wo, ww->y_max - l + 1);
      
      /* create the inner (actual) window */
      if ((ww->sub = newpad(ww->y_max, c - 2)) == NULL)
         return -WDG_EFATAL;
      
      /* set the window color */
      wbkgd(ww->sub, COLOR_PAIR(wo->window_color));
      touchwin(ww->sub);

      /* move to the bottom of the pad */
      wmove(ww->sub, ww->y_scroll + 1, 0);

      /* permit scroll in the pad */
      scrollok(ww->sub, TRUE);
   }
  
   /* refresh the window */
   touchwin(ww->sub);
   wnoutrefresh(ww->win);
   WDG_PAD_REFRESH(ww, c, l, x, y);
   
   wo->flags |= WDG_OBJ_VISIBLE;

   return WDG_ESUCCESS;
}

/* 
 * called when the window gets the focus
 */
static int wdg_scroll_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_scroll_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the window looses the focus
 */
static int wdg_scroll_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* redraw the window */
   wdg_scroll_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the window is focused
 */
static int wdg_scroll_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
  
   /* handle the message */
   switch (key) {
         
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x)) {
            /* get the focus only if it was not focused */
            if (!(wo->flags & WDG_OBJ_FOCUSED))
               wdg_set_focus(wo);
            if (mouse->x == x + c - 1 && (mouse->y >= y + 1 && mouse->y <= y + l - 1)) {
               wdg_mouse_scroll(wo, mouse->y);
               WDG_PAD_REFRESH(ww, c, l, x, y);
               wnoutrefresh(ww->win);
            }
         } else 
            return -WDG_ENOTHANDLED;
         break;

      /* handle scrolling of the pad */
      case KEY_UP:
         wdg_set_scroll(wo, ww->y_scroll - 1);
         WDG_PAD_REFRESH(ww, c, l, x, y);
         wnoutrefresh(ww->win);
         break;
         
      case KEY_DOWN:
         wdg_set_scroll(wo, ww->y_scroll + 1);
         WDG_PAD_REFRESH(ww, c, l, x, y);
         wnoutrefresh(ww->win);
         break;
         
      case KEY_NPAGE:
         wdg_set_scroll(wo, ww->y_scroll + (l - 2));
         WDG_PAD_REFRESH(ww, c, l, x, y);
         wnoutrefresh(ww->win);
         break;
         
      case KEY_PPAGE:
         wdg_set_scroll(wo, ww->y_scroll - (l - 2));
         WDG_PAD_REFRESH(ww, c, l, x, y);
         wnoutrefresh(ww->win);
         break;
         
      /* message not handled */
      default:
         return -WDG_ENOTHANDLED;
         break;
   }
  
   return WDG_ESUCCESS;
}

/*
 * draw the borders and title
 */
static void wdg_scroll_border(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
      
   /* the object was focused */
   if (wo->flags & WDG_OBJ_FOCUSED) {
      wattron(ww->win, A_BOLD);
      wbkgdset(ww->win, COLOR_PAIR(wo->focus_color));
   } else
      wbkgdset(ww->win, COLOR_PAIR(wo->border_color));

   /* draw the borders */
   box(ww->win, 0, 0);

   /* draw the elevator */
   wdg_set_scroll(wo, ww->y_scroll);
   
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
   if (wo->flags & WDG_OBJ_FOCUSED) {
      wattroff(ww->win, A_BOLD);
      wbkgdset(ww->win, COLOR_PAIR(wo->focus_color));
   } else
      wbkgdset(ww->win, COLOR_PAIR(wo->border_color));
}

/*
 * erase the subwindow
 */
void wdg_scroll_erase(wdg_t *wo)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   
   werase(ww->sub);
   
   WDG_PAD_REFRESH(ww, c, l, x, y);
}

/*
 * print a string in the window
 */
void wdg_scroll_print(wdg_t *wo, int color, char *fmt, ...)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
   va_list ap;
   
   WDG_DEBUG_MSG("wdg_scroll_print");

   /* move to the bottom of the pad */
   wdg_set_scroll(wo, ww->y_max - l + 1);

   wbkgdset(ww->sub, COLOR_PAIR(color));

   /* print the message */
   va_start(ap, fmt);
   vw_printw(ww->sub, fmt, ap);
   va_end(ap);
   
   wbkgdset(ww->sub, COLOR_PAIR(wo->window_color));
   
   WDG_PAD_REFRESH(ww, c, l, x, y);
}

/*
 * set the dimension of the pad
 */
void wdg_scroll_set_lines(wdg_t *wo, size_t lines)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t oldlines = ww->y_max;
   
   WDG_DEBUG_MSG("wdg_scroll_set_lines");
  
   /* resize the pad */
   wresize(ww->sub, lines, c - 2);
    
   /* do the proper adjustements to the scroller */
   ww->y_max = lines;
   wdg_set_scroll(wo, ww->y_max - l + 1);
   
   /* adjust only the first time (when the user requests the change) */
   if (oldlines != lines)
      wmove(ww->sub, ww->y_scroll + 1, 0);
}

/*
 * set the scroll pointer and redraw 
 * the window elevator
 */
static void wdg_set_scroll(struct wdg_object *wo, int s)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   int min = 0;
   int max = ww->y_max - l + 1;
   size_t height, vpos;
   
   /* don't go above max and below min */
   if (s < min) s = min;
   if (s > max) s = max;
   
   ww->y_scroll = s;

   /* compute the scroller cohordinates */
   height = (l - 2) * (l - 2) / ww->y_max;
   vpos = l * ww->y_scroll / ww->y_max;

   height = (height < 1) ? 1 : height;

   vpos = (vpos == 0) ? 1 : vpos;
   vpos = (vpos > (l - 1) - height) ? (l - 1) - height : vpos;

   /* hack to make sure to hit the bottom */
   if (ww->y_scroll == (size_t)max)
      vpos = l - 1 - height;

   /* draw the scroller */
   wmove(ww->win, 1, c - 1);
   wvline(ww->win, ACS_CKBOARD, l - 2);
   wattron(ww->win, A_REVERSE);
   wmove(ww->win, vpos, c - 1);
   wvline(ww->win, ACS_DIAMOND, height);
   wattroff(ww->win, A_REVERSE);
   
}

/*
 * jump to a scroll position with the mouse 
 */
static void wdg_mouse_scroll(struct wdg_object *wo, int s)
{
   WDG_WO_EXT(struct wdg_scroll, ww);
   size_t l = wdg_get_nlines(wo);
   size_t y = wdg_get_begin_y(wo);
   size_t base;

   /* calculate the relative displacement */
   base = s - y - 1;

   /* special case for top and bottom */
   if (base == 0)
      s = 0;
   else if (base == l - 3)
      s = ww->y_max - l + 1;
   /* else calulate the proportion */
   else
      s = base * ww->y_max / (l - 2);

   /* set the scroller */
   wdg_set_scroll(wo, s);
}


/* EOF */

// vim:ts=3:expandtab

