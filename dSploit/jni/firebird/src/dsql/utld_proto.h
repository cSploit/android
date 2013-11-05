/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		utld_proto.h
 *	DESCRIPTION:	Prototype Header file for utld.cpp
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
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old Win3.1 port (Windows_Only)
 *
 */

#ifndef DSQL_UTLD_PROTO_H
#define DSQL_UTLD_PROTO_H

#if !defined(SUPERCLIENT)
#include "../jrd/DataTypeUtil.h"
#endif

struct sqlda_sup;

USHORT		UTLD_char_length_to_byte_length(USHORT lengthInChars, USHORT maxBytesPerChar, USHORT overhead);
ISC_STATUS	UTLD_copy_status(const ISC_STATUS*, ISC_STATUS*);
ISC_STATUS	UTLD_parse_sql_info(ISC_STATUS*, USHORT, const SCHAR*, XSQLDA*, USHORT*);
ISC_STATUS	UTLD_parse_sqlda(ISC_STATUS*, sqlda_sup* const, USHORT*, USHORT*,
	USHORT*, USHORT, const XSQLDA*, const USHORT);
void		UTLD_save_status_strings(ISC_STATUS*);
SCHAR*		UTLD_skip_sql_info(SCHAR*);


#if !defined(SUPERCLIENT)

namespace Jrd {

	class CompiledStatement;

	class DSqlDataTypeUtil : public DataTypeUtilBase
	{
	public:
		explicit DSqlDataTypeUtil(CompiledStatement* aStatement)
			: statement(aStatement)
		{
		}

	public:
		virtual UCHAR maxBytesPerChar(UCHAR charSet);
		virtual USHORT getDialect() const;

	private:
		CompiledStatement* statement;
	};

} // namespace

#endif


#endif //  DSQL_UTLD_PROTO_H

