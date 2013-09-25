/*
 *  $Id: ntp.c,v 1.3 2004/02/20 18:53:49 mike Exp $
 *
 *  libnet 1.1
 *  Build an NTP Packet
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
main(int argc, char **argv)
{
    int c;
    libnet_t *l;
    u_long dst_ip;
    libnet_ptag_t t;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 NTP packet shaping[raw -- autobuilding IP]\n");

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_RAW4,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* error buf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    dst_ip  = 0;
    while((c = getopt(argc, argv, "d:")) != EOF)
    {
        switch (c)
        {
            /*
             *  We expect the input to be of the form `ip.ip.ip.ip.port`.  We
             *  point cp to the last dot of the IP address/port string and
             *  then seperate them with a NULL byte.  The optarg now points to
             *  just the IP address, and cp points to the port.
             */
            case 'd':
                if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                    exit(1);
                }
                break;
        }
    }
    if (!dst_ip)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /*
     *  Build the packet, remmebering that order IS important.  We must
     *  build the packet from lowest protocol type on up as it would
     *  appear on the wire.  So for our NTP packet:
     *
     *  --------------------------------------------------------------------
     *  |      IP           |  UDP   |              NTP                    |   
     *  --------------------------------------------------------------------
     *         ^                 ^                      ^
     *         |--------------   |                      |
     *  libnet_build_ipv4()--|   |                      |
     *                           |                      |
     *  libnet_build_udp()-------|                      |
     *                                                  |
     *  libnet_build_ntp()------------------------------|
     */
    t = libnet_build_ntp(
        LIBNET_NTP_LI_AC,                           /* leap indicator */
        LIBNET_NTP_VN_4,                            /* version */
        LIBNET_NTP_MODE_S,                          /* mode */
        LIBNET_NTP_STRATUM_PRIMARY,                 /* stratum */
        4,                                          /* poll interval */
        1,                                          /* precision */
        0xffff,                                     /* delay interval */
        0xffff,                                     /* delay fraction */
        0xffff,                                     /* dispersion interval */
        0xffff,                                     /* dispersion fraction */
        LIBNET_NTP_REF_PPS,                         /* reference id */
        0x11,                                       /* reference ts int */
        0x22,                                       /* reference ts frac */
        0x33,                                       /* originate ts int */
        0x44,                                       /* originate ts frac */
        0x55,                                       /* receive ts int */
        0x66,                                       /* receive ts frac */
        0x77,                                       /* transmit ts interval */
        0x88,                                       /* transmit ts fraction */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build NTP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    libnet_seed_prand(l);
    t = libnet_build_udp(
        libnet_get_prand(LIBNET_PRu16),             /* source port */
        123,                                        /* NTP port */
        LIBNET_UDP_H + LIBNET_NTP_H,                /* UDP packet length */
        0,                                          /* checksum */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_autobuild_ipv4(
        LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_NTP_H,/* packet length */
        IPPROTO_UDP,
        dst_ip,
        l);
    if (t == -1)
    {
        fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    /*
     *  Write it to the wire.
     */

    fprintf(stderr, "l contains a %d byte packet\n", libnet_getpacket_size(l));
    c = libnet_write(l);
    if (c == -1)
    {
        fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
        goto bad;
    }
    else
    {
        fprintf(stderr, "Wrote %d byte NTP packet; check the wire.\n", c);
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}


void
usage(char *name)
{
    fprintf(stderr,
        "usage: %s -d destination_ip\n",
        name);
}

/* EOF */
