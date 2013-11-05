/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sdl.h
 *	DESCRIPTION:	Interface definitions for array handler
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

#ifndef JRD_SDL_H
#define JRD_SDL_H

#include "../common/classes/MetaName.h"

struct sdl_info
{
	USHORT			sdl_info_fid;
	USHORT			sdl_info_rid;
	Firebird::MetaName	sdl_info_field;
	Firebird::MetaName	sdl_info_relation;
	dsc				sdl_info_element;
	USHORT			sdl_info_dimensions;
	SLONG			sdl_info_lower[MAX_ARRAY_DIMENSIONS];
	SLONG			sdl_info_upper[MAX_ARRAY_DIMENSIONS];
};


struct array_slice
{
	enum slice_dir_t { slc_reading_array, slc_writing_array };
	DSC slice_desc;
	const BLOB_PTR* slice_end;
	const BLOB_PTR* slice_high_water;
	BLOB_PTR* slice_base;
	USHORT slice_element_length;
	slice_dir_t slice_direction;
	SLONG slice_count;
};

typedef void (*SDL_walk_callback)(array_slice*, ULONG, dsc*);
#endif /* JRD_SDL_H */

