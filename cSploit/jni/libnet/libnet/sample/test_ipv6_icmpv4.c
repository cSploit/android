/*
 * Regression test for bugs such as reported in:
 *   http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=418975
 *
 * Copyright (c) 2009 Sam Roberts <sroberts@wurldtech.com>
 * All rights reserved.
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

#include <assert.h>

#include <netinet/in.h>

static void print_pblocks(libnet_t* l)
{
    libnet_pblock_t* p = l->protocol_blocks;

    while(p) {
        /* h_len is header length for checksumming? "chksum length"? */
        printf("  tag %d flags %d type %20s/%#x buf %p b_len %2u h_len %2u copied %2u\n",
                p->ptag, p->flags,
                libnet_diag_dump_pblock_type(p->type), p->type,
                p->buf, p->b_len, p->h_len, p->copied);
        p = p->next;
    }
    printf("  link_offset %d aligner %d total_size %u nblocks %d\n",
            l->link_offset, l->aligner, l->total_size, l->n_pblocks);

}

int
main(int argc, char *argv[])
{
    libnet_t *l;
    int r;
    char *device = "eth0";
    struct libnet_ether_addr *mac_address;
    struct in6_addr src_ip;
    struct libnet_in6_addr dst_ip;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t icmp_ptag = 0;
    libnet_ptag_t ipv6_ptag = 0;
    char payload[24] = { 0 };

    memset(&src_ip, 0x66, sizeof(src_ip));

    l = libnet_init( LIBNET_RAW6, device, errbuf);

    assert(l);

    mac_address = libnet_get_hwaddr(l);
    assert(mac_address);

    dst_ip = libnet_name2addr6(l, "::1" /* BCAST_ADDR - defined where? */, LIBNET_DONT_RESOLVE);

    memcpy(payload,src_ip.s6_addr,16);
    payload[16] = 2; /* 2 for Target Link-layer Address */
    payload[17] = 1; /* The length of the option */
    memcpy(payload+18,mac_address->ether_addr_octet, 6);

    /* 0x2000: RSO */
    icmp_ptag = libnet_build_icmpv4_echo(
            136,0,0,0x2000,0,
            (uint8_t *)payload,sizeof(payload), l, LIBNET_PTAG_INITIALIZER);
    assert(icmp_ptag);

    ipv6_ptag = libnet_build_ipv6(
            0, 0,
            LIBNET_ICMPV6_H + sizeof(payload), /* ICMPV6_H == ICMPV4_H, luckily */
            IPPROTO_ICMP6,
            255,
            *(struct libnet_in6_addr*)&src_ip,
            dst_ip,
            NULL, 0,
            l, 0);
    assert(icmp_ptag);

    print_pblocks(l);

    {
       uint8_t* pkt1 = NULL;
       uint32_t pkt1_sz = 0;
       r = libnet_pblock_coalesce(l, &pkt1, &pkt1_sz);
       assert(r >= 0);

       libnet_diag_dump_hex(pkt1, LIBNET_IPV6_H, 0, stdout);
       libnet_diag_dump_hex(pkt1+LIBNET_IPV6_H, pkt1_sz-LIBNET_IPV6_H, 0, stdout);

       free(pkt1);
       pkt1 = NULL;
    }

    r = libnet_write(l);
    assert(r >= 0);

    return (EXIT_SUCCESS);
}

