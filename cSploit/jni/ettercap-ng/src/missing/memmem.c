/*
 * Copyright (c) 2001 Alberto Ornaghi <alor@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ec.h>

#if 0
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#endif

void * memmem(const void *haystack, size_t haystacklen,
              const void *needle, size_t needlelen);

/*
 *  DESCRIPTION
 *        The memmem() function finds the start of the first occurrence
 *        of the substring needle of length needlelen in the memory area
 *        haystack of length haystacklen.
 *
 *  RETURN VALUE
 *        The memmem() function returns a pointer to the beginning of the
 *        substring, or NULL if the substring is not found.
 */


void * memmem(const void *haystack, size_t haystacklen,
              const void *needle, size_t needlelen)
{
	register const char *h = haystack;
	register const char *n = needle;
	register size_t hl = haystacklen;
	register size_t nl = needlelen;
	register size_t i;

	if (nl == 0) return (void *) haystack;    /* The first occurrence of the empty string is deemed to occur at
                                                     the beginning of the string.  */

	for( i = 0; (i < hl) && (i + nl <= hl ); i++ )
		if (h[i] == *n)	/* first match */
			if ( !memcmp(&h[i]+1, n + 1, nl - 1) )
				return (void *)(haystack+i);	/* returns a pointer to the substring */

	return (void *)NULL;	/* not found */
}

/* EOF */
