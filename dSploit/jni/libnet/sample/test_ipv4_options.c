/*
 * Regression test for bugs in ipv4 ip_offset and h_len handling, such as
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

static void assert_eq_(long have, long want, const char* file, int line) {
    if(have != want) {
        printf("%s:%d: fail - have %ld want %ld\n", file, line, have, want);
        abort();
    }
}
#define assert_eq(have, want) assert_eq_(have, want, __FILE__, __LINE__)


static void print_pblocks(libnet_t* l)
{
    libnet_pblock_t* p = l->protocol_blocks;

    while(p) {
        printf("  tag %2d flags %d type %20s/%#x buf %p b_len %2u h_len %2u copied %2u\n",
                p->ptag, p->flags,
                libnet_diag_dump_pblock_type(p->type), p->type,
                p->buf, p->b_len, p->h_len, p->copied);
        p = p->next;
    }
    printf("  link_offset %d aligner %d total_size %u nblocks %d\n",
            l->link_offset, l->aligner, l->total_size, l->n_pblocks);

}

static void ptag_error(libnet_t* l, int ptag)
{
    if(ptag <= 0) {
        printf("error: %s\n", libnet_geterror(l));
    }
    assert(ptag > 0);
}

static int build_ipo(libnet_t* l, libnet_ptag_t ptag, int payload_s)
{
    uint8_t* payload = malloc(payload_s);
    assert(payload);
    memset(payload, '\x88', payload_s);

    ptag = libnet_build_ipv4_options(payload, payload_s, l, ptag);

    ptag_error(l, ptag);

    free(payload);

    return ptag;
}

static int build_ipv4(libnet_t* l, libnet_ptag_t ptag, int payload_s, int ip_len)
{
    u_long src_ip = 0xf101f1f1;
    u_long dst_ip = 0xf102f1f1;
    uint8_t* payload = malloc(payload_s);
    assert(payload);
    memset(payload, '\x99', payload_s);

    if(!ip_len) {
        ip_len = LIBNET_IPV4_H + payload_s;
    }

    ptag = libnet_build_ipv4(
        ip_len,                                     /* length */
        0,                                          /* TOS */
        0xbbbb,                                     /* IP ID */
        0,                                          /* IP Frag */
        0xcc,                                       /* TTL */
        IPPROTO_UDP,                                /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        payload_s ? payload : NULL,                 /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        ptag);                                      /* libnet id */

    ptag_error(l, ptag);

    free(payload);

    return ptag;
}

static int build_ethernet(libnet_t* l, libnet_ptag_t ptag)
{
    uint8_t enet_src[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    uint8_t enet_dst[6] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22};

    ptag = libnet_build_ethernet(
        enet_dst,                                   /* ethernet destination */
        enet_src,                                   /* ethernet source */
        ETHERTYPE_IP,                               /* protocol type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        ptag);                                      /* libnet id */

    ptag_error(l, ptag);

    return ptag;
}

static
void assert_lengths(libnet_t* l, int ip_len, int ip_ihl, int payload_s)
{
    uint8_t* pkt1 = NULL;
    uint32_t pkt1_sz = 0;
    struct libnet_ipv4_hdr* h1;
    uint8_t* payload = NULL;


    int r = libnet_pblock_coalesce(l, &pkt1, &pkt1_sz);
    assert(r >= 0);

    print_pblocks(l);

    libnet_diag_dump_hex(pkt1, 14, 1, stdout);
    libnet_diag_dump_hex(pkt1+14, pkt1_sz-14, 1, stdout);

    /* check ip IHL value, total ip pkt length, and options value */
    h1 = (struct libnet_ipv4_hdr*) (pkt1+14);
    assert_eq(h1->ip_hl, ip_ihl); 
    assert_eq(ntohs(h1->ip_len), ip_len);

    payload = ((uint8_t*) h1) + ip_ihl * 4;
    if(payload_s > 0) {
        assert(payload[0] == (uint8_t)'\x99');
        assert(payload[payload_s-1] == (uint8_t)'\x99');
    }
}

int
main(int argc, char *argv[])
{
    libnet_t *l;
    char *device = "eth0";
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ipo_ptag = 0;
    libnet_ptag_t ip_ptag = 0;
    libnet_ptag_t eth_ptag = 0;
    int ip_len = 0;

    l = libnet_init( LIBNET_LINK, device, errbuf);

    assert(l);

    printf("Packet: options=4, payload=0\n");

    ip_len = 20 + 4 + 0; /* ip + options + payload */
    ipo_ptag = build_ipo(l, ipo_ptag, 4);
    ip_ptag = build_ipv4(l, ip_ptag, 0, 24);
    eth_ptag = build_ethernet(l, eth_ptag);

    assert_lengths(l, 24, 6, 0);

    ipo_ptag = ip_ptag = eth_ptag = 0;

    libnet_clear_packet(l);

    printf("Packet: options=3, payload=1\n");

    ip_len = 20 + 4 + 1; /* ip + options + payload */
    ipo_ptag = build_ipo(l, ipo_ptag, 3);
    ip_ptag = build_ipv4(l, ip_ptag, 1, 25);
    eth_ptag = build_ethernet(l, eth_ptag);

    assert_lengths(l, 25, 6, 1);

    ipo_ptag = ip_ptag = eth_ptag = 0;

    libnet_clear_packet(l);

    printf("Packet: options=3, payload=1\n");

    ip_len = 20 + 4 + 1; /* ip + options + payload */
    ipo_ptag = build_ipo(l, ipo_ptag, 3);
    ip_ptag = build_ipv4(l, ip_ptag, 1, ip_len);
    eth_ptag = build_ethernet(l, eth_ptag);

    assert_lengths(l, 25, 6, 1);

    printf("... modify -> options=40\n");

    ip_len = 20 + 40 + 1; /* ip + options + payload */
    ipo_ptag = build_ipo(l, ipo_ptag, 40);

    assert_lengths(l, ip_len, 15, 1);

    printf("... modify -> options=0\n");

    ip_len = 20 + 0 + 1; /* ip + options + payload */
    ipo_ptag = build_ipo(l, ipo_ptag, 0);

    assert_lengths(l, ip_len, 5, 1);

    printf("... modify -> options=5\n");

    ip_len = 20 + 8 + 1; /* ip + options + payload */
    ipo_ptag = build_ipo(l, ipo_ptag, 5);

    assert_lengths(l, ip_len, 7, 1);

    printf("... modify -> ip_payload=5\n");

    ip_len = 20 + 8 + 5; /* ip + options + payload */
    ip_ptag = build_ipv4(l, ip_ptag, 5, ip_len);

    assert_lengths(l, ip_len, 7, 1);

    ipo_ptag = ip_ptag = eth_ptag = 0;

    libnet_clear_packet(l);


    return (EXIT_SUCCESS);
}

