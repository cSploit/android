/*
 *  $Id: icmp_echo_cq.c,v 1.3 2004/01/03 20:31:01 mike Exp $
 *
 *  libnet 1.1
 *  Build ICMP_ECHO packets using the context queue interface.
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

void usage(char *);


int
main(int argc, char **argv)
{
    libnet_t *l = NULL;
    u_long src_ip = 0, dst_ip = 0;
    u_long count = 10;
    int i, c;
    libnet_ptag_t t;
    char *payload = NULL;
    u_short payload_s = 0;
  
    char *device = NULL;
    char *pDst = NULL, *pSrc = NULL;
    char errbuf[LIBNET_ERRBUF_SIZE];
    char label[LIBNET_LABEL_SIZE];

    printf("libnet 1.1 packet shaping: ICMP[RAW using context queue]\n");

    while((c = getopt(argc, argv, "d:s:i:c:p:")) != EOF)
    {
        switch (c)
        {
        case 'd':
            pDst = optarg;
            break;
        case 's':
            pSrc = optarg;
            break;
        case 'i':
            device = optarg;
            break;
        case 'c':
            count = strtoul(optarg, 0, 10);
            break;
        case 'p':
            payload = optarg;
            payload_s = strlen(payload);
            break;
        }
    }

    if (!pSrc || !pDst)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /*
     *  Fill the context queue with "count" packets, each with their own
     *  context.
     */
    for (i = 0; i < count; i++)
    {
        l = libnet_init(
                LIBNET_RAW4,                  /* injection type */
                device,                       /* network interface */
                errbuf);                      /* errbuf */

        if (l == NULL)
        {
            /* we should run through the queue and free any stragglers */
            fprintf(stderr, "libnet_init() failed: %s", errbuf);
            exit(EXIT_FAILURE);
        }
        /*
         *  Since we need a libnet context for address resolution it is
         *  necessary to put this inside the loop.
         */
        if (!dst_ip && (dst_ip = libnet_name2addr4(l, pDst,
                LIBNET_RESOLVE)) == -1)
        {
            fprintf(stderr, "Bad destination IP address: %s\n", pDst);
            exit(1);
        }
        if (!src_ip && (src_ip = libnet_name2addr4(l, pSrc,
                LIBNET_RESOLVE)) == -1)
        {
            fprintf(stderr, "Bad source IP address: %s\n", pSrc);
            exit(1);
        }

        t = libnet_build_icmpv4_echo(
            ICMP_ECHO,                            /* type */
            0,                                    /* code */
            0,                                    /* checksum */
            0x42,                                 /* id */
            0x42,                                 /* sequence number */
            NULL,                                 /* payload */
            0,                                    /* payload size */
            l,                                    /* libnet handle */
            0);
        if (t == -1)
        {
            fprintf(stderr, "Can't build ICMP header: %s\n",
                    libnet_geterror(l));
            goto bad;
        }

        t = libnet_build_ipv4(
            LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H + payload_s, /* length */
            0,                                    /* TOS */
            0x42,                                 /* IP ID */
            0,                                    /* IP Frag */
            64,                                   /* TTL */
            IPPROTO_ICMP,                         /* protocol */
            0,                                    /* checksum */
            src_ip,                               /* source IP */
            dst_ip,                               /* destination IP */
            payload,                              /* payload */
            payload_s,                            /* payload size */
            l,                                    /* libnet handle */
            0);
        if (t == -1)
        {
            fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
            goto bad;
        }

        /* and finally, put it in the context queue */
        snprintf(label, sizeof(label)-1, "echo %d", i);
        if (libnet_cq_add(l, label) == -1)
        {
            fprintf(stderr, "add error: %s\n", libnet_geterror(l));
            goto bad;
        }
    }

    for_each_context_in_cq(l)
    {
        c = libnet_write(l);
        if (c == -1)
        {
            fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
            goto bad;
        }
        else
        {
            fprintf(stderr, "Wrote %d byte ICMP packet from context \"%s\"; "
                    "check the wire.\n", c, libnet_cq_getlabel(l));
        }
  }

    libnet_cq_destroy();
    return (EXIT_SUCCESS);
bad:
    libnet_cq_destroy();
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    fprintf(stderr, "usage: %s -s source_ip -d destination_ip"
                    " [-i iface] [-c count = 10]\n ", name);
}

/* EOF */
