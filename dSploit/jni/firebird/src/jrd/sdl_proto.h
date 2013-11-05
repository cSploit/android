/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sdl_proto.h
 *	DESCRIPTION:	Prototype header file for sdl.cpp
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

#ifndef JRD_SDL_PROTO_H
#define JRD_SDL_PROTO_H

namespace Ods {
	struct InternalArrayDesc;
}

struct sdl_info;
struct array_alice;

UCHAR* SDL_clone_sdl(const UCHAR*, size_t, UCHAR*, size_t);
SLONG	SDL_compute_subscript(ISC_STATUS*, const Ods::InternalArrayDesc*, USHORT, const SLONG*);
ISC_STATUS SDL_info(ISC_STATUS*, const UCHAR*, sdl_info*, SLONG*);
const UCHAR* SDL_prepare_slice(const UCHAR*, USHORT);
int		SDL_walk(ISC_STATUS*, const UCHAR*, UCHAR*, Ods::InternalArrayDesc*, SLONG*,
				 SDL_walk_callback, array_slice*);

#endif // JRD_SDL_PROTO_H

