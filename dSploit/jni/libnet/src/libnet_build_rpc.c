/*
 *
 *  libnet
 *  libnet_build_rpc.c - RPC packet assembler
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *                            Jason Damron <jdamron@stackheap.org>
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
libnet_build_rpc_call(uint32_t rm, uint32_t xid, uint32_t prog_num, 
uint32_t prog_vers, uint32_t procedure, uint32_t cflavor, uint32_t clength, 
uint8_t *cdata, uint32_t vflavor, uint32_t vlength, const uint8_t *vdata, 
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_rpc_call_tcp_hdr rpc_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* Credential and Verifier buffers not yet implemented.
     * n = LIBNET_RPC_CALL_H + clength + vlength + payload_s;
     */

    if (rm)
    {
        n = LIBNET_RPC_CALL_TCP_H + payload_s;
    } 
    else
    {
        n = LIBNET_RPC_CALL_H + payload_s;
    }
 
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_RPC_CALL_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&rpc_hdr, 0, sizeof(rpc_hdr));
    if (rm)
    {
       rpc_hdr.rpc_record_marking                 = htonl(rm + payload_s);
    }
    rpc_hdr.rpc_common.rpc_xid                    = htonl(xid);
    rpc_hdr.rpc_common.rpc_type                   = LIBNET_RPC_CALL;
    rpc_hdr.rpc_common.rpc_call.rpc_rpcvers       = htonl(LIBNET_RPC_VERS);
    rpc_hdr.rpc_common.rpc_call.rpc_prognum       = htonl(prog_num);
    rpc_hdr.rpc_common.rpc_call.rpc_vers          = htonl(prog_vers);
    rpc_hdr.rpc_common.rpc_call.rpc_procedure     = htonl(procedure);
    /*  XXX Eventually should allow for opaque auth data. */
    rpc_hdr.rpc_common.rpc_call.rpc_credentials.rpc_auth_flavor= htonl(cflavor);
    rpc_hdr.rpc_common.rpc_call.rpc_credentials.rpc_auth_length= htonl(clength);
    rpc_hdr.rpc_common.rpc_call.rpc_verifier.rpc_auth_flavor   = htonl(vflavor);
    rpc_hdr.rpc_common.rpc_call.rpc_verifier.rpc_auth_length   = htonl(vlength);

    if (rm)
    {
        n = libnet_pblock_append(l, p, (uint8_t *)&rpc_hdr, 
                LIBNET_RPC_CALL_TCP_H);
    }
    else
    {
        n = libnet_pblock_append(l, p, (uint8_t *)&rpc_hdr.rpc_common, 
                LIBNET_RPC_CALL_H);
    }

    if (n == -1)
    {
        goto bad;
    }
 
    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_RPC_CALL_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
