/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		sdw_proto.h
 *	DESCRIPTION:	Prototype Header file for sdw.cpp
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

#ifndef JRD_SDW_PROTO_H
#define JRD_SDW_PROTO_H
namespace Jrd {
	class jrd_file;
	class Shadow;
}

void	SDW_add(Jrd::thread_db* tdbb, const TEXT*, USHORT, USHORT);
int		SDW_add_file(Jrd::thread_db* tdbb, const TEXT*, SLONG, USHORT);
void	SDW_check(Jrd::thread_db* tdbb);
bool	SDW_check_conditional(Jrd::thread_db* tdbb);
void	SDW_close();
void	SDW_dump_pages(Jrd::thread_db* tdbb);
void	SDW_get_shadows(Jrd::thread_db* tdbb);
void	SDW_init(Jrd::thread_db* tdbb, bool, bool);
bool	SDW_lck_update(Jrd::thread_db*, SLONG);
void	SDW_notify(Jrd::thread_db* tdbb);
bool	SDW_rollover_to_shadow(Jrd::thread_db* tdbb, Jrd::jrd_file*, const bool);
// It's never called directly, but through SDW_check().
//void	SDW_shutdown_shadow(Jrd::Shadow*);
void	SDW_start(Jrd::thread_db* tdbb, const TEXT*, USHORT, USHORT, bool);

#endif // JRD_SDW_PROTO_H

