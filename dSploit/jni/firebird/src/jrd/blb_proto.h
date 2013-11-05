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

void   BLB_cancel(Jrd::thread_db*, Jrd::blb*);
void   BLB_check_well_formed(Jrd::thread_db*, const dsc* desc, Jrd::blb*);
void   BLB_close(Jrd::thread_db*, Jrd::blb*);
Jrd::blb*   BLB_create(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::bid*);
Jrd::blb*   BLB_create2(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::bid*, USHORT, const UCHAR*);
void   BLB_garbage_collect(Jrd::thread_db*, Jrd::RecordStack&, Jrd::RecordStack&, SLONG, Jrd::jrd_rel*);
void BLB_gen_bpb(SSHORT source, SSHORT target, UCHAR sourceCharset, UCHAR targetCharset, Firebird::UCharBuffer& bpb);
void BLB_gen_bpb_from_descs(const dsc*, const dsc*, Firebird::UCharBuffer&);
Jrd::blb*   BLB_get_array(Jrd::thread_db*, Jrd::jrd_tra*, const Jrd::bid*, Ods::InternalArrayDesc*);
ULONG  BLB_get_data(Jrd::thread_db*, Jrd::blb*, UCHAR*, SLONG, bool = true);
USHORT BLB_get_segment(Jrd::thread_db*, Jrd::blb*, UCHAR*, USHORT);
SLONG  BLB_get_slice(Jrd::thread_db*, Jrd::jrd_tra*, const Jrd::bid*, const UCHAR*, USHORT,
	const UCHAR*, SLONG, UCHAR*);
SLONG  BLB_lseek(Jrd::blb*, USHORT, SLONG);

void BLB_move(Jrd::thread_db*, dsc*, dsc*, Jrd::jrd_nod*);
Jrd::blb* BLB_open(Jrd::thread_db*, Jrd::jrd_tra*, const Jrd::bid*);
Jrd::blb* BLB_open2(Jrd::thread_db*, Jrd::jrd_tra*, const Jrd::bid*, USHORT, const UCHAR*, bool = false);
void BLB_put_data(Jrd::thread_db*, Jrd::blb*, const UCHAR*, SLONG);
void BLB_put_segment(Jrd::thread_db*, Jrd::blb*, const UCHAR*, USHORT);
void BLB_put_slice(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::bid*, const UCHAR*, USHORT,
	const UCHAR*, SLONG, UCHAR*);
void BLB_release_array(Jrd::ArrayField*);
void BLB_scalar(Jrd::thread_db*, Jrd::jrd_tra*, const Jrd::bid*, USHORT, const SLONG*, Jrd::impure_value*);


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
		BLB_close(tdbb, blob);
	}

public:
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
