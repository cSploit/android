/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blb_proto.h
 *	DESCRIPTION:	Prototype header file for Jrd::blb.cpp
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

#ifndef JRD_BLB_PROTO_H
#define JRD_BLB_PROTO_H

#include "../jrd/jrd.h"
#include "../jrd/blb.h"
#include "../jrd/exe.h"
#include "../jrd/lls.h"
#include "../jrd/val.h"
#include "../jrd/req.h"
#include "../common/classes/array.h"

void BLB_garbage_collect(Jrd::thread_db*, Jrd::RecordStack&, Jrd::RecordStack&, ULONG, Jrd::jrd_rel*);
void BLB_gen_bpb(SSHORT source, SSHORT target, UCHAR sourceCharset, UCHAR targetCharset, Firebird::UCharBuffer& bpb);
void BLB_gen_bpb_from_descs(const dsc*, const dsc*, Firebird::UCharBuffer&);


class AutoBlb
{
public:
	AutoBlb(Jrd::thread_db* atdbb, Jrd::blb* ablob)
		: tdbb(atdbb),
		  blob(ablob)
	{
	}

	~AutoBlb()
	{
		blob->BLB_close(tdbb);
	}

	Jrd::blb* getBlb()
	{
		return blob;
	}

	Jrd::blb* operator ->()
	{
		return blob;
	}

private:
	AutoBlb(const AutoBlb&);				// not implemented
	AutoBlb& operator =(const AutoBlb&);	// not implemented

private:
	Jrd::thread_db* tdbb;
	Jrd::blb* blob;
};

#endif	// JRD_BLB_PROTO_H
