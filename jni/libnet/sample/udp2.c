/*
 *  $Id: udp2.c,v 1.6 2004/03/01 20:26:12 mike Exp $
 *
 *  libnet 1.1
 *  Build a UDP packet using port list chains
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
    int c, build_ip;
    struct timeval r;
    struct timeval s;
    struct timeval e;
    libnet_t *l;
    libnet_ptag_t udp;
    char *payload;
    libnet_ptag_t t;
    struct libnet_stats ls;
    uint16_t payload_s;
    uint32_t src_ip, dst_ip;
    uint16_t bport, eport, cport;
    libnet_plist_t plist, *plist_p;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1.2 packet shaping: UDP2[link]\n");

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
    payload = NULL;
    payload_s = 0;
    plist_p = NULL;
    while ((c = getopt(argc, argv, "d:s:p:P:")) != EOF)
    {
        switch (c)
        {
            case 'd':
                if ((dst_ip = libnet_name2addr4(l, optarg,
                        LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);                    
                    exit(1);
                }
                break;
            case 's':
                if ((src_ip = libnet_name2addr4(l, optarg,
                        LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad source IP address: %s\n", optarg);
                    exit(1);
                }
                break;
            case 'P':
                plist_p = &plist;
                if (libnet_plist_chain_new(l, &plist_p, optarg) == -1)
                {
                    fprintf(stderr, "Bad token in port list: %s\n",
                            libnet_geterror(l));
                    exit(1);
                }
                break;
            case 'p':
                payload = optarg;
                payload_s = strlen(payload);
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!src_ip || !dst_ip || !plist_p)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    udp = 0;
#if !(__WIN32__)
    gettimeofday(&s, NULL);
#else
    /* This obviously is not as good - but it compiles now. */
    s.tv_sec = time(NULL);
    s.tv_usec = 0;
#endif

    build_ip = 1;
    while (libnet_plist_chain_next_pair(plist_p, &bport, &eport))
    {
        while (!(bport > eport) && bport != 0)
        {
            cport = bport++;
            udp = libnet_build_udp(
                1025,                               /* source port */
                cport,                              /* destination port */
                LIBNET_UDP_H + payload_s,           /* packet size */
                0,                                  /* checksum */
                payload,                            /* payload */
                payload_s,                          /* payload size */
                l,                                  /* libnet handle */
                udp);                               /* libnet id */
            if (udp == -1)
            {
                fprintf(stderr, "Can't build UDP header (at port %d): %s\n", 
                        cport, libnet_geterror(l));
                goto bad;
            }
    if (build_ip)
    {
        build_ip = 0;
        t = libnet_build_ipv4(
            LIBNET_IPV4_H + LIBNET_UDP_H + payload_s,   /* length */
            0,                                          /* TOS */
            242,                                        /* IP ID */
            0,                                          /* IP Frag */
            64,                                         /* TTL */
            IPPROTO_UDP,                                /* protocol */
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
            enet_dst,                                   /* ethernet dest */
            enet_src,                                   /* ethernet source */
            ETHERTYPE_IP,                               /* protocol type */
            NULL,                                       /* payload */
            0,                                          /* payload size */
            l,                                          /* libnet handle */
            0);
        if (t == -1)
        {
            fprintf(stderr, "Can't build ethernet header: %s\n",
                    libnet_geterror(l));
            goto bad;
        }
    }
            c = libnet_write(l); 
            if (c == -1)
            {
                fprintf(stderr, "write error: %s\n", libnet_geterror(l));
            }
            else
            {
               fprintf(stderr, "wrote %d byte UDP packet to port %d\r", c, 
                       cport);
            }
        }
        fprintf(stderr, "\n");
    }

#if !(__WIN32__)
    gettimeofday(&e, NULL);
#else
    /* This obviously is not as good - but it compiles now. */
    s.tv_sec = time(NULL);
    s.tv_usec = 0;
#endif

    libnet_timersub(&e, &s, &r);
    fprintf(stderr, "Total time spent in loop: %d.%d\n", r.tv_sec, r.tv_usec);

    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %lld\n"
                    "Packet errors: %lld\n"
                    "Bytes written: %lld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}


void
usage(char *name)
{
    fprintf(stderr, "usage: %s -s s_ip -d d_ip -P port list [-p payload]\n", name);
}

/* EOF */
