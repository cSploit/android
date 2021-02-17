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

namespace Jrd
{
	class ExprNode;
	class ValueListNode;
}

void	GEN_descriptor(Jrd::DsqlCompilerScratch* dsqlScratch, const dsc* desc, bool texttype);
void	GEN_expr(Jrd::DsqlCompilerScratch*, Jrd::ExprNode*);
void	GEN_hidden_variables(Jrd::DsqlCompilerScratch* dsqlScratch);
void	GEN_parameter(Jrd::DsqlCompilerScratch*, const Jrd::dsql_par*);
void	GEN_port(Jrd::DsqlCompilerScratch*, Jrd::dsql_msg*);
void	GEN_request(Jrd::DsqlCompilerScratch*, Jrd::DmlNode*);
void	GEN_rse(Jrd::DsqlCompilerScratch*, Jrd::RseNode*);
void	GEN_sort(Jrd::DsqlCompilerScratch*, Jrd::ValueListNode*);
void	GEN_stuff_context(Jrd::DsqlCompilerScratch*, const Jrd::dsql_ctx*);

#endif //  DSQL_GEN_PROTO_H
