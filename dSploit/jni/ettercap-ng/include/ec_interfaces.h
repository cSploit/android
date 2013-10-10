
/* $Id: ec_interfaces.h,v 1.16 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_INTERFACES_H
#define EC_INTERFACES_H

/* colors for curses interface */
struct curses_color {
   int bg;
   int fg;
   int join1;
   int join2;
   int border;
   int title;
   int focus;
   int menu_bg;
   int menu_fg;
   int window_bg;
   int window_fg;
   int selection_bg;
   int selection_fg;
   int error_bg;
   int error_fg;
   int error_border;
};

/* color pairs */
#define EC_COLOR              1
#define EC_COLOR_BORDER       2
#define EC_COLOR_TITLE        3
#define EC_COLOR_FOCUS        4
#define EC_COLOR_MENU         5
#define EC_COLOR_WINDOW       6
#define EC_COLOR_SELECTION    7
#define EC_COLOR_ERROR        8
#define EC_COLOR_ERROR_BORDER 9
#define EC_COLOR_JOIN1        10
#define EC_COLOR_JOIN2        11

/* exported functions */

EC_API_EXTERN void select_daemon_interface(void);
EC_API_EXTERN void select_text_interface(void);
EC_API_EXTERN void select_curses_interface(void);
EC_API_EXTERN void select_gtk_interface(void);

#endif

/* EOF */

// vim:ts=3:expandtab

