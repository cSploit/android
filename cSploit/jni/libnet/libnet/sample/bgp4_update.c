/*
 *
 * libnet 1.1
 * Build a BGP4 update message with what you want as payload
 *
 * Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
 * All rights reserved.
 *
 * Examples:
 *
 *   empty BGP UPDATE message:
 * 
 *   # ./bgp4_update -s 1.1.1.1 -d 2.2.2.2                      
 *   libnet 1.1 packet shaping: BGP4 update + payload[raw]
 *   Wrote 63 byte TCP packet; check the wire.
 *     
 *   13:44:29.216135 1.1.1.1.26214 > 2.2.2.2.179: S [tcp sum ok] 
 *            16843009:16843032(23) win 32767: BGP (ttl 64, id 242, len 63)
 *   0x0000   4500 003f 00f2 0000 4006 73c2 0101 0101        E..?....@.s.....
 *   0x0010   0202 0202 6666 00b3 0101 0101 0202 0202        ....ff..........
 *   0x0020   5002 7fff b288 0000 0101 0101 0101 0101        P...............
 *   0x0030   0101 0101 0101 0101 0017 0200 0000 00          ...............
 *   
 *
 *   BGP UPDATE with Path Attributes and Unfeasible Routes Length
 *
 *   # ./bgp4_update -s 1.1.1.1 -d 2.2.2.2 -a `printf "\x01\x02\x03"` -A 3 -W 13
 *   libnet 1.1 packet shaping: BGP4 update + payload[raw]
 *   Wrote 79 byte TCP packet; check the wire.
 *   
 *   13:45:59.579901 1.1.1.1.26214 > 2.2.2.2.179: S [tcp sum ok] 
 *            16843009:16843048(39) win 32767: BGP (ttl 64, id 242, len 79)
 *   0x0000   4500 004f 00f2 0000 4006 73b2 0101 0101        E..O....@.s.....
 *   0x0010   0202 0202 6666 00b3 0101 0101 0202 0202        ....ff..........
 *   0x0020   5002 7fff 199b 0000 0101 0101 0101 0101        P...............
 *   0x0030   0101 0101 0101 0101 0027 0200 0d41 4141        .........'...AAA
 *   0x0040   4141 4141 4141 4141 4141 0003 0102 03          AAAAAAAAAA.....
 *
 *
 *  BGP UPDATE with Reachability Information
 *
 *   # ./bgp4_update -s 1.1.1.1 -d 2.2.2.2 -I 7                 
 *   libnet 1.1 packet shaping: BGP4 update + payload[raw]
 *   Wrote 70 byte TCP packet; check the wire.
 *   
 *   13:49:02.829225 1.1.1.1.26214 > 2.2.2.2.179: S [tcp sum ok] 
 *            16843009:16843039(30) win 32767: BGP (ttl 64, id 242, len 70)
 *   0x0000   4500 0046 00f2 0000 4006 73bb 0101 0101        E..F....@.s.....
 *   0x0010   0202 0202 6666 00b3 0101 0101 0202 0202        ....ff..........
 *   0x0020   5002 7fff e86d 0000 0101 0101 0101 0101        P....m..........
 *   0x0030   0101 0101 0101 0101 001e 0200 0000 0043        ...............C
 *   0x0040   4343 4343 4343                                 CCCCCC
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


#define set_ptr_and_size(ptr, size, val, flag)                                 \
    if (size && !ptr)                                                          \
    {                                                                          \
	ptr = (u_char *)malloc(size);                                          \
	if (!ptr)                                                              \
	{                                                                      \
	    printf("memory allocation failed (%u bytes requested)\n", size);   \
	    goto bad;                                                          \
	}                                                                      \
	memset(ptr, val, size);                                                \
        flag = 1;                                                              \
    }                                                                          \
                                                                               \
    if (ptr && !size)                                                          \
    {                                                                          \
	size = strlen((char *)ptr);                                            \
    }                                                                          



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

    u_short u_rt_l = 0;
    u_char *withdraw_rt = NULL;
    char flag_w = 0;
    u_short attr_l = 0;
    u_char *attr = NULL;
    char flag_a = 0;
    u_short info_l = 0;
    u_char *info = NULL;
    char flag_i = 0;

    printf("libnet 1.1 packet shaping: BGP4 update + payload[raw]\n");

    /*
     *  Initialize the library.  Root priviledges are required.
     */
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

    while ((c = getopt(argc, argv, "d:s:t:m:p:w:W:a:A:i:I:")) != EOF)
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

	    case 'p':
		payload = (u_char *)optarg;
		payload_s = strlen((char *)payload);
		break;

	    case 'w':
		withdraw_rt = (u_char *)optarg;
		break;

	    case 'W':
		u_rt_l = atoi(optarg);
		break;

	    case 'a':
		attr = (u_char *)optarg;
		break;

	    case 'A':
		attr_l = atoi(optarg);
		break;

	    case 'i':
		info = (u_char *)optarg;
		break;

	    case 'I':
		info_l = atoi(optarg);
		break;

            default:
                exit(EXIT_FAILURE);
        }
    }

    if (!src_ip || !dst_ip)
    {
        usage(argv[0]);
	goto bad;
    }

    set_ptr_and_size(withdraw_rt, u_rt_l, 0x41, flag_w);
    set_ptr_and_size(attr, attr_l, 0x42, flag_a);
    set_ptr_and_size(info, info_l, 0x43, flag_i);

  /*
   * BGP4 update messages are "dynamic" are fields have variable size. The only 
   * sizes we know are those for the 2 first fields ... so we need to count them
   * plus their value.
   */
    length = LIBNET_BGP4_UPDATE_H + u_rt_l + attr_l + info_l + payload_s;
    t = libnet_build_bgp4_update(
	u_rt_l,                                     /* Unfeasible Routes Length */   
	withdraw_rt,                                /* Withdrawn Routes */
	attr_l,                                     /* Total Path Attribute Length */
	attr,                                       /* Path Attributes */
	info_l,                                     /* Network Layer Reachability Information length */
	info,                                       /* Network Layer Reachability Information */
        payload,                                    /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build BGP4 update header: %s\n", libnet_geterror(l));
        goto bad;
    }

    length+=LIBNET_BGP4_HEADER_H;
    t = libnet_build_bgp4_header(
	marker,                                     /* marker */   
	length,                                     /* length */
	LIBNET_BGP4_UPDATE,                         /* message type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
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

    if (flag_w) free(withdraw_rt);
    if (flag_a) free(attr);
    if (flag_i) free(info);

    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    if (flag_w) free(withdraw_rt);
    if (flag_a) free(attr);
    if (flag_i) free(info);

    libnet_destroy(l);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    fprintf(stderr,
        "usage: %s -s source_ip -d destination_ip \n"
        "          [-m marker] [-p payload] [-S payload size]\n"
	"          [-w Withdrawn Routes] [-W Unfeasible Routes Length]\n"
	"          [-a Path Attributes] [-A Attribute Length]\n"
	"          [-i Reachability Information] [-I Reachability Information length]\n",
        name);
}


/* EOF */
