/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Adriano dos Santos Fernandes
 */

#ifndef JRD_VIRTUAL_TABLE_H
#define JRD_VIRTUAL_TABLE_H

namespace Jrd
{
	class VirtualTable
	{
	public:
		static void erase(thread_db*, record_param*);
		static void modify(thread_db*, record_param*, record_param*);
		static void store(thread_db*, record_param*);
	};

} // namespace Jrd

#endif // JRD_VIRTUAL_TABLE_H
