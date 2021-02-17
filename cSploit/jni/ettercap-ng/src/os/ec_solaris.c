/*
    ettercap -- solaris specific functions

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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stropts.h>
#include <inet/nd.h>

#include <net/if.h>
#include <sys/sockio.h>

static char saved_status[2];
/* open it with high privs and use it later */
static int fd;

void disable_ip_forward(void);
static void restore_ip_forward(void);
u_int16 get_iface_mtu(const char *iface);

/*******************************************/

void disable_ip_forward(void)
{
   struct strioctl strIo;
   char buf[65536];
   char *cp;

   cp = "ip_forwarding";
   memset(buf, '\0', sizeof(buf));
   snprintf(buf, 13, "%s", cp);

   if ((fd = open("/dev/ip", O_RDWR)) < 0)
      ERROR_MSG("open failed for /dev/ip");

   strIo.ic_cmd = ND_GET;
   strIo.ic_timout = 0;
   strIo.ic_len = sizeof(buf);
   strIo.ic_dp = buf;

   /* Call IOCTL to return status */

   if ( (ioctl(fd, I_STR, (char *)&strIo)) == -1 )
      ERROR_MSG("ioctl(I_STR)");
 

   if (strIo.ic_cmd == ND_GET) {
      strncpy(saved_status, buf, 2);
                                                      }
   DEBUG_MSG("disable_ip_forward -- previous value = %s", saved_status);

   memset(buf, '\0', sizeof(buf));
   snprintf(buf, 13, "%s", cp);

   /* the format is "element"\0"value"\0 */
   buf[strlen(buf) + 1] = '0';  

   strIo.ic_cmd = ND_SET;
   strIo.ic_timout = 0;
   strIo.ic_len = sizeof(buf);
   strIo.ic_dp = buf;

   if ( (ioctl(fd, I_STR, (char *)&strIo)) == -1 )
      ERROR_MSG("ioctl(I_STR)");

   DEBUG_MSG("Inet_DisableForwarding -- NEW value = 0");

   atexit(restore_ip_forward);
}

static void restore_ip_forward(void)
{
   struct strioctl strIo;
   char buf[65536];
   char *cp;

   /* no need to restore anything */
   if (saved_status[0] == '0')
      return;
   
   cp = "ip_forwarding";
   memset(buf, '\0', sizeof(buf));
   snprintf(buf, 13, "%s", cp);

   /* the format is "element"\0"value"\0 */
   snprintf(buf + strlen(buf)+1, 2, "%s", saved_status);   

   DEBUG_MSG("ATEXIT: restore_ip_forward -- restoring to value = %s", saved_status);

   strIo.ic_cmd = ND_SET;
   strIo.ic_timout = 0;
   strIo.ic_len = sizeof(buf);
   strIo.ic_dp = buf;

   /* Call IOCTL to set the status */
   if ( (ioctl(fd, I_STR, (char *)&strIo)) == -1 )
      FATAL_ERROR("Please restore manually the ip_forwarding value to %s", saved_status);

   close(fd);
                                                
}

/* 
 * get the MTU parameter from the interface 
 */
u_int16 get_iface_mtu(const char *iface)
{
   int sock, mtu;
   struct ifreq ifr;
   
#if !defined(ifr_mtu) && defined(ifr_metric)
   #define ifr_mtu  ifr_metric
#endif

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

