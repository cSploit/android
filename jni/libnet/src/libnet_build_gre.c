/*
 *  libnet
 *  libnet_build_gre.c - GRE packet assembler
 *
 *  Copyright (c) 2003 Frédéric Raynal <pappy@security-labs.org>
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

/*
 * Overall packet
 *
 *  The entire encapsulated packet would then have the form:
 *
 *                  ---------------------------------
 *                  |                               |
 *                  |       Delivery Header         |
 *                  |                               |
 *                  ---------------------------------
 *                  |                               |
 *                  |       GRE Header              |
 *                  |                               |
 *                  ---------------------------------
 *                  |                               |
 *                  |       Payload packet          |
 *                  |                               |
 *                  ---------------------------------
 *
 * RFC 1701 defines a header. 
 * A new RFC (2784) has changed the header and proposed to remove the key 
 * and seqnum.
 * A newer RFC (2890) has changed the header proposed in RFC 2784 by putting
 * back key and seqnum.
 * These will be supported the day IETF'guys stop this mess !
 *
 *   FR
 */


/* 
 * Generic Routing Encapsulation (GRE) 
 * RFC 1701 http://www.faqs.org/rfcs/rfc1701.html
 *				   
 *
 * Packet header
 *
 *   The GRE packet header has the form:
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |C|R|K|S|s|Recur|  Flags  | Ver |         Protocol Type         |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |      Checksum (optional)      |       Offset (optional)       |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                         Key (optional)                        |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                    Sequence Number (optional)                 |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                         Routing (optional)                    |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Enhanced GRE header
 *
 *   See rfc 2637 for details. It is used for PPTP tunneling.
 * 
 *          0                   1                   2                   3
 *          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *         |C|R|K|S|s|Recur|A| Flags | Ver |         Protocol Type         |
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *         |    Key (HW) Payload Length    |       Key (LW) Call ID        |
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *         |                  Sequence Number (Optional)                   |
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *         |               Acknowledgment Number (Optional)                |
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      
 */
#if 0
static void
__libnet_print_gre_flags_ver(uint16_t fv)
{
    printf("version = %d (%d) -> ",
        fv & GRE_VERSION_MASK, libnet_getgre_length(fv));
    if (fv & GRE_CSUM)
    {
        printf("CSUM ");
    }
    if (fv & GRE_ROUTING)
    {
        printf("ROUTING ");
    }
    if (fv & GRE_KEY)
    {
        printf("KEY ");
    }
    if (fv & GRE_SEQ)
    {
        printf("SEQ ");
    }
    if (fv & GRE_ACK)
    {
        printf("ACK ");
    }
    printf("\n");
}
#endif

/* FIXME: what is the portability of the "((struct libnet_gre_hdr*)0)->" ? */
uint32_t
libnet_getgre_length(uint16_t fv)
{

    uint32_t n = LIBNET_GRE_H;
    /*
     * If either the Checksum Present bit or the Routing Present bit are
     * set, BOTH the Checksum and Offset fields are present in the GRE
     * packet.
     */

    if ((!(fv & GRE_VERSION_MASK) && (fv & (GRE_CSUM|GRE_ROUTING))) || /* v0 */
	(fv & GRE_VERSION_MASK) )                                      /* v1 */
    {
	n += sizeof( ((struct libnet_gre_hdr *)0)->gre_sum) + 
	    sizeof( ((struct libnet_gre_hdr *)0)->gre_offset);
    }

    if ((!(fv & GRE_VERSION_MASK) && (fv & GRE_KEY)) ||                /* v0 */
	( (fv & GRE_VERSION_MASK) && (fv & GRE_SEQ)) )                 /* v1 */
    {
	n += sizeof( ((struct libnet_gre_hdr *)0)->gre_key);
    }

    if ((!(fv & GRE_VERSION_MASK) && (fv & GRE_SEQ)) ||                /* v0 */
	( (fv & GRE_VERSION_MASK) && (fv & GRE_ACK)) )                 /* v1 */
    {
	n += sizeof( ((struct libnet_gre_hdr *)0)->gre_seq );
    }

    return (n);
}

libnet_ptag_t
libnet_build_gre(uint16_t fv, uint16_t type, uint16_t sum, 
uint16_t offset, uint32_t key, uint32_t seq, uint16_t len,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n;
    libnet_pblock_t *p;
    struct libnet_gre_hdr gre_hdr;

    if (l == NULL)
    { 
        return (-1); 
    }

    n = libnet_getgre_length(fv) + payload_s;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_GRE_H);
    if (p == NULL)
    {
        return (-1);
    }

    gre_hdr.flags_ver = htons(fv);
    gre_hdr.type      = htons(type);
    n = libnet_pblock_append(l, p, (uint8_t *)&gre_hdr, LIBNET_GRE_H);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad; 
    }

    if ((!(fv & GRE_VERSION_MASK) && (fv & (GRE_CSUM|GRE_ROUTING))) || /* v0 */
	(fv & GRE_VERSION_MASK))                                       /* v1 */
    {
        sum = htons(sum);
        n = libnet_pblock_append(l, p, (uint8_t*)&sum,
                sizeof(gre_hdr.gre_sum));
	if (n == -1)
	{
	    /* err msg set in libnet_pblock_append() */
	    goto bad;
	}
	offset = htons(offset);
	n = libnet_pblock_append(l, p, (uint8_t*)&offset, 
                sizeof(gre_hdr.gre_offset));
	if (n == -1)
	{
	    /* err msg set in libnet_pblock_append() */
	    goto bad;
	}
    }

    if ((!(fv & GRE_VERSION_MASK) && (fv & GRE_KEY)) ||                /* v0 */
	( (fv & GRE_VERSION_MASK) && (fv & GRE_SEQ)) )                 /* v1 */
    {
	key = htonl(key);
	n = libnet_pblock_append(l, p, (uint8_t*)&key,
                sizeof(gre_hdr.gre_key));
	if (n == -1)
	{
	    /* err msg set in libnet_pblock_append() */
	    goto bad;
	}
    }

    if ((!(fv & GRE_VERSION_MASK) && (fv & GRE_SEQ)) ||                /* v0 */
	( (fv & GRE_VERSION_MASK) && (fv & GRE_ACK)) )                 /* v1 */
    {
	seq = htonl(seq);
	n = libnet_pblock_append(l, p, (uint8_t*)&seq, 
                sizeof(gre_hdr.gre_seq));
	if (n == -1)
	{
	    /* err msg set in libnet_pblock_append() */
	    goto bad;
	}
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    if ( (fv & GRE_CSUM) && (!sum) )
    {
	libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }

    return (ptag ? ptag : libnet_pblock_update(l, p, len, LIBNET_PBLOCK_GRE_H));

bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_egre(uint16_t fv, uint16_t type, uint16_t sum, 
uint16_t offset, uint32_t key, uint32_t seq, uint16_t len,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    return (libnet_build_gre(fv, type, sum, offset, key, seq, len, 
           payload, payload_s, l, ptag));
}

/*
 *    Routing (variable)
 *
 *      The Routing field is optional and is present only if the Routing
 *      Present bit is set to 1.
 *
 *      The Routing field is a list of Source Route Entries (SREs).  Each
 *      SRE has the form:
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |       Address Family          |  SRE Offset   |  SRE Length   |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                        Routing Information ...
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
libnet_ptag_t
libnet_build_gre_sre(uint16_t af, uint8_t offset, uint8_t length, 
uint8_t *routing, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n;
    libnet_pblock_t *p;
    struct libnet_gre_sre_hdr sre_hdr;

    if (l == NULL)
    { 
        return (-1); 
    }

    n = LIBNET_GRE_SRE_H + length + payload_s;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_GRE_SRE_H);
    if (p == NULL)
    {
        return (-1);
    }
    sre_hdr.af = htons(af);
    sre_hdr.sre_offset = offset;
    sre_hdr.sre_length = length;
    n = libnet_pblock_append(l, p, (uint8_t *)&sre_hdr, LIBNET_GRE_SRE_H);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad; 
    }

    if ((routing && !length) || (!routing && length))
    {
        sprintf(l->err_buf, "%s(): routing inconsistency\n", __func__);
        goto bad;
    }

    if (routing && length)
    {
        n = libnet_pblock_append(l, p, routing, length);
        if (n == -1)
        {
            /* err msg set in libnet_pblock_append() */
            goto bad;
        }
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, 0, 
           LIBNET_PBLOCK_GRE_SRE_H));

bad:
    libnet_pblock_delete(l, p);
    return (-1);

}

libnet_ptag_t
libnet_build_gre_last_sre(libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, zero = 0;
    libnet_pblock_t *p;

    if (l == NULL)
    { 
        return (-1); 
    }

    n = LIBNET_GRE_SRE_H;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_GRE_H);
    if (p == NULL)
    {
        return (-1);
    }

    n = libnet_pblock_append(l, p, (uint8_t *)&zero, LIBNET_GRE_SRE_H);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad; 
    }

    return (ptag ? ptag : libnet_pblock_update(l, p, 0, 
           LIBNET_PBLOCK_GRE_SRE_H));

bad:
    libnet_pblock_delete(l, p);
    return (-1);

}
/* EOF */
