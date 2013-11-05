/*
 *	PROGRAM:	Firebird utilities
 *	MODULE:		nbk_proto.h
 *	DESCRIPTION:	nbackup prototypes
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Alex Peshkov <peshkoff at mail dot ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef NBK_PROTO_H
#define NBK_PROTO_H

#include "../jrd/ThreadData.h"
#include "../common/UtilSvc.h"

void nbackup(Firebird::UtilSvc*);
THREAD_ENTRY_DECLARE NBACKUP_main(THREAD_ENTRY_PARAM);

#endif // NBK_PROTO_H
