/*
 *  $Id: cdp.c,v 1.3 2004/11/09 07:05:07 mike Exp $
 *
 *  cdppoke
 *  CDP information injection tool
 *  Released as part of the MXFP Layer 2 Toolkit
 *  http://www.packetfactory.net/MXFP
 *
 *  Copyright (c) 2004 Mike D. Schiffman  <mike@infonexus.com>
 *  Copyright (c) 2004 Jeremy Rauch       <jrauch@cadre.org>
 *
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
#if ((_WIN32) && !(__CYGWIN__)) 
#include "../include/win32/config.h"
#else
#include "../include/config.h"
#endif
#endif
#include "./libnet_test.h"


int
main(int argc, char *argv[])
{
    int c, len, index;
    libnet_t *l;
    libnet_ptag_t t;
    u_char *value;
    u_char values[100];
    u_short tmp;
    char errbuf[LIBNET_ERRBUF_SIZE];
    uint8_t oui[3] = { 0x00, 0x00, 0x0c };
    uint8_t cdp_mac[6] = {0x01, 0x0, 0xc, 0xcc, 0xcc, 0xcc};

    if (argc != 3)
    {
        fprintf(stderr, "usage %s device device-id\n", argv[0]);
        return (EXIT_FAILURE);
    }

    fprintf(stderr, "cdppoke...\n");

    l = libnet_init(LIBNET_LINK, argv[1], errbuf);
    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        return (EXIT_FAILURE);
    }

    /* build the TLV's by hand until we get something better */
    memset(values, 0, sizeof(values));
    index = 0;
 
    tmp = htons(LIBNET_CDP_VERSION);
    memcpy(values, &tmp, 2);
    index += 2;
    tmp = htons(9); /* length of string below plus type and length fields */
    memcpy(values + index, &tmp, 2);
    index += 2;
    memcpy(values + index, (u_char *)"1.1.1", 5);
    index += 5;

    /* this TLV is handled by the libnet builder */
    value = argv[2];
    len = strlen(argv[2]);

    /* build CDP header */
    t = libnet_build_cdp(
        1,                                      /* version */
        30,                                     /* time to live */
        0x0,                                    /* checksum */
        0x1,                                    /* type */
        len,                                    /* length */
        value,                                  /* value */
        values,                                 /* payload */
        index,                                  /* payload size */
        l,                                      /* libnet context */
        0);                                     /* libnet ptag */
    if (t == -1)
    {
        fprintf(stderr, "Can't build CDP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    /* build 802.2 header */ 
    t = libnet_build_802_2snap(
        LIBNET_SAP_SNAP,                       /* SAP SNAP code */
        LIBNET_SAP_SNAP,                       /* SAP SNAP code */
        0x03,                                  /* control */
	oui,                                   /* OUI */
        0x2000,                                /* upper layer protocol type */ 
        NULL,                                  /* payload */
        0,                                     /* payload size */
        l,                                     /* libnet context */
        0);                                    /* libnet ptag */
    if (t == -1)
    {
        fprintf(stderr, "Can't build SNAP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    /* build 802.3 header */ 
    t = libnet_build_802_3(
            cdp_mac,                           /* ethernet destination */
            (uint8_t *)libnet_get_hwaddr(l),  /* ethernet source */
            LIBNET_802_2_H + LIBNET_802_2SNAP_H + LIBNET_CDP_H,   /* packet len */
            NULL,                              /* payload */
            0,                                 /* payload size */
            l,                                 /* libnet context */
            0);                                /* libnet ptag */
    if (t == -1)
    {
        fprintf(stderr, "Can't build 802.3 header: %s\n", libnet_geterror(l));
        goto bad;
    }

    /* write the packet out */
    c = libnet_write(l);
    if (c == -1)
    {
        fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
        goto bad;
    }
    else
    {
        fprintf(stderr, "Wrote %d byte CDP frame \"%s\"\n", c, argv[2]);
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

/* EOF */
