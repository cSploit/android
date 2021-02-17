/*
    WDG -- debug module

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

#include <ctype.h>

#ifdef DEBUG

#ifdef HAVE_NCURSES
   #include <ncurses.h>
#endif

#include <stdarg.h>

/* globals */

FILE *wdg_debug_file = NULL;

/* protos */

void wdg_debug_init(void);
void wdg_debug_close(void);
void wdg_debug_msg(const char *message, ...);

/**********************************/

void wdg_debug_init(void)
{
   wdg_debug_file = fopen ("libwdg-"LIBWDG_VERSION"_debug.log", "w");
   WDG_ON_ERROR(wdg_debug_file, NULL, "Couldn't open for writing %s", "libwdg-"LIBWDG_VERSION"_debug.log");
   
   fprintf (wdg_debug_file, "\n==============================================================\n");
                   
  	fprintf (wdg_debug_file, "\n-> libwdg %s\n\n", LIBWDG_VERSION);
   #if defined (__GNUC__) && defined (__GNUC_MINOR__)
      fprintf (wdg_debug_file, "-> compiled with gcc %d.%d (%s)\n", __GNUC__, __GNUC_MINOR__, CC_VERSION);
   #endif
   #if defined (__GLIBC__) && defined (__GLIBC_MINOR__)
      fprintf (wdg_debug_file, "-> glibc version %d.%d\n", __GLIBC__, __GLIBC_MINOR__);
   #endif
   #ifdef HAVE_NCURSES 
      fprintf (wdg_debug_file, "-> %s\n", curses_version());
   #endif
   fprintf (wdg_debug_file, "\n\nDEVICE OPENED FOR libwdg DEBUGGING\n\n");
   fflush(wdg_debug_file);
}



void wdg_debug_close(void)
{
   fprintf (wdg_debug_file, "\n\nDEVICE CLOSED FOR DEBUGGING\n\n");
   fflush(wdg_debug_file);
   fclose (wdg_debug_file);
   /* set it to null and check from other threads */
   wdg_debug_file = NULL;
}


void wdg_debug_msg(const char *message, ...)
{
   va_list ap;
   char debug_message[strlen(message)+2];

   /* if it was closed by another thread on exit */
   if (wdg_debug_file == NULL)
      return;

   memset(debug_message, 0, sizeof(debug_message));
   
   strncpy(debug_message, message, sizeof(debug_message) - 2);
   strcat(debug_message, "\n");

   va_start(ap, message);
   vfprintf(wdg_debug_file, debug_message, ap);
   va_end(ap);

   fflush(wdg_debug_file);
}

#endif /* DEBUG */


/* EOF */

// vim:ts=3:expandtab

