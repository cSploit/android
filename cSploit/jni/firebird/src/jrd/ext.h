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

// External file access block

class ExternalFile : public pool_alloc_rpt<SCHAR, type_ext>
{
public:
	USHORT	ext_flags;			// Misc and cruddy flags
	USHORT	ext_tra_cnt;		// How many transactions used the file
	FILE*	ext_ifi;			// Internal file identifier
	char	ext_filename[1];
};

const int EXT_readonly		= 1;	// File could only be opened for read
const int EXT_last_read		= 2;	// last operation was read
const int EXT_last_write	= 4;	// last operation was write

} //namespace Jrd

#endif // JRD_EXT_H
