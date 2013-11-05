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

typedef Firebird::GenericMap<Firebird::MetaNamePair> MetaNamePairMap;

// forward declarations
namespace Jrd {
	class dsql_req;
	class dsql_str;
	class CompiledStatement;
};

void METD_drop_charset(Jrd::dsql_req*, const Firebird::MetaName&);
void METD_drop_collation(Jrd::dsql_req*, const Jrd::dsql_str*);
void METD_drop_function(Jrd::dsql_req*, const Jrd::dsql_str*);
void METD_drop_procedure(Jrd::dsql_req*, const Jrd::dsql_str*);
void METD_drop_relation(Jrd::dsql_req*, const Jrd::dsql_str*);

Jrd::dsql_intlsym*  METD_get_charset(Jrd::dsql_req*, USHORT, const char* name); // UTF-8
USHORT   METD_get_charset_bpc(Jrd::dsql_req*, SSHORT);
Firebird::MetaName METD_get_charset_name(Jrd::dsql_req*, SSHORT);
Jrd::dsql_intlsym* METD_get_collation(Jrd::dsql_req*, const Jrd::dsql_str*, USHORT charset_id);
USHORT   METD_get_col_default(Jrd::dsql_req*, const char*, const char*, bool*, UCHAR*, USHORT);
Jrd::dsql_str*      METD_get_default_charset(Jrd::dsql_req*);
bool METD_get_domain(Jrd::dsql_req*, class Jrd::dsql_fld*, const char* name); // UTF-8
USHORT   METD_get_domain_default(Jrd::dsql_req*, const TEXT*, bool*, UCHAR*, USHORT);
bool METD_get_exception(Jrd::dsql_req*, const Jrd::dsql_str*);
Jrd::dsql_udf*      METD_get_function(Jrd::dsql_req*, const Jrd::dsql_str*);
Jrd::dsql_nod* METD_get_primary_key(Jrd::dsql_req*, const Jrd::dsql_str*);
Jrd::dsql_prc* METD_get_procedure(Jrd::CompiledStatement*, const Jrd::dsql_str*);
void METD_get_procedure_parameter(Jrd::CompiledStatement* statement,
	const Firebird::MetaName& procedure, const Firebird::MetaName& parameter,
	Firebird::UCharBuffer& buffer);
Jrd::dsql_rel* METD_get_relation(Jrd::CompiledStatement*, const char*);
bool   METD_get_trigger(Jrd::dsql_req*, const Jrd::dsql_str*, Jrd::dsql_str**, USHORT*);
bool   METD_get_type(Jrd::dsql_req*, const Jrd::dsql_str*, const char*, SSHORT*);
Jrd::dsql_rel* METD_get_view_base(Jrd::CompiledStatement* request,
							 const char* view_name,	// UTF-8
							 MetaNamePairMap& fields);
Jrd::dsql_rel* METD_get_view_relation(Jrd::CompiledStatement* request,
								const char* view_name,         // UTF-8
								const char* relation_or_alias); // UTF-8

#endif // DSQL_METD_PROTO_H

