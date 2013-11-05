/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pio_proto.h
 *	DESCRIPTION:	Prototype header file for unix.cpp, vms.cpp & winnt.cpp
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

#ifndef JRD_PIO_PROTO_H
#define JRD_PIO_PROTO_H

#include "../common/classes/fb_string.h"

namespace Jrd {
	class jrd_file;
	class Database;
	class BufferDesc;
}

namespace Ods {
	struct pag;
}

int		PIO_add_file(Jrd::Database*, Jrd::jrd_file*, const Firebird::PathName&, SLONG);
void	PIO_close(Jrd::jrd_file*);
Jrd::jrd_file*	PIO_create(Jrd::Database*, const Firebird::PathName&,
							const bool, const bool, const bool);
bool	PIO_expand(const TEXT*, USHORT, TEXT*, size_t);
void	PIO_extend(Jrd::Database*, Jrd::jrd_file*, const ULONG, const USHORT);
void	PIO_flush(Jrd::Database*, Jrd::jrd_file*);
void	PIO_force_write(Jrd::jrd_file*, const bool, const bool);
ULONG	PIO_get_number_of_pages(const Jrd::jrd_file*, const USHORT);
void	PIO_get_unique_file_id(const Jrd::jrd_file*, Firebird::UCharBuffer&);
void	PIO_header(Jrd::Database*, SCHAR*, int);
USHORT	PIO_init_data(Jrd::Database*, Jrd::jrd_file*, ISC_STATUS*, ULONG, USHORT);
Jrd::jrd_file*	PIO_open(Jrd::Database*, const Firebird::PathName&,
						 const Firebird::PathName&, const bool);
bool	PIO_read(Jrd::jrd_file*, Jrd::BufferDesc*, Ods::pag*, ISC_STATUS*);

#ifdef SUPERSERVER_V2
bool	PIO_read_ahead(Jrd::Database*, SLONG, SCHAR*, SLONG,
				   struct Jrd::phys_io_blk*, ISC_STATUS*);
bool	PIO_status(Jrd::Database*, struct Jrd::phys_io_blk*, ISC_STATUS*);
#endif

#ifdef SUPPORT_RAW_DEVICES
bool	PIO_on_raw_device(const Firebird::PathName&);
int		PIO_unlink(const Firebird::PathName&);
#endif
bool	PIO_write(Jrd::jrd_file*, Jrd::BufferDesc*, Ods::pag*, ISC_STATUS*);

#endif // JRD_PIO_PROTO_H

