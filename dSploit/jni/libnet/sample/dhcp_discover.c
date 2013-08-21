/*
 *  $Id: dhcp_discover.c,v 1.3 2004/01/03 20:31:01 mike Exp $
 *
 *  libnet 1.1
 *  Build a DHCP discover packet
 *  To view: /usr/sbin/tcpdump -vvvvven -s 4096 'port 67 or port 68'
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
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
#ifdef __WIN32__
#include "../include/win32/getopt.h"
#endif

void
usage(char *prog)
{
    fprintf(stderr, "Usage: %s interface\n", prog);
    exit(1);
}


int
main(int argc, char *argv[])
{
    char *intf;
    u_long src_ip, options_len, orig_len;
    int i;
    
    libnet_t *l;
    libnet_ptag_t t;
    libnet_ptag_t ip;
    libnet_ptag_t udp;
    libnet_ptag_t dhcp;
    struct libnet_ether_addr *ethaddr;
    struct libnet_stats ls;
    
    char errbuf[LIBNET_ERRBUF_SIZE];
    
    u_char options_req[] = { LIBNET_DHCP_SUBNETMASK,
                             LIBNET_DHCP_BROADCASTADDR, LIBNET_DHCP_TIMEOFFSET,
                             LIBNET_DHCP_ROUTER, LIBNET_DHCP_DOMAINNAME,
                             LIBNET_DHCP_DNS, LIBNET_DHCP_HOSTNAME };
    u_char *options;
    u_char enet_dst[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    u_char *tmp;
    
    if (argc != 2)
    {
        usage(argv[0]);
    }
    intf = argv[1];
    
    l = libnet_init(LIBNET_LINK, intf, errbuf);
    if (!l)
    {
        fprintf(stderr, "libnet_init: %s", errbuf);
        exit(EXIT_FAILURE);
    }
    else
    {
        src_ip = libnet_get_ipaddr4(l);;
        
        if ((ethaddr = libnet_get_hwaddr(l)) == NULL)
        {
            fprintf(stderr, "libnet_get_hwaddr: %s\n", libnet_geterror(l));
            exit(EXIT_FAILURE);
        }
        
        printf("ip addr     : %s\n", libnet_addr2name4(src_ip,
                LIBNET_DONT_RESOLVE));
        printf("eth addr    : ");
        for (i = 0; i < 6; i++)
        {
            printf("%2.2x", ethaddr->ether_addr_octet[i]);
            if (i != 5)
            {
                printf(":");
            }
        }
        printf("\n");
        
        
        /* build options packet */
        i = 0;
	/* update total payload size */
        options_len = 3;
        
        /* we are a discover packet */
        options = malloc(3);
        options[i++] = LIBNET_DHCP_MESSAGETYPE;     /* type */
        options[i++] = 1;                           /* len */
        options[i++] = LIBNET_DHCP_MSGDISCOVER;     /* data */
        
        orig_len = options_len;
	/* update total payload size */
        options_len += sizeof(options_req) + 2;
        
        tmp = malloc(options_len);
        memcpy(tmp, options, orig_len);
        free(options);
        options = tmp;
        
        /* we are going to request some parameters */
        options[i++] = LIBNET_DHCP_PARAMREQUEST;                 /* type */
        options[i++] = sizeof(options_req);                      /* len */
        memcpy(options + i, options_req, sizeof(options_req));   /* data */
        i += sizeof(options_req);
        
        /* if we have an ip already, let's request it. */
        if (src_ip)
        {
            orig_len = options_len;
            options_len += 2 + sizeof(src_ip);
            
            tmp = malloc(options_len);
            memcpy(tmp, options, orig_len);
            free(options);
            options = tmp;
            
            options[i++] = LIBNET_DHCP_DISCOVERADDR;	          /* type */
            options[i++] = sizeof(src_ip);			  /* len */
            memcpy(options + i, (char *)&src_ip, sizeof(src_ip)); /* data */
            i += sizeof(src_ip);
        }
        
        /* end our options packet */
        orig_len = options_len;
        options_len += 1;
        tmp = malloc(options_len);
        memcpy(tmp, options, orig_len);
        free(options);
        options = tmp;
        options[i++] = LIBNET_DHCP_END;

        
        /* make sure we are at least the minimum length, if not fill */
        /* this could go in libnet, but we will leave it in the app for now */
        if (options_len + LIBNET_DHCPV4_H < LIBNET_BOOTP_MIN_LEN)
        {
            orig_len = options_len;
            options_len = LIBNET_BOOTP_MIN_LEN - LIBNET_DHCPV4_H;
            
            tmp = malloc(options_len);
            memcpy(tmp, options, orig_len);
            free(options);
            options = tmp;
            
            memset(options + i, 0, options_len - i);
        }
        
        dhcp = libnet_build_dhcpv4(
                LIBNET_DHCP_REQUEST,            /* opcode */
                1,                              /* hardware type */
                6,                              /* hardware address length */
                0,                              /* hop count */
                0xdeadbeef,                     /* transaction id */
                0,                              /* seconds since bootstrap */
                0x8000,                         /* flags */
                0,                              /* client ip */
                0,                              /* your ip */
                0,                              /* server ip */
                0,                              /* gateway ip */
                ethaddr->ether_addr_octet,      /* client hardware addr */
                NULL,                           /* server host name */
                NULL,                           /* boot file */
                options,                        /* dhcp options in payload */
                options_len,                    /* length of options */
                l,                              /* libnet context */
                0);                             /* libnet ptag */
        
        udp = libnet_build_udp(
                68,                             /* source port */
                67,                             /* destination port */
                LIBNET_UDP_H + LIBNET_DHCPV4_H + options_len,  /* packet size */
                0,                              /* checksum */
                NULL,                           /* payload */
                0,                              /* payload size */
                l,                              /* libnet context */
                0);                             /* libnet ptag */
        
        ip = libnet_build_ipv4(
                LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_DHCPV4_H
                + options_len,                  /* length */
                0x10,                           /* TOS */
                0,                              /* IP ID */
                0,                              /* IP Frag */
                16,                             /* TTL */
                IPPROTO_UDP,                    /* protocol */
                0,                              /* checksum */
                src_ip,                         /* src ip */
                inet_addr("255.255.255.255"),   /* destination ip */
                NULL,                           /* payload */
                0,                              /* payload size */
                l,                              /* libnet context */
                0);                             /* libnet ptag */
        
        t = libnet_autobuild_ethernet(
                enet_dst,                       /* ethernet destination */
                ETHERTYPE_IP,                   /* protocol type */
                l);                             /* libnet context */
        
        if (libnet_write(l) == -1)
        {
            fprintf(stderr, " %s: libnet_write: %s\n", argv[0],
                    strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        libnet_stats(l, &ls);
        fprintf(stderr, "Packets sent:  %lld\n"
                    "Packet errors: %lld\n"
                    "Bytes written: %lld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
        libnet_destroy(l);
        
        free(options);
        
        exit(0);
    }
    exit(0);
}
