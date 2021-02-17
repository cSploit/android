/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		cme_proto.h
 *	DESCRIPTION:	Prototype header file for cme.cpp
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

#ifndef GPRE_CME_PROTO_H
#define GPRE_CME_PROTO_H

void	CME_expr(gpre_nod*, gpre_req*);
void	CME_get_dtype(const gpre_nod*, gpre_fld*);
void	CME_relation(gpre_ctx*, gpre_req*);
void	CME_rse(gpre_rse*, gpre_req*);

inline void	CME_rse(gpre_nod* node, gpre_req* request)
{
	CME_rse((gpre_rse*) node, request);
}

#endif // GPRE_CME_PROTO_H
