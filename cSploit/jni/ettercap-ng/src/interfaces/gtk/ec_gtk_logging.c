/*
    ettercap -- GTK+ GUI

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
#include <ec_gtk.h>
#include <ec_log.h>

#define FILE_LEN  40

/* proto */

void toggle_compress(void);
void gtkui_log_all(void);
void gtkui_log_info(void);
void gtkui_log_msg(void);
void gtkui_stop_log(void);
void gtkui_stop_msg(void);

static void log_all(void);
static void log_info(void);
static void log_msg(void);

/* globals */

static char *logfile;

/*******************************************/

void toggle_compress(void)
{
   if (GBL_OPTIONS->compress) {
      GBL_OPTIONS->compress = 0;
   } else {
      GBL_OPTIONS->compress = 1;
   }
}

/*
 * display the log dialog 
 */
void gtkui_log_all(void)
{
   DEBUG_MSG("gtk_log_all");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, FILE_LEN, sizeof(char));

   gtkui_input("Log File :", logfile, FILE_LEN, log_all);
}

static void log_all(void)
{
   /* a check on the input */
   if (strlen(logfile) == 0) {
      ui_error("Please specify a filename");
      return;
   }

   set_loglevel(LOG_PACKET, logfile);
   SAFE_FREE(logfile);
}

/*
 * display the log dialog 
 */
void gtkui_log_info(void)
{
   DEBUG_MSG("gtk_log_info");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, FILE_LEN, sizeof(char));

   gtkui_input("Log File :", logfile, FILE_LEN, log_info);
}

static void log_info(void)
{
   /* a check on the input */
   if (strlen(logfile) == 0) {
      ui_error("Please specify a filename");
      return;
   }

   set_loglevel(LOG_INFO, logfile);
   SAFE_FREE(logfile);
}

void gtkui_stop_log(void)
{
   set_loglevel(LOG_STOP, "");
   gtkui_message("Logging was stopped.");
}

/*
 * display the log dialog 
 */
void gtkui_log_msg(void)
{
   DEBUG_MSG("gtk_log_msg");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, FILE_LEN, sizeof(char));

   gtkui_input("Log File :", logfile, FILE_LEN, log_msg);
}

static void log_msg(void)
{
   /* a check on the input */
   if (strlen(logfile) == 0) {
      ui_error("Please specify a filename");
      return;
   }

   set_msg_loglevel(LOG_TRUE, logfile);
   SAFE_FREE(logfile);
}

void gtkui_stop_msg(void)
{
   set_msg_loglevel(LOG_FALSE, NULL);
   gtkui_message("Message logging was stopped.");
}

/* EOF */

// vim:ts=3:expandtab

