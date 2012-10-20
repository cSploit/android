/*
 *  $Id: libnet_build_ospf.c,v 1.12 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_build_ospf.c - OSPF packet assembler
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 *  Copyright (c) 1999, 2000 Andrew Reiter <areiter@bindview.com>
 *  Bindview Development
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
libnet_build_ospfv2(uint16_t len, uint8_t type, uint32_t rtr_id, 
uint32_t area_id, uint16_t sum, uint16_t autype, const uint8_t *payload, 
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_ospf_hdr ospf_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 
 
    n = LIBNET_OSPF_H + payload_s;
    h = LIBNET_OSPF_H + payload_s + len;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_OSPF_H);
    if (p == NULL)
    {
        return (-1);
    }
    
    memset(&ospf_hdr, 0, sizeof(ospf_hdr));
    ospf_hdr.ospf_v               = 2;              /* OSPF version 2 */
    ospf_hdr.ospf_type            = type;           /* Type of pkt */
    ospf_hdr.ospf_len             = htons(h);       /* Pkt len */
    ospf_hdr.ospf_rtr_id.s_addr   = rtr_id;  /* Router ID */
    ospf_hdr.ospf_area_id.s_addr  = area_id; /* Area ID */
    ospf_hdr.ospf_sum             = sum;
    ospf_hdr.ospf_auth_type       = htons(autype);  /* Type of auth */

    n = libnet_pblock_append(l, p, (uint8_t *)&ospf_hdr, LIBNET_OSPF_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_OSPF_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_hello(uint32_t netmask, uint16_t interval, uint8_t opts, 
uint8_t priority, uint32_t dead_int, uint32_t des_rtr, uint32_t bkup_rtr,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_ospf_hello_hdr hello_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_HELLO_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_OSPF_HELLO_H);
    if (p == NULL)
    {
        return (-1);
    }
    
    memset(&hello_hdr, 0, sizeof(hello_hdr));
    hello_hdr.hello_nmask.s_addr    = netmask;  /* Netmask */
    hello_hdr.hello_intrvl          = htons(interval);	/* # seconds since last packet sent */
    hello_hdr.hello_opts            = opts;     /* OSPF_* options */
    hello_hdr.hello_rtr_pri         = priority; /* If 0, can't be backup */
    hello_hdr.hello_dead_intvl      = htonl(dead_int); /* Time til router is deemed down */
    hello_hdr.hello_des_rtr.s_addr  = des_rtr;	/* Networks designated router */
    hello_hdr.hello_bkup_rtr.s_addr = bkup_rtr; /* Networks backup router */
    /*hello_hdr.hello_nbr.s_addr      = htonl(neighbor); */

    n = libnet_pblock_append(l, p, (uint8_t *)&hello_hdr, LIBNET_OSPF_HELLO_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_OSPF_HELLO_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_dbd(uint16_t dgram_len, uint8_t opts, uint8_t type,
uint32_t seqnum, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_dbd_hdr dbd_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_DBD_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_OSPF_DBD_H);
    if (p == NULL)
    {
        return (-1);
    }
    
    memset(&dbd_hdr, 0, sizeof(dbd_hdr));
    dbd_hdr.dbd_mtu_len	= htons(dgram_len); /* Max length of IP packet IF can use */
    dbd_hdr.dbd_opts    = opts;	            /* OSPF_* options */
    dbd_hdr.dbd_type    = type;	            /* Type of exchange occuring */
    dbd_hdr.dbd_seq     = htonl(seqnum);    /* DBD sequence number */

    n = libnet_pblock_append(l, p, (uint8_t *)&dbd_hdr, LIBNET_OSPF_DBD_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_OSPF_DBD_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsr(uint32_t type, uint lsid, uint32_t advrtr, 
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_lsr_hdr lsr_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_LSR_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_OSPF_LSR_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&lsr_hdr, 0, sizeof(lsr_hdr));
    lsr_hdr.lsr_type         = htonl(type);     /* Type of LS being requested */
    lsr_hdr.lsr_lsid	     = htonl(lsid);     /* Link State ID */
    lsr_hdr.lsr_adrtr.s_addr = htonl(advrtr);   /* Advertising router */

    n = libnet_pblock_append(l, p, (uint8_t *)&lsr_hdr, LIBNET_OSPF_LSR_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_OSPF_LSR_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsu(uint32_t num, const uint8_t *payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_lsu_hdr lh_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_LSU_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_OSPF_LSU_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&lh_hdr, 0, sizeof(lh_hdr));
    lh_hdr.lsu_num = htonl(num);   /* Number of LSAs that will be bcasted */

    n = libnet_pblock_append(l, p, (uint8_t *)&lh_hdr, LIBNET_OSPF_LSU_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_OSPF_LSU_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsa(uint16_t age, uint8_t opts, uint8_t type, uint lsid,
uint32_t advrtr, uint32_t seqnum, uint16_t sum, uint16_t len,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_lsa_hdr lsa_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_LSA_H + payload_s;
    h = len + payload_s;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_OSPF_LSA_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&lsa_hdr, 0, sizeof(lsa_hdr));
    lsa_hdr.lsa_age         = htons(age);
    lsa_hdr.lsa_opts        = opts;
    lsa_hdr.lsa_type        = type;
    lsa_hdr.lsa_id          = htonl(lsid);
    lsa_hdr.lsa_adv.s_addr  = htonl(advrtr);
    lsa_hdr.lsa_seq         = htonl(seqnum);
    lsa_hdr.lsa_sum         = sum;
    lsa_hdr.lsa_len         = htons(h);

    n = libnet_pblock_append(l, p, (uint8_t *)&lsa_hdr, LIBNET_OSPF_LSA_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_OSPF_LSA_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsa_rtr(uint16_t flags, uint16_t num, uint32_t id,
uint32_t data, uint8_t type, uint8_t tos, uint16_t metric,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_rtr_lsa_hdr rtr_lsa_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_LS_RTR_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_LS_RTR_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&rtr_lsa_hdr, 0, sizeof(rtr_lsa_hdr));
    rtr_lsa_hdr.rtr_flags       = htons(flags);
    rtr_lsa_hdr.rtr_num         = htons(num);
    rtr_lsa_hdr.rtr_link_id     = htonl(id);
    rtr_lsa_hdr.rtr_link_data   = htonl(data);
    rtr_lsa_hdr.rtr_type        = type;
    rtr_lsa_hdr.rtr_tos_num     = tos;
    rtr_lsa_hdr.rtr_metric      = htons(metric);

    n = libnet_pblock_append(l, p, (uint8_t *)&rtr_lsa_hdr,
            LIBNET_OSPF_LS_RTR_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_LS_RTR_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsa_net(uint32_t nmask, uint32_t rtrid,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_net_lsa_hdr net_lsa_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_LS_NET_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_LS_NET_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&net_lsa_hdr, 0, sizeof(net_lsa_hdr));
    net_lsa_hdr.net_nmask.s_addr    = htonl(nmask);
    net_lsa_hdr.net_rtr_id          = htonl(rtrid);

    n = libnet_pblock_append(l, p, (uint8_t *)&net_lsa_hdr,
            LIBNET_OSPF_LS_NET_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_LS_NET_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsa_sum(uint32_t nmask, uint32_t metric, uint tos, 
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_sum_lsa_hdr sum_lsa_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_OSPF_LS_SUM_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_LS_SUM_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&sum_lsa_hdr, 0, sizeof(sum_lsa_hdr));
    sum_lsa_hdr.sum_nmask.s_addr    = htonl(nmask);
    sum_lsa_hdr.sum_metric          = htonl(metric);
    sum_lsa_hdr.sum_tos_metric      = htonl(tos);

    n = libnet_pblock_append(l, p, (uint8_t *)&sum_lsa_hdr,
            LIBNET_OSPF_LS_SUM_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_LS_SUM_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ospfv2_lsa_as(uint32_t nmask, uint metric, uint32_t fwdaddr,
uint32_t tag, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_as_lsa_hdr as_lsa_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 
   
    n = LIBNET_OSPF_LS_AS_EXT_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_LS_AS_EXT_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&as_lsa_hdr, 0, sizeof(as_lsa_hdr));
    as_lsa_hdr.as_nmask.s_addr      = htonl(nmask);
    as_lsa_hdr.as_metric            = htonl(metric);
    as_lsa_hdr.as_fwd_addr.s_addr   = htonl(fwdaddr);
    as_lsa_hdr.as_rte_tag           = htonl(tag);

    n = libnet_pblock_append(l, p, (uint8_t *)&as_lsa_hdr,
            LIBNET_OSPF_LS_AS_EXT_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_LS_AS_EXT_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
