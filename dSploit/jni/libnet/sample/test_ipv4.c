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

static int build_ipv4(libnet_t* l, libnet_ptag_t ip_ptag, int payload_s)
{
    u_long src_ip = 0xf101f1f1;
    u_long dst_ip = 0xf102f1f1;
    uint8_t* payload = malloc(payload_s);
    assert(payload);
    memset(payload, '\x00', payload_s);

    ip_ptag = libnet_build_ipv4(
        LIBNET_IPV4_H + payload_s,                  /* length */
        0,                                          /* TOS */
        0xbbbb,                                     /* IP ID */
        0,                                          /* IP Frag */
        0xcc,                                      /* TTL */
        IPPROTO_UDP,                                /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        payload,                                    /* payload */
        payload_s,                                  /* payload size */
        l,                                          /* libnet handle */
        ip_ptag);                                   /* libnet id */

    assert(ip_ptag > 0);

    free(payload);

    return ip_ptag;
}

int
main(int argc, char *argv[])
{
    libnet_t *l;
    int r;
    char *device = "eth0";
    uint8_t enet_src[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    uint8_t enet_dst[6] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ip_ptag = 0;
    libnet_ptag_t eth_ptag = 0;
    int pkt1_payload = 10;
    uint8_t* pkt1 = NULL;
    uint32_t pkt1_sz = 0;
    struct libnet_ipv4_hdr* h1;
    int pkt2_payload = 2;
    uint8_t* pkt2 = NULL;
    uint32_t pkt2_sz = 0;
    struct libnet_ipv4_hdr* h2;



    l = libnet_init( LIBNET_LINK, device, errbuf);

    assert(l);

    /* Bug is triggered when rebuilding the ipv4 blocks with smaller payload.
     * If change in payload size is larger than 20 (iph) + 14 (ether) +
     * aligner, it will cause checksum to be written into the unallocated
     * memory before the packet, possibly corrupting glib's memory allocation
     * structures.
     */

    printf("Packet 1:\n");

    ip_ptag = build_ipv4(l, ip_ptag, pkt1_payload);

    eth_ptag = libnet_build_ethernet(
        enet_dst,                                      /* ethernet destination */
        enet_src,                                      /* ethernet source */
        ETHERTYPE_IP,                               /* protocol type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    assert(eth_ptag > 0);

    r = libnet_pblock_coalesce(l, &pkt1, &pkt1_sz);
    assert(r >= 0);

    print_pblocks(l);

    libnet_diag_dump_hex(pkt1, 14, 0, stdout);
    libnet_diag_dump_hex(pkt1+14, pkt1_sz-14, 0, stdout);

    printf("Packet 2:\n");

    ip_ptag = build_ipv4(l, ip_ptag, pkt2_payload);

    r = libnet_pblock_coalesce(l, &pkt2, &pkt2_sz);
    assert(r >= 0);

    print_pblocks(l);

    libnet_diag_dump_hex(pkt2, 14, 0, stdout);
    libnet_diag_dump_hex(pkt2+14, pkt2_sz-14, 0, stdout);

    /* Packets should differ only in the total length and cksum. */
    h1 = (struct libnet_ipv4_hdr*) (pkt1+14);
    h2 = (struct libnet_ipv4_hdr*) (pkt2+14);

    assert(h1->ip_len == htons(20+pkt1_payload));
    assert(h2->ip_len == htons(20+pkt2_payload));

    h1->ip_len = h2->ip_len = 0x5555;
    h1->ip_sum = h2->ip_sum = 0x6666;

    assert(memcmp(pkt1, pkt2, 14 + 20) == 0);

    return (EXIT_SUCCESS);
}

