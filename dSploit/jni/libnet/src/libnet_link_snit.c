/*
 *  $Id: libnet_link_snit.c,v 1.6 2004/01/03 20:31:02 mike Exp $
 *
 *  libnet
 *  libnet_snit.c - snit routines
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Modifications made to accommodate the new SunOS4.0 NIT facility by
 * Micky Liu, micky@cunixc.cc.columbia.edu, Columbia University in May, 1989.
 * This module now handles the STREAMS based NIT.
 */

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#if (!(_WIN32) || (__CYGWIN__)) 
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#endif
#include "../include/gnuc.h"
#ifdef HAVE_OS_PROTO_H
#include "../include/os-proto.h"
#endif


struct libnet_link_int *
libnet_open_link_interface(int8_t *device, int8_t *ebuf)
{
    struct strioctl si;	    /* struct for ioctl() */
    struct ifreq ifr;       /* interface request struct */
    static int8_t dev[] = "/dev/nit";
    register struct libnet_link_int *l;

    l = (struct libnet_link_int *)malloc(sizeof(*l));
    if (l == NULL)
    {
        strcpy(ebuf, strerror(errno));
        return (NULL);
    }

    memset(l, 0, sizeof(*l));

    l->fd  = open(dev, O_RDWR);
    if (l->fd < 0)
    {
        sprintf(ebuf, "%s: %s", dev, strerror(errno));
        goto bad;
    }

    /*
     *  arrange to get discrete messages from the STREAM and use NIT_BUF
     */
    if (ioctl(l->fd, I_SRDOPT, (int8_t *)RMSGD) < 0)
    {
        sprintf(ebuf, "I_SRDOPT: %s", strerror(errno));
        goto bad;
    }
    if (ioctl(l->fd, I_PUSH, "nbuf") < 0)
    {
        sprintf(ebuf, "push nbuf: %s", strerror(errno));
        goto bad;
    }
    /*
     *  request the interface
     */
    strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name) -1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
    si.ic_cmd = NIOCBIND;
    si.ic_len = sizeof(ifr);
    si.ic_dp = (int8_t *)&ifr;
    if (ioctl(l->fd, I_STR, (int8_t *)&si) < 0)
    {
        sprintf(ebuf, "NIOCBIND: %s: %s", ifr.ifr_name, strerror(errno));
        goto bad;
    }

    ioctl(l->fd, I_FLUSH, (int8_t *)FLUSHR);
    /*
     * NIT supports only ethernets.
     */
    l->linktype = DLT_EN10MB;

    return (l);
bad:
    if (l->fd >= 0)
    {
        close(l->fd);
    }
    free(l);
    return (NULL);
}


int
libnet_close_link_interface(struct libnet_link_int *l)
{
    if (close(l->fd) == 0)
    {
        free(l);
        return (1);
    }
    else
    {
        free(l);
        return (-1);
    }
}


int
libnet_write_link_layer(struct libnet_link_int *l, const int8_t *device,
            const uint8_t *buf, int len)
{
    int c;
    struct sockaddr sa;

    memset(&sa, 0, sizeof(sa));
    strncpy(sa.sa_data, device, sizeof(sa.sa_data));

    c = sendto(l->fd, buf, len, 0, &sa, sizeof(sa));
    if (c != len)
    {
        /* err */
        return (-1);
    }
    return (c);
}
