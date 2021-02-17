/*
    etterlog -- display packets or infos

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

    $Id: el_display.c,v 1.40 2004/06/25 14:24:30 alor Exp $
*/

#include <el.h>
#include <ec_version.h>
#include <ec_log.h>
#include <ec_format.h>
#include <el_functions.h>
#include <ec_fingerprint.h>
#include <ec_manuf.h>
#include <ec_services.h>
#include <ec_passive.h>

#include <sys/stat.h>
#include <regex.h>

/* proto */

void display(void);
static void display_packet(void);
static void display_info(void);
static void display_headers(struct log_header_packet *pck);
void set_display_regex(char *regex);
static int match_regex(struct host_profile *h);
static void print_pass(struct host_profile *h);

/*******************************************/

void display(void)
{
   switch(GBL.hdr.type) {
      case LOG_PACKET:
         display_packet();
         break;
      case LOG_INFO:
         display_info();
         break;
   }
}


/* display a packet log file */

static void display_packet(void)
{
   struct log_header_packet pck;
   int ret;
   u_char *buf;
   u_char *tmp;
   int versus;
   
   /* read the logfile */
   LOOP {
      ret = get_packet(&pck, &buf);

      /* on error exit the loop */
      if (ret != ESUCCESS)
         break;

      /* the packet should be compliant to the target specifications */
      if (!is_target_pck(&pck)) {
         SAFE_FREE(buf);
         continue;
      }
      
      /* the packet should be compliant to the connection specifications */
      if (!is_conn(&pck, &versus)) {
         SAFE_FREE(buf);
         continue;
      }
     
      /* if the regex does not match, the packet is not interesting */
      if (GBL.regex && regexec(GBL.regex, buf, 0, NULL, 0) != 0) {
         SAFE_FREE(buf);
         continue;
      }
                  
      /* 
       * prepare the buffer,
       * the max length is hex_fomat
       * so use its length for the buffer
       */
      SAFE_CALLOC(tmp, hex_len(pck.len), sizeof(u_char));

      /* display the headers only if necessary */
      if (!GBL.no_headers)
         display_headers(&pck);
      
      /* 
       * format the packet with the function
       * set by the user
       */
      ret = GBL.format(buf, pck.len, tmp);
     
      /* the ANSI escape for the color */
      if (GBL.color) {
         int color = 0;
         switch (versus) {
            case VERSUS_SOURCE:
               color = COL_GREEN;
               break;
            case VERSUS_DEST:
               color = COL_BLUE;
               break;
         }
         set_color(color);
      }
      
      /* print it */
      write(fileno(stdout), tmp, ret);
      
      if (GBL.color) 
         reset_color();
      
      SAFE_FREE(buf);
      SAFE_FREE(tmp);
   }

   if (!GBL.no_headers)
      write(fileno(stdout), "\n\n", 2);
   
   return;
}

/*
 * display the packet headers 
 */

static void display_headers(struct log_header_packet *pck)
{
   /* it is at least 26... rounding up */
   char time[28];
   char tmp1[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   char flags[8];
   char *p = flags;
   char proto[5];
   char str[128];
   
   memset(flags, 0, sizeof(flags));
   
   write(fileno(stdout), "\n\n", 2);
   
   /* remove the final '\n' */
   strcpy(time, ctime((time_t *)&pck->tv.tv_sec));
   time[strlen(time)-1] = 0;
   
   /* displat the date */
   sprintf(str, "%s [%lu]\n", time, (unsigned long)pck->tv.tv_usec);
   write(fileno(stdout), str, strlen(str));
  
   if (GBL.showmac) {
      /* display the mac addresses */
      mac_addr_ntoa(pck->L2_src, tmp1);
      mac_addr_ntoa(pck->L2_dst, tmp2);
      sprintf(str, "%17s --> %17s\n", tmp1, tmp2 );
      write(fileno(stdout), str, strlen(str));
   }
  
   /* calculate the flags */
   if (pck->L4_flags & TH_SYN) *p++ = 'S';
   if (pck->L4_flags & TH_FIN) *p++ = 'F';
   if (pck->L4_flags & TH_RST) *p++ = 'R';
   if (pck->L4_flags & TH_ACK) *p++ = 'A';
   if (pck->L4_flags & TH_PSH) *p++ = 'P';
  
   /* determine the proto */
   switch(pck->L4_proto) {
      case NL_TYPE_TCP:
         strcpy(proto, "TCP");
         break;
      case NL_TYPE_UDP:
         strcpy(proto, "UDP");
         break;
   }
   
   /* display the ip addresses */
   ip_addr_ntoa(&pck->L3_src, tmp1);
   ip_addr_ntoa(&pck->L3_dst, tmp2);
   sprintf(str, "%s  %s:%d --> %s:%d | %s\n", proto, tmp1, ntohs(pck->L4_src), 
                                                        tmp2, ntohs(pck->L4_dst),
                                                        flags);
   write(fileno(stdout), str, strlen(str));

   write(fileno(stdout), "\n", 1);
}

/*
 * compile the regex
 */

void set_display_regex(char *regex)
{
   int err;
   char errbuf[100];

   /* allocate the new structure */
   SAFE_CALLOC(GBL.regex, 1, sizeof(regex_t));

   /* compile the regex */
   err = regcomp(GBL.regex, regex, REG_EXTENDED | REG_NOSUB | REG_ICASE );

   if (err) {
      regerror(err, GBL.regex, errbuf, sizeof(errbuf));
      FATAL_ERROR("%s\n", errbuf);
   }                      
}

/* display an inf log file */

static void display_info(void)
{
   struct host_profile *h;
   TAILQ_HEAD(, host_profile) *hosts_list_head = get_host_list_ptr();
   
   /* create the hosts' list */
   create_hosts_list(); 

   /* don't load if the user is interested only in passwords... */
   if (!GBL.passwords) {
      /* load the fingerprint database */
      fingerprint_init();

      /* load the manuf database */
      manuf_init();

      /* load the services names */
      services_init();
   }

   /* write the XML prolog */
   if (GBL.xml) {
      time_t tt = time(NULL);
      char time[28];
      /* remove the final '\n' */
      strcpy(time, ctime(&tt));
      time[strlen(time)-1] = 0;
      fprintf(stdout, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\n");
      fprintf(stdout, "<etterlog version=\"%s\" date=\"%s\">\n", EC_VERSION, time);
   } else
      fprintf(stdout, "\n\n");
   
   /* parse the list */
   TAILQ_FOREACH(h, hosts_list_head, next) {

      /* respect the TARGET selection */
      if (!is_target_info(h))
         continue;
     
      /* we are searching one particular user */
      if (find_user(h, GBL.user) == -ENOTFOUND)
         continue;
     
      /* if the regex was set, respect it */
      if (!match_regex(h))
         continue;
      
      /* skip the host respecting the options */
      if (GBL.only_local && (h->type & FP_HOST_NONLOCAL))
         continue;
      
      if (GBL.only_remote && (h->type & FP_HOST_LOCAL))
         continue;
      
      /* set the color */
      if (GBL.color) {
         if (h->type & FP_GATEWAY)
            set_color(COL_RED);
         else if (h->type & FP_HOST_LOCAL)
            set_color(COL_GREEN);
         else if (h->type & FP_HOST_NONLOCAL)
            set_color(COL_BLUE);
      }
     
      /* print the infos */
      if (GBL.passwords)
         print_pass(h);  
      else if (GBL.xml)
         print_host_xml(h);
      else
         print_host(h);
      
      /* reset the color */
      if (GBL.color)
         reset_color();
   }
   
   /* close the global tag */
   if (GBL.xml)
      fprintf(stdout, "</etterlog>\n");
   
   fprintf(stdout, "\n\n");

}

/* 
 * return 1 if h matches the regex 
 */

static int match_regex(struct host_profile *h)
{
   struct open_port *o;
   char os[OS_LEN+1];

   if (!GBL.regex)
      return 1;

   /* check the manufacturer */
   if (regexec(GBL.regex, manuf_search(h->L2_addr), 0, NULL, 0) == 0)
      return 1;
  
   /* check the OS */
   fingerprint_search(h->fingerprint, os);
   
   if (regexec(GBL.regex, os, 0, NULL, 0) == 0)
      return 1;

   /* check the open ports banners and service */
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      if (regexec(GBL.regex, service_search(o->L4_addr, o->L4_proto), 0, NULL, 0) == 0)
         return 1;
      
      if (o->banner && regexec(GBL.regex, o->banner, 0, NULL, 0) == 0)
         return 1;
   }
      
   return 0;
}


/*
 * print the ip and the account collected 
 */
static void print_pass(struct host_profile *h)
{
   struct open_port *o;
   struct active_user *u;
   char tmp[MAX_ASCII_ADDR_LEN];
   
   /* walk the list */
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      
      LIST_FOREACH(u, &(o->users_list_head), next) {

         /* skip client not matching the filter */
         if (!ip_addr_is_zero(&GBL.client) && ip_addr_cmp(&GBL.client, &u->client))
            continue;
        
         fprintf(stdout, " %-15s ", ip_addr_ntoa(&h->L3_addr, tmp));
         if (strcmp(h->hostname, ""))
            fprintf(stdout, "(%s)", h->hostname);
        
         /* print the client if requested */
         if (GBL.showclient)
            fprintf(stdout, "(%s)", ip_addr_ntoa(&u->client, tmp));

         fprintf(stdout, " %s %-5d %s USER: %s \tPASS: %s ",
               (o->L4_proto == NL_TYPE_TCP) ? "TCP" : "UDP" , 
               ntohs(o->L4_addr),
               (u->failed) ? "*" : "",
               u->user,
               u->pass);
         
         if (u->info)
            fprintf(stdout, "  INFO: %s\n\n", u->info);
         else
            fprintf(stdout, "\n\n");
      }
   }
   
}


/* EOF */

// vim:ts=3:expandtab

