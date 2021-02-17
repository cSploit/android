/*
 *	PROGRAM:	JRD Rebuild scrambled database
 *	MODULE:		rebui_proto.h
 *	DESCRIPTION:	Prototype header file for rebuild.cpp
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

#ifndef UTILITIES_REBUI_PROTO_H
#define UTILITIES_REBUI_PROTO_H

namespace Ods {
	struct pag;
}

void*		RBDB_alloc (SLONG);
void		RBDB_close (struct rbdb*);
void		RBDB_open (struct rbdb*);
Ods::pag*	RBDB_read (struct rbdb*, SLONG);
void		RBDB_write (struct rbdb*, Ods::pag*, SLONG);

#endif // UTILITIES_REBUI_PROTO_H

