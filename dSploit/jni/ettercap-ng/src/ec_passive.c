/*
    ettercap -- passive information handling module

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

    $Id: ec_passive.c,v 1.12 2003/11/29 16:02:35 lordnaga Exp $
*/

#include <ec.h>
#include <ec_passive.h>
#include <ec_decode.h>
#include <ec_packet.h>
#include <ec_fingerprint.h>
#include <ec_manuf.h>
#include <ec_services.h>

/* globals */

/* protos */

int is_open_port(u_int8 proto, u_int16 port, u_int8 flags);
void print_host(struct host_profile *h);
void print_host_xml(struct host_profile *h);

/************************************************/
  
/*
 * the strategy for open port discovery is:
 *
 * if the port is less than 1024, it is open at high  probability
 * 
 * if it is a TCP packet, we can rely on the tcp flags.
 *    so syn+ack packet are coming from an open port.
 *
 * as a last resource, look in the registered dissector table.
 * if a port is registered, it might be opened.
 *
 */

int is_open_port(u_int8 proto, u_int16 port, u_int8 flags)
{

   switch (proto) {
      case NL_TYPE_TCP:
#if 0 
         /* detect priviledged port */
         if (ntohs(po->L4.src) > 0 && ntohs(po->L4.src) < 1024 )
            return 1;
#endif
         /* SYN+ACK packet are coming from open ports */
         if ( (flags & TH_SYN) && (flags & TH_ACK) )
            return 1;
         break;
      case NL_TYPE_UDP:
         /* 
          * we cannot determine if the port is open or not...
          * suppose that all priveledged port used are open.
          */
         if (ntohs(port) > 0 && ntohs(port) < 1024 )
            return 1;
         /* look up in the table */
         if ( get_decoder(APP_LAYER_UDP, ntohs(port)) != NULL)
            return 1;
         break;
   }

   return 0;
}

/*
 * prints the infos of a single host
 */

void print_host(struct host_profile *h)
{
   struct open_port *o;
   struct active_user *u;
   char tmp[MAX_ASCII_ADDR_LEN];
   char os[OS_LEN+1];

   memset(os, 0, sizeof(os));
   
   fprintf(stdout, "==================================================\n");
   fprintf(stdout, " IP address   : %s \n", ip_addr_ntoa(&h->L3_addr, tmp));
   if (strcmp(h->hostname, ""))
      fprintf(stdout, " Hostname     : %s \n\n", h->hostname);
   else
      fprintf(stdout, "\n");   
      
   if (h->type & FP_HOST_LOCAL || h->type == FP_UNKNOWN) {
      fprintf(stdout, " MAC address  : %s \n", mac_addr_ntoa(h->L2_addr, tmp));
      fprintf(stdout, " MANUFACTURER : %s \n\n", manuf_search(h->L2_addr));
   }

   fprintf(stdout, " DISTANCE     : %d   \n", h->distance);
   if (h->type & FP_GATEWAY)
      fprintf(stdout, " TYPE         : GATEWAY\n\n");
   else if (h->type & FP_HOST_LOCAL)
      fprintf(stdout, " TYPE         : LAN host\n\n");
   else if (h->type & FP_ROUTER)
      fprintf(stdout, " TYPE         : REMOTE ROUTER\n\n");
   else if (h->type & FP_HOST_NONLOCAL)
      fprintf(stdout, " TYPE         : REMOTE host\n\n");
   else if (h->type == FP_UNKNOWN)
      fprintf(stdout, " TYPE         : unknown\n\n");
      
   
   fprintf(stdout, " FINGERPRINT      : %s\n", h->fingerprint);
   if (fingerprint_search(h->fingerprint, os) == ESUCCESS)
      fprintf(stdout, " OPERATING SYSTEM : %s \n\n", os);
   else {
      fprintf(stdout, " OPERATING SYSTEM : unknown fingerprint (please submit it) \n");
      fprintf(stdout, " NEAREST ONE IS   : %s \n\n", os);
   }
      
   
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      
      fprintf(stdout, "   PORT     : %s %d | %s \t[%s]\n", 
                  (o->L4_proto == NL_TYPE_TCP) ? "TCP" : "UDP" , 
                  ntohs(o->L4_addr),
                  service_search(o->L4_addr, o->L4_proto), 
                  (o->banner) ? o->banner : "");
      
      LIST_FOREACH(u, &(o->users_list_head), next) {
        
         if (u->failed)
            fprintf(stdout, "      ACCOUNT : * %s / %s  (%s)\n", u->user, u->pass, ip_addr_ntoa(&u->client, tmp));
         else
            fprintf(stdout, "      ACCOUNT : %s / %s  (%s)\n", u->user, u->pass, ip_addr_ntoa(&u->client, tmp));
         if (u->info)
            fprintf(stdout, "      INFO    : %s\n\n", u->info);
         else
            fprintf(stdout, "\n");
      }
      fprintf(stdout, "\n");
   }
   
   fprintf(stdout, "\n==================================================\n\n");
}


/*
 * prints the infos of a single host in XML format
 */

void print_host_xml(struct host_profile *h)
{
   struct open_port *o;
   struct active_user *u;
   char tmp[MAX_ASCII_ADDR_LEN];
   char os[OS_LEN+1];

   memset(os, 0, sizeof(os));
   
   fprintf(stdout, "\t<host ip=\"%s\">\n", ip_addr_ntoa(&h->L3_addr, tmp));
   if (strcmp(h->hostname, ""))
      fprintf(stdout, "\t\t<hostname>%s</hostname>\n", h->hostname);
   
   if (h->type & FP_HOST_LOCAL || h->type == FP_UNKNOWN) {
      fprintf(stdout, "\t\t<mac>%s</mac>\n", mac_addr_ntoa(h->L2_addr, tmp));
      fprintf(stdout, "\t\t<manuf>%s</manuf>\n", manuf_search(h->L2_addr));
   }
   
   fprintf(stdout, "\t\t<distance>%d</distance>\n", h->distance);
   if (h->type & FP_GATEWAY)
      fprintf(stdout, "\t\t<type>GATEWAY</type>\n");
   else if (h->type & FP_HOST_LOCAL)
      fprintf(stdout, "\t\t<type>LAN host</type>\n");
   else if (h->type & FP_ROUTER)
      fprintf(stdout, "\t\t<type>REMOTE ROUTER</type>\n");
   else if (h->type & FP_HOST_NONLOCAL)
      fprintf(stdout, "\t\t<type>REMOTE host</type>\n");
   else if (h->type == FP_UNKNOWN)
      fprintf(stdout, "\t\t<type>unknown</type>\n");
  
   
   if (strcmp(h->fingerprint, "")) {
      if (fingerprint_search(h->fingerprint, os) == ESUCCESS) {
         fprintf(stdout, "\t\t<fingerprint type=\"known\">%s</fingerprint>\n", h->fingerprint);
         fprintf(stdout, "\t\t<os type=\"exact\">%s</os>\n", os);
      } else {
         fprintf(stdout, "\t\t<fingerprint type=\"unknown\">%s</fingerprint>\n", h->fingerprint);
         fprintf(stdout, "\t\t<os type=\"nearest\">%s</os>\n", os);
      }
   }
   
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      
      fprintf(stdout, "\t\t<port proto=\"%s\" addr=\"%d\" service=\"%s\">\n", 
            (o->L4_proto == NL_TYPE_TCP) ? "tcp" : "udp", 
            ntohs(o->L4_addr),
            service_search(o->L4_addr, o->L4_proto));
      
      if (o->banner)
         fprintf(stdout, "\t\t\t<banner>%s</banner>\n", o->banner);
      
      LIST_FOREACH(u, &(o->users_list_head), next) {
         
         if (u->failed)  
            fprintf(stdout, "\t\t\t<account user=\"%s\" failed=\"1\">\n", u->user);
         else
            fprintf(stdout, "\t\t\t<account user=\"%s\">\n", u->user);
            
         fprintf(stdout, "\t\t\t\t<user>%s</user>\n", u->user);
         fprintf(stdout, "\t\t\t\t<pass>%s</pass>\n", u->pass);
         fprintf(stdout, "\t\t\t\t<client>%s</client>\n", ip_addr_ntoa(&u->client, tmp));
         if (u->info)
            fprintf(stdout, "\t\t\t\t<info>%s</info>\n", u->info);
         
         fprintf(stdout, "\t\t\t</account>\n");
      }
      fprintf(stdout, "\t\t</port>\n");
   }
   
   fprintf(stdout, "\t</host>\n");
}

/* EOF */

// vim:ts=3:expandtab

