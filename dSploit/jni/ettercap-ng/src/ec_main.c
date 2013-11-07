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

    $Id: ec_main.c,v 1.66 2004/11/04 10:29:06 alor Exp $
*/

#include <ec.h>
#include <ec_version.h>
#include <ec_globals.h>
#include <ec_signals.h>
#include <ec_parser.h>
#include <ec_threads.h>
#include <ec_capture.h>
#include <ec_dispatcher.h>
#include <ec_send.h>
#include <ec_plugins.h>
#include <ec_format.h>
#include <ec_fingerprint.h>
#include <ec_manuf.h>
#include <ec_services.h>
#include <ec_http.h>
#include <ec_scan.h>
#include <ec_ui.h>
#include <ec_conf.h>
#include <ec_mitm.h>
#include <ec_sslwrap.h>

/* global vars */


/* protos */

void clean_exit(int errcode);
static void drop_privs(void);
static void time_check(void);

/*******************************************/

int main(int argc, char *argv[])
{
    // unbuffered stdout
    setbuf(stdout,NULL);
    
   /*
    * Alloc the global structures
    * We can access these structs via the macro in ec_globals.h
    */
        
   globals_alloc();
  
   GBL_PROGRAM = strdup(EC_PROGRAM);
   GBL_VERSION = strdup(EC_VERSION);
   SAFE_CALLOC(GBL_DEBUG_FILE, strlen(EC_PROGRAM) + strlen(EC_VERSION) + strlen("_debug.log") + 1, sizeof(char));
   sprintf(GBL_DEBUG_FILE, "%s%s_debug.log", GBL_PROGRAM, EC_VERSION);
   
   DEBUG_INIT();
   DEBUG_MSG("main -- here we go !!");
   
   /* register the main thread as "init" */
   ec_thread_register(EC_PTHREAD_SELF, "init", "initialization phase");
   
   /* activate the signal handler */
   signal_handler();
   
   /* ettercap copyright */
   fprintf(stdout, "\n" EC_COLOR_BOLD "%s %s" EC_COLOR_END " copyright %s %s\n\n", 
         GBL_PROGRAM, GBL_VERSION, EC_COPYRIGHT, EC_AUTHORS);
   
   /* getopt related parsing...  */
   parse_options(argc, argv);

   /* check the date */
   time_check();

   /* load the configuration file */
   load_conf();
   
   /* 
    * get the list of available interfaces 
    * 
    * this function will not return if the -I option was
    * specified on command line. it will instead print the
    * list and exit
    */
   capture_getifs();
   
   /* initialize the user interface */
   ui_init();
   
   /* initialize libpcap */
   capture_init();

   /* initialize libnet (the function contain all the checks) */
   send_init();
 
   /* get hardware infos */
   get_hw_info();
 
   /* 
    * always disable the kernel ip forwarding (except when reading from file).
    * the forwarding will be done by ettercap.
    */
   if (!GBL_OPTIONS->read && !GBL_OPTIONS->unoffensive && !GBL_OPTIONS->only_mitm)
      disable_ip_forward();
      
   /* binds ports and set redirect for ssl wrapper */
   if (!GBL_OPTIONS->read && !GBL_OPTIONS->unoffensive && !GBL_OPTIONS->only_mitm && GBL_SNIFF->type == SM_UNIFIED)
      ssl_wrap_init();
   
   /* 
    * drop root privileges 
    * we have alread opened the sockets with high privileges
    * we don't need any more root privs.
    */
   drop_privs();

/***** !! NO PRIVS AFTER THIS POINT !! *****/

   /* load all the plugins */
   plugin_load_all();

   /* print how many dissectors were loaded */
   conf_dissectors();
   
   /* load the mac-fingerprints */
   manuf_init();

   /* load the tcp-fingerprints */
   fingerprint_init();
   
   /* load the services names */
   services_init();
   
   /* load http known fileds for user/pass */
   http_fields_init();

   /* set the encoding for the UTF-8 visualization */
   set_utf8_encoding(GBL_CONF->utf8_encoding);
  
   /* print all the buffered messages */
   if (GBL_UI->type == UI_TEXT)
      USER_MSG("\n");
   
   ui_msg_flush(MSG_ALL);

/**** INITIALIZATION PHASE TERMINATED ****/
   
   /* 
    * we are interested only in the mitm attack i
    * if entered, this function will not return...
    */
   if (GBL_OPTIONS->only_mitm)
      only_mitm();
   
   /* create the dispatcher thread */
   ec_thread_new("top_half", "dispatching module", &top_half, NULL);

   /* this thread becomes the UI then displays it */
   ec_thread_register(EC_PTHREAD_SELF, GBL_PROGRAM, "the user interface");
   ui_start();

/******************************************** 
 * reached only when the UI is shutted down 
 ********************************************/

   /* flush the exit message */
   ui_msg_flush(MSG_ALL);
   
   /* stop the mitm attack */
   mitm_stop();

   /* terminate the sniffing engine */
   EXECUTE(GBL_SNIFF->cleanup);
   
   /* kill all the running threads but the current */
   ec_thread_kill_all();
  
   /* clean up the UI */
   ui_cleanup();

   return 0;
}


/* 
 * drop root privs 
 */
static void drop_privs(void)
{
   u_int uid, gid;
   char *var;

#ifdef OS_WINDOWS
   /* do not drop privs under windows */
   return;
#endif
   
   /* are we root ? */
   if (getuid() != 0)
      return;

   /* get the env variable for the UID to drop privs to */
   var = getenv("EC_UID");
   
   /* if the EC_UID variable is not set, default to GBL_CONF->ec_uid (nobody) */
   if (var != NULL)
      uid = atoi(var);
   else
      uid = GBL_CONF->ec_uid;
   
   /* get the env variable for the GID to drop privs to */
   var = getenv("EC_GID");
   
   /* if the EC_UID variable is not set, default to GBL_CONF->ec_gid (nobody) */
   if (var != NULL)
      gid = atoi(var);
   else
      gid = GBL_CONF->ec_gid;
   
   DEBUG_MSG("drop_privs: setuid(%d) setgid(%d)", uid, gid);
   
   /* drop to a good uid/gid ;) */
   if ( setgid(gid) < 0 )
      ERROR_MSG("setgid()");
   
   if ( setuid(uid) < 0 )
      ERROR_MSG("setuid()");

   DEBUG_MSG("privs: UID: %d %d  GID: %d %d", (int)getuid(), (int)geteuid(), (int)getgid(), (int)getegid() );
   USER_MSG("Privileges dropped to UID %d GID %d...\n\n", (int)getuid(), (int)getgid() ); 
}


/*
 * cleanly exit from the program
 */

void clean_exit(int errcode)
{
   DEBUG_MSG("clean_exit: %d", errcode);
  
   INSTANT_USER_MSG("\nTerminating %s...\n", GBL_PROGRAM);

   /* kill all the running threads but the current */
   ec_thread_kill_all();

   /* close the UI */
   ui_cleanup();

   /* call all the ATEXIT functions */
   exit(errcode);
}


static void time_check(void)
{
   /* 
    * a nice easter egg... 
    * just to waste some time of code reviewers... ;) 
    *
    * and no, you can't simply remove this code, you'll break the license...
    *
    * trust me, it's not evil ;) only a boring afternoon, and nothing to do...
    */
   time_t K9=time(NULL);char G5P[1<<6],*o=G5P,*O;uint U4M, _,__=0; char dMG[]= 
   "\n*\n^1U4Mm\x04wW#K\x2e\x0e+X\x7f\f,N'U!I-L5?";struct{char X5T[7];int dMG;
   int U4M;} X5T[]={{"N!WwFr", 0x414c6f52,0},{"S6FfUe", 0x4e614741,0}};sprintf
   (G5P,"%s",ctime(&K9));o+=4;O=strchr(o+4,' ');*O=0; for(U4M=(1<<5)-(1<<2)+1;
   U4M>0;U4M--)dMG[U4M]=dMG[U4M]^dMG[U4M-1];for(U4M=0;U4M<sizeof(X5T)/sizeof(*
   X5T);U4M++){for(_=(1<<2)+1; _>0;_--)X5T[U4M].X5T[_]=X5T[U4M].X5T[_]^X5T[U4M
   ].X5T[_-1];if(!strcmp(X5T[U4M].X5T,o)){char T0Q[]="\n\0O!M4\x14r\x1doO;T0Q"
   "(\bm\x19m\bz\x19x\b(A2\x12s\x1d=X5T=Q&G5Pp\x03l\n~\th\x1a\x7f_dMG\x06hH-@"
   "!H$\x04s\x1av\x1a:X=\x1d|\f|\x0ek\ba\0t\x11u[u[{^-m\fb\x16\x7f\x19v\x04oA"
   "\x2e\\;1;K9\\/\\|9w#f4\x1a\x34\x1a\x1a";for(_=(1<<7)-(1<<3)-(1<<2)+1;_>0;_
   --)T0Q[_]=T0Q[_]^T0Q[_-1];write(1,dMG,1);while(__++<1<<5)printf("%c",(1<<5)
   +(1<<3)+(1<<1));X5T[U4M].dMG=ntohl(X5T[U4M].dMG);printf(dMG,&X5T[U4M].dMG);
   while(--__) printf("%c",(1<<6)-(1<<4)-(1<<3)+(1<<1)); printf(T0Q,&X5T[U4M].
   dMG);getchar();break;}}
}

/* EOF */

// vim:ts=3:expandtab

