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

#include "firebird.h"
#include "../common/gdsassert.h"
#include "../common/dsc.h"
#include "../common/classes/auto.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"

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
	bool isNull(unsigned param) const;
	dsc& getDesc(unsigned param);

	SSHORT getSmallInt(thread_db* tdbb, unsigned param, SCHAR scale = 0)
	{
		fb_assert(param > 0);

		SSHORT value;

		dsc desc;
		desc.makeShort(scale, &value);
		moveDesc(tdbb, param, desc);

		return value;
	}

	SLONG getInt(thread_db* tdbb, unsigned param, SCHAR scale = 0)
	{
		fb_assert(param > 0);

		SLONG value;

		dsc desc;
		desc.makeLong(scale, &value);
		moveDesc(tdbb, param, desc);

		return value;
	}

	SINT64 getBigInt(thread_db* tdbb, unsigned param, SCHAR scale = 0)
	{
		fb_assert(param > 0);

		SINT64 value;

		dsc desc;
		desc.makeInt64(scale, &value);
		moveDesc(tdbb, param, desc);

		return value;
	}

	double getDouble(thread_db* tdbb, unsigned param)
	{
		fb_assert(param > 0);

		double value;

		dsc desc;
		desc.makeDouble(&value);
		moveDesc(tdbb, param, desc);

		return value;
	}

	Firebird::string getString(thread_db* tdbb, unsigned param);
	Firebird::MetaName getMetaName(thread_db* tdbb, unsigned param);

private:
	void moveDesc(thread_db* tdbb, unsigned param, dsc& desc);

private:
	PreparedStatement* stmt;
	jrd_tra* transaction;
	bool firstFetchDone;
};

typedef Firebird::AutoPtr<ResultSet> AutoResultSet;


}	// namespace

#endif	// JRD_RESULT_SET_H
