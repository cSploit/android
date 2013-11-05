/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sqz_proto.h
 *	DESCRIPTION:	Prototype header file for sqz.cpp
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

#ifndef JRD_SQZ_PROTO_H
#define JRD_SQZ_PROTO_H

#include "../jrd/req.h"
#include "../jrd/sqz.h"

namespace Jrd {
	class Record;
}

USHORT	SQZ_apply_differences(Jrd::Record*, const SCHAR*, const SCHAR* const);
USHORT	SQZ_compress(const Jrd::DataComprControl*, const SCHAR*, SCHAR*, int);
USHORT	SQZ_compress_length(const Jrd::DataComprControl*, const SCHAR*, int);
UCHAR*	SQZ_decompress(const UCHAR*, USHORT, UCHAR*, const UCHAR* const);
USHORT	SQZ_differences(const SCHAR*, USHORT, SCHAR*, USHORT, SCHAR*, int);
USHORT	SQZ_no_differences(SCHAR* const, int);
void	SQZ_fast(const Jrd::DataComprControl*, const SCHAR*, SCHAR*);
USHORT	SQZ_length(const SCHAR*, int, Jrd::DataComprControl*);

#endif // JRD_SQZ_PROTO_H

