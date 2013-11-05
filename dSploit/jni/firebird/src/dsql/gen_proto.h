/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		gen_proto.h
 *	DESCRIPTION:	Prototype Header file for gen.cpp
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

#ifndef DSQL_GEN_PROTO_H
#define DSQL_GEN_PROTO_H

void	GEN_descriptor(Jrd::CompiledStatement* statement, const dsc* desc, bool texttype);
void	GEN_expr(Jrd::CompiledStatement*, Jrd::dsql_nod*);
void	GEN_hidden_variables(Jrd::CompiledStatement* statement, bool inExpression);
void	GEN_port(Jrd::CompiledStatement*, Jrd::dsql_msg*);
void	GEN_request(Jrd::CompiledStatement*, Jrd::dsql_nod*);
void	GEN_return(Jrd::CompiledStatement*, const Jrd::dsql_nod*, bool);
void	GEN_start_transaction(Jrd::CompiledStatement*, const Jrd::dsql_nod*);
void	GEN_statement(Jrd::CompiledStatement*, Jrd::dsql_nod*);

// CVC: I think this can be replaced by statement->append_uchar(byte) in the calling code.
inline void stuff(Jrd::CompiledStatement* statement, const UCHAR byte)
{
	statement->req_blr_data.add(byte);
}

#endif //  DSQL_GEN_PROTO_H

