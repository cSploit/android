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
 *  Copyright (c) 2005 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef ODS_PROTO_H
#define ODS_PROTO_H

namespace Ods {

	bool isSupported(USHORT, USHORT);

	size_t bytesBitPIP(size_t page_size);
	size_t pagesPerPIP(size_t page_size);
	size_t pagesPerSCN(size_t page_size);
	size_t maxPagesPerSCN(size_t page_size);
	size_t transPerTIP(size_t page_size);
	size_t gensPerPage(size_t page_size);
	size_t dataPagesPerPP(size_t page_size);
	size_t maxRecsPerDP(size_t page_size);
	size_t maxIndices(size_t page_size);

} // namespace

#endif //ODS_PROTO_H
