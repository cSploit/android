/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		shut_proto.h
 *	DESCRIPTION:	Prototype Header file for shut.cpp
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

#ifndef JRD_SHUT_PROTO_H
#define JRD_SHUT_PROTO_H

namespace Jrd {
	class Database;
}

bool		SHUT_blocking_ast(Jrd::thread_db*);
void		SHUT_database(Jrd::thread_db*, SSHORT, SSHORT);
void		SHUT_init(Jrd::thread_db*);
void        SHUT_online(Jrd::thread_db*, SSHORT);

#endif // JRD_SHUT_PROTO_H
