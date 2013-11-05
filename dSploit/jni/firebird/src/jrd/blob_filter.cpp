/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blob_filter.cpp
 *	DESCRIPTION:	Blob filter driver
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
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <string.h>
#include <stdio.h>

#include "../common/config/config.h"
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ibase.h"
#include "../jrd/blob_filter.h"
#include "../jrd/tra.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blf_proto.h"
#include "../jrd/filte_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/common.h"
#include "../jrd/ibsetjmp.h"
#include "../jrd/isc_s_proto.h"
#include "gen/iberror.h"

using namespace Jrd;
using namespace Firebird;

// System provided internal filters for filtering internal
// subtypes to text.
// (from_type in [0..8], to_type == isc_blob_text)

static const FPTR_BFILTER_CALLBACK filters[] =
{
	filter_text,
	filter_transliterate_text,
	filter_blr,
	filter_acl,
	0,							// filter_ranges,
	filter_runtime,
	filter_format,
	filter_trans,
	filter_trans				// should be filter_external_file
};


static void open_blob(thread_db*, jrd_tra*, BlobControl**, bid*,
					  USHORT, const UCHAR*,
					  FPTR_BFILTER_CALLBACK,
					  USHORT, BlobFilter*);


void BLF_close_blob(thread_db* tdbb, BlobControl** filter_handle)
{
/**************************************
 *
 *	B L F _ c l o s e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Close a blob and close all the intermediate filters.
 *
 **************************************/

	// Walk the chain of filters to find the ultimate source
	BlobControl* next = *filter_handle;
	while (next->ctl_to_sub_type)
		next = next->ctl_handle;

	FPTR_BFILTER_CALLBACK callback = next->ctl_source;

	START_CHECK_FOR_EXCEPTIONS(next->ctl_exception_message.c_str())

	ISC_STATUS_ARRAY localStatus = {0};

	// Sign off from filter
	// Walk the chain again, telling each filter stage to close
	BlobControl* control;
	for (next = *filter_handle; (control = next);)
	{
		// Close this stage of the filter

		control->ctl_status = localStatus;
		(*control->ctl_source) (isc_blob_filter_close, control);

		// Find the next stage

		next = control->ctl_handle;
		if (!control->ctl_to_sub_type)
			next = NULL;

		// Free this stage's control block allocated by calling the
		// final stage with an isc_blob_filter_alloc, which is why we call
		// the final stage with isc_blob_filter_free here.

		(*callback) (isc_blob_filter_free, control);
	}

	END_CHECK_FOR_EXCEPTIONS(next->ctl_exception_message.c_str())
}


void BLF_create_blob(thread_db* tdbb,
					 jrd_tra* tra_handle,
					 BlobControl** filter_handle,
					 bid* blob_id,
					 USHORT bpb_length,
					 const UCHAR* bpb,
					 FPTR_BFILTER_CALLBACK callback,
					 BlobFilter* filter)
{
/**************************************
 *
 *	B L F _ c r e a t e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open a blob and invoke a filter.
 *
 **************************************/

	open_blob(tdbb, tra_handle, filter_handle, blob_id, bpb_length, bpb,
			  callback, isc_blob_filter_create, filter);
}


ISC_STATUS BLF_get_segment(thread_db* tdbb,
						   BlobControl** filter_handle,
						   USHORT* length,
						   USHORT buffer_length,
						   UCHAR* buffer)
{
/**************************************
 *
 *	B L F _ g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get segment from a blob filter.
 *
 **************************************/
	ISC_STATUS_ARRAY localStatus;
	fb_utils::init_status(localStatus);

	BlobControl* control = *filter_handle;
	control->ctl_status = localStatus;
	control->ctl_buffer = buffer;
	control->ctl_buffer_length = buffer_length;

	ISC_STATUS status;

	START_CHECK_FOR_EXCEPTIONS(control->ctl_exception_message.c_str())
	status = (*control->ctl_source) (isc_blob_filter_get_segment, control);
	END_CHECK_FOR_EXCEPTIONS(control->ctl_exception_message.c_str())

	if (!status || status == isc_segment)
		*length = control->ctl_segment_length;
	else
		*length = 0;

	if (status && status != isc_segment && status != isc_segstr_eof)
	{
		if (status != localStatus[1])
		{
			localStatus[1] = status;
			localStatus[2] = isc_arg_end;
		}

		status_exception::raise(localStatus);
	}

	return status;
}


BlobFilter* BLF_lookup_internal_filter(thread_db* tdbb, SSHORT from, SSHORT to)
{
/**************************************
 *
 *	B L F _ l o o k u p _ i n t e r n a l _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *	Lookup blob filter in data structures.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();

	// Check for system defined filter

	if (to == isc_blob_text && from >= 0 && from < FB_NELEM(filters))
	{
		BlobFilter* result = FB_NEW(*dbb->dbb_permanent) BlobFilter(*dbb->dbb_permanent);
		result->blf_next = NULL;
		result->blf_from = from;
		result->blf_to = to;
		result->blf_filter = filters[from];
		result->blf_exception_message.printf("Exception occurred in system provided internal filters for filtering internal subtype %d to text.",
				from);
		return result;
	}

	return NULL;
}


void BLF_open_blob(thread_db* tdbb,
				   jrd_tra* tra_handle,
				   BlobControl** filter_handle,
				   const bid* blob_id,
				   USHORT bpb_length,
				   const UCHAR* bpb,
				   FPTR_BFILTER_CALLBACK callback,
				   BlobFilter* filter)
{
/**************************************
 *
 *	B L F _ o p e n _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open a blob and invoke a filter.
 *
 **************************************/

	// CVC: We have a function that deals both with opening and creating blobs.
	// Therefore, throwing const away is safe because it won't be changed.
	// Someone might create some crazy filter that calls put_slice, though.
	open_blob(tdbb, tra_handle, filter_handle, const_cast<bid*>(blob_id),
			  bpb_length, bpb, callback, isc_blob_filter_open, filter);
}


void BLF_put_segment(thread_db* tdbb,
					 BlobControl** filter_handle,
					 USHORT length,
					 const UCHAR* buffer)
{
/**************************************
 *
 *	B L F _ p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get segment from a blob filter.
 *
 **************************************/

	ISC_STATUS_ARRAY localStatus;
	fb_utils::init_status(localStatus);

	BlobControl* control = *filter_handle;
	control->ctl_status = localStatus;
	// If the filter is ill behaved, it won't respect the constness
	// even though it's job is to process the buffer and write the
	// result.
	control->ctl_buffer = const_cast<UCHAR*>(buffer);
	control->ctl_buffer_length = length;

	ISC_STATUS status;

	START_CHECK_FOR_EXCEPTIONS(control->ctl_exception_message.c_str())
	status = (*control->ctl_source) (isc_blob_filter_put_segment, control);
	END_CHECK_FOR_EXCEPTIONS(control->ctl_exception_message.c_str())

	if (status)
	{
		if (status != localStatus[1])
		{
			localStatus[1] = status;
			localStatus[2] = isc_arg_end;
		}

		status_exception::raise(localStatus);
	}
}

// SEH moved to separate function to avoid conflicts
// with destructor of BlobControl
inline void initializeFilter(thread_db *tdbb,
							 ISC_STATUS &status,
							 BlobControl* control,
							 BlobFilter* filter,
							 USHORT action)
{
	START_CHECK_FOR_EXCEPTIONS(control->ctl_exception_message.c_str())
	status = (*filter->blf_filter) (action, control);
	END_CHECK_FOR_EXCEPTIONS(control->ctl_exception_message.c_str())
}

static void open_blob(thread_db* tdbb,
					  jrd_tra* tra_handle,
					  BlobControl** filter_handle,
					  bid* blob_id,
					  USHORT bpb_length,
					  const UCHAR* bpb,
					  FPTR_BFILTER_CALLBACK callback,
					  USHORT action,
					  BlobFilter* filter)
{
/**************************************
 *
 *	o p e n _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open a blob and invoke a filter.
 *
 **************************************/
	Database* dbb = tdbb->getDatabase();

	SSHORT from, to;
	USHORT from_charset, to_charset;
	gds__parse_bpb2(bpb_length, bpb, &from, &to, &from_charset, &to_charset,
		NULL, NULL, NULL, NULL);

	if (!filter || !filter->blf_filter)
	{
		status_exception::raise(Arg::Gds(isc_nofilter) << Arg::Num(from) << Arg::Num(to));
	}

	// Allocate a filter control block and open blob

	// utilize a temporary control block just to pass the three
	// necessary internal parameters to the filter
	BlobControl temp;
	temp.ctl_internal[0] = dbb;
	temp.ctl_internal[1] = tra_handle;
	temp.ctl_internal[2] = NULL;
	// CVC: Using ISC_STATUS (SLONG) to return a pointer!!!
	// If we change the function signature, we'll change the public API.
	// ISC_STATUS to pointer!
	BlobControl* prior = (BlobControl*) (*callback) (isc_blob_filter_alloc, &temp);
	prior->ctl_source = callback;

	ISC_STATUS_ARRAY localStatus;
	fb_utils::init_status(localStatus);
	prior->ctl_status = localStatus;

	prior->ctl_internal[0] = dbb;
	prior->ctl_internal[1] = tra_handle;
	prior->ctl_internal[2] = blob_id;

	if ((*callback) (action, prior))
	{
		BLF_close_blob(tdbb, &prior);
		status_exception::raise(localStatus);
	}

	// ISC_STATUS to pointer!
	BlobControl* control = (BlobControl*) (*callback) (isc_blob_filter_alloc, &temp);
	control->ctl_source = filter->blf_filter;
	control->ctl_handle = prior;
	control->ctl_status = localStatus;
	control->ctl_exception_message = filter->blf_exception_message;

	// Two types of filtering can be occuring; either between totally
	// different BLOb sub_types, or between two different
	// character sets (both source & destination subtype must be TEXT in that case).
	// For the character set filter we use the to & from_sub_type fields
	// to tell the filter what character sets to translate between.

	if (filter->blf_filter == filter_transliterate_text)
	{
		fb_assert(to == isc_blob_text);
		fb_assert(from == isc_blob_text);
		control->ctl_to_sub_type = to_charset;
		control->ctl_from_sub_type = from_charset;
	}
	else
	{
		control->ctl_to_sub_type = to;
		control->ctl_from_sub_type = from;
	}
	control->ctl_bpb = bpb;
	control->ctl_bpb_length = bpb_length;

	ISC_STATUS status;
	initializeFilter(tdbb, status, control, filter, action);

	if (!status)
		*filter_handle = control;
	else
	{
		BLF_close_blob(tdbb, &control);

		if (status != localStatus[1])
		{
			localStatus[1] = status;
			localStatus[2] = isc_arg_end;
		}

		status_exception::raise(localStatus);
	}
}

