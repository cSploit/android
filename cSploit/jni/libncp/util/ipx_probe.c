/*
    ipx_probe.c - Check the network for frames currently active
    Copyright (C) 1996 by Volker Lendecke
  
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    Revision history:

	0.00  1996			Volker Lendecke
		Initial revision.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ncp/ext/socket.h>
#include <ncp/kernel/ipx.h>
#include <ncp/kernel/if.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;
int verbose = 0;

static void 
usage (void)
{
  fprintf (stderr, _("usage: %s [options]\n"), progname);
  fprintf (stderr, _("type '%s -h' for help\n"), progname);
}

static void 
help (void)
{
  printf (_("\n"
	  "Probe an interface for ipx networks\n"
	  "\n"));
  printf (_("usage: %s [options]\n"), progname);
  printf (_("\n"
	  "-v              Verbose output\n"
	  "-i interface    Interface to probe, default: eth0\n"
	  "-t timeout      Seconds to wait for answer, default: 3\n"
	  "-h              Print this help text\n\n"));
}

#define IPX_SAP_PTYPE (0x04)
#define IPX_SAP_NEAREST_QUERY (0x0003)
#define IPX_SAP_PORT  (0x0452)
#define IPX_BROADCAST_NODE ("\xff\xff\xff\xff\xff\xff")

#define BVAL(buf,pos) (((u_int8_t *)(buf))[pos])
#define BSET(buf,pos,val) (BVAL(buf,pos) = (val))
static inline void 
WSET_HL (u_int8_t * buf, int pos, u_int16_t val)
{
  BSET (buf, pos, val >> 8);
  BSET (buf, pos + 1, val & 0xff);
}

struct frame_type
{
  char *ft_name;
  unsigned char ft_val;
};

static struct frame_type frame_types[] =
{
  {
    "802.2", IPX_FRAME_8022
  }
  ,
#ifdef IPX_FRAME_TR_8022
  {
    "802.2TR", IPX_FRAME_TR_8022
  }
  ,
#endif
  {
    "802.3", IPX_FRAME_8023
  }
  ,
  {
    "SNAP", IPX_FRAME_SNAP
  }
  ,
  {
    "EtherII", IPX_FRAME_ETHERII
  }
};

#define NFTYPES	(sizeof(frame_types)/sizeof(struct frame_type))

static char *
frame_name (int frame_type)
{
  unsigned int i;
  for (i = 0; i < NFTYPES; i++)
    {
      if (frame_types[i].ft_val == frame_type)
	{
	  return frame_types[i].ft_name;
	}
    }
  return NULL;
}

static int 
ipx_recvfrom (int sock, void *buf, int len, unsigned int flags,
	      struct sockaddr_ipx *sender, socklen_t *addrlen, int timeout,
	      long *err)
{
  fd_set rd, wr, ex;
  struct timeval tv;
  int result;

  FD_ZERO (&rd);
  FD_ZERO (&wr);
  FD_ZERO (&ex);
  FD_SET (sock, &rd);

  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  if ((result = select (sock + 1, &rd, &wr, &ex, &tv)) == -1)
    {
      *err = errno;
      return -1;
    }
  if (FD_ISSET (sock, &rd))
    {
      result = recvfrom (sock, buf, len, flags,
			 (struct sockaddr *) sender, addrlen);
    }
  else
    {
      result = -1;
      errno = ETIMEDOUT;
    }
  if (result < 0)
    {
      *err = errno;
    }
  return result;
}

static int 
ipx_recv (int sock, void *buf, int len, unsigned int flags, int timeout,
	  long *err)
{
  struct sockaddr_ipx sender;
  socklen_t addrlen = sizeof (sender);

  return ipx_recvfrom (sock, buf, len, flags, &sender, &addrlen,
		       timeout, err);
}

static int 
probe_frame (char *interface, int frame_type, int timeout, unsigned long *net)
{
  int i, sock, opt;
  int result;
  long err;
  char errmsg[strlen (progname) + 20];
  char data[1024];

  static struct ifreq id;
  struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;

  if (verbose != 0)
    {
      printf (_("probing %s on %s -- "), frame_name (frame_type),
	      interface);
      fflush (stdout);
    }
  sock = socket (AF_IPX, SOCK_DGRAM, IPXPROTO_IPX);
  if (sock < 0)
    {
      int old_errno = errno;

      sprintf (errmsg, _("%s: socket"), progname);
      perror (errmsg);
      if (old_errno == -EINVAL)
	{
	  fprintf (stderr, _("Probably you have no IPX support in "
		   "your kernel\n"));
	}
      close (sock);
      return -1;
    }
  memset (&id, 0, sizeof (id));
  strncpy (id.ifr_name, interface, sizeof (id.ifr_name) - 1);
  sipx->sipx_family = AF_IPX;
  sipx->sipx_action = IPX_CRTITF;
  sipx->sipx_special = IPX_PRIMARY;
  sipx->sipx_network = 0L;
  sipx->sipx_type = frame_type;

  i = 0;
  do
    {
      result = ioctl (sock, SIOCSIFADDR, &id);
      i++;
    }
  while ((i < 5) && (result < 0) && (errno == EAGAIN));

  if (result < 0)
    {
      int old_errno = errno;
      close (sock);
      errno = old_errno;
      return result;
    }
  /* We do a GNS request on the new socket. If something comes
     back, we assume that the frame type is valid. */

  opt = 1;
  if ((result = setsockopt (sock, SOL_SOCKET,
			    SO_BROADCAST, &opt, sizeof (opt))) < 0)
    {
      int old_errno = errno;
      close (sock);
      errno = old_errno;
      return result;
    }
  memset (&id, 0, sizeof (id));
  sipx->sipx_family = AF_IPX;
  sipx->sipx_network = htonl (0x0);
  sipx->sipx_port = htons (0x0);
  sipx->sipx_type = IPX_SAP_PTYPE;

  if ((result = bind (sock, (struct sockaddr *) sipx,
		      sizeof (*sipx))) < 0 - 1)
    {
      int old_errno = errno;
      close (sock);
      errno = old_errno;
      return result;
    }
  WSET_HL (data, 0, IPX_SAP_NEAREST_QUERY);
  WSET_HL (data, 2, 4);

  memset (&id, 0, sizeof (id));
  sipx->sipx_family = AF_IPX;
  sipx->sipx_port = htons (IPX_SAP_PORT);
  sipx->sipx_type = IPX_SAP_PTYPE;
  sipx->sipx_network = htonl (0x0);
  memcpy (sipx->sipx_node, IPX_BROADCAST_NODE, 6);

  if ((result = sendto (sock, data, 4, 0, (struct sockaddr *) sipx,
			sizeof (*sipx))) < 0)
    {
      int old_errno = errno;
      close (sock);
      errno = old_errno;
      return result;
    }
  result = ipx_recv (sock, data, 1024, 0, timeout, &err);

  if (result > 0)
    {
      struct sockaddr_ipx sipx;
      int namelen = sizeof (sipx);

      if (getsockname (sock, (struct sockaddr *) &sipx,
		       &namelen) < 0)
	{
	  fprintf (stderr, _("%s: Could not find socket address\n"),
		   progname);
	  exit (1);
	}
      *net = ntohl (sipx.sipx_network);
    }
  memset (&id, 0, sizeof (id));
  strncpy (id.ifr_name, interface, sizeof (id.ifr_name) - 1);
  sipx->sipx_family = AF_IPX;
  sipx->sipx_action = IPX_DLTITF;
  sipx->sipx_network = 0L;
  sipx->sipx_type = frame_type;
  result = ioctl (sock, SIOCSIFADDR, &id);
  close (sock);

  if (result < 0)
    {
      fprintf (stderr, _("%s: could not delete interface\n"),
	       progname);
      exit (1);
    }
  if (err == ETIMEDOUT)
    {
      if (verbose != 0)
	{
	  printf (_("no network found\n"));
	}
      return -1;
    }
  if (verbose != 0)
    {
      printf (_("found IPX network %8.8lX\n"), *net);
    }
  return 0;
}


static int 
file_lines (char *name)
{
  FILE *f = fopen (name, "r");
  char buf[100];
  int lines = 0;

  if (f == NULL)
    {
      return -errno;
    }
  while (fgets (buf, sizeof (buf), f) != NULL)
    {
      lines += 1;
    }
  fclose (f);
  return lines;
}

static int 
ipx_interfaces (void)
{
  int result = file_lines ("/proc/net/ipx_interface");
  if (result == 0)
    {
      result = -EIO;
    }
  return result - 1;
}

static int 
ipx_auto_off (void)
{
  int s;
  char errmsg[strlen (progname) + 20];
  int val = 0;

  s = socket (AF_IPX, SOCK_DGRAM, AF_IPX);
  if (s < 0)
    {
      int old_errno = errno;

      sprintf (errmsg, _("%s: socket"), progname);
      perror (errmsg);
      if (old_errno == -EINVAL)
	{
	  fprintf (stderr, _("Probably you have no IPX support in "
		   "your kernel\n"));
	}
      close (s);
      return -1;
    }
  sprintf (errmsg, _("%s: ioctl"), progname);

  if (ioctl (s, SIOCAIPXPRISLT, &val) < 0)
    {
      perror (errmsg);
      close (s);
      return -1;
    }
  if (ioctl (s, SIOCAIPXITFCRT, &val) < 0)
    {
      perror (errmsg);
      close (s);
      return -1;
    }
  close (s);
  return 0;
}

int 
main (int argc, char *argv[])
{
  int interfaces;
  char *interface = "eth0";
  int opt;
  int timeout = 3;

  unsigned long network[5] =
  {0,};

  progname = argv[0];

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  
  while ((opt = getopt (argc, argv, "vi:ht:")) != EOF)
    {
      switch (opt)
	{
	case 'v':
	  verbose = 1;
	  break;
	case 'i':
	  interface = optarg;
	  break;
	case 't':
	  timeout = atoi (optarg);
	  break;
	case 'h':
	  help ();
	  exit (1);
	default:
	  usage ();
	  exit (1);
	}
    }

  if (ipx_auto_off () < 0)
    {
      exit (1);
    }
  interfaces = ipx_interfaces ();
  if (interfaces > 0)
    {
      fprintf (stderr, _("%s must be run with no interfaces configured."
	               " Found %d interface%s.\n"),
	               progname, interfaces,
	               interfaces == 1 ? "" : "s");
      exit (1);
    }
  if (interfaces < 0)
    {
      fprintf (stderr, _("%s: %s\n"), progname, strerror (interfaces));
      exit (1);
    }
  probe_frame (interface, IPX_FRAME_8022, timeout, &(network[0]));
  probe_frame (interface, IPX_FRAME_8023, timeout, &(network[1]));
  probe_frame (interface, IPX_FRAME_SNAP, timeout, &(network[2]));
  probe_frame (interface, IPX_FRAME_ETHERII, timeout, &(network[3]));

  if (verbose == 0)
    {
      if (network[0] != 0)
	{
	  printf (_("%s %8.8lX\n"),
		  frame_name (IPX_FRAME_8022), network[0]);
	}
      if (network[1] != 0)
	{
	  printf (_("%s %8.8lX\n"),
		  frame_name (IPX_FRAME_8023), network[1]);
	}
      if (network[2] != 0)
	{
	  printf (_("%s %8.8lX\n"),
		  frame_name (IPX_FRAME_SNAP), network[2]);
	}
      if (network[3] != 0)
	{
	  printf (_("%s %8.8lX\n"),
		  frame_name (IPX_FRAME_ETHERII), network[3]);
	}
    }
  return 0;
}
