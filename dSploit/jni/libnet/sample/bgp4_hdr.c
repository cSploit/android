/*
 *
 * libnet 1.1
 * Build a BGP4 header with what you want as payload
 *
 * Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
 * All rights reserved.
 *
 *
 * Example:
 *
 *   Sending of a BGP KEEPALIVE
 *
 *   # ./bgp4_hdr -s 1.1.1.1 -d 2.2.2.2 -t 4
 *   libnet 1.1 packet shaping: BGP4 hdr + payload[raw]
 *   Wrote 59 byte TCP packet; check the wire.
 *   
 *   13:55:53.811579 1.1.1.1.26214 > 2.2.2.2.179: S [tcp sum ok] 
 *            16843009:16843028(19) win 32767: BGP (ttl 64, id 242, len 59)
 *   0x0000   4500 003b 00f2 0000 4006 73c6 0101 0101        E..;....@.s.....
 *   0x0010   0202 0202 6666 00b3 0101 0101 0202 0202        ....ff..........
 *   0x0020   5002 7fff b090 0000 0101 0101 0101 0101        P...............
 *   0x0030   0101 0101 0101 0101 0013 04                    ...........
 *   
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
    u_long src_ip, dst_ip, length;
    libnet_ptag_t t = 0;
    char errbuf[LIBNET_ERRBUF_SIZE];
    u_char *payload = NULL;
    u_long payload_s = 0;
    u_char marker[LIBNET_BGP4_MARKER_SIZE];
    u_char type;

    printf("libnet 1.1 packet shaping: BGP4 hdr + payload[raw]\n");

    l = libnet_init(
            LIBNET_RAW4,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* error buffer */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    src_ip  = 0;
    dst_ip  = 0;
    memset(marker, 0x1, LIBNET_BGP4_MARKER_SIZE);
    type = 0;

    while ((c = getopt(argc, argv, "d:s:t:m:p:")) != EOF)
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
                if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;

            case 's':
                if ((src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad source IP address: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;

	    case 'm':
		memcpy(marker, optarg, LIBNET_BGP4_MARKER_SIZE);
		break;

	    case 't':
		type = atoi(optarg);
		break;

	    case 'p':
		payload = optarg;
		payload_s = strlen(optarg);
		break;

            default:
                exit(EXIT_FAILURE);
        }
    }

    if (!src_ip || !dst_ip)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }


    length = LIBNET_BGP4_HEADER_H + payload_s;
    t = libnet_build_bgp4_header(
	marker,                                     /* marker */   
	length,                                     /* length */
	type,                                       /* message type */
        payload,                                    /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build BGP4 header: %s\n", libnet_geterror(l));
        goto bad;
    }

    length+=LIBNET_TCP_H;
    t = libnet_build_tcp(
        0x6666,                                     /* source port */
        179,                                        /* destination port */
        0x01010101,                                 /* sequence number */
        0x02020202,                                 /* acknowledgement num */
        TH_SYN,                                     /* control flags */
        32767,                                      /* window size */
        0,                                          /* checksum */
        0,                                          /* urgent pointer */
	length,                                     /* TCP packet size */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    length+=LIBNET_IPV4_H;
    t = libnet_build_ipv4(
        length,                                     /* length */
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
        "usage: %s -s source_ip -d destination_ip"
        " [-m marker] [-p payload] [-t BGP type]\n",
        name);
}

/* EOF */
