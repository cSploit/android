/*
 *  $Id: udp1.c,v 1.6 2004/03/01 20:26:12 mike Exp $
 *
 *  libnet 1.1
 *  Build a UDP packet
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
#ifdef __WIN32__
#include "../include/win32/getopt.h"
#endif

int
main(int argc, char *argv[])
{
    int c, i, j, build_ip;
    char *cp;
    libnet_t *l;
    libnet_ptag_t ip, ipo;
    libnet_ptag_t udp;
    char *payload;
    u_short payload_s;
    struct libnet_stats ls;
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 packet shaping: UDP + IP options[raw]\n"); 

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_RAW4,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    src_ip  = 0;
    dst_ip  = 0;
    src_prt = 0;
    dst_prt = 0;
    payload = NULL;
    payload_s = 0;
    ip = ipo = udp = 0;
    while ((c = getopt(argc, argv, "d:s:p:")) != EOF)
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
                if (!(cp = strrchr(optarg, '.')))
                {
                    usage(argv[0]);
                }
                *cp++ = 0;
                dst_prt = (u_short)atoi(cp);
                if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                if (!(cp = strrchr(optarg, '.')))
                {
                    usage(argv[0]);
                }
                *cp++ = 0;
                src_prt = (u_short)atoi(cp);
                if ((src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad source IP address: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                payload = optarg;
                payload_s = strlen(payload);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (!src_ip || !src_prt || !dst_ip || !dst_prt)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    for (build_ip = 0, i = 0; i < 10; i++)
    {
        udp = libnet_build_udp(
            src_prt,                                /* source port */
            dst_prt + i,                            /* destination port */
            LIBNET_UDP_H + payload_s,               /* packet length */
            0,                                      /* checksum */
            (uint8_t*)payload,                     /* payload */
            payload_s,                              /* payload size */
            l,                                      /* libnet handle */
            udp);                                   /* libnet id */
        if (udp == -1)
        {
            fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
            goto bad;
        }

        if (1)
        {
            build_ip = 0;
            /* this is not a legal options string */
            for (j = 0; j < 20; j++)
            {
                opt[j] = libnet_get_prand(LIBNET_PR2);
            }
            ipo = libnet_build_ipv4_options(
                opt,
                20,
                l,
                ipo);
            if (ipo == -1)
            {
                fprintf(stderr, "Can't build IP options: %s\n", libnet_geterror(l));
                goto bad;
            }

            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + 20 + payload_s + LIBNET_UDP_H, /* length */
                0,                                          /* TOS */
                242,                                        /* IP ID */
                0,                                          /* IP Frag */
                64,                                         /* TTL */
                IPPROTO_UDP,                                /* protocol */
                0,                                          /* checksum */
                src_ip,
                dst_ip,
                NULL,                                       /* payload */
                0,                                          /* payload size */
                l,                                          /* libnet handle */
               ip);                                         /* libnet id */
            if (ip == -1)
            {
                fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
                goto bad;
            }
        }

        /*
         *  Write it to the wire.
         */
        fprintf(stderr, "%d byte packet, ready to go\n",
            libnet_getpacket_size(l));
        c = libnet_write(l);
        if (c == -1)
        {
            fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
            goto bad;
        }
        else
        {
            fprintf(stderr, "Wrote %d byte UDP packet; check the wire.\n", c);
        }
    }
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
    fprintf(stderr,
        "usage: %s -s source_ip.source_port -d destination_ip.destination_port"
        " [-p payload]\n",
        name);
}
/* EOF */
