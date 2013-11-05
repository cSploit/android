/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		sym.h
 *	DESCRIPTION:	Definitions for symbol table
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

#ifndef DSQL_SYM_H
#define DSQL_SYM_H

#include "../jrd/common.h"

namespace Jrd {

// possible symbol types

enum sym_type {
	SYM_statement,
	SYM_cursor,
	SYM_keyword,
	SYM_context,
	SYM_relation,
	SYM_field,
	SYM_stream,
	SYM_udf,
	SYM_procedure,
	SYM_intlsym_charset,
	SYM_intlsym_collation,
	SYM_eof
};

typedef sym_type SYM_TYPE;

// symbol block

class dsql_sym : public pool_alloc_rpt<UCHAR, dsql_type_sym>
{
public:
	void *sym_dbb;				// generic DB structure handle
	const TEXT *sym_string;			// address of asciz string
	USHORT sym_length;			// length of string (exc. term.)
	SYM_TYPE sym_type;			// symbol type
	USHORT sym_keyword;			// keyword number, if keyword
	USHORT sym_version;			// dialect version the symbol was introduced
	void *sym_object;			// general pointer to object
	dsql_sym* sym_collision;	// collision pointer
	dsql_sym* sym_homonym;		// homonym pointer
	TEXT sym_name[2];			// space for name, if necessary
};


} // namespace

#endif // DSQL_SYM_H

