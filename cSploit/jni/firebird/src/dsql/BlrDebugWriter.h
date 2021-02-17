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

#ifndef DSQL_BLR_DEBUG_WRITER_H
#define DSQL_BLR_DEBUG_WRITER_H

#include "../common/classes/BlrWriter.h"

namespace Jrd {


class DeclareSubFuncNode;
class DeclareSubProcNode;

// BLR writer with debug info support.
class BlrDebugWriter : public Firebird::BlrWriter
{
public:
	typedef Firebird::HalfStaticArray<UCHAR, 128> DebugData;

	explicit BlrDebugWriter(MemoryPool& p)
		: Firebird::BlrWriter(p),
		  debugData(p)
	{
	}

	void beginDebug();
	void endDebug();
	void putDebugSrcInfo(ULONG, ULONG);
	void putDebugVariable(USHORT, const Firebird::MetaName&);
	void putDebugArgument(UCHAR, USHORT, const TEXT*);
	void putDebugSubFunction(DeclareSubFuncNode* subFuncNode);
	void putDebugSubProcedure(DeclareSubProcNode* subProcNode);

	DebugData& getDebugData() { return debugData; }

	virtual void raiseError(const Firebird::Arg::StatusVector& vector);

private:
	DebugData debugData;
};


} // namespace Jrd

#endif // DSQL_BLR_DEBUG_WRITER_H
