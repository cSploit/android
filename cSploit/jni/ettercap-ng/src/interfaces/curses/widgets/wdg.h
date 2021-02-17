

#ifndef WDG_H
#define WDG_H

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS_WINDOWS
   #include <windows.h>
#endif

#include <ec_queue.h>

#include <limits.h>

#if defined HAVE_STDINT_H && !defined OS_SOLARIS
	#include <stdint.h>
	#include <sys/types.h>
#elif defined OS_SOLARIS
	#include <sys/inttypes.h>
#endif

#ifndef TYPES_DEFINED
#define TYPES_DEFINED
	typedef int8_t    int8;
	typedef int16_t   int16;
	typedef int32_t   int32;
	typedef int64_t   int64;

	typedef uint8_t   u_int8;
	typedef uint16_t  u_int16;
	typedef uint32_t  u_int32;
	typedef uint64_t  u_int64;
#endif


#define LIBWDG_VERSION "0.10.3"
   
/********************************************/

enum {
   WDG_ESUCCESS    = 0,
   WDG_ENOTHANDLED = 1,
   WDG_EFINISHED   = 2,
   WDG_EFATAL      = 255,
};

/* min and max */

#ifndef MIN
   #define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
   #define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#endif

extern void wdg_debug_init(void);
extern void wdg_debug_close(void);
extern void wdg_debug_msg(const char *message, ...);
#ifdef DEBUG
   #define WDG_DEBUG_INIT()      wdg_debug_init() 
   #define WDG_DEBUG_CLOSE()     wdg_debug_close()
   #define WDG_DEBUG_MSG(x, ...) wdg_debug_msg(x, ## __VA_ARGS__ )
#else
   #define WDG_DEBUG_INIT()
   #define WDG_DEBUG_CLOSE()
   #define WDG_DEBUG_MSG(x, ...)
#endif

extern void wdg_error_msg(char *file, const char *function, int line, char *message, ...);
#define WDG_ON_ERROR(x, y, fmt, ...) do { if (x == y) wdg_error_msg(__FILE__, __FUNCTION__, __LINE__, fmt, ## __VA_ARGS__ ); } while(0)

extern void wdg_bug(char *file, const char *function, int line, char *message);
#define WDG_BUG_IF(x) do { if (x) wdg_bug(__FILE__, __FUNCTION__, __LINE__, #x); }while(0)
#define WDG_NOT_IMPLEMENTED() do { wdg_bug(__FILE__, __FUNCTION__, __LINE__, "Not yet implemented"); } while(0)

#define WDG_SAFE_CALLOC(x, n, s) do { \
   x = calloc(n, s); \
   WDG_ON_ERROR(x, NULL, "virtual memory exhausted"); \
   /* WDG_DEBUG_MSG("[%s:%d] WDG_SAFE_CALLOC: %#x", __FUNCTION__, __LINE__, x); */ \
} while(0)

#define WDG_SAFE_REALLOC(x, s) do { \
   /* WDG_DEBUG_MSG("[%s:%d] WDG_SAFE_REALLOC: before %#x", __FUNCTION__, __LINE__, x); */ \
   x = realloc(x, s); \
   WDG_ON_ERROR(x, NULL, "virtual memory exhausted"); \
   /* WDG_DEBUG_MSG("[%s:%d] WDG_SAFE_REALLOC: after %#x", __FUNCTION__, __LINE__, x); */ \
} while(0)

#define WDG_SAFE_FREE(x) do{                 \
   /* WDG_DEBUG_MSG("[%s:%d] WDG_SAFE_FREE: %#x", __FUNCTION__, __LINE__, x);   */  \
   if (x) {                                  \
      free(x);                               \
      x = NULL;                              \
   }                                         \
}while(0)

#define WDG_SAFE_STRDUP(x, s) do{ \
   x = strdup(s); \
   WDG_ON_ERROR(x, NULL, "virtual memory exhausted"); \
   /* WDG_DEBUG_MSG("[%s:%d] WDG_SAFE_STRDUP: %#x", __FUNCTION__, __LINE__, x); */  \
}while(0)

#define WDG_EXECUTE(x, ...) do{ if(x != NULL) x( __VA_ARGS__ ); }while(0)

#define WDG_LOOP for(;;)

/* used by halfdelay */
#define WDG_INPUT_TIMEOUT  1

/* not defined in curses.h */
#define KEY_RETURN   '\r'
#define KEY_TAB      '\t'
#define KEY_CTRL_L   12
#define CTRL(x)      ((x) & 0x1f)
#define KEY_ESC      CTRL('[')

/* information about the current screen */
struct wdg_scr {
   size_t lines;
   size_t cols;
   size_t flags;
      #define WDG_SCR_HAS_COLORS    1
      #define WDG_SCR_INITIALIZED   (1<<1)
};

/* global scruct for current screen */
extern struct wdg_scr current_screen;

/* struct for mouse events */
struct wdg_mouse_event {
   size_t x;
   size_t y;
   size_t event;
};

#define WDG_MOUSE_ENCLOSE(win, key, mouse) (key == KEY_MOUSE && wenclose(win, mouse->y, mouse->x))

/* struct for all wdg objects */
struct wdg_object {
   /* object flags */
   size_t flags;
      #define WDG_OBJ_WANT_FOCUS     1
      #define WDG_OBJ_FOCUS_MODAL    (1<<1)
      #define WDG_OBJ_FOCUSED        (1<<2)
      #define WDG_OBJ_VISIBLE        (1<<3)
      #define WDG_OBJ_ROOT_OBJECT    (1<<7)
   /* object type */
   size_t type;
      #define WDG_COMPOUND    0
      #define WDG_WINDOW      1
      #define WDG_PANEL       2
      #define WDG_SCROLL      3
      #define WDG_MENU        4
      #define WDG_DIALOG      5
      #define WDG_PERCENTAGE  6
      #define WDG_FILE        7
      #define WDG_INPUT       8
      #define WDG_LIST        9
      #define WDG_DYNLIST    10
   
   /* destructor function */
   int (*destroy)(struct wdg_object *wo);
   int destroy_key;
   void (*destroy_callback)(void);
   /* called to set / reset the size */
   int (*resize)(struct wdg_object *wo);
   /* called upon redrawing of the object */
   int (*redraw)(struct wdg_object *wo);
   /* get / lost focus redrawing functions */
   int (*get_focus)(struct wdg_object *wo);
   int (*lost_focus)(struct wdg_object *wo);
   /* called to process an input from the user */
   int (*get_msg)(struct wdg_object *wo, int key, struct wdg_mouse_event *mouse);

   /* object cohordinates */
   int x1, y1, x2, y2;

   /* object colors */
   u_char screen_color;
   u_char border_color;
   u_char focus_color;
   u_char title_color;
   u_char window_color;
   u_char select_color;

   /* title */
   char *title;
   char align;

   /* here is the pointer to extend a wdg object
    * it is a sort of inheritance...
    */
   void *extend;
};

typedef struct wdg_object wdg_t;

/* WIDGETS */

#define WDG_MOVE_PANEL(pan, y, x)   do{ WDG_ON_ERROR(move_panel(pan, y, x), ERR, "Resized too much... (%d,%d)", x, y); }while(0)
#define WDG_WRESIZE(win, l, c)   do{ WDG_ON_ERROR(wresize(win, l, c), ERR, "Resized too much...(%dx%d)", c, l); }while(0)

#define WDG_WO_EXT(type, var) type *var = (type *)(wo->extend)

/* alignment */
#define WDG_ALIGN_LEFT     0
#define WDG_ALIGN_CENTER   1
#define WDG_ALIGN_RIGHT    2

/* compound ojbects */
extern void wdg_compound_add(wdg_t *wo, wdg_t *widget);
extern void wdg_compound_set_focus(wdg_t *wo, wdg_t *widget);
extern wdg_t * wdg_compound_get_focused(wdg_t *wo);
void wdg_compound_add_callback(wdg_t *wo, int key, void (*callback)(void));
/* window ojbects */
extern void wdg_window_print(wdg_t *wo, size_t x, size_t y, char *fmt, ...);
/* panel ojbects */
extern void wdg_panel_print(wdg_t *wo, size_t x, size_t y, char *fmt, ...);
/* scroll ojbects */
extern void wdg_scroll_erase(wdg_t *wo);
extern void wdg_scroll_print(wdg_t *wo, int color, char *fmt, ...);
extern void wdg_scroll_set_lines(wdg_t *wo, size_t lines);
/* menu objects */
struct wdg_menu {
   char *name;
   int hotkey;
   char *shortcut;
   void (*callback)(void);
};
extern void wdg_menu_add(wdg_t *wo, struct wdg_menu *menu);
/* dialog objects */
extern void wdg_dialog_text(wdg_t *wo, size_t flags, const char *text);
   #define WDG_NO_BUTTONS  0
   #define WDG_OK          1
   #define WDG_YES         (1<<1)
   #define WDG_NO          (1<<2)
   #define WDG_CANCEL      (1<<3)
extern void wdg_dialog_add_callback(wdg_t *wo, size_t flag, void (*callback)(void));
/* percentage objects */
extern int wdg_percentage_set(wdg_t *wo, size_t p, size_t max);
   #define WDG_PERCENTAGE_INTERRUPTED  -1
   #define WDG_PERCENTAGE_FINISHED     0
   #define WDG_PERCENTAGE_UPDATED      1
/* file dialog objects */
extern void wdg_file_set_callback(wdg_t *wo, void (*callback)(char *path, char *file));
/* input dialog objects */
extern void wdg_input_size(wdg_t *wo, size_t x, size_t y);
extern void wdg_input_add(wdg_t *wo, size_t x, size_t y, const char *caption, char *buf, size_t len, size_t lines);
extern void wdg_input_set_callback(wdg_t *wo, void (*callback)(void));
extern void wdg_input_get_input(wdg_t *wo);
/* list objects */
struct wdg_list {
   char *desc;
   void *value;
};
void wdg_list_set_elements(struct wdg_object *wo, struct wdg_list *list);
void wdg_list_add_callback(wdg_t *wo, int key, void (*callback)(void *));
void wdg_list_refresh(wdg_t *wo);
void wdg_list_select_callback(wdg_t *wo, void (*callback)(void *));
/* dynlist objects */
void wdg_dynlist_refresh(wdg_t *wo);
void wdg_dynlist_add_callback(wdg_t *wo, int key, void (*callback)(void *));
void wdg_dynlist_print_callback(wdg_t *wo, void * func(int mode, void *list, char **desc, size_t len));
void wdg_dynlist_select_callback(wdg_t *wo, void (*callback)(void *));
void wdg_dynlist_reset(wdg_t *wo);


/* EXPORTED FUNCTIONS */

extern void wdg_init(void);
extern void wdg_cleanup(void);
extern void wdg_exit(void);
extern void wdg_update_screen(void);
extern void wdg_redraw_all(void);

/* the main dispatching loop */
extern int wdg_events_handler(int exit_key);
/* add/delete functions to be called when idle */
extern void wdg_add_idle_callback(void (*callback)(void));
extern void wdg_del_idle_callback(void (*callback)(void));

/* object creation */
extern int wdg_create_object(wdg_t **wo, size_t type, size_t flags);
extern int wdg_destroy_object(wdg_t **wo);
extern void wdg_add_destroy_key(wdg_t *wo, int key, void (*callback)(void));

/* object modifications */
extern void wdg_set_size(wdg_t *wo, int x1, int y1, int x2, int y2);
extern void wdg_set_colors(wdg_t *wo, int color, size_t type); 
extern void wdg_draw_object(wdg_t *wo);
extern size_t wdg_get_type(wdg_t *wo);
extern void wdg_set_focus(wdg_t *wo);
extern void wdg_set_title(wdg_t *wo, char *title, size_t align);
extern void wdg_init_color(u_char pair, u_char fg, u_char bg);
extern void wdg_set_color(wdg_t *wo, size_t part, u_char pair);
   #define WDG_COLOR_SCREEN   0
   #define WDG_COLOR_TITLE    1
   #define WDG_COLOR_BORDER   2
   #define WDG_COLOR_FOCUS    3
   #define WDG_COLOR_WINDOW   4
   #define WDG_COLOR_SELECT   5
extern void wdg_screen_color(u_char pair);

/* object size */
extern size_t wdg_get_nlines(wdg_t *wo);
extern size_t wdg_get_ncols(wdg_t *wo);
extern size_t wdg_get_begin_x(wdg_t *wo);
extern size_t wdg_get_begin_y(wdg_t *wo);

#endif 

/* EOF */

// vim:ts=3:expandtab

