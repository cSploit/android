/*
    WDG -- dialog widget

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
   
struct wdg_button {
   char *label;
   char selected;
   void (*callback)(void);
};

#define WDG_DIALOG_MAX_BUTTON 4

struct wdg_dialog {
   WINDOW *win;
   WINDOW *sub;
   size_t flags;
   char *text;
   size_t focus_button;
   struct wdg_button buttons[WDG_DIALOG_MAX_BUTTON];
};

/* PROTOS */

void wdg_create_dialog(struct wdg_object *wo);

static int wdg_dialog_destroy(struct wdg_object *wo);
static int wdg_dialog_resize(struct wdg_object *wo);
static int wdg_dialog_redraw(struct wdg_object *wo);
static int wdg_dialog_get_focus(struct wdg_object *wo);
static int wdg_dialog_lost_focus(struct wdg_object *wo);
static int wdg_dialog_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_dialog_border(struct wdg_object *wo);
static void wdg_dialog_buttons(struct wdg_object *wo);
static void wdg_dialog_get_size(struct wdg_object *wo, size_t *lines, size_t *cols);
static void wdg_dialog_move(struct wdg_object *wo, int key);
static int wdg_dialog_mouse_move(struct wdg_object *wo, struct wdg_mouse_event *mouse);
static void wdg_dialog_callback(struct wdg_object *wo);

void wdg_dialog_text(wdg_t *wo, size_t flags, const char *text);
void wdg_dialog_add_callback(wdg_t *wo, size_t flag, void (*callback)(void));

/*******************************************/

/* 
 * called to create a window
 */
void wdg_create_dialog(struct wdg_object *wo)
{
   struct wdg_dialog *ww;

   WDG_DEBUG_MSG("wdg_create_dialog");
   
   /* set the callbacks */
   wo->destroy = wdg_dialog_destroy;
   wo->resize = wdg_dialog_resize;
   wo->redraw = wdg_dialog_redraw;
   wo->get_focus = wdg_dialog_get_focus;
   wo->lost_focus = wdg_dialog_lost_focus;
   wo->get_msg = wdg_dialog_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_dialog));

   ww = (struct wdg_dialog *)wo->extend;
  
   /* initialize the labels, the other fields are zeroed by the calloc */
   ww->buttons[0].label = " Ok ";
   ww->buttons[1].label = " Yes ";
   ww->buttons[2].label = " No ";
   ww->buttons[3].label = " Cancel ";
}

/* 
 * called to destroy a window
 */
static int wdg_dialog_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   
   WDG_DEBUG_MSG("wdg_dialog_destroy");

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

   /* free the text string */
   WDG_SAFE_FREE(ww->text);
   WDG_BUG_IF(ww->text != NULL);

   WDG_SAFE_FREE(wo->extend);

   return WDG_ESUCCESS;
}

/* 
 * called to resize a window
 */
static int wdg_dialog_resize(struct wdg_object *wo)
{
   wdg_dialog_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw a window
 */
static int wdg_dialog_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   size_t c, l, x, y;
   size_t lines, cols;
   
   WDG_DEBUG_MSG("wdg_dialog_redraw");
 
   /* calculate the dimension and position */
   wdg_dialog_get_size(wo, &lines, &cols);

   /* center on the screen, but not outside the edges */
   if (cols + 4 >= current_screen.cols)
      wo->x1 = 0;
   else
      wo->x1 = (current_screen.cols - (cols + 4)) / 2;
   
   wo->y1 = (current_screen.lines - (lines + 4)) / 2;
   wo->x2 = -wo->x1;
   wo->y2 = -wo->y1;
   
   /* get the cohorditates as usual */
   c = wdg_get_ncols(wo);
   l = wdg_get_nlines(wo);
   x = wdg_get_begin_x(wo);
   y = wdg_get_begin_y(wo);
   
   /* deal with rouding */
   if (l != lines + 4) l = lines + 4;
   if (c != cols + 4) c = cols + 4;
 
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
      wdg_dialog_border(wo);
      wdg_dialog_buttons(wo);
      
      /* resize the actual window and touch it */
      mvwin(ww->sub, y + 2, x + 2);
      wresize(ww->sub, l - 4, c - 4);
      /* set the window color */
      wbkgdset(ww->sub, COLOR_PAIR(wo->window_color));

   /* the first time we have to allocate the window */
   } else {

      /* create the outher window */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the borders */
      wdg_dialog_border(wo);
      wdg_dialog_buttons(wo);

      /* create the inner (actual) window */
      if ((ww->sub = newwin(l - 4, c - 4, y + 2, x + 2)) == NULL)
         return -WDG_EFATAL;
      
      /* set the window color */
      wbkgdset(ww->sub, COLOR_PAIR(wo->window_color));
      werase(ww->sub);
      redrawwin(ww->sub);

   }
  
   /* print the message text */
   wmove(ww->sub, 0, 0);
   wprintw(ww->sub, ww->text);
   
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
static int wdg_dialog_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_dialog_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the window looses the focus
 */
static int wdg_dialog_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* redraw the window */
   wdg_dialog_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the window is focused
 */
static int wdg_dialog_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_dialog, ww);

   /* handle the message */
   switch (key) {
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x)) {
            wdg_set_focus(wo);
            /* if the mouse click was over a button */
            if (wdg_dialog_mouse_move(wo, mouse) == WDG_ESUCCESS)
               wdg_dialog_callback(wo);
         } else 
            return -WDG_ENOTHANDLED;
         break;

      case KEY_LEFT:
      case KEY_RIGHT:
         wdg_dialog_move(wo, key);
         wdg_dialog_redraw(wo);
         break;

      case KEY_RETURN:
         wdg_dialog_callback(wo);
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
static void wdg_dialog_border(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   size_t c = wdg_get_ncols(wo);
      
   /* fill the window with color */
   wbkgdset(ww->win, COLOR_PAIR(wo->window_color));
   werase(ww->win);
   
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
 * set the dialog attributes and text message
 */
void wdg_dialog_text(wdg_t *wo, size_t flags, const char *text)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   int first = 1;

   ww->flags = flags;
   WDG_SAFE_STRDUP(ww->text, text);
   
   /* mark the buttons to be used */
   if (ww->flags & WDG_OK) {
      ww->buttons[0].selected = 1;
      if (first) {
         ww->focus_button = 0;
         first = 0;
      }
   }
   if (ww->flags & WDG_YES) {
      ww->buttons[1].selected = 1;
      if (first) {
         ww->focus_button = 1;
         first = 0;
      }
   }
   if (ww->flags & WDG_NO) {
      ww->buttons[2].selected = 1;
      if (first) {
         ww->focus_button = 2;
         first = 0;
      }
   }
   if (ww->flags & WDG_CANCEL) {
      ww->buttons[3].selected = 1;
      if (first) {
         ww->focus_button = 3;
         first = 0;
      }
   }
}

/*
 * returns how many lines and cols are necessary for displaying the message
 */
static void wdg_dialog_get_size(struct wdg_object *wo, size_t *lines, size_t *cols)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   char *p;
   size_t t = 0;

   /* initialize them */
   *lines = 1;
   *cols = 0;
   
   /* 
    * parse the text message and find how many '\n' are present.
    * also calculate the longest string between two '\n'
    */
   for (p = ww->text; p < ww->text + strlen(ww->text); p++) {
      /* count the chars */
      t++;
      /* at the newline (or end of string) make the calculus */
      if (*p == '\n' || *(p + 1) == '\0') {
         (*lines)++;
         /* cols have to be the max line length */
         if (*cols < t)
            *cols = t;
         /* reset the counter */
         t = 0;
      }
   }

   /* if there were no '\n' */
   if (*cols == 0)
      *cols = t;
 
   if (ww->flags != WDG_NO_BUTTONS)
      /* add the lines for the buttons */
      *lines += 2;

}

/*
 * display the dialog buttons
 */
static void wdg_dialog_buttons(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   size_t i, l, c;

   /* no button to be displayed */
   if (ww->flags == WDG_NO_BUTTONS)
      return;
   
   /* get the line of the message */
   wdg_dialog_get_size(wo, &l, &c);

   /* calculate the length of the buttons */
   for (i = 0; i < WDG_DIALOG_MAX_BUTTON; i++) 
      if (ww->buttons[i].selected)
         c -= strlen(ww->buttons[i].label);

   /* move the cursor to the right position (centered) */
   wmove(ww->sub, l - 1, c / 2);
   
   /* print the buttons */
   for (i = 0; i < WDG_DIALOG_MAX_BUTTON; i++) {
      if (ww->buttons[i].selected) {
         if (ww->focus_button == i)
            wattron(ww->sub, A_REVERSE);
         
         wprintw(ww->sub, "%s", ww->buttons[i].label);
         
         wattroff(ww->sub, A_REVERSE);
      }
   }
}

/*
 * move the focus thru menu units 
 */
static void wdg_dialog_move(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   int i = ww->focus_button;
   
   switch(key) {
      case KEY_RIGHT:
         /* move until we found a selected button */
         while (!ww->buttons[++i].selected);
         /* if we are in the edges, set the focus to the new button */
         if ( i < WDG_DIALOG_MAX_BUTTON && ww->buttons[i].selected)
            ww->focus_button = i;
         break;
         
      case KEY_LEFT:
         /* move until we found a selected button */
         while (!ww->buttons[--i].selected);
         /* if we are in the edges, set the focus to the new button */
         if (i >= 0 && ww->buttons[i].selected)
            ww->focus_button = i;
         break;
   }
}

/*
 * select the focus with a mouse event
 */
static int wdg_dialog_mouse_move(struct wdg_object *wo, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   size_t y = wdg_get_begin_y(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t i, l, c;
   
   /* get the line of the message */
   wdg_dialog_get_size(wo, &l, &c);

   /* not on the button line */
   if (mouse->y != y + 2 + l - 1)
      return -WDG_ENOTHANDLED;

   /* calculate the length of the buttons */
   for (i = 0; i < WDG_DIALOG_MAX_BUTTON; i++) 
      if (ww->buttons[i].selected)
         c -= strlen(ww->buttons[i].label);

   /* buttons start here */
   x += c / 2;
  
   for (i = 0; i < WDG_DIALOG_MAX_BUTTON; i++) {
      /* if the mouse is over a title */
      if (mouse->x >= x && mouse->x < x + strlen(ww->buttons[i].label) ) {
         ww->focus_button = i;
         return WDG_ESUCCESS;
      }
      /* move the pointer */   
      x += strlen(ww->buttons[i].label);
   }    
   
   return -WDG_ENOTHANDLED;
}

/*
 * destroy the dialog and
 * call the function associated to the button
 */
static void wdg_dialog_callback(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_dialog, ww);
   void (*callback)(void);
   
   WDG_DEBUG_MSG("wdg_dialog_callback");
   
   callback = ww->buttons[ww->focus_button].callback;
   wdg_destroy_object(&wo);
   wdg_redraw_all();
   WDG_EXECUTE(callback);
}

/*
 * the user should use it to associate a callback to a button
 */
void wdg_dialog_add_callback(wdg_t *wo, size_t flag, void (*callback)(void))
{
   WDG_WO_EXT(struct wdg_dialog, ww);

   if (flag & WDG_OK)
      ww->buttons[0].callback = callback;
   
   if (flag & WDG_YES)
      ww->buttons[1].callback = callback;
   
   if (flag & WDG_NO)
      ww->buttons[2].callback = callback;
   
   if (flag & WDG_CANCEL)
      ww->buttons[3].callback = callback;
}

/* EOF */

// vim:ts=3:expandtab

