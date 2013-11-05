/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		ddl_proto.h
 *	DESCRIPTION:	Prototype Header file for ddl.cpp
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
 * 2004.01.16 Vlad Horsun: added support for default parameters and
 *   EXECUTE BLOCK statement
 */

#ifndef DSQL_DDL_PROTO_H
#define DSQL_DDL_PROTO_H

// This is a DSQL internal file. Not to be used by anything but
// the DSQL module itself.

namespace Jrd {
	class dsql_req;
	class CompiledStatement;
	class dsql_fld;
	class dsql_nod;
	class dsql_str;
};

void DDL_execute(Jrd::dsql_req*);
void DDL_generate(Jrd::CompiledStatement*, Jrd::dsql_nod*);
bool DDL_ids(const Jrd::dsql_req*);
void DDL_put_field_dtype(Jrd::CompiledStatement*, const Jrd::dsql_fld*, bool);
void DDL_resolve_intl_type(Jrd::CompiledStatement*, Jrd::dsql_fld*, const Jrd::dsql_str*);
void DDL_resolve_intl_type2(Jrd::CompiledStatement*, Jrd::dsql_fld*, const Jrd::dsql_str*, bool);
void DDL_gen_block(Jrd::CompiledStatement*, Jrd::dsql_nod*);

#endif // DSQL_DDL_PROTO_H

