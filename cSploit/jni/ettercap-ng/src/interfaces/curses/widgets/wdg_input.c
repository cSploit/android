/*
    WDG -- user input widget

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
#include <form.h>

#include <stdarg.h>

/* GLOBALS */

struct wdg_input_handle {
   WINDOW *win;
   FORM *form;
   WINDOW *fwin;
   FIELD **fields;
   size_t nfields;
   size_t x, y;
   char **buffers;
   void (*callback)(void);
};

/* PROTOS */

void wdg_create_input(struct wdg_object *wo);

static int wdg_input_destroy(struct wdg_object *wo);
static int wdg_input_resize(struct wdg_object *wo);
static int wdg_input_redraw(struct wdg_object *wo);
static int wdg_input_get_focus(struct wdg_object *wo);
static int wdg_input_lost_focus(struct wdg_object *wo);
static int wdg_input_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

static void wdg_input_borders(struct wdg_object *wo);

static int wdg_input_virtualize(struct wdg_object *wo, int key);
static int wdg_input_driver(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);
static void wdg_input_form_destroy(struct wdg_object *wo);
static void wdg_input_form_create(struct wdg_object *wo);
static void wdg_input_consolidate(struct wdg_object *wo);

void wdg_input_size(wdg_t *wo, size_t x, size_t y);
void wdg_input_add(wdg_t *wo, size_t x, size_t y, const char *caption, char *buf, size_t len, size_t lines);
void wdg_input_set_callback(wdg_t *wo, void (*callback)(void));
void wdg_input_get_input(wdg_t *wo);

/*******************************************/

/* 
 * called to create the menu
 */
void wdg_create_input(struct wdg_object *wo)
{
   WDG_DEBUG_MSG("wdg_create_input");
   
   /* set the callbacks */
   wo->destroy = wdg_input_destroy;
   wo->resize = wdg_input_resize;
   wo->redraw = wdg_input_redraw;
   wo->get_focus = wdg_input_get_focus;
   wo->lost_focus = wdg_input_lost_focus;
   wo->get_msg = wdg_input_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_input_handle));
}

/* 
 * called to destroy the menu
 */
static int wdg_input_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   size_t i = 0;
   
   WDG_DEBUG_MSG("wdg_input_destroy");

   /* erase the window */
   wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
   werase(ww->win);
   wnoutrefresh(ww->win);

   /* destroy the internal form */
   wdg_input_form_destroy(wo);
   
   /* dealloc the structures */
   delwin(ww->win);
   
   /* free all the items */
   while(ww->fields[i] != NULL) 
      free_field(ww->fields[i++]);

   /* free the array */
   WDG_SAFE_FREE(ww->fields);

   /* free the buffer array */
   WDG_SAFE_FREE(ww->buffers);
   
   WDG_SAFE_FREE(wo->extend);

   return WDG_ESUCCESS;
}

/* 
 * called to resize the menu
 */
static int wdg_input_resize(struct wdg_object *wo)
{
   wdg_input_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw the menu
 */
static int wdg_input_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   size_t c, l, x, y;
   
   WDG_DEBUG_MSG("wdg_input_redraw");
   
   /* center the window on the screen */
   wo->x1 = (current_screen.cols - (ww->x + 2)) / 2;
   wo->y1 = (current_screen.lines - (ww->y + 2)) / 2;
   wo->x2 = -wo->x1;
   wo->y2 = -wo->y1;
   
   c = wdg_get_ncols(wo);
   l = wdg_get_nlines(wo);
   x = wdg_get_begin_x(wo);
   y = wdg_get_begin_y(wo);
   
   /* deal with rouding */
   if (l != ww->y + 2) l = ww->y + 2;
   if (c != ww->x + 2) c = ww->x + 2;
 
   /* the window already exist */
   if (ww->win) {
      /* erase the border */
      wbkgd(ww->win, COLOR_PAIR(wo->screen_color));
      werase(ww->win);
      /* destroy the internal form */
      wdg_input_form_destroy(wo);
      
      touchwin(ww->win);
      wnoutrefresh(ww->win);

      /* set the form color */
      wbkgd(ww->win, COLOR_PAIR(wo->window_color));
     
      /* resize the window */
      mvwin(ww->win, y, x);
      wresize(ww->win, l, c);
      
      /* redraw the window */
      wdg_input_borders(wo);
      
      /* create the internal form */
      wdg_input_form_create(wo);
      
      touchwin(ww->win);

   /* the first time we have to allocate the window */
   } else {

      /* create the menu window (fixed dimensions) */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* set the window color */
      wbkgd(ww->win, COLOR_PAIR(wo->window_color));
      redrawwin(ww->win);
      
      /* draw the titles */
      wdg_input_borders(wo);

      /* create the internal form */
      wdg_input_form_create(wo);

      /* no scrolling for menu */
      scrollok(ww->win, FALSE);

   }
   
   /* refresh the window */
   touchwin(ww->win);
   wnoutrefresh(ww->win);
   
   touchwin(ww->fwin);
   wnoutrefresh(ww->fwin);

   wo->flags |= WDG_OBJ_VISIBLE;

   return WDG_ESUCCESS;
}

/* 
 * called when the menu gets the focus
 */
static int wdg_input_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_input_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the menu looses the focus
 */
static int wdg_input_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* redraw the window */
   wdg_input_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the menu is focused
 */
static int wdg_input_get_msg(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);

   WDG_DEBUG_MSG("keypress get msg: %d", key);
   
   /* handle the message */
   switch (key) {
         
      case KEY_MOUSE:
         /* is the mouse event within our edges ? */
         if (wenclose(ww->win, mouse->y, mouse->x)) {
            wdg_set_focus(wo);
            /* redraw the menu */
            wdg_input_redraw(wo);
         } else {
            return -WDG_ENOTHANDLED;
         }
         break;
      
      case KEY_ESC:
      case CTRL('Q'):
         wdg_destroy_object(&wo);
         wdg_redraw_all();
         return WDG_EFINISHED;
         break;

      /* message not handled */
      default:
         if (wo->flags & WDG_OBJ_FOCUSED) {
            return wdg_input_driver(wo, key, mouse);
         } else {
            return -WDG_ENOTHANDLED;
         }
         break;
   }
   
   return WDG_ESUCCESS;
}

/*
 * draw the menu titles
 */
static void wdg_input_borders(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
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
 * stransform keys into menu commands 
 */
static int wdg_input_virtualize(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   int c;
   
   switch (key) {
      case KEY_RETURN:
      case KEY_EXIT:
         c = MAX_FORM_COMMAND + 1;
         break;
      case KEY_UP:
      case KEY_LEFT:
         c =  REQ_PREV_FIELD;
         /* we are moving... unfocus the current field */
         set_field_back(current_field(ww->form), A_UNDERLINE);
         break;
      case KEY_DOWN:
      case KEY_RIGHT:
         c =  REQ_NEXT_FIELD;
         /* we are moving... unfocus the current field */
         set_field_back(current_field(ww->form), A_UNDERLINE);
         break;
      case KEY_BACKSPACE:
      case '\b':
      case 127:   /* how many code does it have ?? argh !! */
         c = REQ_DEL_PREV;
         break;
      case KEY_DC:
         c = REQ_DEL_CHAR;
         break;
      case KEY_HOME:
         c = REQ_BEG_FIELD;
         break;
      case KEY_END:
         c = REQ_END_FIELD;
         break;
      default:
         c = key;
         break;
   }
  
   /*    
    * Force the field that the user is typing into to be in reverse video,
    * while the other fields are shown underlined.
    */   
   //if (c <= KEY_MAX)
   //   set_field_back(current_field(ww->form), A_REVERSE);
   //else if (c <= MAX_FORM_COMMAND)
   //  set_field_back(current_field(ww->form), A_UNDERLINE);
   
   return c;
}

/*
 * sends command to the form
 */
static int wdg_input_driver(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   int c, v;
   
   WDG_DEBUG_MSG("keypress driver: %d", key);
   
   /* virtualize the command */
   c = form_driver(ww->form, (v = wdg_input_virtualize(wo, key)) );
   
   set_field_back(current_field(ww->form), A_REVERSE);
   
   /* one item has been selected */
   if (c == E_UNKNOWN_COMMAND) {
      /* send a command to the form in order to validate the current field */
      form_driver(ww->form, REQ_NEXT_FIELD);
      /* 
       * put the temp buffer in the real one 
       * call the callback
       * and destroy the object
       */
      wdg_input_consolidate(wo);
      return WDG_EFINISHED;
   }

   wnoutrefresh(ww->fwin);
      
   return WDG_ESUCCESS;
}

/*
 * delete the internal form 
 */
static void wdg_input_form_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
  
   /* delete the form */
   unpost_form(ww->form);
   free_form(ww->form);
   ww->form = NULL;
   delwin(ww->fwin);
}

/*
 * create the internal form
 */
static void wdg_input_form_create(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   int mrows, mcols;
   size_t c = wdg_get_ncols(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);

   /* the form is already posted */
   if (ww->form)
      return;
  
   /* create the form */
   ww->form = new_form(ww->fields);

   /* get the geometry to make a window */
   scale_form(ww->form, &mrows, &mcols);

   /* create the window for the form */
   ww->fwin = newwin(mrows, MAX(mcols, (int)c - 4), y + 1, x + 2);
   /* set the color */
   wbkgd(ww->fwin, COLOR_PAIR(wo->window_color));
   keypad(ww->fwin, TRUE);
  
   /* associate with the form */
   set_form_win(ww->form, ww->fwin);
   
   /* the subwin for the form */
   set_form_sub(ww->form, derwin(ww->fwin, mrows + 1, mcols, 1, 1));

   /* make the active field in reverse mode */
   set_field_back(current_field(ww->form), A_REVERSE);
   
   /* display the form */
   post_form(ww->form);

   wnoutrefresh(ww->fwin);
}


/*
 * set the size of the dialog 
 */
void wdg_input_size(wdg_t *wo, size_t x, size_t y)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);

   /* add 2 for the space between the label and the field
    * add 2 for the borders 
    */
   ww->x = x + 2 + 4;
   ww->y = y;
   
   /* center the window on the screen */
   wo->x1 = (current_screen.cols - (x + 2)) / 2;
   wo->y1 = (current_screen.lines - (y + 2)) / 2;
   wo->x2 = -wo->x1;
   wo->y2 = -wo->y1;
   
}

/* 
 * add a field to the form 
 */
void wdg_input_add(wdg_t *wo, size_t x, size_t y, const char *caption, char *buf, size_t len, size_t lines)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   
   ww->nfields += 2;
   WDG_SAFE_REALLOC(ww->fields, ww->nfields * sizeof(FIELD *));

   /* remember the pointer to the real buffer (to be used in consolidate) */
   WDG_SAFE_REALLOC(ww->buffers, (ww->nfields/2 + 1) * sizeof(char *));
   ww->buffers[ww->nfields/2 - 1] = buf;
   ww->buffers[ww->nfields/2] = NULL;

   /* create the caption */
   ww->fields[ww->nfields - 2] = new_field(1, strlen(caption), y, x, 0, 0);
   set_field_buffer(ww->fields[ww->nfields - 2], 0, caption);
   field_opts_off(ww->fields[ww->nfields - 2], O_ACTIVE);
   set_field_fore(ww->fields[ww->nfields - 2], COLOR_PAIR(wo->focus_color));

   /* and the modifiable field */
   ww->fields[ww->nfields - 1] = new_field(lines, len, y, x + strlen(caption) + 2, 0, 0);
   set_field_back(ww->fields[ww->nfields - 1], A_UNDERLINE);
   field_opts_off(ww->fields[ww->nfields - 1], O_WRAP);
   set_field_buffer(ww->fields[ww->nfields - 1], 0, buf);
   set_field_fore(ww->fields[ww->nfields - 1], COLOR_PAIR(wo->window_color));
   
   /* null terminate the array */
   WDG_SAFE_REALLOC(ww->fields, (ww->nfields + 1) * sizeof(FIELD *));
   ww->fields[ww->nfields] = NULL;

}

/*
 * copy the temp buffers to the real ones
 */
static void wdg_input_consolidate(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_input_handle, ww);
   char *buf;
   int i = 0, j;
   void (*callback)(void);
   
   WDG_DEBUG_MSG("wdg_input_consolidate");
   
   while(ww->fields[i] != NULL) {
      /* get the buffer */
      buf = field_buffer(ww->fields[i+1], 0);

      /* trim out the trailing spaces */
      for (j = strlen(buf) - 1; j >= 0; j--)
         if (buf[j] == ' ')
            buf[j] = 0;
         else
            break;
      
      /* copy the buffer in the real one */
      strcpy(ww->buffers[i/2], buf);
      
      /* skip the label */
      i += 2;
   }

   /* execute the callback */
   callback = ww->callback;
   wdg_destroy_object(&wo);
   wdg_redraw_all();
   WDG_EXECUTE(callback);
}

/*
 * set the user callback
 */
void wdg_input_set_callback(wdg_t *wo, void (*callback)(void))
{
   WDG_WO_EXT(struct wdg_input_handle, ww);

   ww->callback = callback;
}

/*
 * this function will get the input from the user.
 * CAUTION: this is an hack, since it uses wgetch()
 * it will takeover the main dispatching loop !!!
 */
void wdg_input_get_input(wdg_t *wo)
{
   int key, ret;
   struct wdg_mouse_event mouse;
  
   WDG_DEBUG_MSG("wdg_input_get_input");
   
   /* dispatch keys to self */
   WDG_LOOP {

      key = wgetch(stdscr);
     
      switch (key) {

         /* don't switch focus... */
         case KEY_TAB:
            break;
         
         case KEY_CTRL_L:
            /* redrawing the screen is equivalent to resizing it */
         case KEY_RESIZE:
            /* the screen has been resized */
            wdg_redraw_all();
            /* update the screen */
            doupdate();
            break;
            
         case ERR:
            /* non-blocking input reached the timeout */
            /* sleep for milliseconds */
            napms(WDG_INPUT_TIMEOUT * 10);
            refresh();
            doupdate();
            break;
            
         default:

#ifdef NCURSES_MOUSE_VERSION
            /* handle mouse events */
            if (key == KEY_MOUSE) {
               MEVENT event;
            
               getmouse(&event);
               mouse_trafo(&event.y, &event.x, TRUE);
               mouse.x = event.x;
               mouse.y = event.y;
               mouse.event = event.bstate;
            }
#else            
            /* we don't support mouse events */
            memset(&mouse, 0, sizeof(mouse));
#endif
            /* dispatch the user input */
            ret = wdg_input_get_msg(wo, key, &mouse); 
            /* update the screen */
            doupdate();

            /* 
             * if the object is destroyed or the input finished, 
             * then return to the main loop
             */
            if (ret == WDG_EFINISHED) {
               WDG_DEBUG_MSG("wdg_input_get_input: return to main loop");
               return;
            }
            
            break;
      }
   }
   

}

/* EOF */

// vim:ts=3:expandtab

