/*
    ettercap -- parsing utilities

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

    $Id: ec_parser.c,v 1.65 2004/07/20 09:53:53 alor Exp $
*/


#include <ec.h>
#include <ec_interfaces.h>
#include <ec_sniff.h>
#include <ec_send.h>
#include <ec_log.h>
#include <ec_format.h>
#include <ec_update.h>
#include <ec_mitm.h>
#include <ec_filter.h>
#include <ec_plugins.h>
#include <ec_conf.h>

#include <ctype.h>

#ifdef HAVE_GETOPT_H
   #include <getopt.h>
#else
   #include <missing/getopt.h>
#endif

/* protos... */

static void ec_usage(void);
void parse_options(int argc, char **argv);

int expand_token(char *s, u_int max, void (*func)(void *t, u_int n), void *t );
int set_regex(char *regex);

/* from the ec_wifi.c decoder */
extern int set_wep_key(u_char *string);

/*****************************************/

void ec_usage(void)
{

   fprintf(stdout, "\nUsage: %s [OPTIONS] [TARGET1] [TARGET2]\n", GBL_PROGRAM);

   fprintf(stdout, "\nTARGET is in the format MAC/IPs/PORTs (see the man for further detail)\n");
   
   fprintf(stdout, "\nSniffing and Attack options:\n");
   fprintf(stdout, "  -M, --mitm <METHOD:ARGS>    perform a mitm attack\n");
   fprintf(stdout, "  -o, --only-mitm             don't sniff, only perform the mitm attack\n");
   fprintf(stdout, "  -B, --bridge <IFACE>        use bridged sniff (needs 2 ifaces)\n");
   fprintf(stdout, "  -p, --nopromisc             do not put the iface in promisc mode\n");
   fprintf(stdout, "  -u, --unoffensive           do not forward packets\n");
   fprintf(stdout, "  -r, --read <file>           read data from pcapfile <file>\n");
   fprintf(stdout, "  -f, --pcapfilter <string>   set the pcap filter <string>\n");
   fprintf(stdout, "  -R, --reversed              use reversed TARGET matching\n");
   fprintf(stdout, "  -t, --proto <proto>         sniff only this proto (default is all)\n");
   
   fprintf(stdout, "\nUser Interface Type:\n");
   fprintf(stdout, "  -T, --text                  use text only GUI\n");
   fprintf(stdout, "       -q, --quiet                 do not display packet contents\n");
   fprintf(stdout, "       -s, --script <CMD>          issue these commands to the GUI\n");
   fprintf(stdout, "  -C, --curses                use curses GUI\n");
   fprintf(stdout, "  -G, --gtk                   use GTK+ GUI\n");
   fprintf(stdout, "  -D, --daemon                daemonize ettercap (no GUI)\n");
   
   fprintf(stdout, "\nLogging options:\n");
   fprintf(stdout, "  -w, --write <file>          write sniffed data to pcapfile <file>\n");
   fprintf(stdout, "  -L, --log <logfile>         log all the traffic to this <logfile>\n");
   fprintf(stdout, "  -l, --log-info <logfile>    log only passive infos to this <logfile>\n");
   fprintf(stdout, "  -m, --log-msg <logfile>     log all the messages to this <logfile>\n");
   fprintf(stdout, "  -c, --compress              use gzip compression on log files\n");
   
   fprintf(stdout, "\nVisualization options:\n");
   fprintf(stdout, "  -d, --dns                   resolves ip addresses into hostnames\n");
   fprintf(stdout, "  -V, --visual <format>       set the visualization format\n");
   fprintf(stdout, "  -e, --regex <regex>         visualize only packets matching this regex\n");
   fprintf(stdout, "  -E, --ext-headers           print extended header for every pck\n");
   fprintf(stdout, "  -Q, --superquiet            do not display user and password\n");
   
   fprintf(stdout, "\nGeneral options:\n");
   fprintf(stdout, "  -i, --iface <iface>         use this network interface\n");
   fprintf(stdout, "  -I, --iflist                show all the network interfaces\n");
   fprintf(stdout, "  -n, --netmask <netmask>     force this <netmask> on iface\n");
   fprintf(stdout, "  -P, --plugin <plugin>       launch this <plugin>\n");
   fprintf(stdout, "  -F, --filter <file>         load the filter <file> (content filter)\n");
   fprintf(stdout, "  -z, --silent                do not perform the initial ARP scan\n");
   fprintf(stdout, "  -j, --load-hosts <file>     load the hosts list from <file>\n");
   fprintf(stdout, "  -k, --save-hosts <file>     save the hosts list to <file>\n");
   fprintf(stdout, "  -W, --wep-key <wkey>        use this wep key to decrypt wifi packets\n");
   fprintf(stdout, "  -a, --config <config>       use the alterative config file <config>\n");
   
   fprintf(stdout, "\nStandard options:\n");
   fprintf(stdout, "  -U, --update                updates the databases from ettercap website\n");
   fprintf(stdout, "  -v, --version               prints the version and exit\n");
   fprintf(stdout, "  -h, --help                  this help screen\n");

   fprintf(stdout, "\n\n");

   clean_exit(0);
}


void parse_options(int argc, char **argv)
{
   int c;

   static struct option long_options[] = {
      { "help", no_argument, NULL, 'h' },
      { "version", no_argument, NULL, 'v' },
      { "update", no_argument, NULL, 'U' },
      
      { "iface", required_argument, NULL, 'i' },
      { "iflist", no_argument, NULL, 'I' },
      { "netmask", required_argument, NULL, 'n' },
      { "write", required_argument, NULL, 'w' },
      { "read", required_argument, NULL, 'r' },
      { "pcapfilter", required_argument, NULL, 'f' },
      
      { "reversed", no_argument, NULL, 'R' },
      { "proto", required_argument, NULL, 't' },
      
      { "plugin", required_argument, NULL, 'P' },
      
      { "filter", required_argument, NULL, 'F' },
      
      { "superquiet", no_argument, NULL, 'Q' },
      { "quiet", no_argument, NULL, 'q' },
      { "script", required_argument, NULL, 's' },
      { "silent", no_argument, NULL, 'z' },
      { "unoffensive", no_argument, NULL, 'u' },
      { "load-hosts", required_argument, NULL, 'j' },
      { "save-hosts", required_argument, NULL, 'k' },
      { "wep-key", required_argument, NULL, 'W' },
      { "config", required_argument, NULL, 'a' },
      
      { "dns", no_argument, NULL, 'd' },
      { "regex", required_argument, NULL, 'e' },
      { "visual", required_argument, NULL, 'V' },
      { "ext-headers", no_argument, NULL, 'E' },
      
      { "log", required_argument, NULL, 'L' },
      { "log-info", required_argument, NULL, 'l' },
      { "log-msg", required_argument, NULL, 'm' },
      { "compress", no_argument, NULL, 'c' },
      
      { "text", no_argument, NULL, 'T' },
      { "curses", no_argument, NULL, 'C' },
      { "gtk", no_argument, NULL, 'G' },
      { "daemon", no_argument, NULL, 'D' },
      
      { "mitm", required_argument, NULL, 'M' },
      { "only-mitm", no_argument, NULL, 'o' },
      { "bridge", required_argument, NULL, 'B' },
      { "promisc", no_argument, NULL, 'p' },
      
      { 0 , 0 , 0 , 0}
   };

   for (c = 0; c < argc; c++)
      DEBUG_MSG("parse_options -- [%d] [%s]", c, argv[c]);

   
/* OPTIONS INITIALIZATION */
   
   GBL_PCAP->promisc = 1;
   GBL_FORMAT = &ascii_format;

/* OPTIONS INITIALIZED */
   
   optind = 0;

   while ((c = getopt_long (argc, argv, "a:B:CchDdEe:F:f:GhIi:j:k:L:l:M:m:n:oP:pQqiRr:s:Tt:UuV:vW:w:z", long_options, (int *)0)) != EOF) {

      switch (c) {

         case 'M':
                  GBL_OPTIONS->mitm = 1;
                  if (mitm_set(optarg) != ESUCCESS)
                     FATAL_ERROR("MITM method '%s' not supported...\n", optarg);
                  break;
                  
         case 'o':
                  GBL_OPTIONS->only_mitm = 1;
                  select_text_interface();
                  break;
                  
         case 'B':
                  GBL_OPTIONS->iface_bridge = strdup(optarg);
                  set_bridge_sniff();
                  break;
                  
         case 'p':
                  GBL_PCAP->promisc = 0;
                  break;
                 
         case 'T':
                  select_text_interface();
                  break;
                  
         case 'C':
                  select_curses_interface();
                  break;
                  
         case 'G':
                  select_gtk_interface();
                  break;
         
         case 'D':
                  select_daemon_interface();
                  break;
                  
         case 'R':
                  GBL_OPTIONS->reversed = 1;
                  break;
                  
         case 't':
                  GBL_OPTIONS->proto = strdup(optarg);
                  break;
                  
         case 'P':
                  /* user has requested the list */
                  if (!strcasecmp(optarg, "list")) {
                     plugin_list();
                     clean_exit(0);
                  }
                  /* else set the plugin */
                  GBL_OPTIONS->plugin = strdup(optarg);
                  break;
                  
         case 'i':
                  GBL_OPTIONS->iface = strdup(optarg);
                  break;
                  
         case 'I':
                  /* this option is only useful in the text interface */
                  select_text_interface();
                  GBL_OPTIONS->iflist = 1;
                  break;
         
         case 'n':
                  GBL_OPTIONS->netmask = strdup(optarg);
                  break;
                  
         case 'r':
                  /* we don't want to scan the lan while reading from file */
                  GBL_OPTIONS->silent = 1;
                  GBL_OPTIONS->read = 1;
                  GBL_OPTIONS->pcapfile_in = strdup(optarg);
                  break;
                 
         case 'w':
                  GBL_OPTIONS->write = 1;
                  GBL_OPTIONS->pcapfile_out = strdup(optarg);
                  break;
                  
         case 'f':
                  GBL_PCAP->filter = strdup(optarg);
                  break;
                  
         case 'F':
                  if (filter_load_file(optarg, GBL_FILTERS) != ESUCCESS)
                     FATAL_ERROR("Cannot load filter file \"%s\"", optarg);
                  break;
                  
         case 'L':
                  if (set_loglevel(LOG_PACKET, optarg) == -EFATAL)
                     clean_exit(-EFATAL);
                  break;

         case 'l':
                  if (set_loglevel(LOG_INFO, optarg) == -EFATAL)
                     clean_exit(-EFATAL);
                  break;

         case 'm':
                  if (set_msg_loglevel(LOG_TRUE, optarg) == -EFATAL)
                     clean_exit(-EFATAL);
                  break;
                  
         case 'c':
                  GBL_OPTIONS->compress = 1;
                  break;

         case 'e':
                  if (set_regex(optarg) == -EFATAL)
                     clean_exit(-EFATAL);
                  break;
         
         case 'Q':
                  GBL_OPTIONS->superquiet = 1;
                  /* no break, quiet must be enabled */
         case 'q':
                  GBL_OPTIONS->quiet = 1;
                  break;
                  
         case 's':
                  GBL_OPTIONS->script = strdup(optarg);
                  break;
                  
         case 'z':
                  GBL_OPTIONS->silent = 1;
                  break;
                  
         case 'u':
                  GBL_OPTIONS->unoffensive = 1;
                  break;
                  
         case 'd':
                  GBL_OPTIONS->resolve = 1;
                  break;
                  
         case 'j':
                  GBL_OPTIONS->silent = 1;
                  GBL_OPTIONS->load_hosts = 1;
                  GBL_OPTIONS->hostsfile = strdup(optarg);
                  break;
                  
         case 'k':
                  GBL_OPTIONS->save_hosts = 1;
                  GBL_OPTIONS->hostsfile = strdup(optarg);
                  break;
                  
         case 'V':
                  if (set_format(optarg) != ESUCCESS)
                     clean_exit(-EFATAL);
                  break;
                  
         case 'E':
                  GBL_OPTIONS->ext_headers = 1;
                  break;
                  
         case 'W':
                  set_wep_key(optarg);
                  break;
                  
         case 'a':
                  GBL_CONF->file = strdup(optarg);
                  break;
         
         case 'U':
                  /* load the conf for the connect timeout value */
                  load_conf();
                  global_update();
                  /* NOT REACHED */
                  break;
                  
         case 'h':
                  ec_usage();
                  break;

         case 'v':
                  printf("%s %s\n", GBL_PROGRAM, GBL_VERSION);
                  clean_exit(0);
                  break;

         case ':': // missing parameter
            fprintf(stdout, "\nTry `%s --help' for more options.\n\n", GBL_PROGRAM);
            clean_exit(-1);
         break;

         case '?': // unknown option
            fprintf(stdout, "\nTry `%s --help' for more options.\n\n", GBL_PROGRAM);
            clean_exit(-1);
         break;
      }
   }

   DEBUG_MSG("parse_options: options parsed");
   
   /* TARGET1 and TARGET2 parsing */
   if (argv[optind]) {
      GBL_OPTIONS->target1 = strdup(argv[optind]);
      DEBUG_MSG("TARGET1: %s", GBL_OPTIONS->target1);
      
      if (argv[optind+1]) {
         GBL_OPTIONS->target2 = strdup(argv[optind+1]);
         DEBUG_MSG("TARGET2: %s", GBL_OPTIONS->target2);
      }
   }

   /* create the list form the TARGET format (MAC/IPrange/PORTrange) */
   compile_display_filter();
   
   DEBUG_MSG("parse_options: targets parsed");
   
   /* check for other options */
   
   if (GBL_SNIFF->start == NULL)
      set_unified_sniff();
   
   if (GBL_OPTIONS->read && GBL_PCAP->filter)
      FATAL_ERROR("Cannot read from file and set a filter on interface");
   
   if (GBL_OPTIONS->read && GBL_SNIFF->type != SM_UNIFIED )
      FATAL_ERROR("You can read from a file ONLY in unified sniffing mode !");
   
   if (GBL_OPTIONS->mitm && GBL_SNIFF->type != SM_UNIFIED )
      FATAL_ERROR("You can't do mitm attacks in bridged sniffing mode !");

   if (GBL_SNIFF->type == SM_BRIDGED && GBL_PCAP->promisc == 0)
      FATAL_ERROR("During bridged sniffing the iface must be in promisc mode !");
   
   if (GBL_OPTIONS->quiet && GBL_UI->type != UI_TEXT)
      FATAL_ERROR("The quiet option is useful only with text only UI");
  
   if (GBL_OPTIONS->load_hosts && GBL_OPTIONS->save_hosts)
      FATAL_ERROR("Cannot load and save at the same time the hosts list...");
  
   if (GBL_OPTIONS->unoffensive && GBL_OPTIONS->mitm)
      FATAL_ERROR("Cannot use mitm attacks in unoffensive mode");
   
   if (GBL_OPTIONS->read && GBL_OPTIONS->mitm)
      FATAL_ERROR("Cannot use mitm attacks while reading from file");
   
   if (GBL_UI->init == NULL)
      FATAL_ERROR("Please select an User Interface");
     
   /* force text interface for only mitm attack */
   if (GBL_OPTIONS->only_mitm) {
      if (GBL_OPTIONS->mitm)
         select_text_interface();
      else
         FATAL_ERROR("Only mitm requires at least one mitm method");
   }

   DEBUG_MSG("parse_options: options combination looks good");
   
   return;
}


/*
 * This function parses the input in the form [1-3,17,5-11]
 * and fill the structure with expanded numbers.
 */

int expand_token(char *s, u_int max, void (*func)(void *t, u_int n), void *t )
{
   char *str = strdup(s);
   char *p, *q, r;
   char *end;
   u_int a = 0, b = 0;
   
   DEBUG_MSG("expand_token %s", s);
   
   p = str;
   end = p + strlen(p);
   
   while (p < end) {
      q = p;
      
      /* find the end of the first digit */
      while ( isdigit((int)*q) && q++ < end);
      
      r = *q;   
      *q = 0;
      /* get the first digit */
      a = atoi(p);
      if (a > max) 
         FATAL_MSG("Out of range (%d) !!", max);
      
      /* it is a range ? */
      if ( r == '-') {
         p = ++q;
         /* find the end of the range */
         while ( isdigit((int)*q) && q++ < end);
         *q = 0;
         if (*p == '\0') 
            FATAL_MSG("Invalid range !!");
         /* get the second digit */
         b = atoi(p);
         if (b > max) 
            FATAL_MSG("Out of range (%d)!!", max);
         if (b < a)
            FATAL_MSG("Invalid decrementing range !!");
      } else {
         /* it is not a range */
         b = a; 
      } 
      
      /* process the range and invoke the callback */
      for(; a <= b; a++) {
         func(t, a);
      }
      
      if (q == end) break;
      else  p = q + 1;      
   }
  
   SAFE_FREE(str);
   
   return ESUCCESS;
}

/*
 * compile the regex
 */

int set_regex(char *regex)
{
   int err;
   char errbuf[100];
   
   DEBUG_MSG("set_regex: %s", regex);

   /* free any previous compilation */
   if (GBL_OPTIONS->regex)
      regfree(GBL_OPTIONS->regex);

   /* unset the regex if empty */
   if (!strcmp(regex, "")) {
      SAFE_FREE(GBL_OPTIONS->regex);
      return ESUCCESS;
   }
  
   /* allocate the new structure */
   SAFE_CALLOC(GBL_OPTIONS->regex, 1, sizeof(regex_t));
  
   /* compile the regex */
   err = regcomp(GBL_OPTIONS->regex, regex, REG_EXTENDED | REG_NOSUB | REG_ICASE );

   if (err) {
      regerror(err, GBL_OPTIONS->regex, errbuf, sizeof(errbuf));
      FATAL_MSG("%s\n", errbuf);
   }

   return ESUCCESS;
}



/* EOF */


// vim:ts=3:expandtab

