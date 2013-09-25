/*
 *  $Id: get_addr.c,v 1.4 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet 1.1
 *  get_addr.c - Retrieve the MAC and IP address of an interface
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
    int c;
    u_long i;
    libnet_t *l;
    char *device = NULL;
    struct libnet_ether_addr *e;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 address getter\n");

    while ((c = getopt(argc, argv, "i:")) != EOF)
    {
        switch (c)
        {
            case 'i':
                device = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
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

    printf("Interface:\t%s\n", libnet_getdevice(l));
    e = libnet_get_hwaddr(l);
    if (e == NULL)
    {
        fprintf(stderr, "Can't get hardware address: %s\n", libnet_geterror(l));
    }
    else
    {
        printf("MAC address:\t");
        for (c = 0; c < 6; c++)
        {
            printf("%2.2x", e->ether_addr_octet[c]);
            if (c != 5)
            {
                printf(":");
            }
        }
        printf("\n");
    }

    i = libnet_get_ipaddr4(l);
    if (i == -1)
    {
        fprintf(stderr, "Can't get ip address: %s\n", libnet_geterror(l));
    }
    else
    {
        printf("IP  address:\t");
        printf("%s\n", libnet_addr2name4(i, LIBNET_DONT_RESOLVE));
    }
    exit(EXIT_SUCCESS);
}

/* EOF */
