/*
 *  $Id: libnet_write.c,v 1.14 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_write.c - writes a prebuilt packet to the network
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *  win32 specific code 
 *  Copyright (c) 2002 - 2003 Roberto Larcher <roberto.larcher@libero.it>
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

#include <netinet/in.h>
#include <netinet/udp.h>

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#if (!(_WIN32) || (__CYGWIN__)) 
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#include "../include/win32/config.h"
#include "packet32.h"
#include "Ntddndis.h"
#endif

int
libnet_write(libnet_t *l)
{
    int c;
    uint32_t len;
    uint8_t *packet = NULL;

    if (l == NULL)
    { 
        return (-1);
    }

    c = libnet_pblock_coalesce(l, &packet, &len);
    if (c == - 1)
    {
        /* err msg set in libnet_pblock_coalesce() */
        return (-1);
    }

    /* assume error */
    c = -1;
    switch (l->injection_type)
    {
        case LIBNET_RAW4:
        case LIBNET_RAW4_ADV:
            if (len > LIBNET_MAX_PACKET)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "%s(): packet is too large (%d bytes)\n",
                        __func__, len);
                goto done;
            }
            c = libnet_write_raw_ipv4(l, packet, len);
            break;
        case LIBNET_RAW6:
        case LIBNET_RAW6_ADV:
            c = libnet_write_raw_ipv6(l, packet, len);
            break;
        case LIBNET_LINK:
        case LIBNET_LINK_ADV:
            c = libnet_write_link(l, packet, len);
            break;
        default:
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "%s(): unsuported injection type\n", __func__);
            goto done;
    }

    /* do statistics */
    if (c == len)
    {
        l->stats.packets_sent++;
        l->stats.bytes_written += c;
    }
    else
    {
        l->stats.packet_errors++;
        /*
         *  XXX - we probably should have a way to retrieve the number of
         *  bytes actually written (since we might have written something).
         */
        if (c > 0)
        {
            l->stats.bytes_written += c;
        }
    }
done:
    /*
     *  Restore original pointer address so free won't complain about a
     *  modified chunk pointer.
     */
    if (l->aligner > 0)
    {
        packet = packet - l->aligner;
    }
    free(packet);
    return (c);
}

#if defined (__WIN32__)
libnet_ptag_t
libnet_win32_build_fake_ethernet(uint8_t *dst, uint8_t *src, uint16_t type,
uint8_t *payload, uint32_t payload_s, uint8_t *packet, libnet_t *l,
libnet_ptag_t ptag)
{
    struct libnet_ethernet_hdr eth_hdr;

    if (!packet)
    {
        return (-1);
    }

    memset(&eth_hdr, 0, sizeof(eth_hdr));  
    eth_hdr.ether_type = htons(type);
    memcpy(eth_hdr.ether_dhost, dst, ETHER_ADDR_LEN);  /* destination address */
    memcpy(eth_hdr.ether_shost, src, ETHER_ADDR_LEN);  /* source address */

    if (payload && payload_s)
    {
        /*
         *  Unchecked runtime error for buf + ETH_H payload to be greater than
         *  the allocated heap memory.
         */
        memcpy(packet + LIBNET_ETH_H, payload, payload_s);
    }
    memcpy(packet, &eth_hdr, sizeof(eth_hdr));
    return (1);
}

libnet_ptag_t
libnet_win32_build_fake_token(uint8_t *dst, uint8_t *src, uint16_t type,
uint8_t *payload, uint32_t payload_s, uint8_t *packet, libnet_t *l,
libnet_ptag_t ptag)
{
    struct libnet_token_ring_hdr token_ring_hdr;
   
    if (!packet)
    {
        return (-1);
    }

    memset(&token_ring_hdr, 0, sizeof(token_ring_hdr)); 
    token_ring_hdr.token_ring_access_control    = 0x10;
    token_ring_hdr.token_ring_frame_control     = 0x40;
    token_ring_hdr.token_ring_llc_dsap          = 0xaa;
    token_ring_hdr.token_ring_llc_ssap          = 0xaa;
    token_ring_hdr.token_ring_llc_control_field = 0x03;
    token_ring_hdr.token_ring_type              = htons(type);
    memcpy(token_ring_hdr.token_ring_dhost, dst, ETHER_ADDR_LEN); 
    memcpy(token_ring_hdr.token_ring_shost, libnet_get_hwaddr(l), 
           ETHER_ADDR_LEN); 
    
    if (payload && payload_s)
    {
        /*
         *  Unchecked runtime error for buf + ETH_H payload to be greater than
         *  the allocated heap memory.
         */
        memcpy(packet + LIBNET_TOKEN_RING_H, payload, payload_s);
    }
    memcpy(packet, &token_ring_hdr, sizeof(token_ring_hdr));
    return (1);
}

int
libnet_win32_write_raw_ipv4(libnet_t *l, const uint8_t *payload, uint32_t payload_s)
{    
    static BYTE dst[ETHER_ADDR_LEN];
    static BYTE src[ETHER_ADDR_LEN];
	
    uint8_t *packet      = NULL;
    uint32_t packet_s;

    LPPACKET lpPacket   = NULL;
    DWORD remoteip      = 0;
    DWORD BytesTransfered;
    NetType type;
    struct libnet_ipv4_hdr *ip_hdr = NULL;
	
    memset(dst, 0, sizeof(dst));
    memset(src, 0, sizeof(src));

    packet_s = payload_s + l->link_offset;
    packet = (uint8_t *)malloc(packet_s);
    if (packet == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): failed to allocate packet\n", __func__);
        return (-1);
    }
	
    /* we have to do the IP checksum  */
    if (libnet_do_checksum(l, payload, IPPROTO_IP, LIBNET_IPV4_H) == -1)
    {
        /* error msg set in libnet_do_checksum */
        return (-1);
    } 

    /* MACs, IPs and other stuff... */
    ip_hdr = (struct libnet_ipv4_hdr *)payload;
    memcpy(src, libnet_get_hwaddr(l), sizeof(src));
    remoteip = ip_hdr->ip_dst.S_un.S_addr;
		
    /* check if the remote station is the local station */
    if (remoteip == libnet_get_ipaddr4(l))
    {
        memcpy(dst, src, sizeof(dst));
    }
    else
    {
        memcpy(dst, libnet_win32_get_remote_mac(l, remoteip), sizeof(dst));
    }

    PacketGetNetType(l->lpAdapter, &type);
	
    switch(type.LinkType)
    {
        case NdisMedium802_3:
            libnet_win32_build_fake_ethernet(dst, src, ETHERTYPE_IP, payload,
                    payload_s, packet, l , 0);
            break;
        case NdisMedium802_5:
            libnet_win32_build_fake_token(dst, src, ETHERTYPE_IP, payload,
                    payload_s, packet, l, 0);
            break;
        case NdisMediumFddi:
            break;
        case NdisMediumWan:
        case NdisMediumAtm:
        case NdisMediumArcnet878_2:
        default:
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): network type (%d) is not supported\n", __func__,
                type.LinkType);
            return (-1);
            break;
    }    

    BytesTransfered = -1;
    if ((lpPacket = PacketAllocatePacket()) == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): failed to allocate the LPPACKET structure\n", __func__);
	    return (-1);
    }
    
    PacketInitPacket(lpPacket, packet, packet_s);

    /* PacketSendPacket returns a BOOLEAN */
    if (PacketSendPacket(l->lpAdapter, lpPacket, TRUE))
    {
        BytesTransfered = packet_s;
    }

    PacketFreePacket(lpPacket);
    free(packet);

    return (BytesTransfered);
}

int
libnet_write_raw_ipv4(libnet_t *l, const uint8_t *packet, uint32_t size)
{
    return (libnet_win32_write_raw_ipv4(l, packet, size));
}

int
libnet_write_raw_ipv6(libnet_t *l, const uint8_t *packet, uint32_t size)
{
    /* no difference in win32 */
    return (libnet_write_raw_ipv4(l, packet, size));
}

#else /* __WIN32__ */

int
libnet_write_raw_ipv4(libnet_t *l, const uint8_t *packet, uint32_t size)
{
    int c;
    struct sockaddr_in sin;
    struct libnet_ipv4_hdr *ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    ip_hdr = (struct libnet_ipv4_hdr *)packet;

#if (LIBNET_BSD_BYTE_SWAP)
    /*
     *  For link access, we don't need to worry about the inconsistencies of
     *  certain BSD kernels.  However, raw socket nuances abound.  Certain
     *  BSD implmentations require the ip_len and ip_off fields to be in host
     *  byte order.
     */
    ip_hdr->ip_len = FIX(ip_hdr->ip_len);
    ip_hdr->ip_off = FIX(ip_hdr->ip_off);
#endif /* LIBNET_BSD_BYTE_SWAP */

    memset(&sin, 0, sizeof(sin));
    sin.sin_family  = AF_INET;
    sin.sin_addr.s_addr = ip_hdr->ip_dst.s_addr;
#if (__WIN32__)
    /* set port for TCP */
    /*
     *  XXX - should first check to see if there's a pblock for a TCP
     *  header, if not we can use a dummy value for the port.
     */
    if (ip_hdr->ip_p == 6)
    {
        struct libnet_tcp_hdr *tcph_p =
                (struct libnet_tcp_hdr *)(packet + (ip_hdr->ip_hl << 2));
        sin.sin_port = tcph_p->th_dport;
    }
    /* set port for UDP */
    /*
     *  XXX - should first check to see if there's a pblock for a UDP
     *  header, if not we can use a dummy value for the port.
     */
    else if (ip_hdr->ip_p == 17)
    {
        struct libnet_udp_hdr *udph_p =
                (struct libnet_udp_hdr *)(packet + (ip_hdr->ip_hl << 2));
       sin.sin_port = udph_p->uh_dport;
    }
#endif /* __WIN32__ */

    c = sendto(l->fd, packet, size, 0, (struct sockaddr *)&sin,
            sizeof(struct sockaddr));

#if (LIBNET_BSD_BYTE_SWAP)
    ip_hdr->ip_len = UNFIX(ip_hdr->ip_len);
    ip_hdr->ip_off = UNFIX(ip_hdr->ip_off);
#endif /* LIBNET_BSD_BYTE_SWAP */

    if (c != size)
    {
#if !(__WIN32__)
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): %d bytes written (%s)\n", __func__, c,
                strerror(errno));
#else /* __WIN32__ */
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): %d bytes written (%d)\n", __func__, c,
                WSAGetLastError());
#endif /* !__WIN32__ */
    }
    return (c);
}

int
libnet_write_raw_ipv6(libnet_t *l, const uint8_t *packet, uint32_t size)
{
#if defined HAVE_SOLARIS && !defined HAVE_SOLARIS_IPV6
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s(): no IPv6 support\n",
            __func__, strerror(errno));
    return (-1);
}
#else
    int c;
    struct sockaddr_in6 sin;
    struct libnet_ipv6_hdr *ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    ip_hdr = (struct libnet_ipv6_hdr *)packet;

    memset(&sin, 0, sizeof(sin));
    sin.sin6_family  = AF_INET6;
    memcpy(sin.sin6_addr.s6_addr, ip_hdr->ip_dst.libnet_s6_addr,
            sizeof(ip_hdr->ip_dst.libnet_s6_addr));
       
    c = sendto(l->fd, packet, size, 0, (struct sockaddr *)&sin, sizeof(sin));
    if (c != size)
    {
#if !(__WIN32__)
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): %d bytes written (%s)\n", __func__, c,
                strerror(errno));
#else /* __WIN32__ */
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): %d bytes written (%d)\n", __func__, c,
                WSAGetLastError());
#endif /* !__WIN32__ */
    }
    return (c);
}
#endif
#endif 
/* EOF */
