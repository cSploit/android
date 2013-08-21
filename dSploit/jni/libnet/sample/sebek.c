/*
 *
 *  libnet 1.1
 *  Build a Sebek packet
 *
 *  Copyright (c) 2004 Frederic Raynal <pappy@security-labs.org>
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

void usage(char *name)
{
    fprintf(stderr,
           "usage: %s [-D eth_dst] [-s source_ip] [-d destination_ip]"
           "[-u UDP port] [-m magic] [-v version] [-t type] [-S sec] [-U usec] [-P PID] [-I UID] [-f FD] [-c cmd]"
           " [-i iface] [-p payload]\n",
           name);

}


int
main(int argc, char *argv[])
{
    int c, port = 1101;
    libnet_t *l;
    char *device = NULL;
    char *eth_dst = "11:11:11:11:11:11";
    char *dst = "2.2.2.2", *src = "1.1.1.1";
    u_long src_ip, dst_ip;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ptag = 0;
    u_char *payload = 0;
    char payload_flag = 0;
    u_long payload_s = 0;
    unsigned int magic = 0x0defaced, 
	counter = 0x12345678,
	sec = 0, usec = 0,
	pid = 1,
	uid = 666,
	fd = 2;
    char *cmd = "./h4ckw0r1D";
    unsigned int length = strlen(cmd)+1;
    unsigned short version = SEBEK_PROTO_VERSION, type = SEBEK_TYPE_READ;

    printf("libnet 1.1 packet shaping: Sebek[link]\n"); 


    /*
     * handle options
     */ 
    while ((c = getopt(argc, argv, "D:d:s:u:m:v:t:S:U:P:I:f:c:p:i:h")) != EOF)
    {
        switch (c)
        {
            case 'D':
		eth_dst = optarg;
                break;
            case 'd':
		dst = optarg;
                break;

            case 's':
		src = optarg;
                break;

	    case 'i':
		device = optarg;
		break;

	    case 'u':
		port = atoi(optarg);
		break;

	    case 'm':
		magic = strtoul(optarg, NULL, 10);
		break;

	    case 'v':
		version = (unsigned short) strtoul(optarg, NULL, 10);
		break;

	    case 't':
		type = (unsigned short) strtoul(optarg, NULL, 10);
		break;

	    case 'S':
		sec = strtoul(optarg, NULL, 10);
		break;

	    case 'U':
		usec = strtoul(optarg, NULL, 10);
		break;

	    case 'P':
		pid = strtoul(optarg, NULL, 10);
		break;

	    case 'I':
		uid = strtoul(optarg, NULL, 10);
		break;

	    case 'f':
		fd = strtoul(optarg, NULL, 10);
		break;

	    case 'c':
		cmd = optarg; 
		length = strlen(cmd);
		break;


	    case 'p':
		payload_flag = 1;
		payload = optarg; 
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
	    LIBNET_LINK_ADV,                        /* injection type */
	    device,                                 /* network interface */
            errbuf);                                /* error buffer */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    printf("Using device %s\n", l->device);

    if (payload_flag)
    {
	memset(cmd, 0, sizeof(cmd));
	memcpy(cmd, payload, (payload_s < 12 ? payload_s : 12));
	length = payload_s;
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

    if (!payload)
    {
	payload = cmd;
	payload_s = length;
    }


    ptag = libnet_build_sebek(
	magic,
	version,
	type,
	counter,
	sec,
	usec,
	pid,
	uid,
	fd,
	cmd,
	/* LIBNET_ETH_H + LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_SEBEK_H +*/ length,
	payload,
	payload_s,
	l,
	0
	);

    if (ptag == -1)
    {
	fprintf(stderr, "Can't build Sebek header: %s\n", libnet_geterror(l));
	goto bad;
    }

    ptag = libnet_build_udp(
	port,                                      /* source port */
	port,                                      /* destination port */
	LIBNET_UDP_H + LIBNET_SEBEK_H + payload_s, /* packet length */
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
	LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_SEBEK_H + payload_s,/* length */
	0,                                          /* TOS */
	242,                                        /* IP ID */
	0,                                          /* IP Frag */
	64,                                         /* TTL */
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
        fprintf(stderr, "Wrote %d byte Sebek packet; check the wire.\n", c);
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
  bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);

    return 0;
}
