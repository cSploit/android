/*
    ettercap -- diplay the collected profiles

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

    $Id: ec_text_profile.c,v 1.7 2004/07/20 09:53:53 alor Exp $
*/

#include <ec.h>
#include <ec_threads.h>
#include <ec_poll.h>
#include <ec_interfaces.h>
#include <ec_profiles.h>
#include <ec_passive.h>

#ifdef OS_WINDOWS
   #include <missing/termios_mingw.h>
#else
   #include <termios.h>
#endif

/* globals */

/* declared in ec_text.c */
extern struct termios old_tc;
extern struct termios new_tc;

/* proto */

void text_profiles(void);
static void detail_hosts(int type);
static void detail_select(void);
static void detail_help(void);

/*******************************************/

void text_profiles(void)
{   

   detail_help();
   
   LOOP {
   
      CANCELLATION_POINT();
      
      /* if there is a pending char to be read */
      if ( ec_poll_in(fileno(stdin), 10) || ec_poll_buffer(GBL_OPTIONS->script) ) {
         
         char ch = 0;

         /* get the input from the stdin or the buffer */
         if (ec_poll_buffer(GBL_OPTIONS->script))
            ch = getchar_buffer(&GBL_OPTIONS->script);
         else
            ch = getchar();

         switch(ch) {
            case 'H':
            case 'h':
               detail_help();
               break;
            case 'L':
            case 'l':
               detail_hosts(FP_HOST_LOCAL);
               break;
            case 'R':
            case 'r':
               detail_hosts(FP_HOST_NONLOCAL);
               break;
            case 'S':
            case 's':
               detail_select();
               break;
            case 'p':
               profile_purge_local();
               USER_MSG("LOCAL hosts purged !\n");
               break;
            case 'P':
               profile_purge_remote();
               USER_MSG("REMOTE hosts purged !\n");
               break;
            case 'Q':
            case 'q':
               USER_MSG("Returning to main menu...\n");
               ui_msg_flush(1);
               return;
               break;
         }
      }
      
      /* print pending USER_MSG messages */
      ui_msg_flush(10);
   }
  
   /* NOT REACHED */
      
}

/*
 * print the help screen
 */
static void detail_help(void)
{
   fprintf(stderr, "\n\n [PROFILES] Inline help:\n\n");
   fprintf(stderr, "   [lL]  - detail on local hosts\n");
   fprintf(stderr, "   [rR]  - detail on remote hosts\n");
   fprintf(stderr, "   [sS]  - select a specific host\n");
   fprintf(stderr, "   [p]   - purge local hosts\n");
   fprintf(stderr, "   [P]   - purge remote hosts\n");
   fprintf(stderr, "   [qQ]  - return to main interface\n\n");
}

/*
 * print the list of TYPE (local or remote) hosts 
 */
static void detail_hosts(int type)
{
   struct host_profile *h;
   int at_least_one = 0;
   
   /* go thru the list and print a profile for each host */
   TAILQ_FOREACH(h, &GBL_PROFILES, next) {

      if (h->type & type) {
         
         at_least_one = 1;
  
         /* print the host infos */
         print_host(h);
      }
   }
      
   /* there aren't any profiles */
   if (!at_least_one) {
      if (GBL_OPTIONS->read) {
         fprintf(stdout, "Can't determine host type when reading from file !!\n");
         fprintf(stdout, "Use the select option !!\n");
      } else
         fprintf(stdout, "No collected profiles !!\n");
   }
}

/*
 * print the list of all collected hosts
 * and prompt for a choice
 */
static void detail_select(void)
{
   struct host_profile *h;
   char tmp[MAX_ASCII_ADDR_LEN];
   int i = 0, n = -1;
   
   /* go thru the list and print a profile for each host */
   TAILQ_FOREACH(h, &GBL_PROFILES, next) {
      fprintf(stdout, "%2d) %15s   %s\n", ++i, ip_addr_ntoa(&h->L3_addr, tmp), (h->hostname) ? h->hostname : "");
   }
      
   /* there aren't any profiles */
   if (i == 0) {
      fprintf(stdout, "No collected profiles !!\n");
      return;
   }
   
   fprintf(stdout, "Select an host to display (0 for all, -1 to quit): ");
   fflush(stdout);

   /* enable buffered input and echo */
   tcsetattr(0, TCSANOW, &old_tc);

   /* get the user input */
   scanf("%d", &n);
 
   /* disable buffered input */
   tcsetattr(0, TCSANOW, &new_tc);
   
   fprintf(stdout, "\n\n");

   /* do the proper action */
   switch(n) {
      case -1:
         /* quit with no action */
         return;
         break;
      case 0:
         /* print ALL the profiles */
         TAILQ_FOREACH(h, &GBL_PROFILES, next)
            print_host(h);
         
         break;
      default:
         i = 1;
         TAILQ_FOREACH(h, &GBL_PROFILES, next) {
            if (i++ == n)
               print_host(h);
         }
         break;
   }
}

/* EOF */

// vim:ts=3:expandtab

