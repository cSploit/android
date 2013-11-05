/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		make_proto.h
 *	DESCRIPTION:	Prototype Header file for make.cpp
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
 * 2002-07-20 Arno Brinkman: Added MAKE_desc_from_list
 */

#ifndef DSQL_MAKE_PROTO_H
#define DSQL_MAKE_PROTO_H

#include "../dsql/sym.h"
#include "../dsql/node.h"

namespace Jrd {
	class dsql_nod;
	class dsql_fld;
	class dsql_req;
	class CompiledStatement;

// Parameters to MAKE_constant
	enum dsql_constant_type {
		CONSTANT_STRING		= 0, // stored as a string
//		CONSTANT_SLONG		= 1, // stored as a SLONG
		CONSTANT_DOUBLE		= 2, // stored as a string
		CONSTANT_DATE		= 3, // stored as a SLONG
		CONSTANT_TIME		= 4, // stored as a ULONG
		CONSTANT_TIMESTAMP	= 5, // stored as a QUAD
		CONSTANT_SINT64		= 6  // stored as a SINT64
	};

	// Parameters to MAKE_variable
	enum dsql_var_type
	{
		VAR_input,
		VAR_output,
		VAR_local
	};
}


Jrd::dsql_nod* MAKE_const_slong(SLONG);
Jrd::dsql_nod* MAKE_constant(Jrd::dsql_str*, Jrd::dsql_constant_type);
Jrd::dsql_nod* MAKE_str_constant(Jrd::dsql_str*, SSHORT);
Jrd::dsql_str* MAKE_cstring(const char*);
void MAKE_desc(Jrd::CompiledStatement*, dsc*, Jrd::dsql_nod*, Jrd::dsql_nod*);
void MAKE_desc_from_field(dsc*, const Jrd::dsql_fld*);
void MAKE_desc_from_list(Jrd::CompiledStatement*, dsc*, Jrd::dsql_nod*, Jrd::dsql_nod*, const TEXT*);
Jrd::dsql_nod* MAKE_field(Jrd::dsql_ctx*, Jrd::dsql_fld*, Jrd::dsql_nod*);
Jrd::dsql_nod* MAKE_field_name(const char*);
Jrd::dsql_nod* MAKE_list(Jrd::DsqlNodStack&);
Jrd::dsql_nod* MAKE_node(Dsql::nod_t, int);
Jrd::dsql_par* MAKE_parameter(Jrd::dsql_msg*, bool, bool, USHORT, const Jrd::dsql_nod*);
Jrd::dsql_str* MAKE_string(const char*, int);
Jrd::dsql_sym* MAKE_symbol(Jrd::dsql_dbb*, const TEXT*, USHORT, Jrd::sym_type, Jrd::dsql_req*);
Jrd::dsql_str* MAKE_tagged_string(const char* str, size_t length, const char* charset);
Jrd::dsql_nod* MAKE_trigger_type(Jrd::dsql_nod*, Jrd::dsql_nod*);
Jrd::dsql_nod* MAKE_variable(Jrd::dsql_fld*, const TEXT*, const Jrd::dsql_var_type type, USHORT,
								USHORT, USHORT);

#endif // DSQL_MAKE_PROTO_H

