/*
 *  $Id: libnet_link_nit.c,v 1.6 2004/01/03 20:31:02 mike Exp $
 *
 *  libnet
 *  libnet_nit.c - network interface tap routines
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
    struct sockaddr_nit snit;
    register struct libnet_link_int *l;

    l = (struct libnet_link_int *)malloc(sizeof(*p));
    if (l == NULL)
    {
        strcpy(ebuf, strerror(errno));
        return (NULL);
    }

    memset(l, 0, sizeof(*l));

    l->fd = socket(AF_NIT, SOCK_RAW, NITPROTO_RAW);
    if (l->fd < 0)
    {
        sprintf(ebuf, "socket: %s", strerror(errno));
        goto bad;
    }
    snit.snit_family = AF_NIT;
	strncpy(snit.snit_ifname, device, NITIFSIZ -1);
    snit.snit_ifname[NITIFSIZ] = '\0';

    if (bind(l->fd, (struct sockaddr *)&snit, sizeof(snit)))
    {
        sprintf(ebuf, "bind: %s: %s", snit.snit_ifname, strerror(errno));
        goto bad;
    }

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
write_link_layer(struct libnet_link_int *l, const int8_t *device,
            uint8_t *buf, int len)
{
    int c;
    struct sockaddr sa;

    memset(&sa, 0, sizeof(sa));
    strncpy(sa.sa_data, device, sizeof(sa.sa_data));

    c = sendto(l->fd, buf, len, 0, &sa, sizeof(sa));
    if (c != len)
    {
        /* error */
    }
    return (c);
}

