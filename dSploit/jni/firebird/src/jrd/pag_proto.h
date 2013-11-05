/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pag_proto.h
 *	DESCRIPTION:	Prototype header file for pag.cpp
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

#ifndef JRD_PAG_PROTO_H
#define JRD_PAG_PROTO_H

namespace Jrd {
	class thread_db;
	class Database;
	class PageNumber;
	class PageSpace;
	struct win;
}
namespace Ods {
	struct pag;
	struct header_page;
	enum ClumpOper { CLUMP_ADD, CLUMP_REPLACE, CLUMP_REPLACE_ONLY };

}

void	PAG_add_clump(Jrd::thread_db* tdbb, SLONG, USHORT, USHORT, const UCHAR*, Ods::ClumpOper);
USHORT	PAG_add_file(Jrd::thread_db* tdbb, const TEXT*, SLONG);
bool	PAG_add_header_entry(Jrd::thread_db* tdbb, Ods::header_page*, USHORT, USHORT, const UCHAR*);
void	PAG_attach_temp_pages(Jrd::thread_db*, USHORT pageSpaceID);
bool	PAG_replace_entry_first(Jrd::thread_db* tdbb, Ods::header_page*, USHORT, USHORT, const UCHAR*);
Ods::pag*	PAG_allocate(Jrd::thread_db* tdbb, Jrd::win*);
SLONG	PAG_attachment_id(Jrd::thread_db*);
bool	PAG_delete_clump_entry(Jrd::thread_db* tdbb, SLONG, USHORT);
void	PAG_format_header(Jrd::thread_db*);
void	PAG_format_log(Jrd::thread_db*);
void	PAG_format_pip(Jrd::thread_db*, Jrd::PageSpace& pageSpace);
bool	PAG_get_clump(Jrd::thread_db* tdbb, SLONG, USHORT, USHORT*, UCHAR*);
void	PAG_header(Jrd::thread_db*, bool);
void	PAG_header_init(Jrd::thread_db*);
void	PAG_init(Jrd::thread_db*);
void	PAG_init2(Jrd::thread_db*, USHORT);
SLONG	PAG_last_page(Jrd::thread_db* tdbb);
void	PAG_release_page(Jrd::thread_db* tdbb, const Jrd::PageNumber&, const Jrd::PageNumber&);
void	PAG_set_force_write(Jrd::thread_db* tdbb, bool);
void	PAG_set_no_reserve(Jrd::thread_db* tdbb, bool);
void	PAG_set_db_readonly(Jrd::thread_db* tdbb, bool);
void	PAG_set_db_SQL_dialect(Jrd::thread_db* tdbb, SSHORT);
void	PAG_set_page_buffers(Jrd::thread_db* tdbb, ULONG);
void	PAG_sweep_interval(Jrd::thread_db* tdbb, SLONG);

namespace Jrd {
	class PageCountCallback
	{
	public:
//		PageCountCallback() { }
		virtual void newPage(const SLONG, Ods::pag*) = 0;
		virtual ~PageCountCallback() { }
	};
}
ULONG	PAG_page_count(Jrd::Database*, Jrd::PageCountCallback*);

#endif // JRD_PAG_PROTO_H
