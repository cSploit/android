/*
 * arp.c
 *
 * ARP cache routines.
 * 
 * Copyright (c) 1999 Dug Song <dugsong@monkey.org>
 *
 * $Id: arp.c,v 1.8 2001/03/15 08:32:58 dugsong Exp $
 */
//Modified by Robbie Clemons <robclemons@gmail.com> 7/16/2011 for Android

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#ifdef BSD
#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/route.h>
#ifdef __FreeBSD__	/* XXX */
#define ether_addr_octet octet
#endif
#else /* !BSD */
#include <sys/ioctl.h>
#ifndef __linux__
#include <sys/sockio.h>
#endif
#endif /* !BSD */
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libnet.h>//added for Android
#include <droid.h>//added for Android
#include "arp.h"

#ifdef BSD
int
arp_cache_lookup(in_addr_t ip, struct ether_addr *ether, const char* linf)
{
	int mib[6];
	size_t len;
	char *buf, *next, *end;
	struct rt_msghdr *rtm;
	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;
	
	mib[0] = CTL_NET;
	mib[1] = AF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET;
	mib[4] = NET_RT_FLAGS;
	mib[5] = RTF_LLINFO;
	
	if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
		return (-1);
	
	if ((buf = (char *)malloc(len)) == NULL)
		return (-1);
	
	if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
		free(buf);
		return (-1);
	}
	end = buf + len;
	
	for (next = buf ; next < end ; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin = (struct sockaddr_inarp *)(rtm + 1);
		sdl = (struct sockaddr_dl *)(sin + 1);
		
		if (sin->sin_addr.s_addr == ip && sdl->sdl_alen) {
			memcpy(ether->ether_addr_octet, LLADDR(sdl),
			       ETHER_ADDR_LEN);
			free(buf);
			return (0);
		}
	}
	free(buf);
	
	return (-1);
}

#else /* !BSD */

#ifndef ETHER_ADDR_LEN	/* XXX - Solaris */
#define ETHER_ADDR_LEN	6
#endif

int
arp_cache_lookup(in_addr_t ip, struct ether_addr *ether, const char* lif)
{
	int sock;
	struct arpreq ar;
	struct sockaddr_in *sin;
	
	memset((char *)&ar, 0, sizeof(ar));
#ifdef __linux__
	strncpy(ar.arp_dev, lif, strlen(lif));
#endif
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ip;
	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return (-1);
	}
	if (ioctl(sock, SIOCGARP, (caddr_t)&ar) == -1) {
		close(sock);
		return (-1);
	}
	close(sock);
	memcpy(ether->ether_addr_octet, ar.arp_ha.sa_data, ETHER_ADDR_LEN);
	
	return (0);
}

#endif /* !BSD */
