/*
    ettercap -- curses GUI

    Copyright (C) ALoR & NaGA

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

#include <ec.h>
#include <wdg.h>
#include <ec_curses.h>

#include <ncurses.h>

/* proto */

void help_ettercap(void);
void help_curses(void);
void help_plugins(void);
void help_etterconf(void);
void help_etterfilter(void);
void help_etterlog(void);

/* globals */

struct wdg_menu menu_help[] = { {"?",                 0,       "",    NULL},
                                {"ettercap"  ,        0,       "",    help_ettercap},
                                {"curses gui"  ,      0,       "",    help_curses},
                                {"plugins"  ,         0,       "",    help_plugins},
                                {"etter.conf"  ,      0,       "",    help_etterconf},
                                {"etterfilter"  ,     0,       "",    help_etterfilter},
                                {"etterlog"  ,        0,       "",    help_etterlog},
                                {NULL, 0, NULL, NULL},
                              };

/* macro */

#define SHOW_MAN(x, y)  do {     \
   int ret;                      \
   DEBUG_MSG("curses_help: retriving man page for: " x); \
   endwin();                     \
   ret = system("man " x);       \
   if (ret != 0)                 \
      ret = system("man " y);    \
   refresh();                    \
   if (ret != 0)                 \
      ui_error("Cannot find man page for " x);  \
} while(0)

/*******************************************/

void help_ettercap(void)
{
   SHOW_MAN("ettercap", "./man/ettercap.8");
}

void help_curses(void)
{
   SHOW_MAN("ettercap_curses", "./man/ettercap_curses.8");
}

void help_plugins(void)
{
   SHOW_MAN("ettercap_plugins", "./man/ettercap_plugins.8");
}

void help_etterconf(void)
{
   SHOW_MAN("etter.conf", "./man/etter.conf.5");
}

void help_etterfilter(void)
{
   SHOW_MAN("etterfilter", "./man/etterfilter.8");
}

void help_etterlog(void)
{
   SHOW_MAN("etterlog", "./man/etterlog.8");
}

/* EOF */

// vim:ts=3:expandtab

