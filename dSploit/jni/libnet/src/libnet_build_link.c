/*
 *  $Id: libnet_build_link.c,v 1.10 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_link.c - link-layer packet assembler
 *
 *  Copyright (c) 2003 Roberto Larcher <roberto.larcher@libero.it>
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
libnet_build_link(const uint8_t *dst, const uint8_t *src, const uint8_t *oui, uint16_t type,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)

{
    uint8_t org[3] = {0x00, 0x00, 0x00};
    switch (l->link_type)
    {
        /* add FDDI */
        case DLT_EN10MB:
            return libnet_build_ethernet(dst, src, type, payload, payload_s, l,
                    ptag);
        case DLT_IEEE802:
            return libnet_build_token_ring(LIBNET_TOKEN_RING_FRAME,
                    LIBNET_TOKEN_RING_LLC_FRAME, dst, src, LIBNET_SAP_SNAP,
                    LIBNET_SAP_SNAP, 0x03, org, type, payload, payload_s,
                    l, ptag);
    }
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): linktype %d not supported\n", __func__, l->link_type);
    return -1;
}

libnet_ptag_t
libnet_autobuild_link(const uint8_t *dst, const uint8_t *oui, uint16_t type, libnet_t *l)
{
    uint8_t org[3] = {0x00, 0x00, 0x00};
    switch (l->link_type)
    {
       /* add FDDI */
        case DLT_EN10MB:
            return (libnet_autobuild_ethernet(dst, type, l));
        case DLT_IEEE802:
            return (libnet_autobuild_token_ring(LIBNET_TOKEN_RING_FRAME,
                   LIBNET_TOKEN_RING_LLC_FRAME, dst, LIBNET_SAP_SNAP,
                   LIBNET_SAP_SNAP, 0x03, org, TOKEN_RING_TYPE_IP, l));
    }
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): linktype %d not supported\n", __func__, l->link_type);
    return (-1);
}

