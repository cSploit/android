/*
 *  $Id: icmp6_unreach.c,v 1.1.1.1 2003/06/26 21:55:10 route Exp $
 *
 *  Poseidon++ (c) 1996 - 2002 Mike D. Schiffman <mike@infonexus.com>
 * Redone from synflood example by Stefan Schlott <stefan@ploing.de>
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
    libnet_ptag_t t;
    char *cp;
    char errbuf[LIBNET_ERRBUF_SIZE];
    int i, c, packet_amt, burst_int, burst_amt, build_ip;
	char srcname[100],dstname[100];

    packet_amt  = 0;
    burst_int   = 0;
    burst_amt   = 1;

    printf("libnet 1.1 unreach/admin prohibited request ICMP6[raw]\n");

    /*
     *  Initialize the library.  Root priviledges are required.
     */
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
                if (strncmp((char*)&dst_ip,(char*)&in6addr_error,sizeof(in6addr_error))==0)
                {
                    fprintf(stderr, "Bad IP6 address: %s\n", optarg);
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

    if (!dst_prt || strncmp((char*)&dst_ip,(char*)&in6addr_error,sizeof(in6addr_error))==0 || !packet_amt)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
	
	

    libnet_seed_prand(l);
	libnet_addr2name6_r(src_ip,1,srcname,sizeof(srcname));
	libnet_addr2name6_r(dst_ip,1,dstname,sizeof(dstname));

    for(t = LIBNET_PTAG_INITIALIZER, build_ip = 1; burst_amt--;)
    {
        for (i = 0; i < packet_amt; i++)
        {
			uint8_t payload[56];
			int i;
			for (i=0; i<sizeof(payload); i++)
                            payload[i]='A'+(i%26);
			t = libnet_build_icmpv6_unreach (
                            ICMP6_UNREACH,         /* type */
                            ICMP6_ADM_PROHIBITED,  /* code */
                            0,                     /* checksum */
                            payload,               /* payload */
                            sizeof(payload),       /* payload length */
                            l,                     /* libnet context */
                            t);                    /* libnet ptag */

 

            if (build_ip)
            {
                build_ip = 0;				
                libnet_build_ipv6(0,0,
 				    LIBNET_IPV6_H + LIBNET_ICMPV6_H + sizeof(payload),
 		            IPPROTO_ICMP6,
		            64,
		            src_ip,
		            dst_ip,
                    NULL,
                    0,
                    l,
                    0);
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
