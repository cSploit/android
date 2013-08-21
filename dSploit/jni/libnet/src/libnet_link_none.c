/*
 *  $Id: libnet_link_none.c,v 1.5 2004/01/03 20:31:02 mike Exp $
 *
 *  libnet
 *  libnet_none.c - dummy routines for suckers with no link-layer interface
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Copyright (c) 1993, 1994, 1995, 1996, 1998
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

static void nosupport(libnet_t* l)
{
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): no link support on this platform\n", __func__);
}

int
libnet_open_link(libnet_t *l)
{
    nosupport(l);
    return -1;
}


int
libnet_close_link(libnet_t *l)
{
    nosupport(l);
    return -1;
}


int
libnet_write_link(libnet_t *l, const uint8_t *packet, uint32_t size)
{
    nosupport(l);
    return -1;
}


struct libnet_ether_addr *
libnet_get_hwaddr(libnet_t *l)
{
    nosupport(l);
    return NULL;
}

