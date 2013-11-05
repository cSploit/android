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
 */

#ifndef JRD_VIRTUAL_TABLE_H
#define JRD_VIRTUAL_TABLE_H

namespace Jrd {

// To be refactored to a class

namespace VirtualTable {

void close(Jrd::thread_db*, Jrd::RecordSource*);
void erase(Jrd::thread_db*, Jrd::record_param*);
void fini(Jrd::jrd_rel*);
bool get(Jrd::thread_db*, Jrd::RecordSource*);
void modify(Jrd::thread_db*, Jrd::record_param*, Jrd::record_param*);
void open(Jrd::thread_db*, Jrd::RecordSource*);
Jrd::RecordSource* optimize(Jrd::thread_db*, Jrd::OptimizerBlk*, SSHORT);
void store(Jrd::thread_db*, Jrd::record_param*);

} // namespace VirtualTable

} // namespace Jrd

#endif // JRD_VIRTUAL_TABLE_H
