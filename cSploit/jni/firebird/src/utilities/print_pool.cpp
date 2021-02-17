/*
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
#include "../jrd/svc_undoc.h"
#include "../common/stuff.h"
#include "../common/utils_proto.h"

int CLIB_ROUTINE main( int argc, char **argv)
{
/**************************************
*
*      m a i n
*
**************************************
*
*Functional Description
*  This utility uses the Firebird service api to inform the server
*  to print out the memory pool information into a specified file.
*  This utilitiy is for WIN_NT only, In case of UNIX ibmgr utility will
*  should be used.
*
*************************************************************************/
	char fname[512];

	if (argc != 2 && argc != 1)
	{
		printf("Usage %s \n      %s filename\n");
		exit(1);
	}
	if (argc == 1)
	{
		printf(" Filename : ");
		if (!fgets(fname, sizeof(fname), stdin))
			return 1;
		const size_t len = strlen(fname);
		if (!len)
			return 1;
		if (fname[len - 1] == '\n')
		{
			fname[len - 1] = 0;
			if (len == 1)
				return 1;
		}
	}
	else
	{
		fb_utils::copy_terminate(fname, argv[1], sizeof(fname));
		if (!fname[0])
			return 1;
	}

	printf("Filename to dump pool info = %s \n", fname);

	ISC_STATUS_ARRAY status;

	const char svc_name[] = "localhost:anonymous";
	isc_svc_handle svc_handle = NULL;
	if (isc_service_attach(status, 0, svc_name, &svc_handle, 0, NULL))
	{
		printf("Failed to attach service\n");
		return 1;
	}

	const unsigned short path_length = strlen(fname);

	char sendbuf[520]; // 512 + tag + length_word
	char* sptr = sendbuf;
	*sptr = isc_info_svc_dump_pool_info;
	++sptr;
	add_word(sptr, path_length);
	strcpy(sptr, fname);
	sptr += path_length;

	char respbuf[256];
	if (isc_service_query(status, &svc_handle, NULL, 0, NULL,
		sptr - sendbuf, sendbuf, sizeof(respbuf), respbuf))
	{
		printf("Failed to query service\n");
		isc_service_detach(status, &svc_handle);
		return 1;
	}

	isc_service_detach(status, &svc_handle);
	return 0;
}
