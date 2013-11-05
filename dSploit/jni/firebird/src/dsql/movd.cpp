/*
*	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		movd.cpp
 *	DESCRIPTION:	Data mover and converter and comparator, etc.
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
#include "../jrd/common.h"
#include "../dsql/dsql.h"
#include "gen/iberror.h"
#include "../jrd/jrd.h"
#include "../jrd/mov_proto.h"
#include "../dsql/movd_proto.h"

using namespace Jrd;
using namespace Firebird;


// Move (and possible convert) something to something else.
void MOVD_move(thread_db* tdbb, dsc* from, dsc* to)
{
	try
	{
		MOV_move(tdbb, from, to);
	}
	catch (const status_exception& ex)
	{
		Arg::StatusVector newVector;
		newVector << Arg::Gds(isc_dsql_error) << Arg::Gds(isc_sqlerr) << Arg::Num(-303);
		newVector.append(Arg::StatusVector(ex.value()));
		status_exception::raise(newVector);
	}
}
