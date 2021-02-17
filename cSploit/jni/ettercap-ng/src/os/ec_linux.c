/*
    ettercap -- linux specific functions

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
#include <sys/wait.h>
#include <net/if.h>

/* the old value */
static char saved_status;

/* protos */

void disable_ip_forward(void);
static void restore_ip_forward(void);
u_int16 get_iface_mtu(const char *iface);
void disable_interface_offload(void);
void safe_free_mem(char **param, int *param_length, char *command);

/*******************************************/

void disable_ip_forward(void)
{
   FILE *fd;
   
   fd = fopen("/proc/sys/net/ipv4/ip_forward", "r");
   ON_ERROR(fd, NULL, "failed to open /proc/sys/net/ipv4/ip_forward");

   fscanf(fd, "%c", &saved_status);
   fclose(fd);

   DEBUG_MSG("disable_ip_forward: old value = %c", saved_status);
 
   /* sometimes writing just fails... retry up to 5 times */
   int i = 0;
   fd = NULL;
   do {
      fd = fopen("/proc/sys/net/ipv4/ip_forward", "w");
      i++;
      usleep(20000);
   } while(fd == NULL && i <=50);
   ON_ERROR(fd, NULL, "failed to open /proc/sys/net/ipv4/ip_forward");
   
   fprintf(fd, "0");
   fclose(fd);
   
   atexit(restore_ip_forward);
}

static void restore_ip_forward(void)
{
   FILE *fd;
   char current_status;
   
   /* no modification needed */
   if (saved_status == '0')
      return;
   
   /* read the current status to know if we need to modify it */
   fd = fopen("/proc/sys/net/ipv4/ip_forward", "r");
   ON_ERROR(fd, NULL, "failed to open /proc/sys/net/ipv4/ip_forward");

   fscanf(fd, "%c", &current_status);
   fclose(fd);
   
   DEBUG_MSG("ATEXIT: restore_ip_forward: curr: %c saved: %c", current_status, saved_status);

   if (current_status == saved_status) {
      DEBUG_MSG("ATEXIT: restore_ip_forward: does not need restoration");
      return;
   }

   /* sometimes writing just fails... retry up to 5 times */
   int i = 0;
   fd = NULL;
   do {
      fd = fopen("/proc/sys/net/ipv4/ip_forward", "w");
      i++;
      usleep(20000);
   } while(fd == NULL && i <=50);
   if (fd == NULL) {
      FATAL_ERROR("ip_forwarding was disabled, but we cannot re-enable it now.\n"
                  "remember to re-enable it manually\n");
   }

   fprintf(fd, "%c", saved_status);
   fclose(fd);

   DEBUG_MSG("ATEXIT: restore_ip_forward: restore to %c", saved_status);
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

void safe_free_mem(char **param, int *param_length, char *command)
{
   int k;

   SAFE_FREE(command);
	for(k= 0; k < (*param_length); ++k)
		SAFE_FREE(param[k]);
	SAFE_FREE(param);
}

/*
 * disable segmentation offload on interface
 * this prevents L3 send errors (payload too large)
 */
void disable_interface_offload(void)
{
	int param_length= 0;
	char *command;
	char **param = NULL;
	char *p;
	int ret_val, i = 0;

	SAFE_CALLOC(command, 100, sizeof(char));

	BUG_IF(command==NULL);

	memset(command, '\0', 100);	
	snprintf(command, 99, "ethtool -K %s tso off gso off gro off lro off", GBL_OPTIONS->iface);

	DEBUG_MSG("disable_interface_offload: Disabling offload on %s", GBL_OPTIONS->iface);

	for(p = strsep(&command, " "); p != NULL; p = strsep(&command, " ")) {
		SAFE_REALLOC(param, (i+1) * sizeof(char *));
		param[i++] = strdup(p);	
	}

	SAFE_REALLOC(param, (i+1) * sizeof(char *));
	param[i] = NULL;
	param_length= i + 1; //because there is a SAFE_REALLOC after the for.

	switch(fork()) {
		case 0:
#ifndef DEBUG
			/* don't print on console if the ethtool cannot disable some offloads unless you are in debug mode */
			close(2);
#endif
			execvp(param[0], param);
			WARN_MSG("cannot disable offload on %s, do you have ethtool installed?", GBL_OPTIONS->iface);
			safe_free_mem(param, &param_length, command);
			_exit(EINVALID);
		case -1:
			safe_free_mem(param, &param_length, command);
		default:
			safe_free_mem(param, &param_length, command);
			wait(&ret_val);
	} 	
}

/* EOF */

// vim:ts=3:expandtab

