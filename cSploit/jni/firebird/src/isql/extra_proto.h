/*
 *	PROGRAM:	Interactive SQL utility
 *	MODULE:		extra_proto.h
 *	DESCRIPTION:	Prototype header file for extract.epp
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

#ifndef ISQL_EXTRA_PROTO_H
#define ISQL_EXTRA_PROTO_H

int	EXTRACT_ddl(LegacyTables, const SCHAR*);
int	EXTRACT_list_table(const SCHAR*, const SCHAR*, bool, SSHORT);
processing_state	EXTRACT_list_grants (const SCHAR*);

#endif // ISQL_EXTRA_PROTO_H

