/*
 *  $Id: libnet_advanced.c,v 1.8 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_advanced.c - Advanced routines
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

int
libnet_adv_cull_packet(libnet_t *l, uint8_t **packet, uint32_t *packet_s)
{
    *packet = NULL;
    *packet_s = 0;

    if (l->injection_type != LIBNET_LINK_ADV)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): advanced link mode not enabled\n", __func__);
        return (-1);
    }

    /* checksums will be written in */
    return (libnet_pblock_coalesce(l, packet, packet_s));
}

int
libnet_adv_cull_header(libnet_t *l, libnet_ptag_t ptag, uint8_t **header,
        uint32_t *header_s)
{
    libnet_pblock_t *p;

    *header = NULL;
    *header_s = 0;

    if (l->injection_type != LIBNET_LINK_ADV)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): advanced link mode not enabled\n", __func__);
        return (-1);
    }

    p = libnet_pblock_find(l, ptag);
    if (p == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): ptag not found, you sure it exists?\n", __func__);
        return (-1);
    }
    *header   = p->buf;
    *header_s = p->b_len;

    return (1);
}

int
libnet_adv_write_link(libnet_t *l, const uint8_t *packet, uint32_t packet_s)
{
    int c;

    if (l->injection_type != LIBNET_LINK_ADV)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): advanced link mode not enabled\n", __func__);
        return (-1);
    }
    c = libnet_write_link(l, packet, packet_s);

    /* do statistics */
    if (c == packet_s)
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
    return (c);
}

int
libnet_adv_write_raw_ipv4(libnet_t *l, const uint8_t *packet, uint32_t packet_s)
{
    int c;

    if (l->injection_type != LIBNET_RAW4_ADV)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): advanced raw4 mode not enabled\n", __func__);
        return (-1);
    }
    c = libnet_write_raw_ipv4(l, packet, packet_s);

    /* do statistics */
    if (c == packet_s)
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
    return (c);
}

void
libnet_adv_free_packet(libnet_t *l, uint8_t *packet)
{
    /*
     *  Restore original pointer address so free won't complain about a
     *  modified chunk pointer.
     */
    if (l->aligner > 0)
    {
        packet = packet - l->aligner;
    }
    free(packet);
}

/* EOF */
