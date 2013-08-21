/*
 *  $Id: libnet_build_tcp.c,v 1.11 2004/01/28 19:45:00 mike Exp $
 *
 *  libnet
 *  libnet_build_tcp.c - TCP packet assembler
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

libnet_ptag_t
libnet_build_tcp(
            uint16_t sp, uint16_t dp, uint32_t seq, uint32_t ack,
            uint8_t control, uint16_t win, uint16_t sum, uint16_t urg, uint16_t h_len,
            const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    int n, offset;
    libnet_pblock_t *p = NULL;
    libnet_ptag_t ptag_data = 0;
    struct libnet_tcp_hdr tcp_hdr;

    if (l == NULL)
        return -1;

    if (payload_s && !payload)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
			    "%s(): payload inconsistency\n", __func__);
        return -1;
    }

    p = libnet_pblock_probe(l, ptag, LIBNET_TCP_H, LIBNET_PBLOCK_TCP_H);
    if (p == NULL)
        return -1;

    memset(&tcp_hdr, 0, sizeof(tcp_hdr));
    tcp_hdr.th_sport   = htons(sp);    /* source port */
    tcp_hdr.th_dport   = htons(dp);    /* destination port */
    tcp_hdr.th_seq     = htonl(seq);   /* sequence number */
    tcp_hdr.th_ack     = htonl(ack);   /* acknowledgement number */
    tcp_hdr.th_flags   = control;      /* control flags */
    tcp_hdr.th_x2      = 0;            /* UNUSED */
    tcp_hdr.th_off     = 5;            /* 20 byte header */

    /* check to see if there are TCP options to include */
    if (p->prev && p->prev->type == LIBNET_PBLOCK_TCPO_H)
    {
        /* Note that the tcp options pblock is already padded */
        tcp_hdr.th_off += (p->prev->b_len/4);
    }

    tcp_hdr.th_win     = htons(win);   /* window size */
    tcp_hdr.th_sum     = (sum ? htons(sum) : 0);   /* checksum */ 
    tcp_hdr.th_urp     = htons(urg);          /* urgent pointer */

    n = libnet_pblock_append(l, p, (uint8_t *)&tcp_hdr, LIBNET_TCP_H);
    if (n == -1)
    {
        goto bad;
    }

    if (ptag == LIBNET_PTAG_INITIALIZER)
    {
        libnet_pblock_update(l, p, h_len, LIBNET_PBLOCK_TCP_H);
    }

    offset = payload_s;

    /* If we are going to modify a TCP data block, find it, and figure out the
     * "offset", the possibly negative amount by which we are increasing the ip
     * data length. */
    if (ptag)
    {
        libnet_pblock_t* datablock = p->prev;

        if (datablock && datablock->type == LIBNET_PBLOCK_TCPO_H)
            datablock = datablock->prev;

        if (datablock && datablock->type == LIBNET_PBLOCK_TCPDATA)
        {
            ptag_data = datablock->ptag;
            offset -=  datablock->b_len;
        }
        p->h_len += offset;
    }

    /* If we are modifying a TCP block, we should look forward and apply the offset
     * to our IPv4 header, if we have one.
     */
    if (p->next)
    {
        libnet_pblock_t* ipblock = p->next;

        if(ipblock->type == LIBNET_PBLOCK_IPO_H)
            ipblock = ipblock->next;

        if(ipblock && ipblock->type == LIBNET_PBLOCK_IPV4_H)
        {
            struct libnet_ipv4_hdr * ip_hdr = (struct libnet_ipv4_hdr *)ipblock->buf;
            int ip_len = ntohs(ip_hdr->ip_len) + offset;
            ip_hdr->ip_len = htons(ip_len);
        }
    }

    /* if there is a payload, add it in the context */
    if (payload_s)
    {
        /* update ptag_data with the new payload */
        libnet_pblock_t* p_data = libnet_pblock_probe(l, ptag_data, payload_s, LIBNET_PBLOCK_TCPDATA);
        if (!p_data)
        {
            goto bad;
        }

        if (libnet_pblock_append(l, p_data, payload, payload_s) == -1)
        {
            goto bad;
        }

        if (ptag_data == LIBNET_PTAG_INITIALIZER)
        {
            int insertbefore = p->ptag;

            /* Then we created it, and we need to shuffle it back until it's before
             * the tcp header and options. */
            libnet_pblock_update(l, p_data, payload_s, LIBNET_PBLOCK_TCPDATA);

            if(p->prev && p->prev->type == LIBNET_PBLOCK_TCPO_H)
                insertbefore = p->prev->ptag;

            libnet_pblock_insert_before(l, insertbefore, p_data->ptag);
        }
    }
    else
    {
        libnet_pblock_t* p_data = libnet_pblock_find(l, ptag_data);
        libnet_pblock_delete(l, p_data);
    }

    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (p->ptag);
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_tcp_options(const uint8_t *options, uint32_t options_s, libnet_t *l, 
libnet_ptag_t ptag)
{
    static const uint8_t padding[] = { 0 };
    int offset, underflow;
    uint32_t i, j, adj_size;
    libnet_pblock_t *p, *p_temp;
    struct libnet_ipv4_hdr *ip_hdr;
    struct libnet_tcp_hdr *tcp_hdr;

    if (l == NULL)
    { 
        return (-1);
    }

    underflow = 0;
    offset = 0;

    /* check options list size */
    if (options_s > LIBNET_MAXOPTION_SIZE)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): options list is too large %d\n", __func__, options_s);
        return (-1);
    }

    adj_size = options_s;
    if (adj_size % 4)
    {
        /* size of memory block with padding */
        adj_size += 4 - (options_s % 4);
    }

    /* if this pblock already exists, determine if there is a size diff */
    if (ptag)
    {
        p_temp = libnet_pblock_find(l, ptag);
        if (p_temp)
        {
            if (adj_size >= p_temp->b_len)
            {
                offset = adj_size - p_temp->b_len;
            }
            else
            {
                offset = p_temp->b_len - adj_size;
                underflow = 1;
            }
        }
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, adj_size, LIBNET_PBLOCK_TCPO_H);
    if (p == NULL)
    {
        return (-1);
    }
	
    libnet_pblock_append(l, p, options, options_s);
    libnet_pblock_append(l, p, padding, adj_size - options_s);
	
    if (ptag && p->next)
    {
        p_temp = p->next;
        while ((p_temp->next) && (p_temp->type != LIBNET_PBLOCK_TCP_H))
        {
           p_temp = p_temp->next;
        }
        if (p_temp->type == LIBNET_PBLOCK_TCP_H)
        {
            /*
             *  Count up number of 32-bit words in options list, padding if
             *  neccessary.
             */
            for (i = 0, j = 0; i < p->b_len; i++)
            {
                (i % 4) ? j : j++;
            }
            tcp_hdr = (struct libnet_tcp_hdr *)p_temp->buf;
            tcp_hdr->th_off = j + 5;
            if (!underflow)
            {
                p_temp->h_len += offset;
            }
            else
            {
                p_temp->h_len -= offset;
            }
        }
        while ((p_temp->next) && (p_temp->type != LIBNET_PBLOCK_IPV4_H))
        {
            p_temp = p_temp->next;
        }
        if (p_temp->type == LIBNET_PBLOCK_IPV4_H)
        {
            ip_hdr = (struct libnet_ipv4_hdr *)p_temp->buf;
            if (!underflow)
            {
                ip_hdr->ip_len += htons(offset);
            }
            else
            {
                ip_hdr->ip_len -= htons(offset);
            }
        }
    }

    return (ptag ? ptag : libnet_pblock_update(l, p, adj_size,
            LIBNET_PBLOCK_TCPO_H));
}

/* EOF */
