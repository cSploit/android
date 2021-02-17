/*
    WDG -- percentage widget

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

struct wdg_percentage {
   WINDOW *win;
   WINDOW *sub;
   size_t percent;
   char interrupt;
};

/* PROTOS */

void wdg_create_percentage(struct wdg_object *wo);

static int wdg_percentage_destroy(struct wdg_object *wo);
static int wdg_percentage_resize(struct wdg_object *wo);
static int wdg_percentage_redraw(struct wdg_object *wo);
static int wdg_percentage_get_focus(struct wdg_object *wo);
static int wdg_percentage_lost_focus(struct wdg_object *wo);
static int wdg_percentage_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_percentage_border(struct wdg_object *wo);

int wdg_percentage_set(wdg_t *wo, size_t p, size_t max);

/*******************************************/

/* 
 * called to create a window
 */
void wdg_create_percentage(struct wdg_object *wo)
{
   WDG_DEBUG_MSG("wdg_create_percentage");
   
   /* set the callbacks */
   wo->destroy = wdg_percentage_destroy;
   wo->resize = wdg_percentage_resize;
   wo->redraw = wdg_percentage_redraw;
   wo->get_focus = wdg_percentage_get_focus;
   wo->lost_focus = wdg_percentage_lost_focus;
   wo->get_msg = wdg_percentage_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_percentage));
}

/* 
 * called to destroy a window
 */
static int wdg_percentage_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_percentage, ww);

   WDG_DEBUG_MSG("wdg_percentage_destroy");

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

   WDG_SAFE_FREE(wo->extend);

   return WDG_ESUCCESS;
}

/* 
 * called to resize a window
 */
static int wdg_percentage_resize(struct wdg_object *wo)
{
   wdg_percentage_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw a window
 */
static int wdg_percentage_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_percentage, ww);
   size_t c, l, x, y;
   size_t cols;
   
   /* calculate the dimension and position */
   cols = strlen(wo->title) + 2;

   /* set the minimum */
   if (cols < 45)
      cols = 45;

   /* center on the screen, but not outside the edges */
   if (cols + 4 >= current_screen.cols)
      wo->x1 = 0;
   else
      wo->x1 = (current_screen.cols - (cols + 4)) / 2;
  
   wo->y1 = (current_screen.lines - 7) / 2;
   wo->x2 = -wo->x1;
   wo->y2 = -wo->y1;
   
   c = wdg_get_ncols(wo);
   l = wdg_get_nlines(wo);
   x = wdg_get_begin_x(wo);
   y = wdg_get_begin_y(wo);
 
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
      wdg_percentage_border(wo);
      
      /* resize the actual window and touch it */
      mvwin(ww->sub, y + 1, x + 1);
      wresize(ww->sub, l - 2, c - 2);
      /* set the window color */
      wbkgdset(ww->sub, COLOR_PAIR(wo->window_color));

   /* the first time we have to allocate the window */
   } else {

      /* create the outher window */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the borders */
      wdg_percentage_border(wo);

      /* create the inner (actual) window */
      if ((ww->sub = newwin(l - 2, c - 2, y + 1, x + 1)) == NULL)
         return -WDG_EFATAL;
      
      /* set the window color */
      wbkgdset(ww->sub, COLOR_PAIR(wo->window_color));
      werase(ww->sub);
      redrawwin(ww->sub);

      /* initialize the pointer */
      wmove(ww->sub, 0, 0);

      scrollok(ww->sub, TRUE);

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
static int wdg_percentage_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_percentage_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the window looses the focus
 */
static int wdg_percentage_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* redraw the window */
   wdg_percentage_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the window is focused
 */
static int wdg_percentage_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_percentage, ww);
   
   /* handle the message */
   switch (key) {
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x))
            wdg_set_focus(wo);
         else 
            return -WDG_ENOTHANDLED;
         break;

      case KEY_ESC:
      case CTRL('Q'):
         WDG_DEBUG_MSG("wdg_percentage_get_msg: user interrupt");
         /* 
          * user has requested to stop this task.
          * the next time the percentage will be set
          * the object will be destroyed and a correct value
          * will be returned.
          */
         ww->interrupt = 1;
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
static void wdg_percentage_border(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_percentage, ww);
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
      wmove(ww->sub, 1, 2);
      wprintw(ww->sub, wo->title);
   }
   
   /* restore the attribute */
   if (wo->flags & WDG_OBJ_FOCUSED)
      wattroff(ww->win, A_BOLD);

   /* draw the percentage bar */
   wmove(ww->sub, 3, 2);
   whline(ww->sub, ACS_CKBOARD, c - 6);
   
   wbkgdset(ww->sub, COLOR_PAIR(wo->title_color));
   //wattron(ww->sub, A_REVERSE);
   whline(ww->sub, ' ', ww->percent * (c - 6) / 100);
   //wattroff(ww->sub, A_REVERSE);
}

/*
 * set the percentage
 */
int wdg_percentage_set(wdg_t *wo, size_t p, size_t max)
{
   WDG_WO_EXT(struct wdg_percentage, ww);

   /* set the percentage */
   ww->percent = p * 100 / max;
   
   WDG_DEBUG_MSG("wdg_percentage_set: %d", ww->percent);

   wdg_percentage_redraw(wo);

   /* reached the max, selfdestruct */
   if (p == max) {
      WDG_DEBUG_MSG("wdg_percentage_set: FINISHED");
      wdg_destroy_object(&wo);
      wdg_redraw_all();
      return WDG_PERCENTAGE_FINISHED;
   }

   /* user has requested to stop the task */
   if (ww->interrupt) {
      WDG_DEBUG_MSG("wdg_percentage_set: INTERRUPTED");
      ww->interrupt = 0;
      wdg_destroy_object(&wo);
      wdg_redraw_all();
      return WDG_PERCENTAGE_INTERRUPTED; 
   }

   return WDG_PERCENTAGE_UPDATED;
}

/* EOF */

// vim:ts=3:expandtab

