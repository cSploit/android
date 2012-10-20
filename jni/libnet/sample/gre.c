/*
 *
 *  libnet 1.1
 *  Build a GRE packet
 *  To view: tcpdump -s 0 -n -X -vvv proto gre
 *
 *  Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
 *  All rights reserved.
 *
 *
 *  KNOWN BUG
 *    the encapsulated headers have wrong checksums. I guess this is due to
 *    the one pass libnet_pblock_coalesce() which is really to complicated :(
 *
 *
 *  Default packet:
 *   # ./gre -d 1.2.3.4                
 *   libnet 1.1 packet shaping: GRE 1701 [link]
 *   Wrote 78 byte GRE packet; check the wire.
 *
 *   18:58:12.112157 192.168.1.2 > 1.2.3.4: gre 198.35.123.50.1234 > 103.69.139.107.53: S [bad tcp cksum 698a!] 16843009:16843009(0) win 32767 (ttl 64, id 242, len 40, bad cksum 0!) (ttl 255, id 255, len 64)
 *   0x0000   4500 0040 00ff 0000 ff2f f4df c0a8 0102        E..@...../......
 *   0x0010   0102 0304 0000 0800 4500 0028 00f2 0000        ........E..(....
 *   0x0020   4006 0000 c623 7b32 6745 8b6b 04d2 0035        @....#{2gE.k...5
 *   0x0030   0101 0101 0202 0202 5002 7fff 6666 0000        ........P...ff..
 *
 *  Packet with a computed checksum 
 *   # ./gre -d 1.2.3.4 -c 0
 *   libnet 1.1 packet shaping: GRE 1701 [link]
 *   Wrote 82 byte GRE packet; check the wire.
 *   
 *   18:58:22.587513 192.168.1.2 > 1.2.3.4: gre [Cv0] C:7c62 198.35.123.50.1234 > 103.69.139.107.53: S [bad tcp cksum 698a!] 16843009:16843009(0) win 32767 (ttl 64, id 242, len 40, bad cksum 0!) (ttl 255, id 255, len 68)
 *   0x0000   4500 0044 00ff 0000 ff2f f4db c0a8 0102        E..D...../......
 *   0x0010   0102 0304 8000 0800 7c62 0000 4500 0028        ........|b..E..(
 *   0x0020   00f2 0000 4006 0000 c623 7b32 6745 8b6b        ....@....#{2gE.k
 *   0x0030   04d2 0035 0101 0101 0202 0202 5002 7fff        ...5........P...
 *   0x0040   6666 0000                                      ff..
 *
 *
 *  Packet with a forced checksum 
 *   # ./gre -d 1.2.3.4 -c 6666
 *   libnet 1.1 packet shaping: GRE 1701 [link]
 *   Wrote 68 byte GRE packet; check the wire.
 *   
 *   19:04:12.108080 192.168.1.2 > 1.2.3.4: gre [Cv0] C:1a0a 198.35.123.50.1234 > 103.69.139.107.53: S [bad tcp cksum 698a!] 16843009:16843009(0) win 32767 (ttl 64, id 242, len 40, bad cksum 0!) (ttl 255, id 255, len 68)
 *   0x0000   4500 0044 00ff 0000 ff2f f4db c0a8 0102        E..D...../......
 *   0x0010   0102 0304 8000 0800 1a0a 0000 4500 0028        ............E..(
 *   0x0020   00f2 0000 4006 0000 c623 7b32 6745 8b6b        ....@....#{2gE.k
 *   0x0030   04d2 0035 0101 0101 0202 0202 5002 7fff        ...5........P...
 *   0x0040   6666 0000                                      ff..
 *
 *
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


void
usage(char *prog)
{
    fprintf(stderr, "Usage: %s\n", prog);
    fprintf(stderr, "\t IP options:  -d <dst ip> [-s src ip]\n");
    fprintf(stderr, "\t GRE options: [-v] set RFC 2637 mode (PPP in GRE) (default is RFC 1701 for IP in GRE)\n");
    fprintf(stderr, "\t\t RFC 1701 options (IP in GRE):\n");
    fprintf(stderr, "\t\t   [-c sum]  [-r routing] [-k key] [-n seqnum]\n");
    fprintf(stderr, "\t\t   IP in GRE options: [-S src ip]  [-D dst ip]\n");
    fprintf(stderr, "\t\t RFC 2637 options (PPP in GRE):\n");
    fprintf(stderr, "\t\t   [-a ack]\n");
    
    exit(1);
}

/*
 *                ---------------------------------
 *                |       Delivery Header         |
 *                ---------------------------------
 *                |       GRE Header              |
 *               ---------------------------------
 *                |       Payload packet          |
 *                ---------------------------------
 */
int
main(int argc, char *argv[])
{
    char c;
    libnet_t *l;
    char errbuf[LIBNET_ERRBUF_SIZE];
    u_long src_ip = 0, dst_ip = 0, gre_src_ip = 0, gre_dst_ip = 0;
    u_short checksum = 0, offset = 0;
    u_char *routing = NULL;
    u_long key = 0, seq = 0;
    u_short gre_flags = 0;
    u_long len;
    u_long size = 0;
    libnet_ptag_t t;

    printf("libnet 1.1 packet shaping: GRE [link]\n");
    
    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* error buffer */
  
    if (!l)
    {
        fprintf(stderr, "libnet_init: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    /*
     * parse options
     */
    while ((c = getopt(argc, argv, "d:s:D:S:c:r:k:n:va:")) != EOF)
    {
        switch (c)
        {
	    
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
            case 'D':
                if ((gre_dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address (GRE): %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'S':
                if ((gre_src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad source IP address (GRE): %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
	    case 'c':
		checksum = atoi(optarg);
		gre_flags|=GRE_CSUM;
                break;
	    case 'r':
		routing = optarg;
		gre_flags|=GRE_ROUTING;
                break;
	    case 'k':
		key = atoi(optarg);
		gre_flags|=GRE_KEY;
                break;
	    case 'n':
		seq = atoi(optarg);
		gre_flags|=GRE_SEQ;
                break;
	    case 'v':
		gre_flags|=(GRE_VERSION_1|GRE_KEY);
		break;
	    case 'a':
		if (! (gre_flags & GRE_VERSION_1))
		    usage(argv[0]);
		seq = atoi(optarg);    /* seq in v0 is ack in v1 */
		gre_flags|=GRE_ACK;
		break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    /*
     * check options 
     */
    if (!dst_ip)
    {
	usage(argv[0]);
    }

    if (!src_ip)
    {
	src_ip = libnet_get_ipaddr4(l);
    }

    if (!gre_dst_ip)
    {
	gre_dst_ip = libnet_get_prand(LIBNET_PRu32);
    }

    if (!gre_src_ip)
    {
	gre_src_ip = libnet_get_prand(LIBNET_PRu32);
    }


    if ( (gre_flags & GRE_VERSION_MASK) == 0)
    {
	/*
	 * Build a TCP/IP packet embedded in GRE message
	 */
	size = LIBNET_TCP_H;
	t = libnet_build_tcp(
	    1234,                                       /* source port */
	    53,                                         /* destination port */
	    0x01010101,                                 /* sequence number */
	    0x02020202,                                 /* acknowledgement num */
	    TH_SYN,                                     /* control flags */
	    32767,                                      /* window size */
	    0,                                          /* checksum */
	    0,                                          /* urgent pointer */
	    size,                                       /* TCP packet size */
	    NULL,                                       /* payload */
	    0,                                          /* payload size */
	    l,                                          /* libnet handle */
	    0);                                         /* libnet id */
	if (t == -1)
	{
	    fprintf(stderr, "Can't build TCP header (GRE): %s\n", libnet_geterror(l));
	    goto bad;
	}
	
	size += LIBNET_IPV4_H;
	t = libnet_build_ipv4(
	    size,                                       /* length */
	    0,                                          /* TOS */
	    242,                                        /* IP ID */
	    0,                                          /* IP Frag */
	    64,                                         /* TTL */
	    IPPROTO_TCP,                                /* protocol */
	    0,                                          /* checksum */
	    gre_src_ip,                                 /* source IP */
	    gre_dst_ip,                                 /* destination IP */
	    NULL,                                       /* payload */
	    0,                                          /* payload size */
	    l,                                          /* libnet handle */
	    0);                                         /* libnet id */
	if (t == -1)
	{
	    fprintf(stderr, "Can't build IP header (GRE): %s\n", libnet_geterror(l));
	    goto bad;
	} 
    }

    if ( (gre_flags & GRE_VERSION_MASK) == 1)
    {
	offset = libnet_get_prand(LIBNET_PRu16);
	if (~gre_flags & GRE_ACK)
	{
	    u_char ppp[4] = "\x00\x01"; /* PPP padding */
	    checksum = 2; /* checksum is in fact payload_s in PPP/GRE (v1) */
	    size = 2;
	    gre_flags|=GRE_SEQ;
	    key = libnet_get_prand(LIBNET_PRu32);

	    /*
	     * Build a PPP packet embedded in GRE message
	     */
	    t = libnet_build_data(
		ppp,
		checksum,
		l, 
		0
	    );
	    if (t == -1)
	    {
		fprintf(stderr, "Can't build PPP header (GRE): %s\n", libnet_geterror(l));
		goto bad;
	    }
	}
	gre_flags&=~(GRE_CSUM|GRE_ROUTING);
    }

    /*
     * Build the GRE message
     */
    if (gre_flags & GRE_ROUTING)
    {
	/* as packet are stacked, start by the last one, ie null sre */
	size += LIBNET_GRE_SRE_H;
	t = libnet_build_gre_last_sre(l, 0);
	if (t == -1)
	{
	    fprintf(stderr, "Can't build GRE last SRE header: %s\n", libnet_geterror(l));
	    goto bad;
	}
	size += LIBNET_GRE_SRE_H + strlen(routing);
	t = libnet_build_gre_sre(
	    GRE_IP,                                 /* address family */
	    0,                                      /* offset */
	    strlen(routing),                        /* routing length */
	    routing,                                /* routing info */
	    NULL,                                   /* payload */
	    0,                                      /* payload size */
	    l,                                      /* libnet handle */
	    0);                                     /* libnet id */
	if (t == -1)
	{
	    fprintf(stderr, "Can't build GRE last SRE header: %s\n", libnet_geterror(l));
	    goto bad;
	}
    }

    len = libnet_getgre_length(gre_flags);
    size += len;
    t = libnet_build_gre(
        gre_flags,                                  /* flags & version */
        (gre_flags & GRE_VERSION_1 ? GRE_PPP : GRE_IP), /* type */
        checksum,                                   /* v0: checksum / v1: payload_s */
        offset,                                     /* v0: offset   / v1: callID    */
        key,                                        /* v0: key      / v1: seq bum   */
        seq,                                        /* v0: seq num  / v1: ack       */
	size,                                       /* length */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build GRE header: %s\n", libnet_geterror(l));
        goto bad;
    }

    
    /*
     * Build the "real" IP header
     */
    size+=LIBNET_IPV4_H;
    t = libnet_build_ipv4(
        size,                                       /* length */
        0,                                          /* TOS */
        255,                                        /* IP ID */
        0,                                          /* IP Frag */
        255,                                        /* TTL */
        IPPROTO_GRE,                                /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build IP header (GRE): %s\n", libnet_geterror(l));
        goto bad;
    } 

    t = libnet_autobuild_ethernet(
            "11:11:11:11:11:11",                                    /* ethernet destination */
            ETHERTYPE_IP,                          /* protocol type */
            l);                                     /* libnet handle */
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
        fprintf(stderr, "Wrote %d byte GRE packet; check the wire.\n", c);
    }
    libnet_destroy(l);
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE); 
}
