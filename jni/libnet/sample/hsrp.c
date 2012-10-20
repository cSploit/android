/*
 *
 *  libnet 1.1.2
 *  Build a HSRP packet
 *
 *  Copyright (c) 2004 David Barroso Berrueta <tomac@wasahero.org>
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
#if ((_WIN32) && !(__CYGWIN__)) 
#include "../include/win32/config.h"
#else
#include "../include/config.h"
#endif
#endif
#include "./libnet_test.h"


int
main(int argc, char *argv[])
{
    uint8_t version = LIBNET_HSRP_VERSION;
    uint8_t opcode = LIBNET_HSRP_TYPE_HELLO;
    uint8_t state = LIBNET_HSRP_STATE_ACTIVE;
    uint8_t hello_time = 3;
    uint8_t hold_time = 10;
    uint8_t priority = 1;
    uint8_t group = 1;
    uint8_t reserved = 0;
    uint8_t authdata[8];

    libnet_t *l;
    char *device = NULL;
    char src[] = "1.1.1.1";
    char *eth_dst = "DE:AD:00:00:BE:EF";
    char dst[] = "224.0.0.2";
    int port = 1985;
    int c;
    u_long src_ip, dst_ip;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ptag = 0;

    printf("libnet 1.1.2 packet shaping: HSRP[link]\n"); 

    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
	    LIBNET_LINK_ADV,                        /* injection type */
	    device,                                 /* network interface */
            errbuf);                                /* error buffer */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    printf("Using device %s\n", l->device);

    if ((dst_ip = libnet_name2addr4(l, dst, LIBNET_RESOLVE)) == (u_long)-1)
    {
	fprintf(stderr, "Bad destination IP address: %s\n", dst);
	exit(EXIT_FAILURE);
    }
    
    if ((src_ip = libnet_name2addr4(l, src, LIBNET_RESOLVE)) == (u_long)-1)
    {
	fprintf(stderr, "Bad source IP address: %s\n", src);
	exit(EXIT_FAILURE);
    }

    memset(authdata, 0, 8);
    strncpy(authdata, "cisco", 5);


    ptag = libnet_build_hsrp(
	version,
	opcode,
	state,
	hello_time,                        /* Recommended hello time */
	hold_time,                       /* Recommended hold time */
	priority,                        /* Priority */
	group,                        /* Group */
	reserved,                        /* Reserved */
	authdata,                  /* Password */
        src_ip,                   /* Virtual IP */
        NULL,
        0,
	l,
	0
	);

    if (ptag == -1)
    {
	fprintf(stderr, "Can't build HSRP header: %s\n", libnet_geterror(l));
	goto bad;
    }

    ptag = libnet_build_udp(
	port,                                      /* source port */
	port,                                      /* destination port */
	LIBNET_UDP_H + LIBNET_HSRP_H ,             /* packet length */
	0,                                         /* checksum */
	NULL,                                      /* payload */
	0,                                         /* payload size */
	l,                                         /* libnet handle */
	0);                                        /* libnet id */

    if (ptag == -1)
    {
	fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
	goto bad;
    }

    ptag = libnet_build_ipv4(
	LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_HSRP_H,/* length */
	0,                                          /* TOS */
	666,                                        /* IP ID */
	0,                                          /* IP Frag */
	1 ,                                         /* TTL */
	IPPROTO_UDP,                                /* protocol */
	0,                                          /* checksum */
	src_ip,                                     /* source IP */
	dst_ip,                                     /* destination IP */
	NULL,                                       /* payload */
	0,                                          /* payload size */
	l,                                          /* libnet handle */
	0);                                         /* libnet id */
    
    if (ptag == -1)
    {
	fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
	exit(EXIT_FAILURE);
    }

    
    eth_dst = libnet_hex_aton(eth_dst, &c);
    ptag = libnet_autobuild_ethernet(
	eth_dst,                                /* ethernet destination */
	ETHERTYPE_IP,                           /* protocol type */
	l);                                     /* libnet handle */

    free(eth_dst);
    if (ptag == -1)
    {
        fprintf(stderr, "Can't build ethernet header: %s\n",
                libnet_geterror(l));
        goto bad;
    }


    /*
     * write to the wire
     */
    c = libnet_write(l);
    if (c == -1)
    {
        fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
        goto bad;
    }
    else
    {
        fprintf(stderr, "Wrote %d byte HSRP packet; check the wire.\n", c);
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
  bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);

    return 0;
}
