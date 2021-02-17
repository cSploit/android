/*
    etterlog -- parsing utilities

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

    $Id: el_parser.c,v 1.25 2004/11/04 10:37:16 alor Exp $
*/


#include <el.h>
#include <ec_version.h>
#include <ec_format.h>
#include <el_functions.h>

#include <ctype.h>

#ifdef HAVE_GETOPT_H
   #include <getopt.h>
#else
   #include <missing/getopt.h>
#endif

/* protos... */

static void el_usage(void);
void parse_options(int argc, char **argv);

void expand_token(char *s, u_int max, void (*func)(void *t, int n), void *t );

/*******************************************/

void el_usage(void)
{

   fprintf(stdout, "\nUsage: %s [OPTIONS] logfile\n", GBL_PROGRAM);

   fprintf(stdout, "\nGeneral Options:\n");
   fprintf(stdout, "  -a, --analyze               analyze a log file and return useful infos\n");
   fprintf(stdout, "  -c, --connections           display the table of connections\n");
   fprintf(stdout, "  -f, --filter <TARGET>       print packets only from this target\n");
   fprintf(stdout, "  -t, --proto <proto>         display only this proto (default is all)\n");
   fprintf(stdout, "  -F, --filcon <CONN>         print packets only from this connection \n");
   fprintf(stdout, "      -s, --only-source           print packets only from the source\n");
   fprintf(stdout, "      -d, --only-dest             print packets only from the destination\n");
   fprintf(stdout, "  -r, --reverse               reverse the target/connection matching\n");
   fprintf(stdout, "  -n, --no-headers            skip header informations between packets\n");
   fprintf(stdout, "  -m, --show-mac              show mac addresses in the headers\n");
   fprintf(stdout, "  -k, --color                 colorize the output\n");
   fprintf(stdout, "  -l, --only-local            show only local hosts parsing info files\n");
   fprintf(stdout, "  -L, --only-remote           show only remote hosts parsing info files\n");
   
   fprintf(stdout, "\nSearch Options:\n");
   fprintf(stdout, "  -e, --regex <regex>         display only packets that match the regex\n");
   fprintf(stdout, "  -u, --user <user>           search for info about the user <user>\n");
   fprintf(stdout, "  -p, --passwords             print only accounts information\n");
   fprintf(stdout, "      -i, --show-client       show client address in the password profiles\n");
   fprintf(stdout, "      -I, --client <ip>       search for pass from a specific client\n");
   
   fprintf(stdout, "\nEditing Options:\n");
   fprintf(stdout, "  -C, --concat                concatenate more files into one single file\n");
   fprintf(stdout, "  -o, --outfile <file>        the file used as output for concatenation\n");
   fprintf(stdout, "  -D, --decode                used to extract files from connections\n");
   
   fprintf(stdout, "\nVisualization Method:\n");
   fprintf(stdout, "  -B, --binary                print packets as they are\n");
   fprintf(stdout, "  -X, --hex                   print packets in hex mode\n");
   fprintf(stdout, "  -A, --ascii                 print packets in ascii mode (default)\n");
   fprintf(stdout, "  -T, --text                  print packets in text mode\n");
   fprintf(stdout, "  -E, --ebcdic                print packets in ebcdic mode\n");
   fprintf(stdout, "  -H, --html                  print packets in html mode\n");
   fprintf(stdout, "  -U, --utf8 <encoding>       print packets in uft-8 using the <encoding>\n");
   fprintf(stdout, "  -Z, --zero                  do not print packets, only headers\n");
   fprintf(stdout, "  -x, --xml                   print host infos in xml format\n");
   
   fprintf(stdout, "\nStandard Options:\n");
   fprintf(stdout, "  -v, --version               prints the version and exit\n");
   fprintf(stdout, "  -h, --help                  this help screen\n");

   fprintf(stdout, "\n\n");

   exit(0);
}


void parse_options(int argc, char **argv)
{
   int c;
   struct in_addr ip;

   static struct option long_options[] = {
      { "help", no_argument, NULL, 'h' },
      { "version", no_argument, NULL, 'v' },
      
      { "binary", no_argument, NULL, 'B' },
      { "hex", no_argument, NULL, 'X' },
      { "ascii", no_argument, NULL, 'A' },
      { "text", no_argument, NULL, 'T' },
      { "ebcdic", no_argument, NULL, 'E' },
      { "html", no_argument, NULL, 'H' },
      { "utf8", required_argument, NULL, 'U' },
      { "zero", no_argument, NULL, 'Z' },
      { "xml", no_argument, NULL, 'x' },
      
      { "analyze", no_argument, NULL, 'a' },
      { "connections", no_argument, NULL, 'c' },
      { "filter", required_argument, NULL, 'f' },
      { "filcon", required_argument, NULL, 'F' },
      { "no-headers", no_argument, NULL, 'n' },
      { "only-source", no_argument, NULL, 's' },
      { "only-dest", no_argument, NULL, 'd' },
      { "show-mac", no_argument, NULL, 'm' },
      { "show-client", no_argument, NULL, 'i' },
      { "color", no_argument, NULL, 'k' },
      { "reverse", no_argument, NULL, 'r' },
      { "proto", required_argument, NULL, 't' },
      { "only-local", required_argument, NULL, 'l' },
      { "only-remote", required_argument, NULL, 'L' },
      
      { "outfile", required_argument, NULL, 'o' },
      { "concat", no_argument, NULL, 'C' },
      { "decode", no_argument, NULL, 'D' },
      
      { "user", required_argument, NULL, 'u' },
      { "regex", required_argument, NULL, 'e' },
      { "passwords", no_argument, NULL, 'p' },
      { "client", required_argument, NULL, 'I' },
      
      { 0 , 0 , 0 , 0}
   };

   
   optind = 0;

   while ((c = getopt_long (argc, argv, "AaBCcDdEe:F:f:HhiI:kLlmno:prsTt:U:u:vXxZ", long_options, (int *)0)) != EOF) {

      switch (c) {

         case 'a':
                  GBL.analyze = 1;
                  break;
                  
         case 'c':
                  GBL.connections = 1;
                  break;
                  
         case 'D':
                  GBL.connections = 1;
                  GBL.decode = 1;
                  NOT_IMPLEMENTED();
                  break;
         
         case 'f':
                  target_compile(optarg);
                  break;

         case 'F':
                  filcon_compile(optarg);
                  break;
                  
         case 's':
                  GBL.only_source = 1;
                  break;
                  
         case 'd':
                  GBL.only_dest = 1;
                  break;
                  
         case 'k':
                  GBL.color = 1;
                  break;
                     
         case 'r':
                  GBL.reverse = 1;
                  break;
                  
         case 't':
                  GBL_TARGET->proto = strdup(optarg);
                  break;
                  
         case 'n':
                  GBL.no_headers = 1;
                  break;
                  
         case 'm':
                  GBL.showmac = 1;
                  break;
                  
         case 'i':
                  GBL.showclient = 1;
                  break;
                  
         case 'I':
                  if (inet_aton(optarg, &ip) == 0) {
                     FATAL_ERROR("Invalid client ip address");
                     return;                    
                  }
                  ip_addr_init(&GBL.client, AF_INET, (char *)&ip);
                  break;

         case 'l':
                  GBL.only_local = 1;
                  break;
         
         case 'L':
                  GBL.only_remote = 1;
                  break;
                  
         case 'u':
                  GBL.user = strdup(optarg);
                  break;
                  
         case 'p':
                  GBL.passwords = 1;
                  break;

         case 'e':
                  set_display_regex(optarg);
                  break;
                 
         case 'o':
                  GBL_LOGFILE = strdup(optarg);
                  break;
                  
         case 'C':
                  GBL.concat = 1;
                  break;
                  
         case 'B':
                  GBL.format = &bin_format;
                  break;
                  
         case 'X':
                  GBL.format = &hex_format;
                  break;
                  
         case 'A':
                  GBL.format = &ascii_format;
                  break;
                  
         case 'T':
                  GBL.format = &text_format;
                  break;
                  
         case 'E':
                  GBL.format = &ebcdic_format;
                  break;
                  
         case 'H':
                  GBL.format = &html_format;
                  break;
                  
         case 'U':
                  set_utf8_encoding(optarg);
                  GBL.format = &utf8_format;
                  break;
                  
         case 'Z':
                  GBL.format = &zero_format;
                  break;
                  
         case 'x':
                  GBL.xml = 1;
                  break;
                  
         case 'h':
                  el_usage();
                  break;

         case 'v':
                  printf("%s %s\n", GBL_PROGRAM, EC_VERSION);
                  exit(0);
                  break;

         case ':': // missing parameter
            fprintf(stdout, "\nTry `%s --help' for more options.\n\n", GBL_PROGRAM);
            exit(0);
         break;

         case '?': // unknown option
            fprintf(stdout, "\nTry `%s --help' for more options.\n\n", GBL_PROGRAM);
            exit(0);
         break;
      }
   }

   /* file concatenation */
   if (GBL.concat) {
      if (argv[optind] == NULL)
         FATAL_ERROR("You MUST specify at least one logfile");
   
      /* this function does not return */
      concatenate(optind, argv);
   }

   /* normal file operation */
   if (argv[optind])
      open_log(argv[optind]);
   else
      FATAL_ERROR("You MUST specify a logfile\n");
  
   /* default to ASCII view */ 
   if (GBL.format == NULL)
      GBL.format = &ascii_format;

   return;
}


/*
 * This function parses the input in the form [1-3,17,5-11]
 * and fill the structure with expanded numbers.
 */

void expand_token(char *s, u_int max, void (*func)(void *t, int n), void *t )
{
   char *str = strdup(s);
   char *p, *q, r;
   char *end;
   u_int a = 0, b = 0;
   
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
         FATAL_ERROR("Out of range (%d) !!", max);
      
      /* it is a range ? */
      if ( r == '-') {
         p = ++q;
         /* find the end of the range */
         while ( isdigit((int)*q) && q++ < end);
         *q = 0;
         if (*p == '\0') 
            FATAL_ERROR("Invalid range !!");
         /* get the second digit */
         b = atoi(p);
         if (b > max) 
            FATAL_ERROR("Out of range (%d)!!", max);
         if (b < a)
            FATAL_ERROR("Invalid decrementing range !!");
      } else {
         /* it is not a range */
         b = a; 
      } 
      
      /* process the range */
      for(; a <= b; a++) {
         func(t, a);
      }
      
      if (q == end) break;
      else  p = q + 1;      
   }
  
   SAFE_FREE(str);
}


/* EOF */


// vim:ts=3:expandtab

