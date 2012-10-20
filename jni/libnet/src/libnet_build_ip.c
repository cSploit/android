/*
 *  $Id: libnet_build_ip.c,v 1.19 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_ip.c - IP packet assembler
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


/* TODO len - should be calculated if -1 */
libnet_ptag_t
libnet_build_ipv4(uint16_t ip_len, uint8_t tos, uint16_t id, uint16_t frag,
uint8_t ttl, uint8_t prot, uint16_t sum, uint32_t src, uint32_t dst,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n = LIBNET_IPV4_H; /* size of memory block */
    libnet_pblock_t *p, *p_data, *p_temp;
    struct libnet_ipv4_hdr ip_hdr;
    libnet_ptag_t ptag_data = 0; /* used if there is ipv4 payload */
    libnet_ptag_t ptag_hold;

    if (l == NULL)
    { 
        return (-1);
    } 

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV4_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ip_hdr, 0, sizeof(ip_hdr));
    ip_hdr.ip_v          = 4;      /* version 4 */
    ip_hdr.ip_hl         = 5;      /* 20 byte header,  measured in 32-bit words */

    /* check to see if there are IP options to include */
    if (p->prev)
    {
        if (p->prev->type == LIBNET_PBLOCK_IPO_H)
        {
            /* IPO block's length must be multiple of 4, or it's incorrectly
             * padded, in which case there is no "correct" IP header length,
             * it will too short or too long, we choose too short.
             */
            ip_hdr.ip_hl += p->prev->b_len / 4;
        }
    }
    /* Note that p->h_len is not adjusted. This seems a bug, but it is because
     * it is not used!  libnet_do_checksum() is passed the h_len (as `len'),
     * but for IPPROTO_IP it is ignored in favor of the ip_hl.
     */

    ip_hdr.ip_tos        = tos;                       /* IP tos */
    ip_hdr.ip_len        = htons(ip_len);             /* total length */
    ip_hdr.ip_id         = htons(id);                 /* IP ID */
    ip_hdr.ip_off        = htons(frag);               /* fragmentation flags */
    ip_hdr.ip_ttl        = ttl;                       /* time to live */
    ip_hdr.ip_p          = prot;                      /* transport protocol */
    ip_hdr.ip_sum        = (sum ? htons(sum) : 0);    /* checksum */
    ip_hdr.ip_src.s_addr = src;                       /* source ip */
    ip_hdr.ip_dst.s_addr = dst;                       /* destination ip */
    
    n = libnet_pblock_append(l, p, (uint8_t *)&ip_hdr, LIBNET_IPV4_H);
    if (n == -1)
    {
        goto bad;
    }

    /* save the original ptag value */
    ptag_hold = ptag;

    if (ptag == LIBNET_PTAG_INITIALIZER)
    {
        ptag = libnet_pblock_update(l, p, LIBNET_IPV4_H, LIBNET_PBLOCK_IPV4_H);
    }

    /* find and set the appropriate ptag, or else use the default of 0 */
    /* When updating the ipv4 block, we need to find the data block, and
     * adjust our ip_offset if the new payload size is different from what
     * it used to be.
     */
    if (ptag_hold && p->prev)
    {
        p_temp = p->prev;
        while (p_temp->prev &&
              (p_temp->type != LIBNET_PBLOCK_IPDATA) &&
              (p_temp->type != LIBNET_PBLOCK_IPV4_H))
        {
            p_temp = p_temp->prev;
        }

        if (p_temp->type == LIBNET_PBLOCK_IPDATA)
        {
            ptag_data = p_temp->ptag;
        }
        else
        {
             snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                     "%s(): IPv4 data pblock not found\n", __func__);
        }
    }

    if (payload_s && !payload)
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): payload inconsistency\n", __func__);
        goto bad;
    }

    if (payload_s)
    {
        /* update ptag_data with the new payload */
        /* on create:
         *    b_len = payload_s
         *    l->total_size += b_len
         *    h_len = 0
         * on update:
         *    b_len = payload_s
         *    h_len += <diff in size between new b_len and old b_len>
         *      increments if if b_len goes up, down if it goes down
         * in either case:
         *    copied = 0
	 */
        p_data = libnet_pblock_probe(l, ptag_data, payload_s,
                LIBNET_PBLOCK_IPDATA);
        if (p_data == NULL)
        {
            return (-1);
        }

        if (libnet_pblock_append(l, p_data, payload, payload_s) == -1)
        {
            goto bad;
        }

        if (ptag_data == LIBNET_PTAG_INITIALIZER)
        {
            /* IPDATA's h_len gets set to payload_s in both branches */
            if (p_data->prev->type == LIBNET_PBLOCK_IPV4_H)
            {
                libnet_pblock_update(l, p_data, payload_s,
                        LIBNET_PBLOCK_IPDATA);
                /* swap pblocks to correct the protocol order */
                libnet_pblock_swap(l, p->ptag, p_data->ptag); 
            }
            else
            {
                /* SR - I'm not sure how to reach this code. Maybe if the first
                 * time we added an ipv4 block, there was no payload, but when
                 * we modify the block the next time, we have payload?
		 */

                /* update without setting this as the final pblock */
                p_data->type  =  LIBNET_PBLOCK_IPDATA;
                p_data->ptag  =  ++(l->ptag_state);
                p_data->h_len =  payload_s; /* TODO dead code, data blocks don't have headers */

                /* data was added after the initial construction */
                for (p_temp = l->protocol_blocks;
                        p_temp->type == LIBNET_PBLOCK_IPV4_H ||
                        p_temp->type == LIBNET_PBLOCK_IPO_H;
                        p_temp = p_temp->next)
                {
                    libnet_pblock_insert_before(l, p_temp->ptag, p_data->ptag);
                    break;
                }

                /* the end block needs to have its next pointer cleared */
                l->pblock_end->next = NULL;
            }

            if (p_data->prev && p_data->prev->type == LIBNET_PBLOCK_IPO_H)
            {
                libnet_pblock_swap(l, p_data->prev->ptag, p_data->ptag); 
            }
        }
    }
    else
    {
        p_data = libnet_pblock_find(l, ptag_data);
        if (p_data) 
        {
            libnet_pblock_delete(l, p_data);
        }
        else
        {
            /* 
             * XXX - When this completes successfully, libnet errbuf contains 
             * an error message so to come correct, we'll clear it.
             */ 
            memset(l->err_buf, 0, sizeof (l->err_buf));
        }
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

    return (ptag);
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_autobuild_ipv4(uint16_t len, uint8_t prot, uint32_t dst, libnet_t *l)
{
    uint32_t n, i, j, src;
    uint16_t h;
    libnet_pblock_t *p;
    libnet_ptag_t ptag;
    struct libnet_ipv4_hdr ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_IPV4_H;                                /* size of memory block */
    h = len;                                          /* header length */
    ptag = LIBNET_PTAG_INITIALIZER;
    src = libnet_get_ipaddr4(l);
    if (src == -1)
    {
        /* err msg set in libnet_get_ipaddr() */ 
        return (-1);
    }

    /*
     *  Create a new pblock.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV4_H);
    if (p == NULL)
    {
        return (-1);
    }
	
    memset(&ip_hdr, 0, sizeof(ip_hdr));
    ip_hdr.ip_v          = 4;                         /* version 4 */
    ip_hdr.ip_hl         = 5;                         /* 20 byte header */

    /* check to see if there are IP options to include */
    if (p->prev)
    {
        if (p->prev->type == LIBNET_PBLOCK_IPO_H)
        {
            /*
             *  Count up number of 32-bit words in options list, padding if
             *  neccessary.
             */
            for (i = 0, j = 0; i < p->prev->b_len; i++)
            {
                (i % 4) ? j : j++;
            }
            ip_hdr.ip_hl += j;
        }
    }

    ip_hdr.ip_tos        = 0;                         /* IP tos */
    ip_hdr.ip_len        = htons(h);                  /* total length */
    ip_hdr.ip_id         = htons((l->ptag_state) & 0x0000ffff); /* IP ID */
    ip_hdr.ip_off        = 0;                         /* fragmentation flags */
    ip_hdr.ip_ttl        = 64;                        /* time to live */
    ip_hdr.ip_p          = prot;                      /* transport protocol */
    ip_hdr.ip_sum        = 0;                         /* checksum */
    ip_hdr.ip_src.s_addr = src;                       /* source ip */
    ip_hdr.ip_dst.s_addr = dst;                       /* destination ip */

    n = libnet_pblock_append(l, p, (uint8_t *)&ip_hdr, LIBNET_IPV4_H);
    if (n == -1)
    {
        goto bad;
    }

    libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    ptag = libnet_pblock_update(l, p, LIBNET_IPV4_H, LIBNET_PBLOCK_IPV4_H);

    return (ptag);

bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv4_options(const uint8_t *options, uint32_t options_s, libnet_t *l, 
libnet_ptag_t ptag)
{
    int options_size_increase = 0; /* increase will be negative if it's a decrease */
    uint32_t n, adj_size;
    libnet_pblock_t *p, *p_temp;
    struct libnet_ipv4_hdr *ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    }

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
            options_size_increase = adj_size - p_temp->b_len;
        }
    }
    /* If we aren't modifying an options block, we are pushing a new one, and
     * since it must be pushed before the IPv4 block is pushed, there is no
     * need to remember that options size has "increased".
     */
    
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, adj_size, LIBNET_PBLOCK_IPO_H);
    if (p == NULL)
    {
        return (-1);
    }

    /* append options */
    n = libnet_pblock_append(l, p, options, options_s);
    if (n == -1)
    {
        goto bad;
    }

    /* append padding */
    n = libnet_pblock_append(l, p, (uint8_t*)"\0\0\0", adj_size - options_s);
    if (n == -1)
    {
        goto bad;
    }

    if (ptag && p->next)
    {
        p_temp = p->next;

        /* fix the IP header sizes */
        if (p_temp->type == LIBNET_PBLOCK_IPV4_H)
        {
            ip_hdr = (struct libnet_ipv4_hdr *) p_temp->buf;
            ip_hdr->ip_hl = 5 + adj_size / 4; /* 4 bits wide, so no byte order concerns */
            ip_hdr->ip_len = htons(ntohs(ip_hdr->ip_len) + options_size_increase);

            p_temp->h_len = ip_hdr->ip_hl * 4; /* Dead code, h_len isn't used for IPv4 block */
        }
    }

    return (ptag ? ptag : libnet_pblock_update(l, p, adj_size,
            LIBNET_PBLOCK_IPO_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv6(uint8_t tc, uint32_t fl, uint16_t len, uint8_t nh,
uint8_t hl, struct libnet_in6_addr src, struct libnet_in6_addr dst, 
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{   
    uint32_t n;
    libnet_pblock_t *p;
    struct libnet_ipv6_hdr ip_hdr;

    if (l == NULL)
    {
        return (-1);
    }

    n = LIBNET_IPV6_H + payload_s;          /* size of memory block */
       
    if (LIBNET_IPV6_H + payload_s > IP_MAXPACKET)
    {  
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): IP packet too large\n", __func__);
        return (-1);
    }  
       
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV6_H);
    if (p == NULL)
    {   
        return (-1);
    }  
    
    memset(&ip_hdr, 0, sizeof(ip_hdr));
    ip_hdr.ip_flags[0] = 0x06 << 4;
    ip_hdr.ip_flags[1] = ((tc & 0x0F) << 4) | ((fl & 0xF0000) >> 16);
    ip_hdr.ip_flags[2] = fl & 0x0FF00 >> 8;
    ip_hdr.ip_flags[3] = fl & 0x000FF;
    ip_hdr.ip_len      = htons(len);
    ip_hdr.ip_nh       = nh;
    ip_hdr.ip_hl       = hl;
    ip_hdr.ip_src      = src;
    ip_hdr.ip_dst      = dst;
     
    n = libnet_pblock_append(l, p, (uint8_t *)&ip_hdr, LIBNET_IPV6_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    /* no checksum for IPv6 */
    ptag = ptag ? ptag : libnet_pblock_update(l, p, LIBNET_IPV6_H,
            LIBNET_PBLOCK_IPV6_H);

    return ptag;

bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv6_frag(uint8_t nh, uint8_t reserved, uint16_t frag,
uint32_t id, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n;
    uint16_t h;
    libnet_pblock_t *p;
    struct libnet_ipv6_frag_hdr ipv6_frag_hdr;

    if (l == NULL)
    { 
        return (-1);
    }

    n = LIBNET_IPV6_FRAG_H + payload_s;
    h = 0; 

    if (LIBNET_IPV6_FRAG_H + payload_s > IP_MAXPACKET)
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): IP packet too large\n", __func__);
        return (-1);
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV6_FRAG_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ipv6_frag_hdr, 0 , sizeof(ipv6_frag_hdr));
    ipv6_frag_hdr.ip_nh       = nh;
    ipv6_frag_hdr.ip_reserved = reserved;
    ipv6_frag_hdr.ip_frag     = frag;
    ipv6_frag_hdr.ip_id       = id;

    /*
     *  Appened the protocol unit to the list.
     */
    n = libnet_pblock_append(l, p, (uint8_t *)&ipv6_frag_hdr,
        LIBNET_IPV6_FRAG_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    /*
     *  Update the protocol block's meta information and return the protocol
     *  tag id of this pblock.  This tag will be used to locate the pblock
     *  in order to modify the protocol header in subsequent calls.
     */
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPV6_FRAG_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv6_routing(uint8_t nh, uint8_t len, uint8_t rtype, 
uint8_t segments, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n;
    uint16_t h;
    libnet_pblock_t *p;
    struct libnet_ipv6_routing_hdr ipv6_routing_hdr;

    if (l == NULL)
    { 
        return (-1);
    }

    /* Important: IPv6 routing header routes are specified using the payload
     * interface!
     */
    n = LIBNET_IPV6_ROUTING_H + payload_s;
    h = 0;

    if (LIBNET_IPV6_ROUTING_H + payload_s > IP_MAXPACKET)
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): IP packet too large\n", __func__);
        return (-1);
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV6_ROUTING_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ipv6_routing_hdr, 0 , sizeof(ipv6_routing_hdr));
    ipv6_routing_hdr.ip_nh       = nh;
    ipv6_routing_hdr.ip_len      = len;
    ipv6_routing_hdr.ip_rtype    = rtype;
    ipv6_routing_hdr.ip_segments = segments;

    /*
     *  Appened the protocol unit to the list.
     */
    n = libnet_pblock_append(l, p, (uint8_t *)&ipv6_routing_hdr,
        LIBNET_IPV6_ROUTING_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    /*
     *  Update the protocol block's meta information and return the protocol
     *  tag id of this pblock.  This tag will be used to locate the pblock
     *  in order to modify the protocol header in subsequent calls.
     */
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPV6_ROUTING_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv6_destopts(uint8_t nh, uint8_t len, const uint8_t *payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n;
    uint16_t h;
    libnet_pblock_t *p;
    struct libnet_ipv6_destopts_hdr ipv6_destopts_hdr;

    if (l == NULL)
    { 
        return (-1);
    }

    /* Important: IPv6 dest opts information is specified using the payload
     * interface!
     */
    n = LIBNET_IPV6_DESTOPTS_H + payload_s;
    h = 0;

    if (LIBNET_IPV6_DESTOPTS_H + payload_s > IP_MAXPACKET)
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): IP packet too large\n", __func__);
        return (-1);
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV6_DESTOPTS_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ipv6_destopts_hdr, 0 , sizeof(ipv6_destopts_hdr));
    ipv6_destopts_hdr.ip_nh  = nh;
    ipv6_destopts_hdr.ip_len = len;

    /*
     *  Appened the protocol unit to the list.
     */
    n = libnet_pblock_append(l, p, (uint8_t *)&ipv6_destopts_hdr,
        LIBNET_IPV6_DESTOPTS_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    /*
     *  Update the protocol block's meta information and return the protocol
     *  tag id of this pblock.  This tag will be used to locate the pblock
     *  in order to modify the protocol header in subsequent calls.
     */
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPV6_DESTOPTS_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv6_hbhopts(uint8_t nh, uint8_t len, const uint8_t *payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n;
    uint16_t h;
    libnet_pblock_t *p;
    struct libnet_ipv6_hbhopts_hdr ipv6_hbhopts_hdr;

    if (l == NULL)
    { 
        return (-1);
    }

    /* Important: IPv6 hop by hop opts information is specified using the
     * payload interface!
     */
    n = LIBNET_IPV6_HBHOPTS_H + payload_s;
    h = 0;

    if (LIBNET_IPV6_HBHOPTS_H + payload_s > IP_MAXPACKET)
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): IP packet too large\n", __func__);
        return (-1);
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV6_HBHOPTS_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ipv6_hbhopts_hdr, 0 , sizeof(ipv6_hbhopts_hdr));
    ipv6_hbhopts_hdr.ip_nh  = nh;
    ipv6_hbhopts_hdr.ip_len = len;

    /*
     *  Appened the protocol unit to the list.
     */
    n = libnet_pblock_append(l, p, (uint8_t *)&ipv6_hbhopts_hdr,
        LIBNET_IPV6_HBHOPTS_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    /*
     *  Update the protocol block's meta information and return the protocol
     *  tag id of this pblock.  This tag will be used to locate the pblock
     *  in order to modify the protocol header in subsequent calls.
     */
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPV6_HBHOPTS_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_autobuild_ipv6(uint16_t len, uint8_t nh, struct libnet_in6_addr dst,
            libnet_t *l, libnet_ptag_t ptag)
{

    /* NYI */
     snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
             "%s(): not yet implemented\n", __func__);
    return (-1);
}

/* EOF */
