/*
 *  $Id: libnet_version.c,v 1.7 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_version.c - dummy version function to define version info
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

#if (!(_WIN32) || (__CYGWIN__))
#include <unistd.h>
#include "../version.h"
#else
#include "../include/win32/libnet.h"
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

static const char banner[] = "libnet version "VERSION"";

void
__libnet_print_vers(void)
{
    /*
     *  We don't check for error cos we really don't care.
     */
#if defined (__WIN32__)
     fprintf(stdout, "%s", banner);
#else
     write(STDOUT_FILENO, banner, sizeof(banner) - 1);
#endif
}

const char *
libnet_version(void)
{
    return (banner);
}

/* EOF */
