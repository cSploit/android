/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		msg.h
 *	DESCRIPTION:	Message system definitions
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
 *
 */

#ifndef JRD_MSG_H
#define JRD_MSG_H

#define MSG_NUMBER(facility, code)	((SLONG) facility * 10000 + code)
const USHORT MSG_BUCKET			= 1024;
const UCHAR MSG_MAJOR_VERSION	= 1;
// trick to silence the compiler when FB_MSG_MINOR_VERSION is 0
#define FB_MSG_MINOR_VERSION	1
const UCHAR MSG_MINOR_VERSION	= FB_MSG_MINOR_VERSION;

// Message file header block

struct isc_msghdr
{
	UCHAR msghdr_major_version;	// Version number
	UCHAR msghdr_minor_version;	// Version number
	USHORT msghdr_bucket_size;	// Bucket size of B-tree
	ULONG msghdr_top_tree;		// Start address of first index bucket
	ULONG msghdr_origin;		// Origin for data records, unused.
	USHORT msghdr_levels;		// Levels in tree
};

// Index node

struct msgnod
{
	ULONG msgnod_code;			// Message code
	ULONG msgnod_seek;			// Offset of next bucket or message
};

// Leaf node

struct msgrec
{
	ULONG msgrec_code;			// Message code
	USHORT msgrec_length;		// Length of message text
	USHORT msgrec_flags;		// Misc flags
	TEXT msgrec_text[1];		// Text of message
};

typedef msgrec *MSGREC;
#define NEXT_LEAF(leaf)	(MSGREC) \
	((SCHAR*) leaf + FB_ALIGN(OFFSETA (MSGREC, msgrec_text) + leaf->msgrec_length, sizeof (SLONG)))

#endif // JRD_MSG_H

