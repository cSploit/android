/*
 *  libnet
 *  libnet_build_hsrp.c - HSRP packet assembler
 *
 *  Copyright (c) 2004 David Barroso Berrueta <tomac@wasahero.org>
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
libnet_build_hsrp(uint8_t version, uint8_t opcode, uint8_t state, 
uint8_t hello_time, uint8_t hold_time, uint8_t priority, uint8_t group,
uint8_t reserved, uint8_t authdata[HSRP_AUTHDATA_LENGTH], uint32_t virtual_ip,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n;
    libnet_pblock_t *p;
    struct libnet_hsrp_hdr hsrp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, LIBNET_HSRP_H, LIBNET_PBLOCK_HSRP_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&hsrp_hdr, 0, sizeof(hsrp_hdr));
    hsrp_hdr.version = version;
    hsrp_hdr.opcode = opcode;
    hsrp_hdr.state = state;
    hsrp_hdr.hello_time = hello_time;
    hsrp_hdr.hold_time = hold_time;
    hsrp_hdr.priority = priority;
    hsrp_hdr.group = group;
    hsrp_hdr.reserved = reserved;
    memcpy(hsrp_hdr.authdata, authdata, HSRP_AUTHDATA_LENGTH*sizeof(uint8_t));
    hsrp_hdr.virtual_ip = virtual_ip;

    n = libnet_pblock_append(l, p, (uint8_t *)&hsrp_hdr, LIBNET_HSRP_H);
    if (n == -1)
    {
        goto bad;
    }

    if (payload_s && !payload)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): payload inconsistency\n", __func__);
        goto bad;
    }
 
    if (payload_s)
    {
        n = libnet_pblock_append(l, p, payload, payload_s);
        if (n == -1)
        {
            goto bad;
        }
    }
 
    return (ptag ? ptag : libnet_pblock_update(l, p, 0, LIBNET_PBLOCK_HSRP_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}
/* EOF */
