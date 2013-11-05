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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_RESULT_SET_H
#define JRD_RESULT_SET_H

struct dsc;

namespace Jrd {

class thread_db;
class jrd_tra;
class Attachment;
class PreparedStatement;


class ResultSet
{
friend class PreparedStatement;

public:
	ResultSet(thread_db* tdbb, PreparedStatement* aStmt, jrd_tra* aTransaction);
	~ResultSet();

public:
	bool fetch(thread_db* tdbb);
	bool isNull(int param) const;
	dsc& getDesc(int param);

private:
	PreparedStatement* stmt;
	jrd_tra* transaction;
	bool firstFetchDone;
};


}	// namespace

#endif	// JRD_RESULT_SET_H
