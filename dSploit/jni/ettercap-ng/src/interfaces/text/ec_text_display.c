/*
    ettercap -- formats and displays the packets

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

    $Id: ec_text_display.c,v 1.4 2004/01/06 15:03:15 alor Exp $
*/

#include <ec.h>
#include <ec_packet.h>
#include <ec_interfaces.h>
#include <ec_format.h>

/* proto */

void text_print_packet(struct packet_object *po);
static void display_headers(struct packet_object *po);

/*******************************************/


void text_print_packet(struct packet_object *po)
{
   /* 
    * keep it static so it is always the same
    * memory region used for this operation
    */
   static u_char *tmp = NULL;
   int ret;

   /* don't display the packet */
   if (GBL_OPTIONS->quiet)
      return;
   
   /* 
    * if the regex does not match, the packet is not interesting 
    *
    * should we put this after the format function ?
    * in this way we can match e.t.t.e.r.c.a.p in TEXT mode with
    * the "ettercap" regex
    */
   if (GBL_OPTIONS->regex && 
       regexec(GBL_OPTIONS->regex, po->DATA.disp_data, 0, NULL, 0) != 0) {
      return;
   }
               
   /* 
    * prepare the buffer,
    * the max length is hex_fomat
    * so use its length for the buffer
    */
   SAFE_REALLOC(tmp, hex_len(po->DATA.disp_len) * sizeof(u_char));

   /* 
    * format the packet with the function set by the user
    */
   ret = GBL_FORMAT(po->DATA.disp_data, po->DATA.disp_len, tmp);

   /* print the headers */
   display_headers(po);
   
   /* print it */
   write(fileno(stdout), tmp, ret);

   printf("\n");
}     


static void display_headers(struct packet_object *po)
{
   /* it is at least 26... rounding up */
   char time[28];
   char tmp1[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   char flags[8];
   char *p = flags;
   char proto[5];
   
   memset(flags, 0, sizeof(flags));
   
   fprintf(stdout, "\n\n");
   
   /* remove the final '\n' */
   strcpy(time, ctime((time_t *)&po->ts.tv_sec));
   time[strlen(time)-1] = 0;
   
   /* displat the date */
   fprintf(stdout, "%s\n", time);
   //fprintf(stdout, "%x %x\n", (u_int)po->ts.tv_sec, (u_int)po->ts.tv_usec);
  
   if (GBL_OPTIONS->ext_headers) {
      /* display the mac addresses */
      mac_addr_ntoa(po->L2.src, tmp1);
      mac_addr_ntoa(po->L2.dst, tmp2);
      fprintf(stdout, "%17s --> %17s\n", tmp1, tmp2 );
   }
  
   /* calculate the flags */
   if (po->L4.flags & TH_SYN) *p++ = 'S';
   if (po->L4.flags & TH_FIN) *p++ = 'F';
   if (po->L4.flags & TH_RST) *p++ = 'R';
   if (po->L4.flags & TH_ACK) *p++ = 'A';
   if (po->L4.flags & TH_PSH) *p++ = 'P';
  
   /* determine the proto */
   switch(po->L4.proto) {
      case NL_TYPE_TCP:
         strcpy(proto, "TCP");
         break;
      case NL_TYPE_UDP:
         strcpy(proto, "UDP");
         break;
   }
   
   /* display the ip addresses */
   ip_addr_ntoa(&po->L3.src, tmp1);
   ip_addr_ntoa(&po->L3.dst, tmp2);
   fprintf(stdout, "%s  %s:%d --> %s:%d | %s\n", proto, tmp1, ntohs(po->L4.src), 
                                                        tmp2, ntohs(po->L4.dst),
                                                        flags);

   fprintf(stdout, "\n");
}


/* EOF */

// vim:ts=3:expandtab

