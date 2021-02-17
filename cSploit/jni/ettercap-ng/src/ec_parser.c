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

*/


#include <ec.h>
#include <ec_interfaces.h>
#include <ec_sniff.h>
#include <ec_send.h>
#include <ec_log.h>
#include <ec_format.h>
#include <ec_mitm.h>
#include <ec_filter.h>
#include <ec_plugins.h>
#include <ec_conf.h>
#include <ec_strings.h>
#include <ec_encryption.h>
#ifdef HAVE_EC_LUA
#include <ec_lua.h>
#endif

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
static char **parse_iflist(char *list);

/* set functions */
void set_mitm(char *mitm);
void set_onlymitm(void);
void set_broadcast(void);
void set_iface_bridge(char *iface);
void set_promisc(void);
void set_reversed(void);
void set_proto(char *arg);
void set_plugin(char *plugin);
void set_iface(char *iface);
void set_lifaces(void);
void set_secondary(char *iface);
void set_netmask(char *netmask);
void set_address(char *address);
void set_read_pcap(char *pcap_file);
void set_write_pcap(char *pcap_file);
void set_pcap_filter(char *filter);
void load_filter(char *end, char *filter);
void set_loglevel_packet(char *arg);
void set_loglevel_info(char *arg);
void set_loglevel_true(char *arg);
void set_compress(void);
void opt_set_regex(char *regex);
void set_quiet(void);
void set_superquiet(void);
void set_script(char *script);
void set_silent(void);
void set_unoffensive(void);
void disable_sslmitm(void);
void set_resolve(void);
void load_hosts(char *file);
void save_hosts(char *file);
void opt_set_format(char *format);
void set_ext_headers(void);
void set_wifi_key(char *key);
void set_conf_file(char *file);
void set_ssl_cert(char *cert);
void set_ssl_key(char *key);
#ifdef HAVE_EC_LUA
void set_lua_args(char *args);
void set_lua_script(char *script);
#endif
void set_target_target1(char *target1);
void set_target_target2(char *target2);


/*****************************************/

void ec_usage(void)
{

   fprintf(stdout, "\nUsage: %s [OPTIONS] [TARGET1] [TARGET2]\n", GBL_PROGRAM);

#ifdef WITH_IPV6
   fprintf(stdout, "\nTARGET is in the format MAC/IP/IPv6/PORTs (see the man for further detail)\n");
#else
   fprintf(stdout, "\nTARGET is in the format MAC/IP/PORTs (see the man for further detail)\n");
#endif

   
   fprintf(stdout, "\nSniffing and Attack options:\n");
   fprintf(stdout, "  -M, --mitm <METHOD:ARGS>    perform a mitm attack\n");
   fprintf(stdout, "  -o, --only-mitm             don't sniff, only perform the mitm attack\n");
   fprintf(stdout, "  -b, --broadcast             sniff packets destined to broadcast\n");
   fprintf(stdout, "  -B, --bridge <IFACE>        use bridged sniff (needs 2 ifaces)\n");
   fprintf(stdout, "  -p, --nopromisc             do not put the iface in promisc mode\n");
   fprintf(stdout, "  -S, --nosslmitm             do not forge SSL certificates\n");
   fprintf(stdout, "  -u, --unoffensive           do not forward packets\n");
   fprintf(stdout, "  -r, --read <file>           read data from pcapfile <file>\n");
   fprintf(stdout, "  -f, --pcapfilter <string>   set the pcap filter <string>\n");
   fprintf(stdout, "  -R, --reversed              use reversed TARGET matching\n");
   fprintf(stdout, "  -t, --proto <proto>         sniff only this proto (default is all)\n");
   fprintf(stdout, "      --certificate <file>    certificate file to use for SSL MiTM\n");
   fprintf(stdout, "      --private-key <file>    private key file to use for SSL MiTM\n");
   
   fprintf(stdout, "\nUser Interface Type:\n");
   fprintf(stdout, "  -T, --text                  use text only GUI\n");
   fprintf(stdout, "       -q, --quiet                 do not display packet contents\n");
   fprintf(stdout, "       -s, --script <CMD>          issue these commands to the GUI\n");
   fprintf(stdout, "  -C, --curses                use curses GUI\n");
   fprintf(stdout, "  -D, --daemon                daemonize ettercap (no GUI)\n");
   fprintf(stdout, "  -G, --gtk                   use GTK+ GUI\n");

   
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

#ifdef HAVE_EC_LUA
   fprintf(stdout, "\nLUA options:\n");
   fprintf(stdout, "      --lua-script <script1>,[<script2>,...]     comma-separted list of LUA scripts\n");
   fprintf(stdout, "      --lua-args n1=v1,[n2=v2,...]               comma-separated arguments to LUA script(s)\n");
#endif
   
   fprintf(stdout, "\nGeneral options:\n");
   fprintf(stdout, "  -i, --iface <iface>         use this network interface\n");
   fprintf(stdout, "  -I, --liface                show all the network interfaces\n");
   fprintf(stdout, "  -Y, --secondary <ifaces>    list of secondary network interfaces\n");
   fprintf(stdout, "  -n, --netmask <netmask>     force this <netmask> on iface\n");
   fprintf(stdout, "  -A, --address <address>     force this local <address> on iface\n");
   fprintf(stdout, "  -P, --plugin <plugin>       launch this <plugin>\n");
   fprintf(stdout, "  -F, --filter <file>         load the filter <file> (content filter)\n");
   fprintf(stdout, "  -z, --silent                do not perform the initial ARP scan\n");
   fprintf(stdout, "  -j, --load-hosts <file>     load the hosts list from <file>\n");
   fprintf(stdout, "  -k, --save-hosts <file>     save the hosts list to <file>\n");
   fprintf(stdout, "  -W, --wifi-key <wkey>       use this key to decrypt wifi packets (wep or wpa)\n");
   fprintf(stdout, "  -a, --config <config>       use the alterative config file <config>\n");
   
   fprintf(stdout, "\nStandard options:\n");
   fprintf(stdout, "  -v, --version               prints the version and exit\n");
   fprintf(stdout, "  -h, --help                  this help screen\n");

   fprintf(stdout, "\n\n");

   //clean_exit(0);
   exit(0);
}


void parse_options(int argc, char **argv)
{
   int c;

   static struct option long_options[] = {
      { "help", no_argument, NULL, 'h' },
      { "version", no_argument, NULL, 'v' },
      
      { "iface", required_argument, NULL, 'i' },
      { "lifaces", no_argument, NULL, 'I' },
      { "netmask", required_argument, NULL, 'n' },
      { "address", required_argument, NULL, 'A' },
      { "write", required_argument, NULL, 'w' },
      { "read", required_argument, NULL, 'r' },
      { "pcapfilter", required_argument, NULL, 'f' },
      
      { "reversed", no_argument, NULL, 'R' },
      { "proto", required_argument, NULL, 't' },
      
      { "plugin", required_argument, NULL, 'P' },
      
      { "filter", required_argument, NULL, 'F' },
#ifdef HAVE_EC_LUA
      { "lua-script", required_argument, NULL, 0 },
      { "lua-args", required_argument, NULL, 0 },
#endif
      
      { "superquiet", no_argument, NULL, 'Q' },
      { "quiet", no_argument, NULL, 'q' },
      { "script", required_argument, NULL, 's' },
      { "silent", no_argument, NULL, 'z' },
      { "unoffensive", no_argument, NULL, 'u' },
      { "nosslmitm", no_argument, NULL, 'S' },
      { "load-hosts", required_argument, NULL, 'j' },
      { "save-hosts", required_argument, NULL, 'k' },
      { "wifi-key", required_argument, NULL, 'W' },
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
      { "daemon", no_argument, NULL, 'D' },
      { "gtk", no_argument, NULL, 'G' },

      
      { "mitm", required_argument, NULL, 'M' },
      { "only-mitm", no_argument, NULL, 'o' },
      { "bridge", required_argument, NULL, 'B' },
      { "broadcast", required_argument, NULL, 'b' },
      { "promisc", no_argument, NULL, 'p' },
      { "gateway", required_argument, NULL, 'Y' },
      { "certificate", required_argument, NULL, 0 },
      { "private-key", required_argument, NULL, 0 },

      
      { 0 , 0 , 0 , 0}
   };

   for (c = 0; c < argc; c++)
      DEBUG_MSG("parse_options -- [%d] [%s]", c, argv[c]);

   
/* OPTIONS INITIALIZATION */
   
   GBL_PCAP->promisc = 1;
   GBL_FORMAT = &ascii_format;
   GBL_OPTIONS->ssl_mitm = 1;
   GBL_OPTIONS->broadcast = 0;
   GBL_OPTIONS->ssl_cert = NULL;
   GBL_OPTIONS->ssl_pkey = NULL;

/* OPTIONS INITIALIZED */
   
   optind = 0;
   int option_index = 0;

   while ((c = getopt_long (argc, argv, "A:a:bB:CchDdEe:F:f:GhIi:j:k:L:l:M:m:n:oP:pQqiRr:s:STt:uV:vW:w:Y:z", long_options, &option_index)) != EOF) {
      /* used for parsing arguments */
      char *opt_end = optarg;
      while (opt_end && *opt_end) opt_end++;
      /* enable a loaded filter script? */

      switch (c) {

         case 'M':
		  set_mitm(optarg);
                  break;
                  
         case 'o':
		  set_onlymitm();
                  //select_text_interface();
                  break;

         case 'b':
		  set_broadcast();
		  break;
                  
         case 'B':
		  set_iface_bridge(optarg);
                  break;
                  
         case 'p':
		  set_promisc();
                  break;
#ifndef JUST_LIBRARY 
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
#endif
                  
         case 'R':
		  set_reversed();
                  break;
                  
         case 't':
		  set_proto(optarg);
                  break;
                  
         case 'P':
		  set_plugin(optarg);
                  break;
                  
         case 'i':
		  set_iface(optarg);
                  break;
                  
         case 'I':
                  /* this option is only useful in the text interface */
	          set_lifaces();
                  break;

         case 'Y':
                  set_secondary(optarg);
                  break;
         
         case 'n':
                  set_netmask(optarg);
                  break;

         case 'A':
                  set_address(optarg);
                  break;
                  
         case 'r':
                  set_read_pcap(optarg);
                  break;
                 
         case 'w':
		  set_write_pcap(optarg);
                  break;
                  
         case 'f':
		  set_pcap_filter(optarg);
                  break;
                  
         case 'F':
		  load_filter(opt_end, optarg);
                  break;
                  
         case 'L':
		  set_loglevel_packet(optarg);

         case 'l':
		  set_loglevel_info(optarg);
                  break;

         case 'm':
	          set_loglevel_true(optarg);
                  break;
                  
         case 'c':
		  set_compress();
                  break;

         case 'e':
                  opt_set_regex(optarg);
                  break;
         
         case 'Q':
                  set_superquiet();
                  /* no break, quiet must be enabled */
         case 'q':
		  set_quiet();
                  break;
                  
         case 's':
                  set_script(optarg);
                  break;
                  
         case 'z':
                  set_silent();
                  break;
                  
         case 'u':
                  set_unoffensive();
                  break;

         case 'S':
                  disable_sslmitm();
                  break;
 
         case 'd':
                  set_resolve();
                  break;
                  
         case 'j':
                  load_hosts(optarg);
                  break;
                  
         case 'k':
	          save_hosts(optarg);
                  break;
                  
         case 'V':
                  opt_set_format(optarg);
                  break;
                  
         case 'E':
                  set_ext_headers();
                  break;
                  
         case 'W':
                  set_wifi_key(optarg);
                  break;
                  
         case 'a':
                  set_conf_file(optarg);
                  break;
         
         case 'h':
                  ec_usage();
                  break;

         case 'v':
                  printf("%s %s\n", GBL_PROGRAM, GBL_VERSION);
                  clean_exit(0);
                  break;

        /* Certificate and private key options */
         case 0:
		if (!strcmp(long_options[option_index].name, "certificate")) {
			GBL_OPTIONS->ssl_cert = strdup(optarg);	
		} else if (!strcmp(long_options[option_index].name, "private-key")) {
			GBL_OPTIONS->ssl_pkey = strdup(optarg);
#ifdef HAVE_EC_LUA
                } else if (!strcmp(long_options[option_index].name,"lua-args")) {
                    ec_lua_cli_add_args(strdup(optarg));
                } 
                else if (!strcmp(long_options[option_index].name,"lua-script")) {
                    ec_lua_cli_add_script(strdup(optarg));
        break;
#endif
		} else {
			fprintf(stdout, "\nTry `%s --help' for more options.\n\n", GBL_PROGRAM);
			clean_exit(-1);
		}

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
  
#ifndef JUST_LIBRARY 
   if (GBL_UI->init == NULL)
      FATAL_ERROR("Please select an User Interface");
#endif
     
   /* force text interface for only mitm attack */
  /* Do not select text interface for only MiTM mode 

   if (GBL_OPTIONS->only_mitm) {
      if (GBL_OPTIONS->mitm)
         select_text_interface();
      else
         FATAL_ERROR("Only mitm requires at least one mitm method");
   } */

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

static char **parse_iflist(char *list)
{
   int i, n;
   char **r, *t, *p;

   for(i = 0, n = 1; list[i] != '\0'; list[i++] == ',' ? n++ : n);
   SAFE_CALLOC(r, n + 1, sizeof(char*));

   /* its self-explaining */
   for(r[i=0]=ec_strtok(list,",",&p);i<n&&(t=ec_strtok(NULL,",",&p))!=NULL;r[++i]=strdup(t));
   r[n] = NULL;

   return r;
}

/* set functions */
void set_mitm(char *mitm) 
{
	GBL_OPTIONS->mitm = 1;
	if(mitm_set(mitm) != ESUCCESS)
		FATAL_ERROR("MiTM method '%s' not supported...\n", mitm);
}

void set_onlymitm(void)
{
	GBL_OPTIONS->only_mitm = 1;
}

void set_broadcast(void)
{
	GBL_OPTIONS->broadcast = 1;
}

void set_iface_bridge(char *iface)
{
	GBL_OPTIONS->iface_bridge = strdup(iface);
	set_bridge_sniff();
}

void set_promisc(void)
{
	GBL_PCAP->promisc = 0;
}

void set_reversed(void)
{
	GBL_OPTIONS->reversed = 1;
}

void set_plugin(char *plugin)
{
	if(!strcasecmp(plugin, "list")) {
		plugin_list();
		clean_exit(0);
	}

	GBL_OPTIONS->plugin = strdup(plugin);
}

void set_proto(char *proto)
{
	GBL_OPTIONS->proto = strdup(proto);
}

void set_iface(char *iface)
{
	GBL_OPTIONS->iface = strdup(iface);
}

void set_lifaces(void)
{
#ifndef JUST_LIBRARY
	GBL_OPTIONS->lifaces = 1;
	select_text_interface();
#endif
}

void set_secondary(char *iface)
{
	GBL_OPTIONS->secondary = parse_iflist(iface);
}

void set_netmask(char *netmask)
{
	GBL_OPTIONS->netmask = strdup(netmask);
}

void set_address(char *address)
{
	GBL_OPTIONS->address = strdup(address);
}

void set_read_pcap(char *pcap_file)
{
	/* we don't want to scan th eLAN while reading from file */
	GBL_OPTIONS->silent = 1;
	GBL_OPTIONS->read = 1;
	GBL_OPTIONS->pcapfile_in = strdup(pcap_file);
}

void set_write_pcap(char *pcap_file)
{
	GBL_OPTIONS->write = 1;
	GBL_OPTIONS->pcapfile_out = strdup(pcap_file);
}

void set_pcap_filter(char *filter)
{
	GBL_PCAP->filter = strdup(filter);
}

void load_filter(char *end, char *filter)
{
	uint8_t f_enabled = 1;
	if ( (end-filter >=2) && *(end-2) == ':') {
		*(end-2) = '\0';
		f_enabled = !( *(end-1) == '0' );
	}	
	
	if (filter_load_file(filter, GBL_FILTERS, f_enabled) != ESUCCESS)
		FATAL_ERROR("Cannot load filter file \"%s\"", filter);
}


void set_loglevel_packet(char *arg)
{
	if (set_loglevel(LOG_PACKET, arg) == -EFATAL)
		clean_exit(-EFATAL);
}

void set_loglevel_info(char *arg)
{
	if (set_loglevel(LOG_INFO, arg) == -EFATAL)
		clean_exit(-EFATAL);
}

void set_loglevel_true(char *arg)
{
	if (set_msg_loglevel(LOG_TRUE, arg) == -EFATAL)
		clean_exit(-EFATAL);
}

void set_compress(void)
{
	GBL_OPTIONS->compress = 1;
}

void opt_set_regex(char *regex)
{
	if (set_regex(regex) == -EFATAL)
		clean_exit(-EFATAL);
}

void set_superquiet()
{
	GBL_OPTIONS->superquiet = 1;
}

void set_quiet(void)
{
	GBL_OPTIONS->quiet = 1;
}

void set_script(char *script)
{
	GBL_OPTIONS->script = strdup(script);
}

void set_silent(void)
{
	GBL_OPTIONS->silent = 1;
}

void set_unoffensive(void)
{
	GBL_OPTIONS->unoffensive = 1;
}

void disable_sslmitm(void)
{
	GBL_OPTIONS->ssl_mitm = 0;
}

void set_resolve(void)
{
	GBL_OPTIONS->resolve = 1;
}

void load_hosts(char *file)
{
	GBL_OPTIONS->silent = 1;
	GBL_OPTIONS->load_hosts = 1;
	GBL_OPTIONS->hostsfile = strdup(file);
}

void save_hosts(char *file)
{
	GBL_OPTIONS->save_hosts = 1;
	GBL_OPTIONS->hostsfile = strdup(file);
}

void opt_set_format(char *format)
{
	if (set_format(format) != ESUCCESS)
		clean_exit(-EFATAL);	
}

void set_ext_headers(void)
{
	GBL_OPTIONS->ext_headers = 1;
}

void set_wifi_key(char *key)
{
	wifi_key_prepare(key);
}

void set_conf_file(char *file)
{
	GBL_CONF->file = strdup(file);
}

void set_ssl_cert(char *cert)
{
	GBL_OPTIONS->ssl_cert = strdup(cert);
}

void set_ssl_key(char *key)
{
	GBL_OPTIONS->ssl_pkey = strdup(key);
}

#ifdef HAVE_EC_LUA
void set_lua_args(char *args)
{
	ec_lua_cli_add_args(strdup(args));
}

void set_lua_script(char *script)
{
	ec_lua_cli_add_script(strdup(script));
}
#endif

void set_target_target1(char *target1)
{
	GBL_OPTIONS->target1 = strdup(target1);
}

void set_target_target2(char *target2)
{
	GBL_OPTIONS->target2 = strdup(target2);
}

/* EOF */


// vim:ts=3:expandtab

