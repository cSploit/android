/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ext.h
 *	DESCRIPTION:	External file access definitions
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

#ifndef JRD_EXT_H
#define JRD_EXT_H

#include <stdio.h>

namespace Jrd {

/* External file access block */

class ExternalFile : public pool_alloc_rpt<SCHAR, type_ext>
{
public:
#ifdef VMS
	Format*	ext_format;			/* External format */
#endif
	//UCHAR*	ext_stuff;			// Random stuff
	USHORT	ext_flags;			/* Misc and cruddy flags */
	USHORT	ext_tra_cnt;		// How many transactions used the file
#ifdef VMS
	int		ext_ifi;			/* Internal file identifier */
	int		ext_isi;			/* Internal stream (default) */
#else
	FILE*	ext_ifi;			/* Internal file identifier */
	//int*	ext_isi;			// Internal stream (default)
#endif
	//USHORT	ext_record_length;	// Record length
#ifdef VMS
	USHORT	ext_file_type;		/* File type */

	USHORT	ext_index_count;	/* Number of indices */
	UCHAR*	ext_indices;		/* Index descriptions */
	UCHAR	ext_dbkey[8];		/* DBKEY */
#endif
	char	ext_filename[1];
};

//const int EXT_opened	= 1;	// File has been opened
const int EXT_eof		= 2;	/* Positioned at EOF */
const int EXT_readonly	= 4;	/* File could only be opened for read */
const int EXT_last_read		= 8;	// last operation was read
const int EXT_last_write	= 16;	// last operation was write

#ifdef VMS
struct irsb_ext
{
	USHORT irsb_flags;			/* flags (a whole word!) */
	UCHAR irsb_ext_dbkey[8];	/* DBKEY */
};
#endif


/* Overload record parameter block with external file stuff */

#ifdef VMS
#define rpb_ext_isi	rpb_f_page
#define rpb_ext_dbkey	rpb_b_page
#endif

} //namespace Jrd

#endif // JRD_EXT_H

