/*
 *
 * libnet 1.1
 * Build a IPv4 packet with what you want as payload
 *
 * Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
 * All rights reserved.
 *
 * Ex:
 * - send an UDP packet from port 4369 to port 8738
 *    ./ip -s 1.1.1.1 -d 2.2.2.2
 *
 * - send a TCP SYN from port 4369 to port 8738
 *   ./ip -s 1.1.1.1 -d 2.2.2.2 -t -p `printf "\x04\x57\x08\xae\x01\x01\x01\x01\x02\x02\x02\x02\x50\x02\x7f\xff\xd5\x91\x41\x41"`
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
    char *device = NULL;
    char *dst = "2.2.2.2", *src = "1.1.1.1";
    u_long src_ip, dst_ip;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ip_ptag = 0;
    u_short proto = IPPROTO_UDP;
    u_char payload[255] = {0x11, 0x11, 0x22, 0x22, 0x00, 0x08, 0xc6, 0xa5};
    u_long payload_s = 8;

    printf("libnet 1.1 packet shaping: IP + payload[raw]\n");

    /*
     * handle options
     */ 
    while ((c = getopt(argc, argv, "d:s:tp:i:h")) != EOF)
    {
        switch (c)
        {
            case 'd':
		dst = optarg;
                break;

            case 's':
		src = optarg;
                break;

	    case 'i':
		device = optarg;
		break;

	    case 't':
		proto = IPPROTO_TCP;
		break;

	    case 'p':
		strncpy(payload, optarg, sizeof(payload)-1);
		payload_s = strlen(payload);
		break;

	    case 'h':
		usage(argv[0]);
		exit(EXIT_SUCCESS);

            default:
                exit(EXIT_FAILURE);
        }
    }


    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
	    LIBNET_RAW4,                            /* injection type */
	    device,                                 /* network interface */
            errbuf);                                /* error buffer */

    printf("Using device %s\n", l->device);

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    if ((dst_ip = libnet_name2addr4(l, dst, LIBNET_RESOLVE)) == -1)
    {
	fprintf(stderr, "Bad destination IP address: %s\n", dst);
	exit(EXIT_FAILURE);
    }
    
    if ((src_ip = libnet_name2addr4(l, src, LIBNET_RESOLVE)) == -1)
    {
	fprintf(stderr, "Bad source IP address: %s\n", src);
	exit(EXIT_FAILURE);
    }
    

    /*
     * Build the packet
     */ 
    ip_ptag = libnet_build_ipv4(
        LIBNET_IPV4_H + payload_s,                  /* length */
        0,                                          /* TOS */
        242,                                        /* IP ID */
        0,                                          /* IP Frag */
        64,                                         /* TTL */
        proto,                                      /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        payload,                                    /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        ip_ptag);                                   /* libnet id */
    if (ip_ptag == -1)
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
        fprintf(stderr, "Wrote %d byte IP packet; check the wire.\n", c);
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
        "usage: %s [-s source_ip] [-d destination_ip]"
	    " [-i iface] [-p payload] [-t]\n",
	    name);
}

/* EOF */
