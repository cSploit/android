/*
 *	PROGRAM:	Service manager
 *	MODULE:		run_service.cpp
 *	DESCRIPTION:	Run a utility as a Firebird service
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
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../yvalve/gds_proto.h"

static const SCHAR recv_items[] = { isc_info_svc_to_eof };
static const SCHAR send_timeout[] = { isc_info_svc_timeout, 1, 0, 30 };


int CLIB_ROUTINE main( int argc, char *argv[])
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Initialize lock manager for process.
 *
 **************************************/
	if (argc < 2)
	{
		printf("usage: run_service service_path [args]\n");
		exit(FINI_ERROR);
	}

	const char* service_path = argv[1];

	char spb_buffer[2048];
	char* spb = spb_buffer;
	const char* const spb_end = spb_buffer + sizeof(spb_buffer) - 1;
	if (argc > 2)
	{
		*spb++ = isc_spb_version1;
		*spb++ = isc_spb_command_line;
		spb++;
		for (argv += 2, argc -= 2; argc && spb < spb_end; --argc)
		{
			for (const char* p = *argv++; spb < spb_end && (*spb = *p++); spb++)
				;
			*spb++ = ' ';
		}
		*--spb = 0;
		fb_assert(spb < spb_end);
		spb_buffer[2] = strlen(spb_buffer + 3);
	}

	SLONG* handle = NULL;
	isc_service_attach(NULL, 0, service_path, &handle, (SSHORT) (spb - spb_buffer), spb_buffer);

	SCHAR send_buffer[2048];

	const char* send_items;
	SSHORT send_item_length;
	if (strstr(service_path, "start_cache"))
	{
		send_items = send_timeout;
		send_item_length = sizeof(send_timeout);
	}
	else
	{
		send_items = send_buffer;
		send_item_length = 0;
	}

	if (send_item_length)
	{
		printf
			("It will take about 30 seconds to confirm that the cache manager\nis running.  Please wait...\n");
	}

	char buffer[2048];
	char item = isc_info_end;
	do {
		isc_service_query(NULL, &handle, send_item_length, send_items,
						  sizeof(recv_items), recv_items, sizeof(buffer), buffer);
		if (send_items == send_buffer)
			send_item_length = 0;
		const char* p = buffer;
		while (p < buffer + sizeof(buffer) && (item = *p) != isc_info_end &&
			item != isc_info_truncated && item != isc_info_svc_timeout)
		{
			SSHORT len = gds__vax_integer(p + 1, 2);
			p += 2;
			while (len--)
			{
				p++;
				if (*p != '\001')
					putchar(*p);
			}
			if (*p++ == '\001')
			{
				send_buffer[0] = isc_info_svc_line;
				fgets(send_buffer + 3, sizeof(send_buffer) - 3, stdin);
				len = strlen(send_buffer + 3);
				send_buffer[1] = len;
				send_buffer[2] = len >> 8;
				send_item_length = len + 3;
			}
		}
	} while (item == isc_info_truncated || (send_items == send_buffer && send_item_length));

	isc_service_detach(NULL, &handle);

	exit(FINI_OK);
}
