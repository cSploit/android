/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		misc.cpp
 *	DESCRIPTION:	Miscellaneous useful routines
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../burp/burp.h"
#include "../burp/burp_proto.h"
#include "../burp/misc_proto.h"


UCHAR *MISC_alloc_burp(ULONG size)
{
/**************************************
 *
 *	M I S C _ a l l o c _ b u r p
 *
 **************************************
 *
 * Functional description
 *	Allocate block of memory. Note that it always zeros out memory.
 *  This could be optimized.
 *
 **************************************/

	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	// Add some header space to store a list of blocks allocated for this gbak
	size += ROUNDUP(sizeof(UCHAR *), FB_ALIGNMENT);

	UCHAR* block = (UCHAR*)gds__alloc(size);

	if (!block)
	{
		// NOMEM: message & abort FREE: all items freed at gbak exit
		BURP_error(238, true);
		// msg 238: System memory exhaused
		return NULL;
	}

	memset(block, 0, size);

	// FREE: We keep a linked list of all gbak memory allocations, which
	// are then freed when gbak exits.  This is important for
	// NETWARE in particular.

	*((UCHAR**) block) = tdgbl->head_of_mem_list;
	tdgbl->head_of_mem_list = block;

	return (block + ROUNDUP(sizeof(UCHAR *), FB_ALIGNMENT));
}


void MISC_free_burp( void *free)
{
/**************************************
 *
 *	M I S C _ f r e e _ b u r p
 *
 **************************************
 *
 * Functional description
 *	Release an unwanted block.
 *
 **************************************/
	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	if (free != NULL)
	{
		// Point at the head of the allocated block
		UCHAR** block = (UCHAR**) ((UCHAR*) free - ROUNDUP(sizeof(UCHAR*), FB_ALIGNMENT));

		// Scan for this block in the list of blocks
		for (UCHAR **ptr = &tdgbl->head_of_mem_list; *ptr; ptr = (UCHAR **) *ptr)
		{
			if (*ptr == (UCHAR *) block) {
				// Found it - remove it from the list
				*ptr = *block;

				// and free it
				gds__free(block);
				return;
			}
		}

		// We should always find the block in the list
		BURP_error(238, true);
		// msg 238: System memory exhausted
		// (too lazy to add a better message)
	}
}


// Since this code appears everywhere, it makes more sense to isolate it
// in a function visible to all gbak components.
// Given a request, if it's non-zero (compiled), deallocate it but
// without caring about a possible error.
void MISC_release_request_silent(isc_req_handle& req_handle)
{
	if (req_handle)
	{
		ISC_STATUS_ARRAY req_status;
		isc_release_request(req_status, &req_handle);
	}
}


int MISC_symbol_length( const TEXT* symbol, ULONG size_len)
{
/**************************************
 *
 *	M I S C _ s y m b o l _ l e n g t h
 *
 **************************************
 *
 * Functional description
 * Compute length of null terminated symbol.
 *      CVC: This function should acknowledge embedded blanks.
 *
 **************************************/
	if (size_len < 2) {
		return 0;
	}

	--size_len;

	const TEXT* p = symbol;
	const TEXT* const q = p + size_len;

	while (*p && p < q) {  // find end of string (null or end).
		p++;
	}

	--p;

	while (p >= symbol && *p == ' ') {  // skip trailing blanks
		--p;
	}

	return p + 1 - symbol;
}


void MISC_terminate(const TEXT* from, TEXT* to, ULONG length, ULONG max_length)
{
/**************************************
 *
 *	M I S C _ t e r m i n a t e
 *
 **************************************
 *
 * Functional description
 *	Null-terminate a possibly non-
 *	null-terminated string with max
 *	buffer room.
 *
 **************************************/

	fb_assert(max_length != 0);
	if (length) {
		length = MIN(length, max_length - 1);
		do {
			*to++ = *from++;
		} while (--length);
		*to++ = '\0';
	}
	else {
		while (max_length-- && (*to++ = *from++));
		*--to = '\0';
	}
}

