/*
    ettercap -- darwin specific functions

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

#include <sys/sysctl.h>
#include <sys/socket.h>

#include <sys/ioctl.h>
#include <net/if.h>

static int saved_status;

void disable_ip_forward(void);
static void restore_ip_forward(void);
u_int16 get_iface_mtu(const char *iface);

/*******************************************/

void disable_ip_forward(void)
{
   int mib[4]; 
   int val = 0;
   size_t len;

   mib[0] = CTL_NET;
   mib[1] = PF_INET;
   mib[2] = IPPROTO_IP;
   mib[3] = IPCTL_FORWARDING;

   len = sizeof(saved_status);

   if( (sysctl(mib, 4, &saved_status, &len, &val, sizeof(val))) == -1)
      ERROR_MSG("sysctl() | net.inet.ip.forwarding");

   DEBUG_MSG("disable_ip_forward | net.inet.ip.forwarding = %d  old_value = %d\n", val, saved_status);
                                       
   atexit(restore_ip_forward);
}


static void restore_ip_forward(void)
{
   int mib[4];

   mib[0] = CTL_NET;
   mib[1] = PF_INET;
   mib[2] = IPPROTO_IP;
   mib[3] = IPCTL_FORWARDING;
   
   /* no need to restore anything */
   if (saved_status == 0)
      return;
   
   /* restore the old value */
   if( (sysctl(mib, 4, NULL, NULL, &saved_status, sizeof(saved_status))) == -1)
      FATAL_ERROR("Please restore manually the value of net.inet.ip.forwarding to %d", saved_status);

   DEBUG_MSG("ATEXIT: restore_ip_forward | net.inet.ip.forwarding = %d\n", saved_status);
                        
}

/* 
 * get the MTU parameter from the interface 
 */
u_int16 get_iface_mtu(const char *iface)
{
   int sock, mtu;
   struct ifreq ifr;

   /* open the socket to work on */
   sock = socket(PF_INET, SOCK_DGRAM, 0);
               
   memset(&ifr, 0, sizeof(ifr));
   strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name));
                        
   /* get the MTU */
   if ( ioctl(sock, SIOCGIFMTU, &ifr) < 0)  {
      DEBUG_MSG("get_iface_mtu: MTU FAILED... assuming 1500");
      mtu = 1500;
   } else {
      DEBUG_MSG("get_iface_mtu: %d", ifr.ifr_mtu);
      mtu = ifr.ifr_mtu;
   }
   
   close(sock);
   
   return mtu;
}

/* EOF */

// vim:ts=3:expandtab

