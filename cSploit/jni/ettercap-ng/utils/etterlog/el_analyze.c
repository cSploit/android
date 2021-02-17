/*
    etterlog -- analysis module

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

    $Id: el_analyze.c,v 1.17 2004/09/30 14:54:14 alor Exp $
*/

#include <el.h>
#include <ec_log.h>
#include <ec_profiles.h>
#include <el_functions.h>

#include <sys/stat.h>

void analyze(void);
void analyze_packet(void);
void analyze_info(void);

void create_hosts_list(void);

/*******************************************/

void analyze(void)
{
   switch(GBL.hdr.type) {
      case LOG_PACKET:
         analyze_packet();
         break;
      case LOG_INFO:
         analyze_info();
         break;
   }
}


/* analyze a packet log file */

void analyze_packet(void)
{
   struct log_header_packet pck;
   int ret, count = 0;
   int tot_size = 0, pay_size = 0;
   u_char *buf;
   struct stat st;
   
   fprintf(stdout, "\nAnalyzing the log file (one dot every 100 packets)\n");
 
   tot_size = sizeof(struct log_global_header);
   
   /* read the logfile */
   LOOP {
      
      memset(&pck, 0, sizeof(struct log_header_packet));
      
      ret = get_packet(&pck, &buf);

      /* on error exit the loop */
      if (ret != ESUCCESS)
         break;
      
      count++;
      tot_size += sizeof(struct log_header_packet) + pck.len;
      pay_size += pck.len;
    
      if (count % 100 == 0) {
         fprintf(stderr, ".");
         fflush(stderr);
      }
      
      SAFE_FREE(buf);
   }

   /* get the file stat */
   ret = stat(GBL.logfile, &st);
   ON_ERROR(ret, -1, "Cannot stat file");
   
   fprintf(stdout, "\n\n");
   fprintf(stdout, "Log file size (compressed)   : %d\n", (int)st.st_size);   
   fprintf(stdout, "Log file size (uncompressed) : %d\n", tot_size);
   if (tot_size != 0)
      fprintf(stdout, "Compression ratio            : %.2f %%\n\n", 100 - ((float)st.st_size * 100 / (float)tot_size) );
   fprintf(stdout, "Effective payload size       : %d\n", pay_size);
   if (tot_size != 0)
      fprintf(stdout, "Wasted percentage            : %.2f %%\n\n", 100 - ((float)pay_size * 100 / (float)tot_size) );
   
   fprintf(stdout, "Number of packets            : %d\n", count);
   if (count != 0)
      fprintf(stdout, "Average size per packet      : %d\n", pay_size / count );
   fprintf(stdout, "\n");
   
   return;
}



/*
 * extract data form the file
 * and create the host list
 */

void create_hosts_list(void)
{
   struct log_header_info inf;
   int ret;
   struct dissector_info buf;
   
   /* read the logfile */
   LOOP {

      memset(&inf, 0, sizeof(struct log_header_info));
      memset(&buf, 0, sizeof(struct dissector_info));
      
      ret = get_info(&inf, &buf);

      /* on error exit the loop */
      if (ret != ESUCCESS)
         break;
   
      profile_add_info(&inf, &buf);
      
      SAFE_FREE(buf.user);
      SAFE_FREE(buf.pass);
      SAFE_FREE(buf.info);
      SAFE_FREE(buf.banner);
   }
}


/* 
 * analyze an info log file 
 */

void analyze_info(void)
{
   struct host_profile *h;
   struct open_port *o;
   struct active_user *u;
   TAILQ_HEAD(, host_profile) *hosts_list_head = get_host_list_ptr();
   int nhl = 0, nhnl = 0, ngw = 0;
   int nports = 0, nusers = 0, nhosts = 0;
   
   /* create the hosts' list */
   create_hosts_list(); 


   TAILQ_FOREACH(h, hosts_list_head, next) {
      if (h->type & FP_HOST_LOCAL)
         nhl++;
      
      if (h->type & FP_HOST_NONLOCAL)
         nhnl++;
      
      if (h->type & FP_GATEWAY)
         ngw++;
      
      nhosts++;
     
      LIST_FOREACH(o, &(h->open_ports_head), next) {
         nports++;
         
         LIST_FOREACH(u, &(o->users_list_head), next) {
            nusers++;
         }
      }
   }
   
   fprintf(stdout, "\n\n");
   fprintf(stdout, "Number of hosts (total)       : %d\n\n", nhosts);   
   fprintf(stdout, "Number of local hosts         : %d\n", nhl);   
   fprintf(stdout, "Number of non local hosts     : %d\n", nhnl);   
   fprintf(stdout, "Number of gateway             : %d\n\n", ngw);   
   
   fprintf(stdout, "Number of discovered services : %d\n", nports);   
   fprintf(stdout, "Number of accounts captured   : %d\n\n", nusers);   
   
   fprintf(stdout, "\n");
   
   return;
}



/* EOF */

// vim:ts=3:expandtab

