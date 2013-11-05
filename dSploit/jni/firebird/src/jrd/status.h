/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			status.h
 *	DESCRIPTION:	Status vector filling and parsing.
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Mike Nordell
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2001 Mike Nordell <tamlin at algonet.se>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


#ifndef FB_MISC_STATUS_H
#define FB_MISC_STATUS_H

#include <stdlib.h>				// size_t
#include "../jrd/common.h"		// ISC_STATUS

const int MAX_ERRMSG_LEN	= 128;
const int MAX_ERRSTR_LEN	= 1024;

void PARSE_STATUS(const ISC_STATUS* status_vector, int &length, int &warning);

#endif // FB_MISC_STATUS_H
