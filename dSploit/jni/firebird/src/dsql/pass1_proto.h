/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		pass1_proto.h
 *	DESCRIPTION:	Prototype Header file for pass1.cpp
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

#ifndef DSQL_PASS1_PROTO_H
#define DSQL_PASS1_PROTO_H

Jrd::dsql_ctx* PASS1_make_context(Jrd::CompiledStatement* statement, const Jrd::dsql_nod* relation_node);
Jrd::dsql_nod* PASS1_node(Jrd::CompiledStatement*, Jrd::dsql_nod*);
Jrd::dsql_nod* PASS1_rse(Jrd::CompiledStatement*, Jrd::dsql_nod*, Jrd::dsql_nod*);
Jrd::dsql_nod* PASS1_statement(Jrd::CompiledStatement*, Jrd::dsql_nod*);

#endif // DSQL_PASS1_PROTO_H

