/*
    ettercap -- cygwin specific functions

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

#include <windows.h>

static int saved_status;
static HKEY handle;

void disable_ip_forward(void);
static void restore_ip_forward(void);
u_int16 get_iface_mtu(const char *iface);

/*******************************************/

void disable_ip_forward(void)
{
   long       Status;
#ifdef WIN9X
   DWORD      dim = 2;
   char       IpForwardSz[2];
#else
   DWORD      dim = 4;
#endif
   DWORD      value = 3;


   Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
#ifdef WIN9X
                         TEXT("SYSTEM\\CurrentControlSet\\Services\\VxD\\Mstcp"),
#else
                         TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
#endif
                         0,
                         KEY_READ|KEY_SET_VALUE,
                         &handle);

#ifdef WIN9X
   Status = RegQueryValueEx(handle, TEXT("EnableRouting"), NULL, NULL, (LPBYTE)IpForwardSz, &dim);
   value = IpForwardSz[0]-'0';
#else
   Status = RegQueryValueEx(handle, TEXT("IPEnableRouter"), NULL, NULL, (LPBYTE)&value, &dim);
#endif

   DEBUG_MSG("Inet_DisableForwarding -- previous value %d", (int)value);

   if (value == 0) {
      /* if forward is already 0 */
      return;  
   }

   saved_status = value;

   value = 0;

#ifdef WIN9X
   IpForwardSz[0] = value + '0';
   IpForwardSz[1] = '\0';
   Status = RegSetValueEx(handle, TEXT("EnableRouting"), 0, REG_SZ, (LPBYTE)IpForwardSz, dim);
#else
   Status = RegSetValueEx(handle, TEXT("IPEnableRouter"), 0, REG_DWORD, (LPBYTE)&value, dim);
#endif

   if (Status != 0) {
      fprintf(stderr, "\n\nRegSetValueEx() | ERROR %ld\n\n", Status);
      fprintf(stderr, "Please manually disable ip forwarding\n");
#ifdef WIN9X
      fprintf(stderr, "set to 0 the HKLM\\SYSTEM\\CurrentControlSet\\Services\\VxD\\Mstcp\\EnableRouting\n\n");
#else
      fprintf(stderr, "set to 0 the HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\IPEnableRouter\n\n");
#endif
      exit(-1);
   }

   atexit(restore_ip_forward);
}

static void restore_ip_forward(void)
{
#ifdef WIN9X
   DWORD      dim = 2;
   char       IpForwardSz[2];
#else
   DWORD      dim = 4;
#endif

   /* the handle was already opened in disable_ip_forward */
   
   DEBUG_MSG("ATEXIT: restore_ip_forward: retoring to value %d", saved_status);
   
#ifdef WIN9X
   IpForwardSz[0] = saved_status + '0';
   IpForwardSz[1] = '\0';
   RegSetValueEx(handle, TEXT("EnableRouting"), 0, REG_SZ, (LPBYTE)IpForwardSz, dim);
#else
   RegSetValueEx(handle, TEXT("IPEnableRouter"), 0, REG_DWORD, (LPBYTE)&saved_status, dim);
#endif

   RegCloseKey(handle);

}

/* 
 * get the MTU parameter from the interface 
 */
u_int16 get_iface_mtu(const char *iface)
{
   int mtu = 1500;
   NOT_IMPLEMENTED();
   return mtu;
}

/* EOF */

// vim:ts=3:expandtab

