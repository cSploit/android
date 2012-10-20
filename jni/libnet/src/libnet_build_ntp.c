/*
 *  $Id: libnet_build_ntp.c,v 1.11 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_build_ntp.c - NTP packet assembler
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
libnet_build_ntp(uint8_t leap_indicator, uint8_t version, uint8_t mode,
uint8_t stratum, uint8_t poll, uint8_t precision, uint16_t delay_int,
uint16_t delay_frac, uint16_t dispersion_int, uint16_t dispersion_frac,
uint32_t reference_id, uint32_t ref_ts_int, uint32_t ref_ts_frac,
uint32_t orig_ts_int, uint32_t orig_ts_frac, uint32_t rec_ts_int,
uint32_t rec_ts_frac, uint32_t xmt_ts_int, uint32_t xmt_ts_frac,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_ntp_hdr ntp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_NTP_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_NTP_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ntp_hdr, 0, sizeof(ntp_hdr));
    ntp_hdr.ntp_li_vn_mode          = LIBNET_NTP_DO_LI_VN_MODE(
                                      leap_indicator, version, mode);
    ntp_hdr.ntp_stratum             = stratum;
    ntp_hdr.ntp_poll                = poll;
    ntp_hdr.ntp_precision           = precision;
    ntp_hdr.ntp_delay.integer       = htons(delay_int);
    ntp_hdr.ntp_delay.fraction      = htons(delay_frac);
    ntp_hdr.ntp_dispersion.integer  = htons(dispersion_int);
    ntp_hdr.ntp_dispersion.fraction = htons(dispersion_frac);
    ntp_hdr.ntp_reference_id        = htonl(reference_id);
    ntp_hdr.ntp_ref_ts.integer      = htonl(ref_ts_int);
    ntp_hdr.ntp_ref_ts.fraction     = htonl(ref_ts_frac);
    ntp_hdr.ntp_orig_ts.integer     = htonl(orig_ts_int);
    ntp_hdr.ntp_orig_ts.fraction    = htonl(orig_ts_frac);
    ntp_hdr.ntp_rec_ts.integer      = htonl(rec_ts_int);
    ntp_hdr.ntp_rec_ts.fraction     = htonl(rec_ts_frac);
    ntp_hdr.ntp_xmt_ts.integer      = htonl(xmt_ts_int);
    ntp_hdr.ntp_xmt_ts.fraction     = htonl(xmt_ts_frac);

    n = libnet_pblock_append(l, p, (uint8_t *)&ntp_hdr, LIBNET_NTP_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_NTP_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
