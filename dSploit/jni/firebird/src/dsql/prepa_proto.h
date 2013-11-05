/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		prepa_proto.h
 *	DESCRIPTION:	Prototype Header file for preparse.cpp
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

#ifndef DSQL_PREPA_PROTO_H
#define DSQL_PREPA_PROTO_H

bool PREPARSE_execute(ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*, USHORT, const SCHAR*,
	bool*, USHORT);

#endif //  DSQL_PREPA_PROTO_H

