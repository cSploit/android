/*
 *  $Id: stp.c,v 1.3 2004/01/21 19:01:29 mike Exp $
 *
 *  libnet 1.1
 *  Build an STP frame
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

#define CONF    1
#define TCN     2

int
main(int argc, char *argv[])
{
    int c, len, type;
    libnet_t *l;
    libnet_ptag_t t;
    u_char *dst, *src;
    u_char rootid[8], bridgeid[8];
    char *device = NULL;
    char errbuf[LIBNET_ERRBUF_SIZE];

    printf("libnet 1.1 packet shaping: [STP]\n"); 

    device = NULL;
    src = dst = NULL;
    type = CONF;
    while ((c = getopt(argc, argv, "cd:i:s:t")) != EOF)
    {
        switch (c)
        {
            case 'c':
                type = CONF;
                break;
            case 'd':
                dst = libnet_hex_aton(optarg, &len);
                break;
            case 'i':
                device = optarg;
                break;
            case 's':
                src = libnet_hex_aton(optarg, &len);
                break;
            case 't':
                type = TCN;
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (src == NULL || dst == NULL)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
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

    if (type == CONF)
    {
        rootid[0] = 0x80;
        rootid[1] = 0x00;
        rootid[2] = 0x00;
        rootid[3] = 0x07;
        rootid[4] = 0xec;
        rootid[5] = 0xae;
        rootid[6] = 0x30;
        rootid[7] = 0x41;

        bridgeid[0] = 0x80;
        bridgeid[1] = 0x00;
        bridgeid[2] = 0x00;
        bridgeid[3] = 0x07;
        bridgeid[4] = 0xec;
        bridgeid[5] = 0xae;
        bridgeid[6] = 0x30;
        bridgeid[7] = 0x41;

        t = libnet_build_stp_conf(
            0x0000,                             /* protocol id */
            0x00,                               /* protocol version */
            0x00,                               /* BPDU type */
            0x00,                               /* BPDU flags */
            rootid,                             /* root id */
            0x00000001,                         /* root path cost */
            bridgeid,                           /* bridge id */
            0x8002,                             /* port id */
            0x00,                               /* message age */
            0x0014,                             /* max age */
            0x0002,                             /* hello time */
            0x000f,                             /* forward delay */
            NULL,                               /* payload */
            0,                                  /* payload size */
            l,                                  /* libnet handle */
            0);                                 /* libnet id */
        if (t == -1)
        {
            fprintf(stderr, "Can't build STP conf header: %s\n",
                    libnet_geterror(l));
            goto bad;
        }
    }
    else
    {
        t = libnet_build_stp_tcn(
            0x0000,                             /* protocol id */
            0x00,                               /* protocol version */
            0x80,                               /* BPDU type */
            NULL,                               /* payload */
            0,                                  /* payload size */
            l,                                  /* libnet handle */
            0);                                 /* libnet id */
        if (t == -1)
        {
            fprintf(stderr, "Can't build STP tcn header: %s\n",
                    libnet_geterror(l));
            goto bad;
        }
    }

    t = libnet_build_802_2(
        LIBNET_SAP_STP,                         /* DSAP */   
        LIBNET_SAP_STP,                         /* SSAP */
        0x03,                                   /* control */
        NULL,                                   /* payload */  
        0,                                      /* payload size */
        l,                                      /* libnet handle */
        0);                                     /* libnet id */
    if (t == -1) 
    {
        fprintf(stderr, "Can't build ethernet header: %s\n",
                libnet_geterror(l));
        goto bad;
    }  

    t = libnet_build_802_3(
        dst,                                    /* ethernet destination */
        src,                                    /* ethernet source */
        LIBNET_802_2_H + ((type == CONF) ? LIBNET_STP_CONF_H : 
        LIBNET_STP_TCN_H),                       /* frame size */
        NULL,                                   /* payload */
        0,                                      /* payload size */
        l,                                      /* libnet handle */
        0);                                     /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build ethernet header: %s\n",
                libnet_geterror(l));
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
        fprintf(stderr, "Wrote %d byte STP packet; check the wire.\n", c);
    }
    free(dst);
    free(src);
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    free(dst);
    free(src);
    libnet_destroy(l);
    return (EXIT_FAILURE);
}


void
usage(char *name)
{
    fprintf(stderr, "usage %s -d dst -s src -t -c [-i interface]\n",
                name);
}

/* EOF */
