/*
 *	PROGRAM:		JRD access method
 *	MODULE:			Mapping.h
 *	DESCRIPTION:	Maps names in authentication block
 *
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2014 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef JRD_MAPPING
#define JRD_MAPPING

#include "../common/classes/alloc.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/ClumpletReader.h"
#include "../jrd/recsrc/RecordSource.h"
#include "../jrd/DatabaseSnapshot.h"

namespace Jrd {

void mapUser(Firebird::string& name, Firebird::string& trusted_role, Firebird::string* auth_method,
	Firebird::AuthReader::AuthBlock* newAuthBlock, const Firebird::AuthReader::AuthBlock& authBlock,
	const char* alias, const char* db, const char* securityDb);
void clearMap(const char* dbName);

class GlobalMappingScan: public VirtualTableScan
{
public:
	GlobalMappingScan(CompilerScratch* csb, const Firebird::string& name, StreamType stream)
		: VirtualTableScan(csb, name, stream)
	{}

protected:
	const Format* getFormat(thread_db* tdbb, jrd_rel* relation) const;
	bool retrieveRecord(thread_db* tdbb, jrd_rel* relation, FB_UINT64 position, Record* record) const;
};

class MappingList : public DataDump
{
public:
	explicit MappingList(jrd_tra* tra);

	RecordBuffer* getList(thread_db* tdbb, jrd_rel* relation);

private:
	RecordBuffer* makeBuffer(thread_db* tdbb);
};

} // namespace Jrd


#endif // JRD_MAPPING
