/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		utl_proto.h
 *	DESCRIPTION:	Prototype header file for utl.cpp
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

#ifndef JRD_UTL_PROTO_H
#define JRD_UTL_PROTO_H

#include <firebird/Utl.h>

#include "fb_types.h"
#include "../yvalve/YObjects.h"
#include "../common/classes/ImplementHelper.h"

#ifdef __cplusplus
extern "C" {
#endif

int		API_ROUTINE gds__blob_size(FB_API_HANDLE*, SLONG *, SLONG *, SLONG *);
void	API_ROUTINE_VARARG isc_expand_dpb(SCHAR**, SSHORT*, ...);
int		API_ROUTINE isc_modify_dpb(SCHAR**, SSHORT*, USHORT, const SCHAR*, SSHORT);
int		API_ROUTINE gds__edit(const TEXT*, USHORT);
SLONG	API_ROUTINE_VARARG isc_event_block(UCHAR**, UCHAR**, USHORT, ...);
USHORT	API_ROUTINE isc_event_block_a(SCHAR**, SCHAR**, USHORT, TEXT**);
void	API_ROUTINE isc_event_block_s(SCHAR**, SCHAR**, USHORT, TEXT**, USHORT*);

void	API_ROUTINE isc_event_counts(ULONG*, SSHORT, UCHAR*, const UCHAR*);
void	API_ROUTINE isc_get_client_version(SCHAR *);
int		API_ROUTINE isc_get_client_major_version();
int		API_ROUTINE isc_get_client_minor_version();
void	API_ROUTINE gds__map_blobs(int*, int*);
void	API_ROUTINE isc_set_debug(int);
void	API_ROUTINE isc_set_login(const UCHAR**, SSHORT*);
void	API_ROUTINE isc_set_single_user(const UCHAR**, SSHORT*, const TEXT*);
int		API_ROUTINE isc_version(FB_API_HANDLE*, FPTR_VERSION_CALLBACK, void*);
void	API_ROUTINE isc_format_implementation(USHORT, USHORT, TEXT *,
												  USHORT, USHORT, TEXT *);
uintptr_t	API_ROUTINE isc_baddress(SCHAR*);
void	API_ROUTINE isc_baddress_s(const SCHAR*, uintptr_t*);
int		API_ROUTINE BLOB_close(struct bstream *);
int		API_ROUTINE blob__display(SLONG*, FB_API_HANDLE*, FB_API_HANDLE*, const TEXT*, const SSHORT*);
int		API_ROUTINE BLOB_display(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const TEXT*);
int		API_ROUTINE blob__dump(SLONG*, FB_API_HANDLE*, FB_API_HANDLE*, const TEXT*, const SSHORT*);
int		API_ROUTINE BLOB_dump(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const SCHAR*);
int		API_ROUTINE blob__edit(SLONG*, FB_API_HANDLE*, FB_API_HANDLE*, const TEXT*, const SSHORT*);
int		API_ROUTINE BLOB_edit(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const SCHAR*);
int		API_ROUTINE BLOB_get(struct bstream*);
int		API_ROUTINE blob__load(SLONG*, FB_API_HANDLE*, FB_API_HANDLE*, const TEXT*, const SSHORT*);
int		API_ROUTINE BLOB_load(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const TEXT*);
int		API_ROUTINE BLOB_text_dump(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const SCHAR*);
int		API_ROUTINE BLOB_text_load(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const TEXT*);
struct	bstream* API_ROUTINE Bopen(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, const SCHAR*);
struct  bstream* API_ROUTINE BLOB_open(FB_API_HANDLE, SCHAR*, int);
int		API_ROUTINE BLOB_put(SCHAR, struct bstream*);
int		API_ROUTINE gds__thread_start(FPTR_INT_VOID_PTR*, void*, int, int, void*);
#ifdef __cplusplus
} /* extern "C" */
#endif

// new utl
namespace Firebird
{
	class ClumpletWriter;
}
void setLogin(Firebird::ClumpletWriter& dpb, bool spbFlag);

namespace Why
{

class UtlInterface : public Firebird::AutoIface<Firebird::IUtl, FB_UTL_VERSION>
{
	// IUtl implementation
public:
	void FB_CARG getFbVersion(Firebird::IStatus* status, Firebird::IAttachment* att,
		Firebird::IVersionCallback* callback);
	void FB_CARG loadBlob(Firebird::IStatus* status, ISC_QUAD* blobId, Firebird::IAttachment* att,
		Firebird::ITransaction* tra, const char* file, FB_BOOLEAN txt);
	void FB_CARG dumpBlob(Firebird::IStatus* status, ISC_QUAD* blobId, Firebird::IAttachment* att,
		Firebird::ITransaction* tra, const char* file, FB_BOOLEAN txt);
	void FB_CARG getPerfCounters(Firebird::IStatus* status, Firebird::IAttachment* att,
		const char* countersSet, ISC_INT64* counters);			// in perf.cpp
	virtual YAttachment* FB_CARG executeCreateDatabase(Firebird::IStatus* status,
		unsigned stmtLength, const char* creatDBstatement, unsigned dialect,
		FB_BOOLEAN* stmtIsCreateDb = NULL);
};

}

#endif // JRD_UTL_PROTO_H

