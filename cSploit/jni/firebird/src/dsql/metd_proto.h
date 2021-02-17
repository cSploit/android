/*
 *	PROGRAM:	 Dynamic SQL runtime support
 *	MODULE:		 metd_proto.h
 *	DESCRIPTION: Prototype Header file for metd.epp
 *               This is a DSQL private header file. It is not included
 *               by anything but DSQL itself.
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

#ifndef DSQL_METD_PROTO_H
#define DSQL_METD_PROTO_H

#include "../common/classes/GenericMap.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/fb_pair.h"

// forward declarations
namespace Jrd {
	typedef Firebird::GenericMap<Firebird::MetaNamePair> MetaNamePairMap;

	class dsql_req;
	class DsqlCompilerScratch;
	class jrd_tra;
	class dsql_intlsym;
	class dsql_fld;
	class dsql_udf;
	class dsql_prc;
	class dsql_rel;
	class FieldNode;
};

void METD_drop_charset(Jrd::jrd_tra*, const Firebird::MetaName&);
void METD_drop_collation(Jrd::jrd_tra*, const Firebird::MetaName&);
void METD_drop_function(Jrd::jrd_tra*, const Firebird::QualifiedName&);
void METD_drop_procedure(Jrd::jrd_tra*, const Firebird::QualifiedName&);
void METD_drop_relation(Jrd::jrd_tra*, const Firebird::MetaName&);

Jrd::dsql_intlsym* METD_get_charset(Jrd::jrd_tra*, USHORT, const char* name);
USHORT METD_get_charset_bpc(Jrd::jrd_tra*, SSHORT);
Firebird::MetaName METD_get_charset_name(Jrd::jrd_tra*, SSHORT);
Jrd::dsql_intlsym* METD_get_collation(Jrd::jrd_tra*, const Firebird::MetaName&, USHORT charset_id);
USHORT METD_get_col_default(Jrd::jrd_tra*, const char*, const char*, bool*, UCHAR*, USHORT);
Firebird::MetaName METD_get_default_charset(Jrd::jrd_tra*);
bool METD_get_domain(Jrd::jrd_tra*, class Jrd::TypeClause*, const Firebird::MetaName& name);
USHORT METD_get_domain_default(Jrd::jrd_tra*, const Firebird::MetaName&, bool*, UCHAR*, USHORT);
Jrd::dsql_udf* METD_get_function(Jrd::jrd_tra*, Jrd::DsqlCompilerScratch*,
	const Firebird::QualifiedName&);
void METD_get_primary_key(Jrd::jrd_tra*, const Firebird::MetaName&,
	Firebird::Array<NestConst<Jrd::FieldNode> >&);
Jrd::dsql_prc* METD_get_procedure(Jrd::jrd_tra*, Jrd::DsqlCompilerScratch*,
	const Firebird::QualifiedName&);
Jrd::dsql_rel* METD_get_relation(Jrd::jrd_tra*, Jrd::DsqlCompilerScratch*, const Firebird::MetaName&);
bool METD_get_type(Jrd::jrd_tra*, const Firebird::MetaName&, const char*, SSHORT*);
Jrd::dsql_rel* METD_get_view_base(Jrd::jrd_tra*, Jrd::DsqlCompilerScratch*, const char* view_name,
	Jrd::MetaNamePairMap& fields);
Jrd::dsql_rel* METD_get_view_relation(Jrd::jrd_tra*, Jrd::DsqlCompilerScratch*, const char* view_name,
	const char* relation_or_alias);

#endif // DSQL_METD_PROTO_H
