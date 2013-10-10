/*
    ettercap -- GUI management

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

    $Id: ec_interfaces.c,v 1.4 2004/07/09 08:27:19 alor Exp $
*/

#include <ec.h>

/* extern from various interfaces */

extern void set_daemon_interface(void);
extern void set_text_interface(void);
extern void set_curses_interface(void);
extern void set_gtk_interface(void);

/* protos */

void select_daemon_interface(void);
void select_text_interface(void);
void select_curses_interface(void);
void select_gtk_interface(void);

/*******************************************/

void select_daemon_interface(void)
{
   DEBUG_MSG("select_daemon_interface");
   
   set_daemon_interface();
}

void select_text_interface(void)
{
   DEBUG_MSG("select_text_interface");
   
   set_text_interface();
}

void select_curses_interface(void)
{
   DEBUG_MSG("select_curses_interface");
#ifdef HAVE_NCURSES 
   /* check if the stdout is available */
   if (isatty(fileno(stdout)) <= 0)
      FATAL_ERROR("Cannot use Curses if stdout is redirected");
   
   set_curses_interface();
#else
   FATAL_ERROR("Curses support not compiled in %s", GBL_PROGRAM);
#endif
   
}

void select_gtk_interface(void)
{
   DEBUG_MSG("select_gtk_interface");
#ifdef HAVE_GTK 
   set_gtk_interface();
#else
   FATAL_ERROR("GTK support not compiled in %s", GBL_PROGRAM);
#endif
   
}

/* EOF */


// vim:ts=3:expandtab

