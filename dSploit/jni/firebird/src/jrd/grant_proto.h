/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		grant_proto.h
 *	DESCRIPTION:	Function prototypes for file grant.epp
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

#ifndef JRD_GRANT_PROTO_H
#define JRD_GRANT_PROTO_H

#include "../common/classes/fb_string.h"

namespace Jrd
{
	class DeferredWork;
}

void GRANT_privileges(Jrd::thread_db*, const Firebird::string&, USHORT, Jrd::jrd_tra*);

#endif // JRD_GRANT_PROTO_H
