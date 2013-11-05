/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef STRING_H_
#define STRING_H_
#include "libssh/priv.h"

/* must be 32 bits number + immediately our data */
#ifdef _MSC_VER
#pragma pack(1)
#endif
struct ssh_string_struct {
	uint32_t size;
	unsigned char string[MAX_PACKET_LEN];
}
#if !defined(__SUNPRO_C) && !defined(_MSC_VER)
__attribute__ ((packed))
#endif
#ifdef _MSC_VER
#pragma pack()
#endif
;

#endif /* STRING_H_ */
