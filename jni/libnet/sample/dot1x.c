/*
 *  $Id: dot1x.c,v 1.2 2004/01/03 20:31:01 mike Exp $
 *
 *  libnet 1.1
 *  Build a dot1x packet
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
main(int argc, char *argv[])
{
    int c;
    libnet_t *l;
    libnet_ptag_t t;
    u_char eap_dst[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x03};
    char *device = NULL;
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* Code Id Length(2) Type DesiredType */
    char payload[] = {0x01, 0x0a, 0x00, 0x7f, 0x03, 0x05};

    printf("libnet 1.1 packet shaping: dot1x\n");

    if (argc > 1)
    {
         device = argv[1];
    }
    l = libnet_init(
            LIBNET_LINK_ADV,                        /* injection type */
            device,                                 /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    t = libnet_build_802_1x(
            0,
            LIBNET_802_1X_PACKET,
            sizeof(payload),
            payload,
            sizeof(payload),
            l,
            0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build dot1x header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_autobuild_ethernet(
            eap_dst,                                /* ethernet destination */
            ETHERTYPE_EAP,                          /* protocol type */
            l);                                     /* libnet handle */
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
        fprintf(stderr, "Wrote %d byte dot1x packet from context \"%s\"; "
                "check the wire.\n", c, libnet_cq_getlabel(l));
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

/* EOF */
