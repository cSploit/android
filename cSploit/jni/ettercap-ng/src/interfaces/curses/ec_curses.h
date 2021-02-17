

#ifndef EC_CURSES_H
#define EC_CURSES_H

#include <wdg.h>

#define SYSMSG_WIN_SIZE -8


extern void curses_input(const char *title, char *input, size_t n, void (*callback)(void));
extern void curses_message(const char *msg);

extern void curses_flush_msg(void);
extern void curses_sniff_offline(void);
extern void curses_sniff_live(void);

/* menus */
extern struct wdg_menu menu_filters[]; 
extern struct wdg_menu menu_logging[]; 
extern struct wdg_menu menu_help[]; 
extern struct wdg_menu menu_hosts[]; 
extern struct wdg_menu menu_mitm[]; 
extern struct wdg_menu menu_plugins[]; 
extern struct wdg_menu menu_start[]; 
extern struct wdg_menu menu_targets[]; 
extern struct wdg_menu menu_view[]; 

#endif

/* EOF */

// vim:ts=3:expandtab

