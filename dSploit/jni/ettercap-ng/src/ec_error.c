/*
    ettercap -- error handling module

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

    $Id: ec_error.c,v 1.12 2004/07/23 07:25:27 alor Exp $
*/

#include <ec.h>
#include <ec_ui.h>

#include <stdarg.h>
#include <errno.h>

#define ERROR_MSG_LEN 200

void error_msg(char *file, const char *function, int line, char *message, ...);
void fatal_error_msg(char *message, ...);
void bug(char *file, const char *function, int line, char *message);

/*******************************************/

/*
 * raise an error
 */
void error_msg(char *file, const char *function, int line, char *message, ...)
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

   DEBUG_MSG("ERROR : %d, %s\n[%s:%s:%d] %s \n",  err_code, strerror(err_code),
                   file, function, line, errmsg );
   
   /* close the interface and display the error */
   ui_cleanup();
  
   fprintf(stderr, "ERROR : %d, %s\n[%s:%s:%d]\n\n %s \n\n",  err_code, strerror(err_code),
                   file, function, line, errmsg );

   exit(-err_code);
}


/*
 * raise a fatal error
 */
void fatal_error(char *message, ...)
{
   va_list ap;
   char errmsg[ERROR_MSG_LEN + 1];    /* should be enough */

   va_start(ap, message);
   vsnprintf(errmsg, ERROR_MSG_LEN, message, ap);
   va_end(ap);

   /* if debug was initialized... */
#ifdef DEBUG
   if (debug_file != NULL)
      DEBUG_MSG("FATAL: %s", errmsg);
#endif

   /* invoke the ui method */
   ui_fatal_error(errmsg);

   /* the ui should exits, but to be sure... */
   exit(-1);
}

/*
 * used in sanity check
 * it represent a BUG in the software
 */

void bug(char *file, const char *function, int line, char *message)
{
   DEBUG_MSG("BUG : [%s:%s:%d] %s \n", file, function, line, message );
   
   /* close the interface and display the error */
   ui_cleanup();
  
   fprintf(stderr, "\n\nBUG at [%s:%s:%d]\n\n %s \n\n", file, function, line, message );

   exit(-666);
}


/* EOF */

// vim:ts=3:expandtab

