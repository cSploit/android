/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blob_filter.h
 *	DESCRIPTION:	Blob filter interface definitions
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

#ifndef JRD_BLF_H
#define JRD_BLF_H

#include "../include/fb_blk.h"

namespace Jrd {

/* Note: The BlobControl class is the internal version of the
 * blob control structure (ISC_BLOB_CTL) which is in ibase.h.
 * Therefore this structure should be kept in sync with that one,
 * with the exception of the internal members, which are all the
 * ones which appear after ctl_internal. */

// There's already a PTR defined for events, equivalent to SLONG.
// Therefore, redefined PTR here as FPTR_BFILTER_CALLBACK
class BlobControl;
class blb;
typedef ISC_STATUS (*FPTR_BFILTER_CALLBACK)(USHORT, BlobControl*);


class BlobControl : public pool_alloc<type_ctl>
{
public:
	FPTR_BFILTER_CALLBACK	ctl_source;	/* Source filter */
	union {
		blb*		source_handle;		/* Argument to pass to source filter */
		BlobControl* ctl_handle;
	};
	SSHORT		ctl_to_sub_type;		/* Target type */
	SSHORT		ctl_from_sub_type;		/* Source type */
	USHORT		ctl_buffer_length;		/* Length of buffer */
	USHORT		ctl_segment_length;		/* Length of current segment */
	USHORT		ctl_bpb_length;			/* Length of blob parameter block */
	// left char* for API compatibility
	const UCHAR*	ctl_bpb;			/* Address of blob parameter block */
	UCHAR*		ctl_buffer;				/* Address of segment buffer */
	SLONG		ctl_max_segment;		/* Length of longest segment */
	SLONG		ctl_number_segments;	/* Total number of segments */
	SLONG		ctl_total_length;		/* Total length of blob */
	ISC_STATUS*		ctl_status;			/* Address of status vector */
	IPTR		ctl_data[8];			/* Application specific data */
	void*	ctl_internal[3];			/* Firebird internal-use only */
	Firebird::string	ctl_exception_message;	/* Message to use in case of filter exception */
public:
	explicit BlobControl(MemoryPool& p)
		: ctl_exception_message(p)
	{ }
	BlobControl()
		: ctl_exception_message()
	{ }
};



/* Blob filter management */

class BlobFilter : public pool_alloc<type_blf>
{
    public:
	BlobFilter*	blf_next;				/* Next known filter */
	SSHORT		blf_from;				/* Source sub-type */
	SSHORT		blf_to;					/* Target sub-type */
	FPTR_BFILTER_CALLBACK	blf_filter;	/* Entrypoint of filter */
	Firebird::string	blf_exception_message;	/* message to be used in case of filter exception */
    public:
	BlobFilter(MemoryPool& p)
		: blf_exception_message(p)
	{ }
};

// BRS 29-Apr-2004
// replace those constants with public defined ones isc_blob_filter_
//
// const int ACTION_open			= 0;
// const int ACTION_get_segment	= 1;
// const int ACTION_close			= 2;
// const int ACTION_create			= 3;
// const int ACTION_put_segment	= 4;
// const int ACTION_alloc			= 5;
// const int ACTION_free			= 6;
// const int ACTION_seek			= 7;
//

static const char* const EXCEPTION_MESSAGE = "The blob filter: \t\t%s\n"
										"\treferencing entrypoint: \t%s\n"
										"\t             in module: \t%s\n"
										"\tcaused the fatal exception:";

} //namespace Jrd

#endif /* JRD_BLF_H */

