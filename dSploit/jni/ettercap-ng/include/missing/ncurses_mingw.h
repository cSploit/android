#ifndef _NCURSES_FOR_WINDOWS_H
#define _NCURSES_FOR_WINDOWS_H

/*
 * A little helper for ncurses code on Windows.
 * OS_WINDOWS doesn't really use the full ncurses yet, but a small
 * layer on-top of PD-curses.
 */
#include <stdarg.h>

#undef MOUSE_MOVED        /* <wincon.h> */

#include <curses.h>       /* PDcurses */
#include <ncurses_dll.h>  /* part of ncurses */

/* from ncurses 5.4 <curses.h> */
extern const char *curses_version (void);

extern int vwprintw (WINDOW *, const char *,va_list);
#define vw_printw vwprintw

extern int  wresize (WINDOW *, int, int);
extern bool wenclose (const WINDOW *, int, int);

#endif  /* _NCURSES_FOR_WINDOWS_H */

