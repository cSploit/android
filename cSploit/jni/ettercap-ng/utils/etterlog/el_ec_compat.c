/*
    etterlog -- ettercap compatibility module
                here are stored functions needed by ec_* source
                but not linked in etterlog

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

    $Id: el_ec_compat.c,v 1.10 2003/10/12 15:28:27 alor Exp $
*/

#include <el.h>

#include <stdarg.h>

/* globals */
FILE *debug_file = (void *)1;  /* not NULL to avoid FATAL_ERROR */

/* protos */
void debug_msg(const char *message, ...);
void ui_msg(const char *fmt, ...);
void ui_error(const char *fmt, ...);
void ui_fatal_error(const char *fmt, ...);
void ui_cleanup(void);
void * get_decoder(u_int8 level, u_int32 type);
void open_socket(char *host, u_int16 port);
void socket_send(int s, u_char *payload, size_t size);
void socket_recv(int s, u_char *payload, size_t size);
void close_socket(int s);

/************************************************/
 
/* the void implemetation */

void debug_msg(const char *message, ...) { }

/* fake the UI implementation */
void ui_msg(const char *fmt, ...) 
{ 
   va_list ap;
   /* print the mesasge */ 
   va_start(ap, fmt);
   vfprintf (stderr, fmt, ap);
   va_end(ap);
}

void ui_error(const char *fmt, ...) 
{ 
   va_list ap;
   /* print the mesasge */ 
   va_start(ap, fmt);
   vfprintf (stderr, fmt, ap);
   va_end(ap);
   fprintf(stderr, "\n");
}

void ui_fatal_error(const char *fmt, ...) 
{ 
   va_list ap;
   /* print the mesasge */ 
   va_start(ap, fmt);
   vfprintf (stderr, fmt, ap);
   va_end(ap);
   fprintf(stderr, "\n");

   exit(-1);
}

void ui_cleanup(void) { }

/* for ec_passive is_open_port */
void * get_decoder(u_int8 level, u_int32 type)
{
   return NULL;
}

/* fake socket connections */
void open_socket(char *host, u_int16 port) { }
void socket_send(int s, u_char *payload, size_t size) { }
void socket_recv(int s, u_char *payload, size_t size) { }
void close_socket(int s) { }

/* EOF */

// vim:ts=3:expandtab

