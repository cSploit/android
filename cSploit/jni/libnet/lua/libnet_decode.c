/*
Copyright (C) 2010 Wurldtech Security Technologies All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <libnet.h>

/* push data pblock, if there is any data to push */
static int pushdata(const uint8_t* pkt, size_t pkt_s, libnet_t *l, int ptag)
{
    (void) ptag;

    if(pkt_s)
      return libnet_build_data(pkt, pkt_s, l, 0);

    return 0;
}

int libnet_decode_tcp(const uint8_t* pkt, size_t pkt_s, libnet_t *l)
{
    const struct libnet_tcp_hdr* tcp_hdr = (const struct libnet_tcp_hdr*) pkt;
    size_t th_off = 0;
    const uint8_t* payload = NULL;
    size_t payload_s = 0;
    int otag;
    int ptag;

    if(pkt_s < sizeof(*tcp_hdr))
      return pushdata(pkt, pkt_s, l, 0);

    th_off = tcp_hdr->th_off * 4;

    if(pkt_s < th_off || th_off < LIBNET_TCP_H)
      return pushdata(pkt, pkt_s, l, 0);

    payload = pkt + th_off;
    payload_s = pkt + pkt_s - payload;

    if(th_off > LIBNET_TCP_H) {
	const uint8_t* options = pkt + LIBNET_TCP_H;
	size_t options_s = th_off - LIBNET_TCP_H;
	otag = libnet_build_tcp_options(options, options_s, l, 0);
	if(otag < 0)
	    return otag;
    }

    ptag = libnet_build_tcp(
            ntohs(tcp_hdr->th_sport),
            ntohs(tcp_hdr->th_dport),
            ntohl(tcp_hdr->th_seq),
            ntohl(tcp_hdr->th_ack),
            tcp_hdr->th_flags,
            ntohs(tcp_hdr->th_win),
            ntohs(tcp_hdr->th_sum),
            ntohs(tcp_hdr->th_urp),
            pkt_s,
            payload, payload_s,
            l, 0);

    if(ptag > 0) {
        /* Patch the reserved flags to equal those in the original packet. Even
         * though they should usually be zero.
         */
        struct libnet_tcp_hdr* hdr = (struct libnet_tcp_hdr*) libnet_pblock_find(l, ptag)->buf;
        hdr->th_x2 = tcp_hdr->th_x2;
    }

    if(ptag > 0)
        libnet_pblock_setflags(libnet_pblock_find(l, ptag), LIBNET_PBLOCK_DO_CHECKSUM);

    return ptag;
}

int libnet_decode_udp(const uint8_t* pkt, size_t pkt_s, libnet_t *l)
{
    const struct libnet_udp_hdr* udp_hdr = (const struct libnet_udp_hdr*) pkt;
    const uint8_t* payload = pkt + LIBNET_UDP_H;
    size_t payload_s = pkt + pkt_s - payload;
    int ptag;

    if(pkt_s < sizeof(*udp_hdr))
      return pushdata(pkt, pkt_s, l, 0);

    ptag = libnet_build_udp(
            ntohs(udp_hdr->uh_sport),
            ntohs(udp_hdr->uh_dport),
            ntohs(udp_hdr->uh_ulen),
            ntohs(udp_hdr->uh_sum),
            payload, payload_s,
            l, 0);

    if(ptag > 0)
        libnet_pblock_setflags(libnet_pblock_find(l, ptag), LIBNET_PBLOCK_DO_CHECKSUM);

    return ptag;
}

int libnet_decode_ipv4(const uint8_t* pkt, size_t pkt_s, libnet_t *l)
{
    const struct libnet_ipv4_hdr* ip_hdr = (const struct libnet_ipv4_hdr*) pkt;
    size_t ip_hl = 0;
    const uint8_t* payload = NULL;
    size_t payload_s = 0;
    int ptag = 0; /* payload tag */
    int otag = 0; /* options tag */
    int itag = 0; /* ip tag */

    if(pkt_s < sizeof(*ip_hdr))
      return pushdata(pkt, pkt_s, l, 0);

    ip_hl = ip_hdr->ip_hl * 4;

    if(pkt_s < ip_hl || ip_hl < LIBNET_IPV4_H)
      return pushdata(pkt, pkt_s, l, 0);

    /* pcaps often contain trailing garbage after the IP packet, drop that, but not the ip hdr
     * no matter how damaged the ip_len field is */
    /* TODO this means its not possible to reencode pcaps... the padding that comes
       off the wire for eth frames shorter than 60 bytes gets lost. However, the next-hdr
       only knows its length because of what tcp tells it... maybe ipv4 should push pblock
       with any trailing garbage, afterwards? But that might cause lengths to be reevaluated
       unless there was a specific pblock type for ethernet-padding. */
    if(pkt_s > ntohs(ip_hdr->ip_len))
        pkt_s = ntohs(ip_hdr->ip_len);

    if(pkt_s < ip_hl)
        pkt_s = ip_hl;

    payload = pkt + ip_hl;
    payload_s = pkt + pkt_s - payload;

    switch(ip_hdr->ip_p) {
        case IPPROTO_UDP:
            ptag = libnet_decode_udp(payload, payload_s, l);
            payload_s = 0;
            break;
        case IPPROTO_TCP:
            ptag = libnet_decode_tcp(payload, payload_s, l);
            payload_s = 0;
            break;
    }

    if(ptag < 0) return ptag;

    if(ip_hl > LIBNET_IPV4_H) {
        const uint8_t* options = pkt + LIBNET_IPV4_H;
        size_t options_s = ip_hl - LIBNET_IPV4_H;
        otag = libnet_build_ipv4_options(options, options_s, l, 0);
        if(otag < 0) {
            return otag;
        }
    }

    itag = libnet_build_ipv4(
            ntohs(ip_hdr->ip_len),
            ip_hdr->ip_tos,
            ntohs(ip_hdr->ip_id),
            ntohs(ip_hdr->ip_off),
            ip_hdr->ip_ttl,
            ip_hdr->ip_p,
            ntohs(ip_hdr->ip_sum),
            ip_hdr->ip_src.s_addr,
            ip_hdr->ip_dst.s_addr,
            payload, payload_s,
            l, 0
            );

    if(itag > 0)
        libnet_pblock_setflags(libnet_pblock_find(l, itag), LIBNET_PBLOCK_DO_CHECKSUM);

    return itag;
}

int libnet_decode_ip(const uint8_t* pkt, size_t pkt_s, libnet_t *l)
{
    const struct libnet_ipv4_hdr* ip_hdr = (const struct libnet_ipv4_hdr*) pkt;

    if(pkt_s < sizeof(*ip_hdr))
      return pushdata(pkt, pkt_s, l, 0);

    switch(ip_hdr->ip_v) {
        case 4:
            return libnet_decode_ipv4(pkt, pkt_s, l);
        /* TODO - IPv6 */
    }

    return pushdata(pkt, pkt_s, l, 0);
}

int libnet_decode_eth(const uint8_t* pkt, size_t pkt_s, libnet_t *l)
{
    const struct libnet_ethernet_hdr* hdr = (const struct libnet_ethernet_hdr*) pkt;
    const uint8_t* payload = pkt + LIBNET_ETH_H;
    size_t payload_s = pkt + pkt_s - payload;
    int ptag = 0; /* payload tag */
    int etag = 0; /* eth tag */

    if(pkt_s < sizeof(*hdr))
      return pushdata(pkt, pkt_s, l, 0);

    switch(ntohs(hdr->ether_type)) {
        case ETHERTYPE_IP:
            ptag = libnet_decode_ip(payload, payload_s, l);
            payload_s = 0;
            break;
    }

    if(ptag < 0) return ptag;

    etag = libnet_build_ethernet(
            hdr->ether_dhost,
            hdr->ether_shost,
            ntohs(hdr->ether_type),
            payload, payload_s,
            l, 0
            );

    return etag;
}


