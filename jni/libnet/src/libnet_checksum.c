/*
 *  $Id: libnet_checksum.c,v 1.14 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_checksum.c - checksum routines
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

/* FIXME - unit test these - 0 is debian's version, else is -RC1's */
/* Note about aliasing warning:
 *
 *   http://mail.opensolaris.org/pipermail/tools-gcc/2005-August/000047.html
 *
 * See RFC 1071, and:
 *
 *   http://mathforum.org/library/drmath/view/54379.html
 */
#undef DEBIAN
/* Note: len is in bytes, not 16-bit words! */
int
libnet_in_cksum(uint16_t *addr, int len)
{
    int sum;
#ifdef DEBIAN
    uint16_t last_byte;

    sum = 0;
    last_byte = 0;
#else
    union
    {
        uint16_t s;
        uint8_t b[2];
    }pad;

    sum = 0;
#endif

    while (len > 1)
    {
        sum += *addr++;
        len -= 2;
    }
#ifdef DEBIAN
    if (len == 1)
    {
        *(uint8_t *)&last_byte = *(uint8_t *)addr;
        sum += last_byte;
#else
    if (len == 1)
    {
        pad.b[0] = *(uint8_t *)addr;
        pad.b[1] = 0;
        sum += pad.s;
#endif
    }

    return (sum);
}

int
libnet_toggle_checksum(libnet_t *l, libnet_ptag_t ptag, int mode)
{
    libnet_pblock_t *p;

    p = libnet_pblock_find(l, ptag);
    if (p == NULL)
    {
        /* err msg set in libnet_pblock_find() */
        return (-1);
    }
    if (mode == LIBNET_ON)
    {
        if ((p->flags) & LIBNET_PBLOCK_DO_CHECKSUM)
        {
            return (1);
        }
        else
        {
            (p->flags) |= LIBNET_PBLOCK_DO_CHECKSUM;
            return (1);
        }
    }
    else
    {
        if ((p->flags) & LIBNET_PBLOCK_DO_CHECKSUM)
        {
            (p->flags) &= ~LIBNET_PBLOCK_DO_CHECKSUM;
            return (1);
        }
        else
        {
            return (1);
        }
    }
}

static int check_ip_payload_size(libnet_t*l, const uint8_t *iphdr, int ip_hl, int h_len, const uint8_t * end, const char* func)
{
    if((iphdr+ip_hl+h_len) > end)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): ip payload not inside packet (pktsz %d, iphsz %d, payloadsz %d)\n", func,
		(int)(end - iphdr), ip_hl, h_len);
        return -1;
    }

    return 0;
}


/*
 * For backwards binary compatibility. The calculations done here can easily
 * result in buffer overreads and overwrites. You have been warned. And no, it
 * is not possible to fix, the API contains no information on the buffer's
 * boundary. libnet itself calls the safe function, libnet_inet_checksum(). So
 * should you.
 */
int
libnet_do_checksum(libnet_t *l, uint8_t *iphdr, int protocol, int h_len)
{
    uint16_t ip_len = 0;
    struct libnet_ipv4_hdr* ip4 = (struct libnet_ipv4_hdr *)iphdr;
    struct libnet_ipv6_hdr* ip6 = (struct libnet_ipv6_hdr *)iphdr;

    if(ip4->ip_v == 6) {
        ip_len = ntohs(ip6->ip_len);
    } else {
        ip_len = ntohs(ip4->ip_len);
    }

    return libnet_inet_checksum(l, iphdr, protocol, h_len,
            iphdr, iphdr + ip_len
            );
}


#define CHECK_IP_PAYLOAD_SIZE() do { \
    int e=check_ip_payload_size(l,iphdr,ip_hl, h_len, end, __func__);\
    if(e) return e;\
} while(0)


/*
 * We are checksumming pblock "q"
 *
 * iphdr is the pointer to it's encapsulating IP header
 * protocol describes the type of "q", expressed as an IPPROTO_ value
 * len is the h_len from "q"
 */
int
libnet_inet_checksum(libnet_t *l, uint8_t *iphdr, int protocol, int h_len, const uint8_t *beg, const uint8_t * end)
{
    /* will need to update this for ipv6 at some point */
    struct libnet_ipv4_hdr *iph_p = (struct libnet_ipv4_hdr *)iphdr;
    struct libnet_ipv6_hdr *ip6h_p = NULL; /* default to not using IPv6 */
    int ip_hl   = 0;
    int sum     = 0;
    int is_ipv6 = 0; /* TODO - remove this, it is redundant with ip6h_p */

    /* Check for memory under/over reads/writes. */
    if(iphdr < beg || (iphdr+sizeof(*iph_p)) > end)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): ipv4 hdr not inside packet (where %d, size %d)\n", __func__,
	    (int)(iphdr-beg), (int)(end-beg));
        return -1;
    }

    /*
     *  Figure out which IP version we're dealing with.  We'll assume v4
     *  and overlay a header structure to yank out the version.
     */
    if (iph_p->ip_v == 6)
    {
        ip6h_p = (struct libnet_ipv6_hdr *)iph_p;
        iph_p = NULL;
        ip_hl   = 40;
        if((uint8_t*)(ip6h_p+1) > end)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): ipv6 hdr not inside packet\n", __func__);
            return -1;
        }
    }
    else
    {
        ip_hl = iph_p->ip_hl << 2;
    }

    if((iphdr+ip_hl) > end)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): ip hdr len not inside packet\n", __func__);
        return -1;
    }

    /*
     *  Dug Song came up with this very cool checksuming implementation
     *  eliminating the need for explicit psuedoheader use.  Check it out.
     */
    switch (protocol)
    {
        case IPPROTO_TCP:
        {
            struct libnet_tcp_hdr *tcph_p =
                (struct libnet_tcp_hdr *)(iphdr + ip_hl);

	    h_len = end - (uint8_t*) tcph_p; /* ignore h_len, sum the packet we've coalesced */

            CHECK_IP_PAYLOAD_SIZE();

#if (STUPID_SOLARIS_CHECKSUM_BUG)
            tcph_p->th_sum = tcph_p->th_off << 2;
            return (1);
#endif /* STUPID_SOLARIS_CHECKSUM_BUG */
#if (HAVE_HPUX11)   
            if (l->injection_type != LIBNET_LINK)
            {
                /*
                 *  Similiar to the Solaris Checksum bug - but need to add
                 *  the size of the TCP payload (only for raw sockets).
                 */
                tcph_p->th_sum = (tcph_p->th_off << 2) +
                        (h_len - (tcph_p->th_off << 2));
                return (1); 
            }
#endif
            /* TCP checksum is over the IP pseudo header:
             * ip src
             * ip dst
             * tcp protocol (IPPROTO_TCP)
             * tcp length, including the header
             * + the TCP header (with checksum set to zero) and data
             */
            tcph_p->th_sum = 0;
            if (is_ipv6)
            {
                sum = libnet_in_cksum((uint16_t *)&ip6h_p->ip_src, 32);
            }
            else
            {
                /* 8 = src and dst */
                sum = libnet_in_cksum((uint16_t *)&iph_p->ip_src, 8);
            }
            sum += ntohs(iph_p->ip_p + h_len);
            sum += libnet_in_cksum((uint16_t *)tcph_p, h_len);
            tcph_p->th_sum = LIBNET_CKSUM_CARRY(sum);
#if 0
            printf("tcp sum calculated: %#x/%d h_len %d\n",
                    ntohs(tcph_p->th_sum),
                    ntohs(tcph_p->th_sum),
                    h_len
                  );
#endif
            break;
        }
        case IPPROTO_UDP:
        {
            struct libnet_udp_hdr *udph_p =
                (struct libnet_udp_hdr *)(iphdr + ip_hl);

	    h_len = end - (uint8_t*) udph_p; /* ignore h_len, sum the packet we've coalesced */

            CHECK_IP_PAYLOAD_SIZE();

            udph_p->uh_sum = 0;
            if (is_ipv6)
            {
                sum = libnet_in_cksum((uint16_t *)&ip6h_p->ip_src, 32);
            }
            else
            {
                sum = libnet_in_cksum((uint16_t *)&iph_p->ip_src, 8);
            }
            sum += ntohs(IPPROTO_UDP + h_len);
            sum += libnet_in_cksum((uint16_t *)udph_p, h_len);
            udph_p->uh_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
        case IPPROTO_ICMP:
        {
            struct libnet_icmpv4_hdr *icmph_p =
                (struct libnet_icmpv4_hdr *)(iphdr + ip_hl);

            h_len = end - (uint8_t*) icmph_p; /* ignore h_len, sum the packet we've coalesced */

            CHECK_IP_PAYLOAD_SIZE();

            icmph_p->icmp_sum = 0;
            /* Hm, is this valid? Is the checksum algorithm for ICMPv6 encapsulated in IPv4
             * actually defined?
             */
            if (is_ipv6)
            {
                sum = libnet_in_cksum((uint16_t *)&ip6h_p->ip_src, 32);
                sum += ntohs(IPPROTO_ICMP6 + h_len);
            }
            sum += libnet_in_cksum((uint16_t *)icmph_p, h_len);
            icmph_p->icmp_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
        case IPPROTO_ICMPV6:
        {
            struct libnet_icmpv6_hdr *icmph_p =
                (struct libnet_icmpv6_hdr *)(iphdr + ip_hl);

            h_len = end - (uint8_t*) icmph_p; /* ignore h_len, sum the packet we've coalesced */

            CHECK_IP_PAYLOAD_SIZE();

            icmph_p->icmp_sum = 0;
            if (is_ipv6)
            {
                sum = libnet_in_cksum((u_int16_t *)&ip6h_p->ip_src, 32);
                sum += ntohs(IPPROTO_ICMP6 + h_len);
            }
            sum += libnet_in_cksum((u_int16_t *)icmph_p, h_len);
            icmph_p->icmp_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
        case IPPROTO_IGMP:
        {
            struct libnet_igmp_hdr *igmph_p =
                (struct libnet_igmp_hdr *)(iphdr + ip_hl);

	    h_len = end - (uint8_t*) igmph_p; /* ignore h_len, sum the packet we've coalesced */

            CHECK_IP_PAYLOAD_SIZE();

            igmph_p->igmp_sum = 0;
            sum = libnet_in_cksum((uint16_t *)igmph_p, h_len);
            igmph_p->igmp_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
	case IPPROTO_GRE:
	{
            /* checksum is always at the same place in GRE header
             * in the multiple RFC version of the protocol ... ouf !!!
             */
	    struct libnet_gre_hdr *greh_p = 
		(struct libnet_gre_hdr *)(iphdr + ip_hl);
	    uint16_t fv = ntohs(greh_p->flags_ver);

            CHECK_IP_PAYLOAD_SIZE();

	    if (!(fv & (GRE_CSUM|GRE_ROUTING | GRE_VERSION_0)) ||
                !(fv & (GRE_CSUM|GRE_VERSION_1)))
	    {
		snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't compute GRE checksum (wrong flags_ver bits: 0x%x )\n",  __func__, fv);
		return (-1);
	    }
	    sum = libnet_in_cksum((uint16_t *)greh_p, h_len);
	    greh_p->gre_sum = LIBNET_CKSUM_CARRY(sum);
	    break;
	}
        case IPPROTO_OSPF:
        {
            struct libnet_ospf_hdr *oh_p =
                (struct libnet_ospf_hdr *)(iphdr + ip_hl);

            CHECK_IP_PAYLOAD_SIZE();

            oh_p->ospf_sum = 0;
            sum += libnet_in_cksum((uint16_t *)oh_p, h_len);
            oh_p->ospf_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
        case IPPROTO_OSPF_LSA:
        {
            struct libnet_ospf_hdr *oh_p =
                (struct libnet_ospf_hdr *)(iphdr + ip_hl);
            struct libnet_lsa_hdr *lsa_p =
                (struct libnet_lsa_hdr *)(iphdr + 
                ip_hl + oh_p->ospf_len);

            /* FIXME need additional length check, to account for ospf_len */
            lsa_p->lsa_sum = 0;
            sum += libnet_in_cksum((uint16_t *)lsa_p, h_len);
            lsa_p->lsa_sum = LIBNET_CKSUM_CARRY(sum);
            break;
#if 0
            /*
             *  Reworked fletcher checksum taken from RFC 1008.
             */
            int c0, c1;
            struct libnet_lsa_hdr *lsa_p = (struct libnet_lsa_hdr *)buf;
            uint8_t *p, *p1, *p2, *p3;

            c0 = 0;
            c1 = 0;

            lsa_p->lsa_cksum = 0;

            p = buf;
            p1 = buf;
            p3 = buf + len;             /* beginning and end of buf */

            while (p1 < p3)
            {
                p2 = p1 + LIBNET_MODX;
                if (p2 > p3)
                {
                    p2 = p3;
                }
  
                for (p = p1; p < p2; p++)
                {
                    c0 += (*p);
                    c1 += c0;
                }

                c0 %= 255;
                c1 %= 255;      /* modular 255 */
 
                p1 = p2;
            }

#if AWR_PLEASE_REWORK_THIS
            lsa_p->lsa_cksum[0] = (((len - 17) * c0 - c1) % 255);
            if (lsa_p->lsa_cksum[0] <= 0)
            {
                lsa_p->lsa_cksum[0] += 255;
            }

            lsa_p->lsa_cksum[1] = (510 - c0 - lsa_p->lsa_cksum[0]);
            if (lsa_p->lsa_cksum[1] > 255)
            {
                lsa_p->lsa_cksum[1] -= 255;
            }
#endif
            break;
#endif
        }
        case IPPROTO_IP:
        {
            if(!iph_p) {
                /* IPv6 doesn't have a checksum */
            } else {
                iph_p->ip_sum = 0;
                sum = libnet_in_cksum((uint16_t *)iph_p, ip_hl);
                iph_p->ip_sum = LIBNET_CKSUM_CARRY(sum);
            }
            break;
        }
        case IPPROTO_VRRP:
        {
            struct libnet_vrrp_hdr *vrrph_p =
                (struct libnet_vrrp_hdr *)(iphdr + ip_hl);
            CHECK_IP_PAYLOAD_SIZE();

            vrrph_p->vrrp_sum = 0;
            sum = libnet_in_cksum((uint16_t *)vrrph_p, h_len);
            vrrph_p->vrrp_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
        case LIBNET_PROTO_CDP:
        {   /* XXX - Broken: how can we easily get the entire packet size? */
	    /* FIXME you can't, checksumming non-IP protocols was not supported by libnet */
            struct libnet_cdp_hdr *cdph_p =
                (struct libnet_cdp_hdr *)iphdr;

            if((iphdr+h_len) > end)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "%s(): cdp payload not inside packet\n", __func__);
                return -1;
            }

            cdph_p->cdp_sum = 0;
            sum = libnet_in_cksum((uint16_t *)cdph_p, h_len);
            cdph_p->cdp_sum = LIBNET_CKSUM_CARRY(sum);
            break;
        }
        case LIBNET_PROTO_ISL:
        {
#if 0
            struct libnet_isl_hdr *islh_p =
                (struct libnet_isl_hdr *)buf;
#endif
            /*
             *  Need to compute 4 byte CRC for the ethernet frame and for
             *  the ISL frame itself.  Use the libnet_crc function.
             */
        }
        default:
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): unsuported protocol %d\n", __func__, protocol);
            return (-1);
        }
    }
    return (1);
}


uint16_t
libnet_ip_check(uint16_t *addr, int len)
{
    int sum;

    sum = libnet_in_cksum(addr, len);
    return (LIBNET_CKSUM_CARRY(sum));
}

/* EOF */
