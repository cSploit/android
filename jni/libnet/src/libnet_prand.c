/*
 *  $Id: libnet_prand.c,v 1.7 2004/01/28 19:45:00 mike Exp $
 *
 *  libnet
 *  libnet_prand.c - pseudo-random number generation
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
libnet_seed_prand(libnet_t *l)
{
	#if !(__WIN32__)
    struct timeval seed;
	#endif

    if (l == NULL)
    { 
        return (-1);
    } 

	#if __WIN32__
    srand((unsigned)time(NULL));
	#else
    if (gettimeofday(&seed, NULL) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): cannot gettimeofday\n", __func__);
        return (-1);
    }

    /*
     *  More entropy then just seeding with time(2).
     */
    srandom((unsigned)(seed.tv_sec ^ seed.tv_usec));
	#endif
    return (1);
}


uint32_t
libnet_get_prand(int mod)
{
    uint32_t n;  /* 0 to 4,294,967,295 */
#ifndef _WIN32
    n = random();
#else
	HCRYPTPROV hProv = 0;
	CryptAcquireContext(&hProv, 
		0, 0, PROV_RSA_FULL, 
		CRYPT_VERIFYCONTEXT);
	
	CryptGenRandom(hProv, 
		sizeof(n), (BYTE*)&n);
	CryptReleaseContext(hProv, 0); 
#endif
    switch (mod)
    {
        case LIBNET_PR2:
            return (n % 0x2);           /* 0 - 1 */
        case LIBNET_PR8:
            return (n % 0xff);          /* 0 - 255 */
        case LIBNET_PR16:
            return (n % 0x7fff);        /* 0 - 32767 */
        case LIBNET_PRu16:
            return (n % 0xffff);        /* 0 - 65535 */
        case LIBNET_PR32:
            return (n % 0x7fffffff);    /* 0 - 2147483647 */
        case LIBNET_PRu32:
            return (n);                 /* 0 - 4294967295 */
    }
    return (0);                         /* NOTTREACHED */
}

/* EOF */
