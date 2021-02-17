/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		sqe_proto.h
 *	DESCRIPTION:	Prototype header file for sqe.cpp
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

#ifndef GPRE_SQE_PROTO_H
#define GPRE_SQE_PROTO_H

typedef gpre_nod* (*pfn_SQE_list_cb) (gpre_req*, bool, USHORT*, bool*);

gpre_nod*	SQE_boolean(gpre_req*, USHORT*);
gpre_ctx*	SQE_context(gpre_req*);
gpre_nod*	SQE_field(gpre_req*, bool);
gpre_nod*	SQE_list(pfn_SQE_list_cb, gpre_req*, bool);
ref*		SQE_parameter(gpre_req*);
void		SQE_post_field(gpre_nod*, gpre_fld*);
ref*		SQE_post_reference(gpre_req*, gpre_fld*, gpre_ctx*, gpre_nod*);
bool		SQE_resolve(gpre_nod*, gpre_req*, gpre_rse*);
gpre_rse*	SQE_select(gpre_req*, bool);
gpre_nod*	SQE_value(gpre_req*, bool, USHORT*, bool*);
gpre_nod*	SQE_variable(gpre_req*, bool, USHORT*, bool*);

#endif // GPRE_SQE_PROTO_H

