/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blf_proto.h
 *	DESCRIPTION:	Prototype header file for blob_filter.cpp
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

#ifndef JRD_BLF_PROTO_H
#define JRD_BLF_PROTO_H

namespace Jrd {
	struct bid;
}

// Does this file really need the extern C for external blob filters?
extern "C" {

//class Jrd::BlobControl;

void	BLF_close_blob(Jrd::thread_db*, Jrd::BlobControl**);
void	BLF_create_blob(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::BlobControl**, Jrd::bid*,
										 USHORT, const UCHAR*,
										 Jrd::FPTR_BFILTER_CALLBACK, Jrd::BlobFilter*);
ISC_STATUS	BLF_get_segment(Jrd::thread_db*, Jrd::BlobControl**, USHORT*, USHORT, UCHAR*);
Jrd::BlobFilter*	BLF_lookup_internal_filter(Jrd::thread_db*, SSHORT, SSHORT);
void	BLF_open_blob(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::BlobControl**, const Jrd::bid*,
									   USHORT, const UCHAR*,
									   Jrd::FPTR_BFILTER_CALLBACK, Jrd::BlobFilter*);
void	BLF_put_segment(Jrd::thread_db*, Jrd::BlobControl**, USHORT, const UCHAR*);

} /* extern "C" */

#endif /* JRD_BLF_PROTO_H */

