/*
    ettercap -- everything starts from this file... 

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
#include <ec_ui.h>
#include <ec_mitm.h>
#include <ec_threads.h>
#ifdef HAVE_EC_LUA
#include <ec_lua.h>
#endif

void clean_exit(int errcode);

/*
 * cleanly exit from the program
 */

void clean_exit(int errcode)
{
   DEBUG_MSG("clean_exit: %d", errcode);
  
   INSTANT_USER_MSG("\nTerminating %s...\n", GBL_PROGRAM);

#ifdef HAVE_EC_LUA
   /* Cleanup lua */
   ec_lua_fini();
#endif

   /* flush the exit message */
   ui_msg_flush(MSG_ALL);

   /* stop the mitm attack */
   mitm_stop();

   /* terminate the sniffing engine */
   EXECUTE(GBL_SNIFF->cleanup);

   /* kill all the running threads but the current */
   ec_thread_kill_all();

   /* close the UI */
   ui_cleanup();

   /* call all the ATEXIT functions */
   exit(errcode);

}

