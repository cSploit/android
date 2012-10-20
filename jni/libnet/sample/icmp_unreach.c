/*
 *  $Id: icmp_unreach.c,v 1.4 2004/03/16 18:40:59 mike Exp $
 *
 *  libnet 1.1
 *  Build an ICMP unreachable packet
 *
 *  Hacked by Frederic Raynal <pappy@security-labs.org> to illustrate
 *  the new API of ICMP error messages fixing Aaron's bugs.
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
    int c, i;
    libnet_t *l = NULL;
    libnet_ptag_t ip_err = 0, icmp = 0, ip = 0, eth = 0;
    u_long src_ip, dst_ip; 
    u_char payload[8] = {0x11, 0x11, 0x22, 0x22, 0x00, 0x08, 0xc6, 0xa5};
    u_long payload_s = 8;
    int mode = LIBNET_LINK;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 packet shaping: ICMP unreachable[link]\n"); 

    src_ip = 0;
    dst_ip = 0;

    while((c = getopt(argc, argv, "d:s:r")) != EOF)
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
            case 'r':
                mode = LIBNET_RAW4;
                break;
        }
    }
    if (!src_ip || !dst_ip)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            mode,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* errbuf */
 
    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }
    
    for (i=0; i<255; i++)
    {
        ip_err = libnet_build_ipv4(
        LIBNET_IPV4_H + payload_s,              /* o length */
        IPTOS_LOWDELAY | IPTOS_THROUGHPUT,      /* o IP tos */
        (uint16_t)i,                           /* o IP ID */
        0,                                      /* o frag */
        64,                                     /* o TTL */
        IPPROTO_UDP,                            /* o protocol */
        0,                                      /* o checksum */
        dst_ip,                                 /* o source IP */
        src_ip,                                 /* o destination IP */
        payload,                                /* payload */
        payload_s,                              /* payload size */
        l,
        ip_err);
    if (ip_err == -1)
    {
        fprintf(stderr, "Can't build error IPv4 header: %s\n", 
                libnet_geterror(l));
        goto bad;
    }

    icmp = libnet_build_icmpv4_unreach(
        ICMP_UNREACH,                               /* type */
        ICMP_UNREACH_PORT,                          /* code */
        0,                                          /* checksum */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        icmp);
    if (icmp == -1)
    {
        fprintf(stderr, "Can't build ICMP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    ip = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_ICMPV4_UNREACH_H +
        LIBNET_IPV4_H + payload_s,                  /* length */
        IPTOS_LOWDELAY | IPTOS_THROUGHPUT,          /* TOS */
        (uint16_t)i + 1,                           /* IP ID */
        0,                                          /* IP Frag */
        64,                                         /* TTL */
        IPPROTO_ICMP,                               /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        ip);
    if (ip == -1)
    {
        fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    if (mode == LIBNET_LINK) {
    eth = libnet_build_ethernet(
       enet_dst,                                   /* ethernet destination */
        enet_src,                                   /* ethernet source */
        ETHERTYPE_IP,                               /* protocol type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        eth);                                         /* libnet id */

    if (eth == -1)
    {
        fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
        goto bad;
    }
    }
    /*
     *  Write it to the wire.
     */
    libnet_diag_dump_pblock(l);
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
    fprintf(stderr, "usage: %s [-r] -s source_ip -d destination_ip\n ", name);
}

/* EOF */
