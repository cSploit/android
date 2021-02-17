/*
    WDG -- widgets helper for ncurses

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


/* GLOBALS */

/* information about the current screen */
struct wdg_scr current_screen;

/* called when idle */
struct wdg_call_list {
   void (*idle_callback)(void);
   SLIST_ENTRY(wdg_call_list) next;
};
static SLIST_HEAD(, wdg_call_list) wdg_callbacks_list;

/* the root object (usually the menu) */
static struct wdg_object *wdg_root_obj;

/* the focus list */
struct wdg_obj_list {
   struct wdg_object *wo;
   TAILQ_ENTRY(wdg_obj_list) next;
};
static TAILQ_HEAD(wol, wdg_obj_list) wdg_objects_list = TAILQ_HEAD_INITIALIZER(wdg_objects_list);

/* the currently focused object */
static struct wdg_obj_list *wdg_focused_obj;

/* pressing this key, the event_handler will exit */
static int wdg_exit_key;

/* PROTOS */

void wdg_init(void);
void wdg_cleanup(void);
void wdg_exit(void);
void wdg_update_screen(void);
void wdg_redraw_all(void);

void wdg_add_idle_callback(void (*callback)(void));
void wdg_del_idle_callback(void (*callback)(void));

int wdg_events_handler(int exit_key);
static void wdg_dispatch_msg(int key, struct wdg_mouse_event *mouse);
static void wdg_switch_focus(int type);
#define SWITCH_FOREWARD 1
#define SWITCH_BACKWARD 2
void wdg_set_focus(struct wdg_object *wo);

int wdg_create_object(struct wdg_object **wo, size_t type, size_t flags);
int wdg_destroy_object(struct wdg_object **wo);
void wdg_add_destroy_key(struct wdg_object *wo, int key, void (*callback)(void));

void wdg_set_size(struct wdg_object *wo, int x1, int y1, int x2, int y2);
void wdg_draw_object(struct wdg_object *wo);
size_t wdg_get_type(struct wdg_object *wo);
void wdg_init_color(u_char pair, u_char fg, u_char bg);
void wdg_set_color(wdg_t *wo, size_t part, u_char pair);
void wdg_screen_color(u_char pair);

size_t wdg_get_nlines(struct wdg_object *wo);
size_t wdg_get_ncols(struct wdg_object *wo);
size_t wdg_get_begin_x(struct wdg_object *wo);
size_t wdg_get_begin_y(struct wdg_object *wo);

/* creation function from other widgets */
extern void wdg_create_compound(struct wdg_object *wo);
extern void wdg_create_window(struct wdg_object *wo);
extern void wdg_create_panel(struct wdg_object *wo);
extern void wdg_create_scroll(struct wdg_object *wo);
extern void wdg_create_menu(struct wdg_object *wo);
extern void wdg_create_dialog(struct wdg_object *wo);
extern void wdg_create_percentage(struct wdg_object *wo);
extern void wdg_create_file(struct wdg_object *wo);
extern void wdg_create_input(struct wdg_object *wo);
extern void wdg_create_list(struct wdg_object *wo);
extern void wdg_create_dynlist(struct wdg_object *wo);

/*******************************************/

/*
 * init the widgets interface
 */
void wdg_init(void)
{
   WDG_DEBUG_INIT();
   
   WDG_DEBUG_MSG("wdg_init: setting up the term...");
   
   /* initialize the curses interface */
   initscr(); 

   /* disable buffering until carriage return */
   cbreak(); 

   /* disable echo of typed chars */
   noecho();
  
   /* better compatibility with return key */
   nonl();

   /* set controlling key (^C^X^Z^S^Q) uninterpreted */
   raw();

   /* set the non-blocking timeout (10th of seconds) */
   halfdelay(WDG_INPUT_TIMEOUT);

   /* don't flush input on break */
   intrflush(stdscr, FALSE);
  
   /* enable function and arrow keys */ 
   keypad(stdscr, TRUE);
  
   /* activate colors if available */
   if (has_colors()) {
      current_screen.flags |= WDG_SCR_HAS_COLORS;
      start_color();
   }

   /* hide the cursor */
   curs_set(FALSE);

   /* remember the current screen size */
   getmaxyx(stdscr, current_screen.lines, current_screen.cols);

   /* the wdg is initialized */
   current_screen.flags |= WDG_SCR_INITIALIZED;

   /* clear the screen */
   clear();

   /* sync the virtual and the physical screen */
   refresh();

#ifdef NCURSES_MOUSE_VERSION
   /* activate the mask to receive mouse events */
   mousemask(ALL_MOUSE_EVENTS, (mmask_t *) 0);
#endif

   WDG_DEBUG_MSG("wdg_init: initialized");
}


/*
 * cleanup the widgets interface
 */
void wdg_cleanup(void)
{
   /* can only cleanup if it was initialized */
   if (!(current_screen.flags & WDG_SCR_INITIALIZED))
      return;
   
   WDG_DEBUG_MSG("wdg_cleanup");
   
   /* show the cursor */
   curs_set(TRUE);

   /* clear the screen */
   clear();

   /* do the refresh */
   refresh();

   /* end the curses interface */
   endwin();

   /* wdg is not initialized */
   current_screen.flags &= ~WDG_SCR_INITIALIZED;

#ifdef NCURSES_MOUSE_VERSION
   /* reset the mouse event reception */
   mousemask(0, (mmask_t *) 0);
#endif
   
   WDG_DEBUG_CLOSE();
}

/*
 * used to exit from the events_handler.
 * this function will put in the input buffer
 * the exit key, so the handler will get it 
 * and perfrom a clean exit 
 */
void wdg_exit(void)
{
   WDG_DEBUG_MSG("wdg_exit");
   
   /* put the exit key in the input buffer */
   ungetch(wdg_exit_key);
}

/*
 * update the screen
 */
void wdg_update_screen(void)
{
   doupdate();
}

/* 
 * called upon screen resize
 */
void wdg_redraw_all(void)
{
   struct wdg_obj_list *wl;
   
   WDG_DEBUG_MSG("wdg_redraw_all");
   
   /* remember the current screen size */
   getmaxyx(stdscr, current_screen.lines, current_screen.cols);

   /* call the redraw function upon all the objects */
   TAILQ_FOREACH(wl, &wdg_objects_list, next) {
      WDG_BUG_IF(wl->wo->redraw == NULL);
      WDG_EXECUTE(wl->wo->redraw, wl->wo);
   }

}

/*
 * this function handles all the inputed keys 
 * from the user and dispatches them to the
 * wdg objects
 */
int wdg_events_handler(int exit_key)
{
   int key;
   struct wdg_mouse_event mouse;
   
   WDG_DEBUG_MSG("wdg_events_handler (%c)", exit_key);

   /* set the global exit key (used by wdg_exit()) */
   wdg_exit_key = exit_key;
   
   /* infinite loop */
   WDG_LOOP {

      /* get the input from user */
      key = wgetch(stdscr);

      switch (key) {
         
         case KEY_TAB:
            /* switch focus between objects */
            wdg_switch_focus(SWITCH_FOREWARD);
            /* update the screen */
            doupdate();
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
            /* 
             * non-blocking input reached the timeout:
             * call the idle function if present, else
             * sleep to not eat up all the cpu
             */
            if (SLIST_EMPTY(&wdg_callbacks_list)) {
               /* sleep for milliseconds */
               napms(WDG_INPUT_TIMEOUT * 10);
               /* XXX - too many refresh ? */
               refresh();
            } else {
               struct wdg_call_list *cl;
               SLIST_FOREACH(cl, &wdg_callbacks_list, next)
                  cl->idle_callback();
            }
            /* 
             * update the screen.
             * all the function uses wnoutrefresh() funcions 
             */
            doupdate();
            break;
            
         default:
            /* emergency exit key */
            if (key == wdg_exit_key)
               return WDG_ESUCCESS;

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
            wdg_dispatch_msg(key, &mouse);
            /* update the screen */
            doupdate();
            break;
      }
   }
   
   /* NOT REACHED */
   
   return WDG_ESUCCESS;
}

/*
 * add a function to the idle callbacks list 
 */
void wdg_add_idle_callback(void (*callback)(void))
{
   struct wdg_call_list *cl;
   
   WDG_DEBUG_MSG("wdg_add_idle_callback (%p)", callback);

   WDG_SAFE_CALLOC(cl, 1, sizeof(struct wdg_call_list));

   /* store the callback */
   cl->idle_callback = callback;

   /* insert in the list */
   SLIST_INSERT_HEAD(&wdg_callbacks_list, cl, next);
}

/*
 * delete a function from the callbacks list
 */
void wdg_del_idle_callback(void (*callback)(void))
{
   struct wdg_call_list *cl;
   
   WDG_DEBUG_MSG("wdg_del_idle_callback (%p)", callback);

   SLIST_FOREACH(cl, &wdg_callbacks_list, next) {
      if (cl->idle_callback == callback) {
         SLIST_REMOVE(&wdg_callbacks_list, cl, wdg_call_list, next);
         WDG_SAFE_FREE(cl);
         return;
      }
   }
}

/*
 * dispatch the user input to the list of objects.
 * first dispatch to the root_obj, if not handled
 * dispatch to the focused object.
 */
static void wdg_dispatch_msg(int key, struct wdg_mouse_event *mouse)
{
   /* the focused object is modal ! send only to it */
   if (wdg_focused_obj && (wdg_focused_obj->wo->flags & WDG_OBJ_FOCUS_MODAL)) {

      /* check the destroy callback */
      if (wdg_focused_obj->wo && key == wdg_focused_obj->wo->destroy_key) {
         struct wdg_object *wo = wdg_focused_obj->wo;

         WDG_DEBUG_MSG("wdg_destroy_callback (%p)", wo);
         WDG_EXECUTE(wdg_focused_obj->wo->destroy_callback);
         wdg_destroy_object(&wo);
         wdg_redraw_all();
         
         /* object was destroyed */
         return;
      }

      /* deliver the message normally */
      wdg_focused_obj->wo->get_msg(wdg_focused_obj->wo, key, mouse);
         
      /* other objects must not receive the msg */
      return;
   }
   
   /*
    * if it is a mouse event, we have to dispatch to all 
    * the object in the list until someone handles it
    */
   if (key == KEY_MOUSE) {
      struct wdg_obj_list *wl;
      
      /* 
       * first dispatch to the root object,
       * since it is usually a menu, it may overlap
       * other objects and needs to get the event first
       */
      if (wdg_root_obj != NULL) {
         if (wdg_root_obj->get_msg(wdg_root_obj, key, mouse) == WDG_ESUCCESS)
            /* the root object handled the message */
            return;
      }

      /* 
       * then dispatch to the focused object.
       * it may overlap and needs to event before the others
       * to prevent an undesired raising of underlaying objects
       */
      if (wdg_focused_obj != NULL) {
         if (wdg_focused_obj->wo->get_msg(wdg_focused_obj->wo, key, mouse) == WDG_ESUCCESS)
            /* the focused object handled the message */
            return;     
      }

      /* dispatch to all the other objects */
      TAILQ_FOREACH(wl, &wdg_objects_list, next) {
         if ((wl->wo->flags & WDG_OBJ_WANT_FOCUS) && (wl->wo->flags & WDG_OBJ_VISIBLE) ) {
            if (wl->wo->get_msg(wl->wo, key, mouse) == WDG_ESUCCESS)
               return;
         }
      }

   /* it is a keyboard event */
   } else {

      /* first dispatch to the root object */
      if (wdg_root_obj != NULL) {
         if (wdg_root_obj->get_msg(wdg_root_obj, key, mouse) == WDG_ESUCCESS)
            /* the root object handled the message */
            return;
         
         /* check the destroy callback */
         if (key == wdg_root_obj->destroy_key) {
            WDG_EXECUTE(wdg_root_obj->destroy_callback);
            wdg_destroy_object(&wdg_root_obj);
            wdg_redraw_all();
            return;
         }
      }

      /* 
       * the root_object has not handled it.
       * dispatch to the focused one
       */
      if (wdg_focused_obj != NULL) {
         if (wdg_focused_obj->wo->get_msg(wdg_focused_obj->wo, key, mouse) == WDG_ESUCCESS)
            /* the focused object handled the message */
            return;      
         
         /* check the destroy callback */
         if (wdg_focused_obj->wo && key == wdg_focused_obj->wo->destroy_key) {
            struct wdg_object *wo = wdg_focused_obj->wo;
            WDG_EXECUTE(wdg_focused_obj->wo->destroy_callback);
            wdg_destroy_object(&wo);
            wdg_redraw_all();
            return;
         }
      }
      
      /* noone handled the message, flash an error */
      flash();
      beep();
   }
}

/*
 * move the focus to the next object.
 * only WDG_OBJ_WANT_FOCUS could get the focus
 */
static void wdg_switch_focus(int type)
{
   struct wdg_obj_list *wl;

   /* the focused object is modal ! don't switch */
   if (wdg_focused_obj && (wdg_focused_obj->wo->flags & WDG_OBJ_FOCUS_MODAL))
      return;
  
   /* if there is not a focused object, create it */
   if (wdg_focused_obj == NULL) {
   
      /* search the first "focusable" object */
      TAILQ_FOREACH(wl, &wdg_objects_list, next) {
         if ((wl->wo->flags & WDG_OBJ_WANT_FOCUS) && (wl->wo->flags & WDG_OBJ_VISIBLE) ) {
            /* set the focused object */
            wdg_focused_obj = wl;
            /* focus current object */
            WDG_BUG_IF(wdg_focused_obj->wo->get_focus == NULL);
            WDG_EXECUTE(wdg_focused_obj->wo->get_focus, wdg_focused_obj->wo);
            return;
         }
      }
   }
  
   /* unfocus the current object */
   WDG_BUG_IF(wdg_focused_obj->wo->lost_focus == NULL);
   WDG_EXECUTE(wdg_focused_obj->wo->lost_focus, wdg_focused_obj->wo);
   
   /* 
    * focus the next/prev element in the list.
    * only focus objects that have the WDG_OBJ_WANT_FOCUS flag
    */
   do {
      switch (type) {
         case SWITCH_FOREWARD:
            wdg_focused_obj = TAILQ_NEXT(wdg_focused_obj, next);
            /* we are at the end, move to the first element */
            if (wdg_focused_obj == TAILQ_END(&wdg_objects_list))
               wdg_focused_obj = TAILQ_FIRST(&wdg_objects_list);
            break;
         case SWITCH_BACKWARD:
            /* we are on the head, move to the last element */
            if (wdg_focused_obj == TAILQ_FIRST(&wdg_objects_list))
               wdg_focused_obj = TAILQ_LAST(&wdg_objects_list, wol);
            else
               wdg_focused_obj = TAILQ_PREV(wdg_focused_obj, wol, next);
            break;
      }
   } while ( !(wdg_focused_obj->wo->flags & WDG_OBJ_WANT_FOCUS) || !(wdg_focused_obj->wo->flags & WDG_OBJ_VISIBLE) );

   /* focus current object */
   WDG_BUG_IF(wdg_focused_obj->wo->get_focus == NULL);
   WDG_EXECUTE(wdg_focused_obj->wo->get_focus, wdg_focused_obj->wo);
   
}

/*
 * set focus to the given object
 */
void wdg_set_focus(struct wdg_object *wo)
{
   struct wdg_obj_list *wl;

   /* search the object and focus it */
   TAILQ_FOREACH(wl, &wdg_objects_list, next) {
      if ( wl->wo == wo ) {
         /* unfocus the current object */
         if (wdg_focused_obj)
            WDG_EXECUTE(wdg_focused_obj->wo->lost_focus, wdg_focused_obj->wo);
         /* set the focused object */
         wdg_focused_obj = wl;
         /* focus current object */
         WDG_BUG_IF(wdg_focused_obj->wo->get_focus == NULL);
         WDG_EXECUTE(wdg_focused_obj->wo->get_focus, wdg_focused_obj->wo);
         return;
      }
   }
}

/*
 * create a wdg object 
 */
int wdg_create_object(struct wdg_object **wo, size_t type, size_t flags)
{
   struct wdg_obj_list *wl;
   
   WDG_DEBUG_MSG("wdg_create_object (%d)", type);
   
   /* alloc the struct */
   WDG_SAFE_CALLOC(*wo, 1, sizeof(struct wdg_object));

   /* set the flags */
   (*wo)->flags = flags;
   (*wo)->type = type;
  
   /* call the specific function */
   switch (type) {
      case WDG_COMPOUND:
         wdg_create_compound(*wo);
         break;
         
      case WDG_WINDOW:
         wdg_create_window(*wo);
         break;
         
      case WDG_PANEL:
         wdg_create_panel(*wo);
         break;
         
      case WDG_SCROLL:
         wdg_create_scroll(*wo);
         break;
         
      case WDG_MENU:
         wdg_create_menu(*wo);
         break;
         
      case WDG_DIALOG:
         wdg_create_dialog(*wo);
         break;
         
      case WDG_PERCENTAGE:
         wdg_create_percentage(*wo);
         break;
         
      case WDG_FILE:
         wdg_create_file(*wo);
         break;
         
      case WDG_INPUT:
         wdg_create_input(*wo);
         break;
         
      case WDG_LIST:
         wdg_create_list(*wo);
         break;
         
      case WDG_DYNLIST:
         wdg_create_dynlist(*wo);
         break;
         
      default:
         WDG_SAFE_FREE(*wo);
         return -WDG_EFATAL;
         break;
   }
   
   /* alloc the element in the object list */
   WDG_SAFE_CALLOC(wl, 1, sizeof(struct wdg_obj_list));

   /* link the object */
   wl->wo = *wo;

   /* insert it in the list */
   TAILQ_INSERT_TAIL(&wdg_objects_list, wl, next);
   
   /* this is the root object */
   if (flags & WDG_OBJ_ROOT_OBJECT)
      wdg_root_obj = *wo;
   
   return WDG_ESUCCESS;
}

/*
 * destroy a wdg object by calling the callback function
 */
int wdg_destroy_object(struct wdg_object **wo)
{
   struct wdg_obj_list *wl;
   
   WDG_DEBUG_MSG("wdg_destroy_object (%p)", *wo);
  
   /* sanity check */
   if (*wo == NULL)
      return -WDG_ENOTHANDLED;
  
   /* delete it from the obj_list */
   TAILQ_FOREACH(wl, &wdg_objects_list, next) {
      if (wl->wo == *wo) {
         
         /* was it the root object ? */
         if ((*wo)->flags & WDG_OBJ_ROOT_OBJECT)
            wdg_root_obj = NULL;
  
         /* it was the focused one */
         if (wdg_focused_obj && wdg_focused_obj->wo == *wo) {
            /* remove the modal flat to enable the switch */
            (*wo)->flags &= ~WDG_OBJ_FOCUS_MODAL;
            wdg_switch_focus(SWITCH_BACKWARD);
         }
         
         /* 
          * check if it was the only object in the list
          * and it has gained the focus with the previous
          * call to wdg_switch_focus();
          */
         if (wl == wdg_focused_obj)
            wdg_focused_obj = NULL;

         /* remove the object */
         TAILQ_REMOVE(&wdg_objects_list, wl, next);
         WDG_SAFE_FREE(wl);
         
         /* call the specialized destroy function */
         WDG_BUG_IF((*wo)->destroy == NULL);
         WDG_EXECUTE((*wo)->destroy, *wo);
   
         /* free the title */
         WDG_SAFE_FREE((*wo)->title);
         /* then free the object */
         WDG_SAFE_FREE(*wo);
         
         return WDG_ESUCCESS;
      }
   }

   return -WDG_ENOTHANDLED;
}

/*
 * set the destroy key and callback
 */
void wdg_add_destroy_key(struct wdg_object *wo, int key, void (*callback)(void))
{
   wo->destroy_key = key;
   wo->destroy_callback = callback;
}

/*
 * set or reset the size of an object
 */
void wdg_set_size(struct wdg_object *wo, int x1, int y1, int x2, int y2)
{
   /* set the new object cohordinates */
   wo->x1 = x1;
   wo->y1 = y1;
   wo->x2 = x2;
   wo->y2 = y2;

   /* call the specialized function */
   WDG_BUG_IF(wo->resize == NULL);
   WDG_EXECUTE(wo->resize, wo);
}

/*
 * display the object by calling the redraw function
 */
void wdg_draw_object(struct wdg_object *wo)
{
   WDG_DEBUG_MSG("wdg_draw_object (%p)", wo);
   
   /* display the object */
   WDG_BUG_IF(wo->redraw == NULL);
   WDG_EXECUTE(wo->redraw, wo);
}

/*
 * return the type of the object
 */
size_t wdg_get_type(struct wdg_object *wo)
{
   return wo->type;
}

/* 
 * set the color of an object
 */
void wdg_set_color(wdg_t *wo, size_t part, u_char pair)
{
   switch (part) {
      case WDG_COLOR_SCREEN:
         wo->screen_color = pair;
         break;
      case WDG_COLOR_TITLE:
         wo->title_color = pair;
         break;
      case WDG_COLOR_BORDER:
         wo->border_color = pair;
         break;
      case WDG_COLOR_FOCUS:
         wo->focus_color = pair;
         break;
      case WDG_COLOR_WINDOW:
         wo->window_color = pair;
         break;
      case WDG_COLOR_SELECT:
         wo->select_color = pair;
         break;
   }
}

/*
 * init a color pair
 */
void wdg_init_color(u_char pair, u_char fg, u_char bg)
{
   init_pair(pair, fg, bg);
}

/*
 * erase the screen with the specified color
 */
void wdg_screen_color(u_char pair)
{
   wbkgd(stdscr, COLOR_PAIR(pair));
   erase();
   refresh();
}

/*
 * set the object's title 
 */
void wdg_set_title(struct wdg_object *wo, char *title, size_t align)
{
   /* copy the values */
   wo->align = align;
   WDG_SAFE_STRDUP(wo->title, title);
}

/* 
 * functions to calculate real dimensions
 * from the given relative cohordinates 
 */

size_t wdg_get_nlines(struct wdg_object *wo)
{
   size_t a, b;
   int c = current_screen.lines;
   
   if (wo->y1 >= 0)
      a = wo->y1;
   else 
      a = (c + wo->y1 > 0) ? (c + wo->y1) : 0;

   if (wo->y2 > 0)
      b = wo->y2;
   else
      b = (c + wo->y2 > 0) ? (c + wo->y2) : 0;
   
   /* only return positive values */
   return (b > a) ? b - a : 0;
}

size_t wdg_get_ncols(struct wdg_object *wo)
{
   size_t a, b;
   int c = current_screen.cols;
   
   if (wo->x1 >= 0)
      a = wo->x1;
   else 
      a = (c + wo->x1 > 0) ? (c + wo->x1) : 0;

   if (wo->x2 > 0)
      b = wo->x2;
   else
      b = (c + wo->x2 > 0) ? (c + wo->x2) : 0;
   
   /* only return positive values */
   return (b > a) ? b - a : 0;
}

size_t wdg_get_begin_x(struct wdg_object *wo)
{
   int c = current_screen.cols;

   if (wo->x1 >= 0)
      return wo->x1;
   else
      return (c + wo->x1 >= 0) ? (c + wo->x1) : 0;
}

size_t wdg_get_begin_y(struct wdg_object *wo)
{
   int c = current_screen.lines;

   if (wo->y1 >= 0)
      return wo->y1;
   else
      return (c + wo->y1 >= 0) ? (c + wo->y1) : 0;
}

/* EOF */

// vim:ts=3:expandtab

