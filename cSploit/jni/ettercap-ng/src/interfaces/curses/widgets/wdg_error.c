/*
    WDG -- errors handling module

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

#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define ERROR_MSG_LEN 200

/* PROTOS */

void wdg_bug(char *file, const char *function, int line, char *message);
void wdg_error_msg(char *file, const char *function, int line, char *message, ...);

/*******************************************/

/*
 * raise an error
 */
void wdg_error_msg(char *file, const char *function, int line, char *message, ...)
{
   va_list ap;
   char errmsg[ERROR_MSG_LEN + 1];    /* should be enough */
   int err_code;

#ifdef OS_WINDOWS
   err_code = GetLastError();  /* Most likely not a libc error */
   if (err_code == 0)
      err_code = errno;
#else
   err_code = errno;
#endif

   va_start(ap, message);
   vsnprintf(errmsg, ERROR_MSG_LEN, message, ap);
   va_end(ap);

   /* close the interface and display the error */
   wdg_cleanup();
  
   fprintf(stderr, "WDG ERROR : %d, %s\n[%s:%s:%d]\n\n %s \n\n",  err_code, strerror(err_code),
                   file, function, line, errmsg );

   exit(-err_code);
}

/*
 * used in sanity check
 * it represent a BUG in the software
 */
void wdg_bug(char *file, const char *function, int line, char *message)
{
   /* close the interface and display the error */
   wdg_cleanup();
  
   fprintf(stderr, "\n\nWDG BUG at [%s:%s:%d]\n\n %s \n\n", file, function, line, message );

   exit(-666);
}

/* EOF */

// vim:ts=3:expandtab

