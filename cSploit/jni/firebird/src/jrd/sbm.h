/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sbm.h
 *	DESCRIPTION:	Sparse bit map block definitions
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_SBM_H
#define JRD_SBM_H

#include "../common/classes/sparse_bitmap.h"

namespace Jrd {

// Bitmap of record numbers
typedef Firebird::SparseBitmap<FB_UINT64> RecordBitmap;

// Bitmap of page numbers
typedef Firebird::SparseBitmap<ULONG> PageBitmap;

// Bitmap of generic 32-bit integers
typedef Firebird::SparseBitmap<ULONG> UInt32Bitmap;

// Please do not try to turn these macros to inline routines simply.
// They are used in very performance-sensitive places and we don't want
// pool thread-specific pool pointer be expanded unless necessary.
//
// Only if you kill thread-specific pool usage (which is certainly incorrect now)
// you may eliminate these macros safely.

// Bitmap of 64-bit record numbers
#define RBM_SET(POOL_PTR, BITMAP_PPTR, VALUE) \
	(*(BITMAP_PPTR) ? *(BITMAP_PPTR) : (*(BITMAP_PPTR) = FB_NEW(*(POOL_PTR)) Jrd::RecordBitmap(*(POOL_PTR))))->set(VALUE)

// Bitmap of 32-bit integers
#define SBM_SET(POOL_PTR, BITMAP_PPTR, VALUE) \
	(*(BITMAP_PPTR) ? *(BITMAP_PPTR) : (*(BITMAP_PPTR) = FB_NEW(*(POOL_PTR)) Jrd::UInt32Bitmap(*(POOL_PTR))))->set(VALUE)

// Bitmap of page numbers
#define PBM_SET(POOL_PTR, BITMAP_PPTR, VALUE) \
	(*(BITMAP_PPTR) ? *(BITMAP_PPTR) : (*(BITMAP_PPTR) = FB_NEW(*(POOL_PTR)) Jrd::PageBitmap(*(POOL_PTR))))->set(VALUE)

} //namespace Jrd

#endif	// JRD_SBM_H

