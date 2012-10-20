/*
 *  $Id: libnet_if_addr.c,v 1.23 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_if_addr.c - interface selection code
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#if (!(_WIN32) || (__CYGWIN__)) 
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include "../include/ifaddrlist.h"

#define MAX_IPADDR 512

#if !(__WIN32__)

/*
 * By testing if we can retrieve the FLAGS of an iface
 * we can know if it exists or not and if it is up.
 */
int 
libnet_check_iface(libnet_t *l)
{
    struct ifreq ifr;
    int fd, res;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s() socket: %s\n", __func__,
                strerror(errno));
        return (-1);
    }

    strncpy(ifr.ifr_name, l->device, sizeof(ifr.ifr_name) -1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
    
    res = ioctl(fd, SIOCGIFFLAGS, (int8_t *)&ifr);

    if (res < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s() ioctl: %s\n", __func__,
                strerror(errno));
    }
    else
    {
        if ((ifr.ifr_flags & IFF_UP) == 0)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s(): %s is down\n",
                    __func__, l->device);
	    res = -1;
        }
    }
    close(fd);
    return (res);
}


/*
 *  Return the interface list
 */

#ifdef HAVE_SOCKADDR_SA_LEN
#define NEXTIFR(i) \
((struct ifreq *)((u_char *)&i->ifr_addr + i->ifr_addr.sa_len))
#else
#define NEXTIFR(i) (i + 1)
#endif

#ifndef BUFSIZE
#define BUFSIZE 2048
#endif

#ifdef HAVE_LINUX_PROCFS
#define PROC_DEV_FILE "/proc/net/dev"
#endif

int
libnet_ifaddrlist(register struct libnet_ifaddr_list **ipaddrp, char *dev,
register char *errbuf)
{
    register struct libnet_ifaddr_list *al;
    struct ifreq *ifr, *lifr, *pifr, nifr;
    char device[sizeof(nifr.ifr_name)];
    static struct libnet_ifaddr_list ifaddrlist[MAX_IPADDR];
    
    char *p;
    struct ifconf ifc;
    struct ifreq ibuf[MAX_IPADDR];
    register int fd, nipaddr;
    
#ifdef HAVE_LINUX_PROCFS
    FILE *fp;
    char buf[2048];
#endif
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
	snprintf(errbuf, LIBNET_ERRBUF_SIZE, "%s(): socket error: %s\n",
                __func__, strerror(errno));
	return (-1);
    }

#ifdef HAVE_LINUX_PROCFS
    if ((fp = fopen(PROC_DEV_FILE, "r")) == NULL)
    {
	snprintf(errbuf, LIBNET_ERRBUF_SIZE,
                "%s(): fopen(proc_dev_file) failed: %s\n",  __func__,
                strerror(errno));
	return (-1);
    }
#endif

    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_len = sizeof(ibuf);
    ifc.ifc_buf = (caddr_t)ibuf;

    if(ioctl(fd, SIOCGIFCONF, &ifc) < 0)
    {
	snprintf(errbuf, LIBNET_ERRBUF_SIZE,
                "%s(): ioctl(SIOCGIFCONF) error: %s\n", 
                __func__, strerror(errno));
	return(-1);
    }

    pifr = NULL;
    lifr = (struct ifreq *)&ifc.ifc_buf[ifc.ifc_len];
    
    al = ifaddrlist;
    nipaddr = 0;

#ifdef HAVE_LINUX_PROCFS
    while (fgets(buf, sizeof(buf), fp))
    {
	if ((p = strchr(buf, ':')) == NULL)
        {
            continue;
        }
        *p = '\0';
        for(p = buf; *p == ' '; p++) ;
	
        strncpy(nifr.ifr_name, p, sizeof(nifr.ifr_name) - 1);
        nifr.ifr_name[sizeof(nifr.ifr_name) - 1] = '\0';
	
#else /* !HAVE_LINUX_PROCFS */

    for (ifr = ifc.ifc_req; ifr < lifr; ifr = NEXTIFR(ifr))
    {
	/* XXX LINUX SOLARIS ifalias */
	if((p = strchr(ifr->ifr_name, ':')))
        {
            *p='\0';
        }
	if (pifr && strcmp(ifr->ifr_name, pifr->ifr_name) == 0)
        {
            continue;
        }
	strncpy(nifr.ifr_name, ifr->ifr_name, sizeof(nifr.ifr_name) - 1);
	nifr.ifr_name[sizeof(nifr.ifr_name) - 1] = '\0';
#endif

        /* save device name */
        strncpy(device, nifr.ifr_name, sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';

        if (ioctl(fd, SIOCGIFFLAGS, &nifr) < 0)
        {
            pifr = ifr;
            continue;
	}
        if ((nifr.ifr_flags & IFF_UP) == 0)
	{
            pifr = ifr;
            continue;	
	}

        if (dev == NULL && LIBNET_ISLOOPBACK(&nifr))
	{
            pifr = ifr;
            continue;
	}
	
        strncpy(nifr.ifr_name, device, sizeof(device) - 1);
        nifr.ifr_name[sizeof(nifr.ifr_name) - 1] = '\0';
        if (ioctl(fd, SIOCGIFADDR, (int8_t *)&nifr) < 0)
        {
            if (errno != EADDRNOTAVAIL)
            {
                snprintf(errbuf, LIBNET_ERRBUF_SIZE,
                        "%s(): SIOCGIFADDR: dev=%s: %s\n", __func__, device,
                        strerror(errno));
                close(fd);
                return (-1);
	    }
            else /* device has no IP address => set to 0 */
            {
                al->addr = 0;
            }
        }
        else
        {
            al->addr = ((struct sockaddr_in *)&nifr.ifr_addr)->sin_addr.s_addr;
        }
        
        free(al->device);
        al->device = NULL;

        if ((al->device = strdup(device)) == NULL)
        {
            snprintf(errbuf, LIBNET_ERRBUF_SIZE, 
                    "%s(): strdup not enough memory\n", __func__);
            return(-1);
        }

        ++al;
        ++nipaddr;

#ifndef HAVE_LINUX_PROCFS
        pifr = ifr;
#endif

    } /* while|for */
	
#ifdef HAVE_LINUX_PROCFS
    if (ferror(fp))
    {
        snprintf(errbuf, LIBNET_ERRBUF_SIZE,
                "%s(): ferror: %s\n", __func__, strerror(errno));
	return (-1);
    }
    fclose(fp);
#endif

    *ipaddrp = ifaddrlist;
    return (nipaddr);
}
#else
/* From tcptraceroute, convert a numeric IP address to a string */
#define IPTOSBUFFERS    12
static int8_t *iptos(uint32_t in)
{
    static int8_t output[IPTOSBUFFERS][ 3 * 4 + 3 + 1];
    static int16_t which;
    uint8_t *p;

    p = (uint8_t *)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    snprintf(output[which], IPTOSBUFFERS, "%d.%d.%d.%d", 
            p[0], p[1], p[2], p[3]);
    return output[which];
}

int
libnet_ifaddrlist(register struct libnet_ifaddr_list **ipaddrp, char *dev,
register char *errbuf)
{
    int nipaddr = 0;    int i = 0;

    static struct libnet_ifaddr_list ifaddrlist[MAX_IPADDR];
    pcap_if_t *alldevs;
    pcap_if_t *d;
    int8_t err[PCAP_ERRBUF_SIZE];

    /* Retrieve the interfaces list */
    if (pcap_findalldevs(&alldevs, err) == -1)
    {
        snprintf(errbuf, LIBNET_ERRBUF_SIZE, 
                "%s(): error in pcap_findalldevs: %s\n", __func__, err);
        return (-1);
    }

    /* Scan the list printing every entry */
	for (d = alldevs; d; d = d->next)
    {
		if((!d->addresses) || (d->addresses->addr->sa_family != AF_INET))
            continue;
        if(d->flags & PCAP_IF_LOOPBACK)
            continue;
    
		/* XXX - strdup */
        ifaddrlist[i].device = strdup(d->name);
        ifaddrlist[i].addr = (uint32_t)
                strdup(iptos(((struct sockaddr_in *)
                d->addresses->addr)->sin_addr.s_addr));
        ++i;
        ++nipaddr;
    }

    *ipaddrp = ifaddrlist;
    return (nipaddr);
}
#endif /* __WIN32__ */

int
libnet_select_device(libnet_t *l)
{
    int c, i;
    char err_buf[LIBNET_ERRBUF_SIZE];
    struct libnet_ifaddr_list *address_list, *al;
    uint32_t addr;


    if (l == NULL)
    { 
        return (-1);
    }

    if (l->device && !isdigit(l->device[0]))
    {
#if !(__WIN32__)
	if (libnet_check_iface(l) < 0)
	{
            /* err msg set in libnet_check_iface() */
	    return (-1);
	}
#endif
	return (1);
    }

    /*
     *  Number of interfaces.
     */
    c = libnet_ifaddrlist(&address_list, l->device, err_buf);
    if (c < 0)
    {
        /* err msg set in libnet_ifaddrlist() */
        return (-1);
    }
    else if (c == 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): no network interface found\n", __func__);
        return (-1);
    }
	
    al = address_list;
    if (l->device)
    {
        /*
         *  Then we have an IP address in l->device => do lookup
         */
	addr = libnet_name2addr4(l, l->device, 0);

        for (i = c; i; --i, ++address_list)
        {
            if (((addr == -1) && !(strncmp(l->device, address_list->device,
                   strlen(l->device)))) || 
                    (address_list->addr == addr))
            {
                /* free the "user supplied device" - see libnet_init() */
                free(l->device);
                l->device =  strdup(address_list->device);
                goto good;
            }
        }
        if (i <= 0)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): can't find interface for IP %s\n", __func__,
                    l->device);
	    goto bad;
        }
    }
    else
    {
        l->device = strdup(address_list->device);
    }

good:
    for (i = 0; i < c; i++)
    {
        free(al[i].device);
        al[i].device = NULL;
    }
    return (1);

bad:
    for (i = 0; i < c; i++)
    {
        free(al[i].device);
        al[i].device = NULL;
    }
    return (-1);
}

/* EOF */
