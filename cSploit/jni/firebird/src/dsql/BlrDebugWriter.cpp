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
 * Adriano dos Santos Fernandes - refactored from others modules.
 * Alex Peshkov
 */

#include "firebird.h"
#include "consts_pub.h"
#include "dyn_consts.h"
#include "gen/iberror.h"
#include "../jrd/jrd.h"
#include "../jrd/exe.h"
#include "../dsql/BlrDebugWriter.h"
#include "../dsql/StmtNodes.h"
#include "../dsql/dsql.h"
#include "../jrd/blr.h"
#include "../jrd/DebugInterface.h"
#include "../dsql/errd_proto.h"

using namespace Firebird;

namespace Jrd {

void BlrDebugWriter::raiseError(const Arg::StatusVector& vector)
{
	ERRD_post(vector);
}

void BlrDebugWriter::beginDebug()
{
	fb_assert(debugData.isEmpty());

	debugData.add(fb_dbg_version);
	debugData.add(CURRENT_DBG_INFO_VERSION);
}

void BlrDebugWriter::endDebug()
{
	debugData.add(fb_dbg_end);
}

void BlrDebugWriter::putDebugSrcInfo(ULONG line, ULONG col)
{
	debugData.add(fb_dbg_map_src2blr);

	debugData.add(line);
	debugData.add(line >> 8);
	debugData.add(line >> 16);
	debugData.add(line >> 24);

	debugData.add(col);
	debugData.add(col >> 8);
	debugData.add(col >> 16);
	debugData.add(col >> 24);

	const ULONG offset = (getBlrData().getCount() - getBaseOffset());
	debugData.add(offset);
	debugData.add(offset >> 8);
	debugData.add(offset >> 16);
	debugData.add(offset >> 24);
}

void BlrDebugWriter::putDebugVariable(USHORT number, const MetaName& name)
{
	debugData.add(fb_dbg_map_varname);

	debugData.add(number);
	debugData.add(number >> 8);

	USHORT len = MIN(name.length(), MAX_UCHAR);
	debugData.add(len);

	debugData.add(reinterpret_cast<const UCHAR*>(name.c_str()), len);
}

void BlrDebugWriter::putDebugArgument(UCHAR type, USHORT number, const TEXT* name)
{
	fb_assert(name);

	debugData.add(fb_dbg_map_argument);

	debugData.add(type);
	debugData.add(number);
	debugData.add(number >> 8);

	USHORT len = strlen(name);
	if (len > MAX_UCHAR)
		len = MAX_UCHAR;
	debugData.add(len);

	debugData.add(reinterpret_cast<const UCHAR*>(name), len);
}

void BlrDebugWriter::putDebugSubFunction(DeclareSubFuncNode* subFuncNode)
{
	debugData.add(fb_dbg_subfunc);

	dsql_udf* subFunc = subFuncNode->dsqlFunction;
	const MetaName& name = subFunc->udf_name.identifier;
	USHORT len = MIN(name.length(), MAX_UCHAR);

	debugData.add(len);
	debugData.add(reinterpret_cast<const UCHAR*>(name.c_str()), len);

	HalfStaticArray<UCHAR, 128>& subDebugData = subFuncNode->blockScratch->debugData;
	const ULONG count = ULONG(subDebugData.getCount());
	debugData.add(UCHAR(count));
	debugData.add(UCHAR(count >> 8));
	debugData.add(UCHAR(count >> 16));
	debugData.add(UCHAR(count >> 24));
	debugData.add(subDebugData.begin(), count);
}

void BlrDebugWriter::putDebugSubProcedure(DeclareSubProcNode* subProcNode)
{
	debugData.add(fb_dbg_subproc);

	dsql_prc* subProc = subProcNode->dsqlProcedure;
	const MetaName& name = subProc->prc_name.identifier;
	USHORT len = MIN(name.length(), MAX_UCHAR);

	debugData.add(len);
	debugData.add(reinterpret_cast<const UCHAR*>(name.c_str()), len);

	HalfStaticArray<UCHAR, 128>& subDebugData = subProcNode->blockScratch->debugData;
	const ULONG count = ULONG(subDebugData.getCount());
	debugData.add(UCHAR(count));
	debugData.add(UCHAR(count >> 8));
	debugData.add(UCHAR(count >> 16));
	debugData.add(UCHAR(count >> 24));
	debugData.add(subDebugData.begin(), count);
}

}	// namespace Jrd
