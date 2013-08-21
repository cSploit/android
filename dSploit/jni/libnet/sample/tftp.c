/*
 *
 * libnet 1.1
 * Build a TFTP scanner using payload
 *
 * Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
 * All rights reserved.
 *
 * Ex:
 *    ./tftp -s 192.168.0.1 -d 192.168.0.66 -p plop
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
    u_long src_ip, dst_ip;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t udp = 0, ip = 0;
    char *filename = "/etc/passwd";
    char mode[] = "netascii";
    u_char *payload = NULL;
    uint payload_s = 0;
    

    printf("libnet 1.1 packet shaping: UDP + payload[raw] == TFTP\n");

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
	    LIBNET_RAW4,                  /* injection type */
            NULL,                         /* network interface */
            errbuf);                      /* error buffer */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    src_ip  = 0;
    dst_ip  = 0;
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
                if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);
		    goto bad;
                }
                break;

            case 's':
                if ((src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad source IP address: %s\n", optarg);
		    goto bad;
                }
                break;

	    case 'p':
		filename = optarg;
		break;

            default:
		fprintf(stderr, "unkown option [%s]: bye bye\n", optarg);
		goto bad;

        }
    }

    if (!src_ip || !dst_ip)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 
     * build payload
     *
     *      2 bytes     string    1 byte     string   1 byte
     *       ------------------------------------------------
     *      | Opcode |  Filename  |   0  |    Mode    |   0  |
     *       ------------------------------------------------
     *
     */
    payload_s = 2 + strlen(filename) + 1 + strlen(mode) + 1;
    payload = malloc(sizeof(char)*payload_s);
    if (!payload)
    {
        fprintf(stderr, "malloc error for payload\n");
        goto bad;
    }
    memset(payload, 0, payload_s);
    payload[1] = 1; /* opcode - GET */
    memcpy(payload + 2, filename, strlen(filename));
    memcpy(payload + 2 +  strlen(filename) + 1 , mode, strlen(mode));
    
    /*
     * Build pblocks
     */
    udp = libnet_build_udp(
	0x1234,                           /* source port */
	69,                               /* destination port */
	LIBNET_UDP_H + payload_s,         /* packet length */
	0,                                /* checksum */
	payload,                          /* payload */
	payload_s,                        /* payload size */
	l,                                /* libnet handle */
	0);                               /* libnet id */
    if (udp == -1)
    {
	fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
	goto bad;
    }

    ip = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_UDP_H + payload_s, /* length - dont forget the UDP's payload */
        0,                                /* TOS */
        0x4242,                           /* IP ID */
        0,                                /* IP Frag */
        0x42,                             /* TTL */
        IPPROTO_UDP,                      /* protocol */
        0,                                /* checksum */
        src_ip,                           /* source IP */
        dst_ip,                           /* destination IP */
        NULL,                             /* payload (already in UDP) */
        0,                                /* payload size */
        l,                                /* libnet handle */
        0);                               /* libnet id */
    if (ip == -1)
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
        fprintf(stderr, "Wrote %d byte TFTP packet; check the wire.\n", c);
    }

    libnet_destroy(l);
    free(payload);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    free(payload);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    fprintf(stderr,
        "usage: %s -s source_ip -d destination_ip"
        " [-p payload] [-t|u|i] \n",
        name);
}

/* EOF */
