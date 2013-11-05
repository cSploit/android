/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sdw.h
 *	DESCRIPTION:	Shadowing definitions
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

#ifndef JRD_SDW_H
#define JRD_SDW_H

#include "../include/fb_blk.h"

namespace Jrd {
	class jrd_file;

/* Shadowing block */

class Shadow : public pool_alloc<type_sdw>
{
public:
	Shadow* sdw_next;				// next in linked list
	jrd_file* sdw_file;				// Stack of shadow files
	USHORT sdw_number;				// number of shadow
	USHORT sdw_flags;
};

// sdw_flags

const USHORT SDW_dumped	= 1;		/* bit set when file has been copied */
const USHORT SDW_shutdown	= 2;		/* stop shadowing on next cache flush */
const USHORT SDW_manual	= 4;		/* shadow is a manual shadow--don't delete */
const USHORT SDW_delete	= 8;		/* delete the shadow at the next shutdown */
const USHORT SDW_found		= 16;		/* flag to mark shadow found in database */
const USHORT SDW_rollover	= 32;		/* this shadow was rolled over to when the main db file went away */
const USHORT SDW_conditional	= 64;	/* shadow to be used if another shadow becomes unavailable */

/* these macros are a convenient combination of switches:
   the first specifies the shadow is invalid for writing to;
   the second specifies that the shadow no SLONGer exists and the
   shadow block simply hasn't been cleared out yet */

const USHORT SDW_INVALID	= (SDW_shutdown | SDW_delete | SDW_rollover | SDW_conditional);
const USHORT SDW_IGNORE	= (SDW_shutdown | SDW_delete);

} //namespace Jrd

#endif // JRD_SDW_H

