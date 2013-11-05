/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		inf_proto.h
 *	DESCRIPTION:	Prototype header file for inf.cpp
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

#ifndef JRD_INF_PROTO_H
#define JRD_INF_PROTO_H

namespace Jrd {
	class jrd_req;
	class jrd_tra;
	class blb;
}

void INF_blob_info(const Jrd::blb*, const UCHAR*, const SSHORT, UCHAR*, const SSHORT);
USHORT INF_convert(SINT64, UCHAR*);
void INF_database_info(const UCHAR*, const SSHORT, UCHAR*, const SSHORT);
UCHAR* INF_put_item(UCHAR, USHORT, const UCHAR*, UCHAR*, const UCHAR*, const bool inserting = false);
void INF_request_info(const Jrd::jrd_req*, const UCHAR*, const SSHORT, UCHAR*, const SLONG);
void INF_transaction_info(const Jrd::jrd_tra*, const UCHAR*, const SSHORT, UCHAR*, const SSHORT);

#endif // JRD_INF_PROTO_H
