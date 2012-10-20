/*
 *  $Id: icmp_timeexceed.c,v 1.3 2004/03/16 18:40:58 mike Exp $
 *
 *  libnet 1.1
 *  Build an ICMP timeexceed packet
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
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
    libnet_ptag_t t;
    u_long src_ip, dst_ip; 
    u_char payload[8] = {0x11, 0x11, 0x22, 0x22, 0x00, 0x08, 0xc6, 0xa5};
    u_long payload_s = 8;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 packet shaping: ICMP timeexceed[link]\n"); 

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* errbuf */
 
    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    src_ip = 0;
    dst_ip = 0;
    while((c = getopt(argc, argv, "d:s:")) != EOF)
    {
        switch (c)
        {
            case 'd':
                if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                    exit(1);
                }
                break;
            case 's':
                if ((src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad source IP address: %s\n", optarg);
                    exit(1);
                }
                break;
        }
    }
    if (!src_ip || !dst_ip)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    t = libnet_build_ipv4(
        LIBNET_IPV4_H + payload_s,                  /* length */
        IPTOS_LOWDELAY | IPTOS_THROUGHPUT,          /* TOS */
        0xee,                                       /* IP ID */
        0,                                          /* IP Frag */
        64,                                         /* TTL */
        IPPROTO_UDP,                                /* protocol */
        0,                                          /* checksum */
        dst_ip,                                     /* source IP */
        src_ip,                                     /* destination IP */
        payload,                                    /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build error IP header: %s\n", 
               libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_icmpv4_timeexceed(
        ICMP_TIMXCEED,                              /* type */
        ICMP_TIMXCEED_INTRANS,                      /* code */
        0,                                          /* checksum */
        NULL,
        0,
        l,                                          /* libnet handle */
        0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build ICMP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_ICMPV4_TIMXCEED_H +
        LIBNET_IPV4_H + payload_s,                  /* length */
        IPTOS_LOWDELAY | IPTOS_THROUGHPUT,          /* TOS */
        0xee,                                       /* IP ID */
        0,                                          /* IP Frag */
        64,                                         /* TTL */
        IPPROTO_ICMP,                               /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_ethernet(
        enet_dst,                                   /* ethernet destination */
        enet_src,                                   /* ethernet source */
        ETHERTYPE_IP,                               /* protocol type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
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
        fprintf(stderr, "Wrote %d byte ICMP packet; check the wire.\n", c);
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
    fprintf(stderr, "usage: %s -s source_ip -d destination_ip\n ", name);
}

/* EOF */
