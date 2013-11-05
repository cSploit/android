/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		dsql_proto.h
 *	DESCRIPTION:	Prototype Header file for dsql.cpp
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
 * Adriano dos Santos Fernandes
 */

#ifndef DSQL_DSQL_PROTO_H
#define DSQL_DSQL_PROTO_H

#include "../common/classes/fb_string.h"

namespace Jrd {
	class Attachment;
	class jrd_tra;
	class dsql_req;
}

Jrd::dsql_req* DSQL_allocate_statement(Jrd::thread_db*, Jrd::Attachment*);
void DSQL_execute(Jrd::thread_db*,  Jrd::jrd_tra**, Jrd::dsql_req*,
				  USHORT, const UCHAR*,
				  USHORT, USHORT, const UCHAR*,
				  USHORT, UCHAR*,
				  /*USHORT,*/ USHORT, UCHAR*);
void DSQL_execute_immediate(Jrd::thread_db*, Jrd::Attachment*, Jrd::jrd_tra**,
							USHORT, const TEXT*, USHORT,
							USHORT, const UCHAR*,
							/*USHORT,*/ USHORT, const UCHAR*,
							USHORT, UCHAR*,
							/*USHORT,*/ USHORT, UCHAR*);
#ifdef SCROLLABLE_CURSORS
ISC_STATUS DSQL_fetch(Jrd::thread_db*, Jrd::dsql_req*,
					  USHORT, const UCHAR*,
					  /*USHORT,*/ USHORT, UCHAR*,
					  USHORT, SLONG);
#else
ISC_STATUS DSQL_fetch(Jrd::thread_db*, Jrd::dsql_req*,
					  USHORT, const UCHAR*,
					  /*USHORT,*/ USHORT, UCHAR*);
#endif // SCROLLABLE_CURSORS
void DSQL_free_statement(Jrd::thread_db*, Jrd::dsql_req*, USHORT);
void DSQL_insert(Jrd::thread_db*, Jrd::dsql_req*,
				 USHORT, const UCHAR*,
				 /*USHORT,*/ USHORT, const UCHAR*);
void DSQL_prepare(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::dsql_req**,
				  USHORT, const TEXT*,
				  USHORT, USHORT, const UCHAR*,
				  USHORT, UCHAR*);
void DSQL_set_cursor(Jrd::thread_db*, Jrd::dsql_req*, const TEXT*); //, USHORT);
void DSQL_sql_info(Jrd::thread_db*, Jrd::dsql_req*,
				   USHORT, const UCHAR*,
				   ULONG, UCHAR*);
ULONG DSQL_get_plan_info(Jrd::thread_db*,
						  const Jrd::dsql_req*,
						  SLONG,
						  SCHAR**,
						  const bool realloc = true);

#endif //  DSQL_DSQL_PROTO_H
