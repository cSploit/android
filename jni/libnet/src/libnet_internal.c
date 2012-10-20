/*
 *  $Id: libnet_internal.c,v 1.14 2004/03/16 18:40:59 mike Exp $
 *
 *  libnet
 *  libnet_internal.c - secret routines!
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
#if (!(_WIN32) || (__CYGWIN__)) 
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#endif

void
libnet_diag_dump_hex(const uint8_t *packet, uint32_t len, int swap, FILE *stream)
{
    int i, s_cnt;
    uint16_t *p;

    p     = (uint16_t *)packet;
    s_cnt = len / sizeof(uint16_t);

    fprintf(stream, "\t");
    for (i = 0; --s_cnt >= 0; i++)
    {
        if ((!(i % 8)))
        {
            fprintf(stream, "\n%02x\t", (i * 2));
        }
        fprintf(stream, "%04x ", swap ? ntohs(*(p++)) : *(p++));
    }

    /*
     *  Mop up an odd byte.
     */
    if (len & 1)
    {
        if ((!(i % 8)))
        {
            fprintf(stream, "\n%02x\t", (i * 2));
        }
        fprintf(stream, "%02x ", *(uint8_t *)p);
    }
    fprintf(stream, "\n");
}


void
libnet_diag_dump_context(libnet_t *l)
{
    if (l == NULL)
    { 
        return;
    } 

    fprintf(stderr, "fd:\t\t%d\n", l->fd);
 
    switch (l->injection_type)
    {
        case LIBNET_LINK:
            fprintf(stderr, "injection type:\tLIBNET_LINK\n");
            break;
        case LIBNET_RAW4:
            fprintf(stderr, "injection type:\tLIBNET_RAW4\n");
            break;
        case LIBNET_RAW6:
            fprintf(stderr, "injection type:\tLIBNET_RAW6\n");
            break;
        case LIBNET_LINK_ADV:
            fprintf(stderr, "injection type:\tLIBNET_LINK_ADV\n");
            break;
        case LIBNET_RAW4_ADV:
            fprintf(stderr, "injection type:\tLIBNET_RAW4_ADV\n");
            break;
        case LIBNET_RAW6_ADV:
            fprintf(stderr, "injection type:\tLIBNET_RAW6_ADV\n");
            break;
        default:
            fprintf(stderr, "injection type:\tinvalid injection type %d\n", 
                    l->injection_type);
            break;
    }
    
    fprintf(stderr, "pblock start:\t%p\n", l->protocol_blocks);
    fprintf(stderr, "pblock end:\t%p\n", l->pblock_end);
    fprintf(stderr, "link type:\t%d\n", l->link_type);
    fprintf(stderr, "link offset:\t%d\n", l->link_offset);
    fprintf(stderr, "aligner:\t%d\n", l->aligner);
    fprintf(stderr, "device:\t\t%s\n", l->device);
    fprintf(stderr, "packets sent:\t%lld\n", (long long int)l->stats.packets_sent);
    fprintf(stderr, "packet errors:\t%lld\n", (long long int)l->stats.packet_errors);
    fprintf(stderr, "bytes written:\t%lld\n", (long long int)l->stats.bytes_written);
    fprintf(stderr, "ptag state:\t%d\n", l->ptag_state);
    fprintf(stderr, "context label:\t%s\n", l->label);
    fprintf(stderr, "last errbuf:\t%s\n", l->err_buf);
    fprintf(stderr, "total size:\t%d\n", l->total_size);
}

void
libnet_diag_dump_pblock(libnet_t *l)
{
    uint32_t n;
    libnet_pblock_t *p;

    for (p = l->protocol_blocks; p; p = p->next)
    {
        fprintf(stderr, "pblock type:\t%s\n", 
                libnet_diag_dump_pblock_type(p->type));
        fprintf(stderr, "ptag number:\t%d\n", p->ptag);
        fprintf(stderr, "pblock address:\t%p\n", p);
        fprintf(stderr, "next pblock\t%p ", p->next);
        if (p->next)
        {
            fprintf(stderr, "(%s)",
                    libnet_diag_dump_pblock_type(p->next->type));
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "prev pblock\t%p ", p->prev);
        if (p->prev)
        {
            fprintf(stderr, "(%s)",
                    libnet_diag_dump_pblock_type(p->prev->type));
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "buf:\t\t");
        for (n = 0; n < p->b_len; n++)
        {
            fprintf(stderr, "%02x", p->buf[n]);
        }
        fprintf(stderr, "\nbuffer length:\t%d\n", p->b_len);
        if ((p->flags) & LIBNET_PBLOCK_DO_CHECKSUM)
        {
            fprintf(stderr, "checksum flag:\tYes\n");
            fprintf(stderr, "chksum length:\t%d\n", p->h_len);
        }
        else
        {
            fprintf(stderr, "checksum flag:\tNo\n");
        }
        fprintf(stderr, "bytes copied:\t%d\n\n", p->copied);
    }
}

char *
libnet_diag_dump_pblock_type(uint8_t type)
{
    switch (type)
    {
        /* below text can be regenerated using ./map-pblock-types */
        case LIBNET_PBLOCK_ARP_H:
            return ("arp");
        case LIBNET_PBLOCK_DHCPV4_H:
            return ("dhcpv4");
        case LIBNET_PBLOCK_DNSV4_H:
            return ("dnsv4");
        case LIBNET_PBLOCK_ETH_H:
            return ("eth");
        case LIBNET_PBLOCK_ICMPV4_H:
            return ("icmpv4");
        case LIBNET_PBLOCK_ICMPV4_ECHO_H:
            return ("icmpv4_echo");
        case LIBNET_PBLOCK_ICMPV4_MASK_H:
            return ("icmpv4_mask");
        case LIBNET_PBLOCK_ICMPV4_UNREACH_H:
            return ("icmpv4_unreach");
        case LIBNET_PBLOCK_ICMPV4_TIMXCEED_H:
            return ("icmpv4_timxceed");
        case LIBNET_PBLOCK_ICMPV4_REDIRECT_H:
            return ("icmpv4_redirect");
        case LIBNET_PBLOCK_ICMPV4_TS_H:
            return ("icmpv4_ts");
        case LIBNET_PBLOCK_IGMP_H:
            return ("igmp");
        case LIBNET_PBLOCK_IPV4_H:
            return ("ipv4");
        case LIBNET_PBLOCK_IPO_H:
            return ("ipo");
        case LIBNET_PBLOCK_IPDATA:
            return ("ipdata");
        case LIBNET_PBLOCK_OSPF_H:
            return ("ospf");
        case LIBNET_PBLOCK_OSPF_HELLO_H:
            return ("ospf_hello");
        case LIBNET_PBLOCK_OSPF_DBD_H:
            return ("ospf_dbd");
        case LIBNET_PBLOCK_OSPF_LSR_H:
            return ("ospf_lsr");
        case LIBNET_PBLOCK_OSPF_LSU_H:
            return ("ospf_lsu");
        case LIBNET_PBLOCK_OSPF_LSA_H:
            return ("ospf_lsa");
        case LIBNET_PBLOCK_OSPF_AUTH_H:
            return ("ospf_auth");
        case LIBNET_PBLOCK_OSPF_CKSUM:
            return ("ospf_cksum");
        case LIBNET_PBLOCK_LS_RTR_H:
            return ("ls_rtr");
        case LIBNET_PBLOCK_LS_NET_H:
            return ("ls_net");
        case LIBNET_PBLOCK_LS_SUM_H:
            return ("ls_sum");
        case LIBNET_PBLOCK_LS_AS_EXT_H:
            return ("ls_as_ext");
        case LIBNET_PBLOCK_NTP_H:
            return ("ntp");
        case LIBNET_PBLOCK_RIP_H:
            return ("rip");
        case LIBNET_PBLOCK_TCP_H:
            return ("tcp");
        case LIBNET_PBLOCK_TCPO_H:
            return ("tcpo");
        case LIBNET_PBLOCK_TCPDATA:
            return ("tcpdata");
        case LIBNET_PBLOCK_UDP_H:
            return ("udp");
        case LIBNET_PBLOCK_VRRP_H:
            return ("vrrp");
        case LIBNET_PBLOCK_DATA_H:
            return ("data");
        case LIBNET_PBLOCK_CDP_H:
            return ("cdp");
        case LIBNET_PBLOCK_IPSEC_ESP_HDR_H:
            return ("ipsec_esp_hdr");
        case LIBNET_PBLOCK_IPSEC_ESP_FTR_H:
            return ("ipsec_esp_ftr");
        case LIBNET_PBLOCK_IPSEC_AH_H:
            return ("ipsec_ah");
        case LIBNET_PBLOCK_802_1Q_H:
            return ("802_1q");
        case LIBNET_PBLOCK_802_2_H:
            return ("802_2");
        case LIBNET_PBLOCK_802_2SNAP_H:
            return ("802_2snap");
        case LIBNET_PBLOCK_802_3_H:
            return ("802_3");
        case LIBNET_PBLOCK_STP_CONF_H:
            return ("stp_conf");
        case LIBNET_PBLOCK_STP_TCN_H:
            return ("stp_tcn");
        case LIBNET_PBLOCK_ISL_H:
            return ("isl");
        case LIBNET_PBLOCK_IPV6_H:
            return ("ipv6");
        case LIBNET_PBLOCK_802_1X_H:
            return ("802_1x");
        case LIBNET_PBLOCK_RPC_CALL_H:
            return ("rpc_call");
        case LIBNET_PBLOCK_MPLS_H:
            return ("mpls");
        case LIBNET_PBLOCK_FDDI_H:
            return ("fddi");
        case LIBNET_PBLOCK_TOKEN_RING_H:
            return ("token_ring");
        case LIBNET_PBLOCK_BGP4_HEADER_H:
            return ("bgp4_header");
        case LIBNET_PBLOCK_BGP4_OPEN_H:
            return ("bgp4_open");
        case LIBNET_PBLOCK_BGP4_UPDATE_H:
            return ("bgp4_update");
        case LIBNET_PBLOCK_BGP4_NOTIFICATION_H:
            return ("bgp4_notification");
        case LIBNET_PBLOCK_GRE_H:
            return ("gre");
        case LIBNET_PBLOCK_GRE_SRE_H:
            return ("gre_sre");
        case LIBNET_PBLOCK_IPV6_FRAG_H:
            return ("ipv6_frag");
        case LIBNET_PBLOCK_IPV6_ROUTING_H:
            return ("ipv6_routing");
        case LIBNET_PBLOCK_IPV6_DESTOPTS_H:
            return ("ipv6_destopts");
        case LIBNET_PBLOCK_IPV6_HBHOPTS_H:
            return ("ipv6_hbhopts");
        case LIBNET_PBLOCK_SEBEK_H:
            return ("sebek");
        case LIBNET_PBLOCK_HSRP_H:
            return ("hsrp");
        case LIBNET_PBLOCK_ICMPV6_H:
            return ("icmpv6");
        case LIBNET_PBLOCK_ICMPV6_UNREACH_H:
            return ("icmpv6_unreach");
    }
    return ("unrecognized pblock");
}

