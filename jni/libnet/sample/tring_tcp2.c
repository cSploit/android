/*
 *  libnet 1.1
 *  Build a TCP packet to ride on token ring (autobuild)
 *
 *  Copyright (c) 1998 - 2001 Mike D. Schiffman <mike@infonexus.com>
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
    int c;
    char *cp;
    libnet_t *l;
    libnet_ptag_t t;
    char *payload;
    u_short payload_s;
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 packet shaping: TCP (over token ring) [link]\n");

    /*
     *  Initialize the library.  Root priviledges are required.
     *
     *  Currently hard-coded for tr0.
     */
    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            "tr0",                                  /* network interface */
            errbuf);                                /* error buffer */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    src_ip  = 0;
    dst_ip  = 0;
    src_prt = 0;
    dst_prt = 0;
    payload = NULL;
    payload_s = 0;
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
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                    exit(EXIT_FAILURE);
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


    t = libnet_build_tcp(
        src_prt,                                    /* source port */
        dst_prt,                                    /* destination port */
        0x01010101,                                 /* sequence number */
        0x02020202,                                 /* acknowledgement num */
        TH_SYN,                                     /* control flags */
        32767,                                      /* window size */
        0,                                          /* checksum */
        0,                                          /* urgent pointer */
        LIBNET_TCP_H + payload_s,                   /* TCP packet size */
        payload,                                    /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_TCP_H + payload_s,   /* length */
        0,                                          /* TOS */
        242,                                        /* IP ID */
        0,                                          /* IP Frag */
        64,                                         /* TTL */
        IPPROTO_TCP,                                /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_autobuild_token_ring(
        LIBNET_TOKEN_RING_FRAME,
        LIBNET_TOKEN_RING_LLC_FRAME,                /* LLC - Normal buffer */
        tr_dst,                                     /* token ring destination */
        LIBNET_SAP_SNAP,                            /* DSAP -> SNAP encap */
        LIBNET_SAP_SNAP,                            /* SSAP -> SNAP encap */
        0x03,                                       /* Unnumbered info/frame */
        org_code,                                   /* Organization Code */
        TOKEN_RING_TYPE_IP,                         /* protocol type */
        l);                                         /* libnet handle */
    if (t == -1)
    {
        fprintf(stderr, "Can't build token ring header: %s\n", libnet_geterror(l));
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
        fprintf(stderr, "Wrote %d byte TCP packet; check the wire.\n", c);
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
        "usage: %s -s source_ip.source_port -d destination_ip.destination_port"
        " [-p payload]\n",
        name);
}

/* EOF */
