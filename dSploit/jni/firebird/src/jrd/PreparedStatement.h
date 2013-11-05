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

#ifndef JRD_PREPARED_STATEMENT_H
#define JRD_PREPARED_STATEMENT_H

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"

struct dsc;

namespace Jrd {

class thread_db;
class jrd_tra;
class Attachment;
class dsql_req;
class dsql_msg;
class ResultSet;


class PreparedStatement : public Firebird::PermanentStorage
{
friend class ResultSet;

public:
	PreparedStatement(thread_db* tdbb, Firebird::MemoryPool& aPool, Attachment* attachment,
		jrd_tra* transaction, const Firebird::string& text);
	~PreparedStatement();

public:
	void execute(thread_db* tdbb, jrd_tra* transaction);
	ResultSet* executeQuery(thread_db* tdbb, jrd_tra* transaction);

	int getResultCount() const;

	dsql_req* getRequest()
	{
		return request;
	}

	static void parseDsqlMessage(dsql_msg* dsqlMsg, Firebird::Array<dsc>& values, Firebird::UCharBuffer& blr, Firebird::UCharBuffer& msg);

private:
	static void generateBlr(const dsc* desc, Firebird::UCharBuffer& blr);

private:
	dsql_req* request;
	Firebird::Array<dsc> values;
	Firebird::UCharBuffer blr;
	Firebird::UCharBuffer message;
	ResultSet* resultSet;
};


}	// namespace

#endif	// JRD_PREPARED_STATEMENT_H
