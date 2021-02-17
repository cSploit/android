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
#include <ec_capture.h>

#include <pcap.h>

/* globals */

static wdg_t *sysmsg_win;
static char tag_unoff[] = " ";
static char tag_promisc[] = " ";

/* proto */

void set_curses_interface(void);
static void curses_interface(void);
void curses_flush_msg(void);

void curses_message(const char *msg);
   
static void curses_init(void);
static void curses_cleanup(void);
static void curses_msg(const char *msg);
static void curses_error(const char *msg);
static void curses_fatal_error(const char *msg);
void curses_input(const char *title, char *input, size_t n, void (*callback)(void));
static int curses_progress(char *title, int value, int max);
static void curses_update(int target);

static void curses_setup(void);
static void curses_exit(void);

static void toggle_unoffensive(void);
static void toggle_nopromisc(void);

static void curses_file_open(void);
static void read_pcapfile(char *path, char *file);
static void curses_file_write(void);
static void write_pcapfile(void);
static void curses_unified_sniff(void);
static void curses_bridged_sniff(void);
static void bridged_sniff(void);
static void curses_pcap_filter(void);
static void curses_set_netmask(void);


/*******************************************/


void set_curses_interface(void)
{
   struct ui_ops ops;

   /* wipe the struct */
   memset(&ops, 0, sizeof(ops));

   /* register the functions */
   ops.init = &curses_init;
   ops.start = &curses_interface;
   ops.cleanup = &curses_cleanup;
   ops.msg = &curses_msg;
   ops.error = &curses_error;
   ops.fatal_error = &curses_fatal_error;
   ops.input = &curses_input;
   ops.progress = &curses_progress;
   ops.update = &curses_update;
   ops.type = UI_CURSES;
   
   ui_register(&ops);
   
}


/*
 * set the terminal as non blocking 
 */
static void curses_init(void)
{
   DEBUG_MSG("curses_init");

   /* init the widgets library */
   wdg_init();

   /* 
    * we have to set it because we ask user interaction
    * during this function.
    * we cant wait to return to set the flag...
    */
   GBL_UI->initialized = 1;

   DEBUG_MSG("curses_init: screen %dx%d colors: %d", (int)current_screen.cols, (int)current_screen.lines,
                                                     (int)(current_screen.flags & WDG_SCR_HAS_COLORS));

   /* initialize the colors */
   wdg_init_color(EC_COLOR, GBL_CONF->colors.fg, GBL_CONF->colors.bg);
   wdg_init_color(EC_COLOR_JOIN1, GBL_CONF->colors.join1, GBL_CONF->colors.bg);
   wdg_init_color(EC_COLOR_JOIN2, GBL_CONF->colors.join2, GBL_CONF->colors.bg);
   wdg_init_color(EC_COLOR_BORDER, GBL_CONF->colors.border, GBL_CONF->colors.bg);
   wdg_init_color(EC_COLOR_TITLE, GBL_CONF->colors.title, GBL_CONF->colors.bg);
   wdg_init_color(EC_COLOR_FOCUS, GBL_CONF->colors.focus, GBL_CONF->colors.bg);
   wdg_init_color(EC_COLOR_MENU, GBL_CONF->colors.menu_fg, GBL_CONF->colors.menu_bg);
   wdg_init_color(EC_COLOR_WINDOW, GBL_CONF->colors.window_fg, GBL_CONF->colors.window_bg);
   wdg_init_color(EC_COLOR_SELECTION, GBL_CONF->colors.selection_fg, GBL_CONF->colors.selection_bg);
   wdg_init_color(EC_COLOR_ERROR, GBL_CONF->colors.error_fg, GBL_CONF->colors.error_bg);
   wdg_init_color(EC_COLOR_ERROR_BORDER, GBL_CONF->colors.error_border, GBL_CONF->colors.error_bg);

   /* set the screen color */
   wdg_screen_color(EC_COLOR);
   
   /* call the setup interface */
   curses_setup();

   /* reached only after the setup interface has quit */
}

/*
 * exit from the setup interface 
 */
static void curses_exit(void)
{
   DEBUG_MSG("curses_exit");
   wdg_cleanup();
   clean_exit(0);
}

/*
 * reset to the previous state
 */
static void curses_cleanup(void)
{
   DEBUG_MSG("curses_cleanup");

   wdg_cleanup();
}

/*
 * this function is called on idle loop in wdg
 */
void curses_flush_msg(void)
{
   ui_msg_flush(MSG_ALL);
}

/*
 * print a USER_MSG() extracting it from the queue
 */
static void curses_msg(const char *msg)
{

   /* if the object does not exist yet */
   if (sysmsg_win == NULL)
      return;

   wdg_scroll_print(sysmsg_win, EC_COLOR, "%s", (char *)msg);
}


/*
 * print an error
 */
static void curses_error(const char *msg)
{
   wdg_t *dlg;
   
   DEBUG_MSG("curses_error: %s", msg);

   /* create the dialog */
   wdg_create_object(&dlg, WDG_DIALOG, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   
   wdg_set_title(dlg, "ERROR:", WDG_ALIGN_LEFT);
   wdg_set_color(dlg, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(dlg, WDG_COLOR_WINDOW, EC_COLOR_ERROR);
   wdg_set_color(dlg, WDG_COLOR_FOCUS, EC_COLOR_ERROR_BORDER);
   wdg_set_color(dlg, WDG_COLOR_TITLE, EC_COLOR_ERROR);

   /* set the message */
   wdg_dialog_text(dlg, WDG_OK, msg);
   wdg_draw_object(dlg);
   
   wdg_set_focus(dlg);
}


/*
 * handle a fatal error and exit
 */
static void curses_fatal_error(const char *msg)
{
   DEBUG_MSG("curses_fatal_error: %s", msg);
   
   /* cleanup the curses mode */
   wdg_cleanup();

   fprintf(stderr, "FATAL ERROR: %s\n\n\n", msg);

   clean_exit(-1);
}


/*
 * get an input from the user 
 */
void curses_input(const char *title, char *input, size_t n, void (*callback)(void))
{
   wdg_t *in;

   wdg_create_object(&in, WDG_INPUT, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   wdg_set_color(in, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(in, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(in, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(in, WDG_COLOR_TITLE, EC_COLOR_MENU);
   wdg_input_size(in, strlen(title) + n, 3);
   wdg_input_add(in, 1, 1, title, input, n, 1);
   wdg_input_set_callback(in, callback);
   
   wdg_draw_object(in);
   
   wdg_set_focus(in);
  
   /* block until user input */
   wdg_input_get_input(in);
}

/* 
 * implement the progress bar 
 */
static int curses_progress(char *title, int value, int max)
{
   static wdg_t *per = NULL;
   int ret;
   
   /* the first time, create the object */
   if (per == NULL) {
      wdg_create_object(&per, WDG_PERCENTAGE, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
      
      wdg_set_title(per, title, WDG_ALIGN_CENTER);
      wdg_set_color(per, WDG_COLOR_SCREEN, EC_COLOR);
      wdg_set_color(per, WDG_COLOR_WINDOW, EC_COLOR);
      wdg_set_color(per, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
      wdg_set_color(per, WDG_COLOR_TITLE, EC_COLOR_MENU);
      
      wdg_draw_object(per);
      
      wdg_set_focus(per);
      
   } 
   
   /* the subsequent calls have to only update the object */
   ret = wdg_percentage_set(per, value, max);
   wdg_update_screen();

   switch (ret) {
      case WDG_PERCENTAGE_FINISHED:
         /* 
          * the object is self-destructing... 
          * so we have only to set the pointer to null
          */
         per = NULL;
         return UI_PROGRESS_FINISHED;
         break;
         
      case WDG_PERCENTAGE_INTERRUPTED: 
         /*
          * the user has requested to stop the current task.
          * the percentage was self-destructed, we have to 
          * set the pointer to null and return the proper value
          */
         per = NULL;
         return UI_PROGRESS_INTERRUPTED;
         break;
         
      case WDG_PERCENTAGE_UPDATED: 
         return UI_PROGRESS_UPDATED;
         break;
   }
  
   return UI_PROGRESS_UPDATED;
}

/*
 * process an update notification
 */
static void curses_update(int target)
{
   switch(target) {
      case UI_UPDATE_HOSTLIST:   curses_hosts_update();
                                 break;

      default:                   break;
   }
}

/*
 * print a message
 */
void curses_message(const char *msg)
{
   wdg_t *dlg;
   
   DEBUG_MSG("curses_message: %s", msg);

   /* create the dialog */
   wdg_create_object(&dlg, WDG_DIALOG, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   
   wdg_set_color(dlg, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(dlg, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(dlg, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(dlg, WDG_COLOR_TITLE, EC_COLOR_TITLE);

   /* set the message */
   wdg_dialog_text(dlg, WDG_OK, msg);
   wdg_draw_object(dlg);
   
   wdg_set_focus(dlg);
}


/* the interface */

void curses_interface(void)
{
   DEBUG_MSG("curses_interface");
   
   /* which interface do we have to display ? */
   if (GBL_OPTIONS->read)
      curses_sniff_offline();
   else
      curses_sniff_live();
   

   /* destroy the previously allocated object */
   wdg_destroy_object(&sysmsg_win);
}

static void toggle_unoffensive(void)
{
   if (GBL_OPTIONS->unoffensive) {
      tag_unoff[0] = ' ';
      GBL_OPTIONS->unoffensive = 0;
   } else {
      tag_unoff[0] = '*';
      GBL_OPTIONS->unoffensive = 1;
   }
}

static void toggle_nopromisc(void)
{
   if (GBL_PCAP->promisc) {
      tag_promisc[0] = ' ';
      GBL_PCAP->promisc = 0;
   } else {
      tag_promisc[0] = '*';
      GBL_PCAP->promisc = 1;
   }
}

/*
 * display the initial menu to setup global options
 * at startup.
 */
static void curses_setup(void)
{
   wdg_t *menu;
   
   struct wdg_menu file[] = { {"File",            'F',       "",    NULL},
                              {"Open...",         CTRL('O'), "C-o", curses_file_open},
                              {"Dump to file...", CTRL('D'), "C-d", curses_file_write},
                              {"-",               0,         "",    NULL},
                              {"Exit",            CTRL('X'), "C-x", curses_exit},
                              {NULL, 0, NULL, NULL},
                            };
   
   struct wdg_menu live[] = { {"Sniff",               'S', "",  NULL},
                              {"Unified sniffing...", 'U', "U", curses_unified_sniff},
                              {"Bridged sniffing...", 'B', "B", curses_bridged_sniff},
                              {"-",                    0,   "",  NULL},
                              {"Set pcap filter...",  'p', "p", curses_pcap_filter},
                              {NULL, 0, NULL, NULL},
                            };
   
   struct wdg_menu options[] = { {"Options",       'O', "",          NULL},
                                 {"Unoffensive",    0,  tag_unoff,   toggle_unoffensive},
                                 {"Promisc mode",   0,  tag_promisc, toggle_nopromisc},
                                 {"Set netmask",   'n', "n" , curses_set_netmask},
                                 {NULL, 0, NULL, NULL},
                               };
   
   
   DEBUG_MSG("curses_setup");
   
   wdg_create_object(&menu, WDG_MENU, WDG_OBJ_WANT_FOCUS | WDG_OBJ_ROOT_OBJECT);
   
   wdg_set_title(menu, GBL_VERSION, WDG_ALIGN_RIGHT);
   wdg_set_color(menu, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(menu, WDG_COLOR_WINDOW, EC_COLOR_MENU);
   wdg_set_color(menu, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(menu, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_menu_add(menu, file);
   wdg_menu_add(menu, live);
   wdg_menu_add(menu, options);
   wdg_menu_add(menu, menu_help);
   wdg_draw_object(menu);
   
   DEBUG_MSG("curses_setup: menu created");

   /* create the bottom windows for user messages */
   wdg_create_object(&sysmsg_win, WDG_SCROLL, WDG_OBJ_WANT_FOCUS);
   
   wdg_set_title(sysmsg_win, "User messages:", WDG_ALIGN_LEFT);
   wdg_set_size(sysmsg_win, 0, SYSMSG_WIN_SIZE, 0, 0);
   wdg_set_color(sysmsg_win, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(sysmsg_win, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(sysmsg_win, WDG_COLOR_BORDER, EC_COLOR_BORDER);
   wdg_set_color(sysmsg_win, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(sysmsg_win, WDG_COLOR_TITLE, EC_COLOR_TITLE);
   wdg_scroll_set_lines(sysmsg_win, 500);
   wdg_draw_object(sysmsg_win);
 
   /* give the focus to the menu */
   wdg_set_focus(menu);
   
   DEBUG_MSG("curses_setup: sysmsg created");
  
   /* initialize the options */
   if (GBL_OPTIONS->unoffensive)
      tag_unoff[0] = '*';
   else
      tag_unoff[0] = ' ';

   if (GBL_PCAP->promisc)
      tag_promisc[0] = '*';
   else
      tag_promisc[0] = ' ';
  
   
   /* give the control to the interface */
   wdg_events_handler('u');
   
   wdg_destroy_object(&menu);
   
   DEBUG_MSG("curses_setup: end");
}

/*
 * display the file open dialog
 */
static void curses_file_open(void)
{
   wdg_t *fop;
   
   DEBUG_MSG("curses_file_open");
   
   wdg_create_object(&fop, WDG_FILE, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   
   wdg_set_title(fop, "Select a pcap file...", WDG_ALIGN_LEFT);
   wdg_set_color(fop, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(fop, WDG_COLOR_WINDOW, EC_COLOR_MENU);
   wdg_set_color(fop, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(fop, WDG_COLOR_TITLE, EC_COLOR_TITLE);

   wdg_file_set_callback(fop, read_pcapfile);
   
   wdg_draw_object(fop);
   
   wdg_set_focus(fop);
}

static void read_pcapfile(char *path, char *file)
{
   char pcap_errbuf[PCAP_ERRBUF_SIZE];
   
   DEBUG_MSG("read_pcapfile %s/%s", path, file);
   
   SAFE_CALLOC(GBL_OPTIONS->pcapfile_in, strlen(path)+strlen(file)+2, sizeof(char));

   snprintf(GBL_OPTIONS->pcapfile_in, strlen(path)+strlen(file)+2, "%s/%s", path, file);

   /* check if the file is good */
   if (is_pcap_file(GBL_OPTIONS->pcapfile_in, pcap_errbuf) != ESUCCESS) {
      ui_error("%s", pcap_errbuf);
      SAFE_FREE(GBL_OPTIONS->pcapfile_in);
      return;
   }
   
   /* set the options for reading from file */
   GBL_OPTIONS->silent = 1;
   GBL_OPTIONS->unoffensive = 1;
   GBL_OPTIONS->write = 0;
   GBL_OPTIONS->read = 1;
   
   /* exit the setup interface, and go to the primary one */
   wdg_exit();
}

/*
 * display the write file menu
 */
static void curses_file_write(void)
{
#define FILE_LEN  40
   
   DEBUG_MSG("curses_file_write");
   
   SAFE_CALLOC(GBL_OPTIONS->pcapfile_out, FILE_LEN, sizeof(char));

   curses_input("Output file :", GBL_OPTIONS->pcapfile_out, FILE_LEN, write_pcapfile);
}

static void write_pcapfile(void)
{
   FILE *f;
   
   DEBUG_MSG("write_pcapfile");
   
   /* check if the file is writeable */
   f = fopen(GBL_OPTIONS->pcapfile_out, "w");
   if (f == NULL) {
      ui_error("Cannot write %s", GBL_OPTIONS->pcapfile_out);
      SAFE_FREE(GBL_OPTIONS->pcapfile_out);
      return;
   }
 
   /* if ok, delete it */
   fclose(f);
   unlink(GBL_OPTIONS->pcapfile_out);

   /* set the options for writing to a file */
   GBL_OPTIONS->write = 1;
   GBL_OPTIONS->read = 0;
}

/*
 * display the interface selection dialog
 */
static void curses_unified_sniff(void)
{
   char err[PCAP_ERRBUF_SIZE];
   
#define IFACE_LEN  50
   
   DEBUG_MSG("curses_unified_sniff");
  
   /* if the user has not specified an interface, get the first one */
   if (GBL_OPTIONS->iface == NULL) {
      char *iface;
      
      SAFE_CALLOC(GBL_OPTIONS->iface, IFACE_LEN, sizeof(char));
      iface = pcap_lookupdev(err);
      ON_ERROR(iface, NULL, "pcap_lookupdev: %s", err);

      strncpy(GBL_OPTIONS->iface, iface, IFACE_LEN - 1);
   }

   /* calling wdg_exit will go to the next interface :) */
   curses_input("Network interface :", GBL_OPTIONS->iface, IFACE_LEN, wdg_exit);
}

/*
 * display the interface selection for bridged sniffing
 */
static void curses_bridged_sniff(void)
{
   wdg_t *in;
   char err[PCAP_ERRBUF_SIZE];
   
   DEBUG_MSG("curses_bridged_sniff");
   
   /* if the user has not specified an interface, get the first one */
   if (GBL_OPTIONS->iface == NULL) {
      SAFE_CALLOC(GBL_OPTIONS->iface, IFACE_LEN, sizeof(char));
   /* if ettercap is started with a non root account pcap_lookupdev(err) == NULL (Fedora bug 783675) */
      if(pcap_lookupdev(err) != NULL)
         strncpy(GBL_OPTIONS->iface, pcap_lookupdev(err), IFACE_LEN - 1);
   /* else
	here we have to gracefully exit, since we don't have any available interface
  */
	
   }
   
   SAFE_CALLOC(GBL_OPTIONS->iface_bridge, IFACE_LEN, sizeof(char));

   wdg_create_object(&in, WDG_INPUT, WDG_OBJ_WANT_FOCUS | WDG_OBJ_FOCUS_MODAL);
   wdg_set_color(in, WDG_COLOR_SCREEN, EC_COLOR);
   wdg_set_color(in, WDG_COLOR_WINDOW, EC_COLOR);
   wdg_set_color(in, WDG_COLOR_FOCUS, EC_COLOR_FOCUS);
   wdg_set_color(in, WDG_COLOR_TITLE, EC_COLOR_MENU);
   wdg_input_size(in, strlen("Second network interface :") + IFACE_LEN, 4);
   wdg_input_add(in, 1, 1, "First network interface  :", GBL_OPTIONS->iface, IFACE_LEN, 1);
   wdg_input_add(in, 1, 2, "Second network interface :", GBL_OPTIONS->iface_bridge, IFACE_LEN, 1);
   wdg_input_set_callback(in, bridged_sniff);
   
   wdg_draw_object(in);
      
   wdg_set_focus(in);
}

static void bridged_sniff(void)
{
   set_bridge_sniff();
   
   wdg_exit();
}

/*
 * display the pcap filter dialog
 */
static void curses_pcap_filter(void)
{
#define PCAP_FILTER_LEN  50
   
   DEBUG_MSG("curses_pcap_filter");
   
   SAFE_CALLOC(GBL_PCAP->filter, PCAP_FILTER_LEN, sizeof(char));

   /* 
    * no callback, the filter is set but we have to return to
    * the interface for other user input
    */
   curses_input("Pcap filter :", GBL_PCAP->filter, PCAP_FILTER_LEN, NULL);
}

/*
 * set a different netmask than the system one 
 */
static void curses_set_netmask(void)
{
   struct in_addr net;
   
   DEBUG_MSG("curses_set_netmask");
  
   if (GBL_OPTIONS->netmask == NULL)
      SAFE_CALLOC(GBL_OPTIONS->netmask, IP_ASCII_ADDR_LEN, sizeof(char));

   /* 
    * no callback, the filter is set but we have to return to
    * the interface for other user input
    */
   curses_input("Netmask :", GBL_OPTIONS->netmask, IP_ASCII_ADDR_LEN, NULL);

   /* sanity check */
   if (strcmp(GBL_OPTIONS->netmask, "") && inet_aton(GBL_OPTIONS->netmask, &net) == 0)
      ui_error("Invalid netmask %s", GBL_OPTIONS->netmask);
            
   /* if no netmask was specified, free it */
   if (!strcmp(GBL_OPTIONS->netmask, ""))
      SAFE_FREE(GBL_OPTIONS->netmask);
            
}

/* EOF */

// vim:ts=3:expandtab

