/*
 *  $Id: ospf_hello.c,v 1.3 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet 1.1
 *  Build an OSPF Hello packet
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 *  Copyright (c) 1999, 2000 Andrew Reiter <areiter@bindview.com>
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
    u_long src, dst;
    char errbuf[LIBNET_ERRBUF_SIZE];
    char *to, *from;
    u_char auth[8] = {0,0,0,0,0,0,0,0};


    printf("libnet 1.1 OSPF Hello packet shaping[raw]\n");

    if (argc != 3) 
    {
        usage(argv[0]);
    }

    from        = argv[1];
    to          = argv[2];

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_RAW4,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* errbuf */
 
    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    /* Too lazy to check for error */
    src = libnet_name2addr4(l, from, LIBNET_DONT_RESOLVE);
    dst = libnet_name2addr4(l, to, LIBNET_DONT_RESOLVE);

    t = libnet_build_ospfv2_hello(
        0xffffffff,                                 /* netmask */
        2,                                          /* interval */
        0x00,                                       /* options */
        0x00,                                       /* priority */
        30,                                         /* dead int */
        src,                                        /* router */
        src,                                        /* router */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build OSPF HELLO header: %s\n", libnet_geterror(l));
        goto bad;
    }

    /* authentication data */
    t = libnet_build_data(
        auth,                                       /* auth data */
        LIBNET_OSPF_AUTH_H,                         /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build OSPF auth header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_ospfv2(
        LIBNET_OSPF_HELLO_H + LIBNET_OSPF_AUTH_H,   /* OSPF packet length */
        LIBNET_OSPF_HELLO,                          /* OSPF packet type */
        htonl(0xd000000d),                          /* router id */
        htonl(0xc0ffee00),                          /* area id */
        0,                                          /* checksum */
        LIBNET_OSPF_AUTH_NULL,                      /* auth type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */

    if (t == -1)
    {
        fprintf(stderr, "Can't build OSPF header: %s\n", libnet_geterror(l));
        goto bad;
    }
  
    t = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_OSPF_H +
        LIBNET_OSPF_HELLO_H + LIBNET_OSPF_AUTH_H,   /* packet total legnth */
        0,                                          /* TOS */
        101,                                        /* IP iD */
        IP_DF,                                      /* IP frag */
        254,                                        /* TTL */
        IPPROTO_OSPF,                               /* protocol */
        0,                                          /* checksum */
        src,                                        /* source IP */
        dst,                                        /* destination IP */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
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
        fprintf(stderr, "Wrote %d byte OSPF packet; check the wire.\n", c);
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}


void
usage(char *pname)
{
    printf("Usage: %s <source ip> <dest. ip> <neighbor>\n", pname);
    exit(EXIT_SUCCESS);
}
