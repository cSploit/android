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
#include <ec_log.h>

#define FILE_LEN  40

/* proto */

static void toggle_compress(void);
static void curses_log_all(void);
static void log_all(void);
static void curses_log_info(void);
static void log_info(void);
static void curses_log_msg(void);
static void log_msg(void);
static void curses_stop_log(void);
static void curses_stop_msg(void);

/* globals */

static char tag_compress[] = " ";
static char *logfile;

struct wdg_menu menu_logging[] = { {"Logging",                      'L', "",  NULL},
                                   {"Log all packets and infos...", 'I', "I", curses_log_all},
                                   {"Log only infos...",            'i', "i", curses_log_info},
                                   {"Stop logging infos",           0,   "",  curses_stop_log},
                                   {"-",                            0,   "",  NULL},
                                   {"Log user messages...",         'm', "m", curses_log_msg},
                                   {"Stop logging messages",        0,   "",  curses_stop_msg},
                                   {"-",                            0,   "",  NULL},
                                   {"Compressed file",              0, tag_compress,  toggle_compress},
                                   {NULL, 0, NULL, NULL},
                                 };

/*******************************************/

static void toggle_compress(void)
{
   if (GBL_OPTIONS->compress) {
      tag_compress[0] = ' ';
      GBL_OPTIONS->compress = 0;
   } else {
      tag_compress[0] = '*';
      GBL_OPTIONS->compress = 1;
   }
}

/*
 * display the log dialog 
 */
static void curses_log_all(void)
{
   DEBUG_MSG("curses_log_all");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, FILE_LEN, sizeof(char));

   curses_input("Log File :", logfile, FILE_LEN, log_all);
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
static void curses_log_info(void)
{
   DEBUG_MSG("curses_log_info");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, FILE_LEN, sizeof(char));

   curses_input("Log File :", logfile, FILE_LEN, log_info);
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

static void curses_stop_log(void)
{
   set_loglevel(LOG_STOP, "");
   curses_message("Logging was stopped.");
}

/*
 * display the log dialog 
 */
static void curses_log_msg(void)
{
   DEBUG_MSG("curses_log_msg");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, FILE_LEN, sizeof(char));

   curses_input("Log File :", logfile, FILE_LEN, log_msg);
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

static void curses_stop_msg(void)
{
   set_msg_loglevel(LOG_FALSE, NULL);
   curses_message("Message logging was stopped.");
}

/* EOF */

// vim:ts=3:expandtab

