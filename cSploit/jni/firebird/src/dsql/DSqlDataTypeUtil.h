/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		DSqlDataTypeUtil.h
 *	DESCRIPTION:	DSqlDataTypeUtil
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
 */

#ifndef DSQL_DSQLDATATYPEUTIL_H
#define DSQL_DSQLDATATYPEUTIL_H

#include "../jrd/DataTypeUtil.h"

namespace Jrd {

	class DsqlCompilerScratch;

	class DSqlDataTypeUtil : public DataTypeUtilBase
	{
	public:
		explicit DSqlDataTypeUtil(DsqlCompilerScratch* aDsqlScratch)
			: dsqlScratch(aDsqlScratch)
		{
		}

	public:
		virtual UCHAR maxBytesPerChar(UCHAR charSet);
		virtual USHORT getDialect() const;

	private:
		DsqlCompilerScratch* dsqlScratch;
	};

} // namespace

#endif // DSQL_DSQLDATATYPEUTIL_H
