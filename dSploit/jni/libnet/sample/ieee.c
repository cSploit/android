/*
 *  $Id: ieee.c,v 1.2 2004/01/03 20:31:01 mike Exp $
 *
 *  libnet 1.1
 *  Build an ieee 802.1q/ARP packet
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
#include "./libnet_test.h"

int
main(int argc, char *argv[])
{
    int c, len, do_802_2;
    u_long i;
    libnet_t *l;
    libnet_ptag_t t;
    u_char *dst, *src, oui[3];
    char *device = NULL;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 packet shaping: ieee[802.1q / 802.2 / ARP]\n"); 

    do_802_2 = 0;
    device = NULL;
    src = dst = NULL;
    while ((c = getopt(argc, argv, "8d:i:s:")) != EOF)
    {
        switch (c)
        {
            case '8':
                do_802_2 = 1;
                break;
            case 'd':
                dst = libnet_hex_aton(optarg, &len);
                break;
            case 'i':
                device = optarg;
                break;
            case 's':
                src = libnet_hex_aton(optarg, &len);
                break;
        }
    }

    if (src == NULL || dst == NULL)
    {
        fprintf(stderr, "usage %s -d dst -s src [-8 ] [-i interface]\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            device,                                 /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    i = libnet_get_ipaddr4(l);
     
    t = libnet_build_arp(
            ARPHRD_ETHER,                           /* hardware addr */
            ETHERTYPE_IP,                           /* protocol addr */
            6,                                      /* hardware addr size */
            4,                                      /* protocol addr size */
            ARPOP_REPLY,                            /* operation type */
            src,                                    /* sender hardware addr */
            (u_char *)&i,                           /* sender protocol addr */
            dst,                                    /* target hardware addr */
            (u_char *)&i,                           /* target protocol addr */
            NULL,                                   /* payload */
            0,                                      /* payload size */
            l,                                      /* libnet handle */
            0);                                     /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build ARP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    if (do_802_2)
    {
        memset(&oui, 0, 3);
        t = libnet_build_802_2snap(
            0xaa,                                   /* SNAP DSAP */
            0xaa,                                   /* SNAP SSAP */
            0,                                      /* control */
            oui,                                    /* oui */
            ETHERTYPE_ARP,                          /* ARP header follows */
            NULL,                                   /* payload */
            0,                                      /* payload size */
            l,                                      /* libnet handle */
            0);                                     /* libnet id */
        if (t == -1)
        {
            fprintf(stderr, "Can't build 802.2 header: %s\n", libnet_geterror(l));
            goto bad;
        }
    }
    t = libnet_build_802_1q(
            dst,                                    /* dest mac */
            src,                                    /* source mac */
            ETHERTYPE_VLAN,                         /* TPI */
            0x006,                                  /* priority (0 - 7) */
            0x001,                                  /* CFI flag */
            0x100,                                  /* vid (0 - 4095) */
            do_802_2 ? LIBNET_802_2SNAP_H + LIBNET_ARP_ETH_IP_H : 0x0806,
            NULL,                                   /* payload */
            0,                                      /* payload size */
            l,                                      /* libnet handle */
            0);                                     /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build 802.1q header: %s\n", libnet_geterror(l));
        goto bad;
    }
    /*
     *  Write it to the wire.
     */
    c = libnet_write(l);

    if (c == -1)
    {
        fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
        goto bad;
    }
    else
    {
        fprintf(stderr, "Wrote %d byte 802.1q packet; check the wire.\n", c);
    }
    free(dst);
    free(src);
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    free(dst);
    free(src);
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

/* EOF */
