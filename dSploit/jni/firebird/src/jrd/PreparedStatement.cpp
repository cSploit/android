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

#include "../jrd/PreparedStatement.h"
#include "../jrd/ResultSet.h"
#include "../jrd/align.h"
#include "../jrd/jrd.h"
#include "../jrd/dsc.h"
#include "../dsql/dsql.h"
#include "../dsql/sqlda_pub.h"
#include "../dsql/dsql_proto.h"

namespace Jrd {


PreparedStatement::PreparedStatement(thread_db* tdbb, Firebird::MemoryPool& pool,
			Attachment* attachment, jrd_tra* transaction, const Firebird::string& text)
	: Firebird::PermanentStorage(pool),
	  values(pool),
	  blr(pool),
	  message(pool),
	  resultSet(NULL)
{
	request = DSQL_allocate_statement(tdbb, attachment);
	try
	{
		const Database& dbb = *tdbb->getDatabase();
		const int dialect = dbb.dbb_flags & DBB_DB_SQL_dialect_3 ? SQL_DIALECT_V6 : SQL_DIALECT_V5;

		DSQL_prepare(tdbb, transaction, &request, text.length(), text.c_str(), dialect,
			0, NULL, 0, NULL);

		if (request->req_receive) {
			parseDsqlMessage(request->req_receive, values, blr, message);
		}
	}
	catch (const Firebird::Exception&)
	{
		DSQL_free_statement(tdbb, request, DSQL_drop);
		throw;
	}
}


PreparedStatement::~PreparedStatement()
{
	thread_db* tdbb = JRD_get_thread_data();

	DSQL_free_statement(tdbb, request, DSQL_drop);

	if (resultSet)
		resultSet->stmt = NULL;
}


void PreparedStatement::execute(thread_db* tdbb, jrd_tra* transaction)
{
	fb_assert(resultSet == NULL);
	DSQL_execute(tdbb, &transaction, request, 0, NULL, 0, 0, NULL, 0, NULL, /*0,*/ 0, NULL);
}


ResultSet* PreparedStatement::executeQuery(thread_db* tdbb, jrd_tra* transaction)
{
	fb_assert(resultSet == NULL && request->req_receive);
	return FB_NEW(getPool()) ResultSet(tdbb, this, transaction);
}


int PreparedStatement::getResultCount() const
{
	return values.getCount() / 2;
}


void PreparedStatement::parseDsqlMessage(dsql_msg* dsqlMsg, Firebird::Array<dsc>& values,
	Firebird::UCharBuffer& blr, Firebird::UCharBuffer& msg)
{
	// Parameters in dsqlMsg->msg_parameters almost always linked in descending
	// order by par_index. The only known exception is EXECUTE BLOCK statement.
	// To generate correct BLR we must walk params in ascending par_index order.
	// So store all params in array in an ascending par_index order despite of
	// order in linked list.

	Firebird::HalfStaticArray<const dsql_par*, 16> params;
	USHORT first_index = 0;
	for (const dsql_par* par = dsqlMsg->msg_parameters; par; par = par->par_next)
	{
		if (par->par_index)
		{
			if (!first_index)
				first_index = par->par_index;

			if (first_index > par->par_index)
				params.insert(0, par);
			else
				params.add(par);
		}
	}

	size_t msgLength = 0;
	int paramCount = params.getCount();
	int i = 0;

	for (; i < paramCount; i++)
	{
		const dsql_par* par = params[i];
#ifdef DEV_BUILD
		// Verify that par_index is in ascending order
		if (i < paramCount - 1)
		{
			fb_assert(par->par_index < params[i + 1]->par_index);
		}
#endif
		if (type_alignments[par->par_desc.dsc_dtype])
			msgLength = FB_ALIGN(msgLength, type_alignments[par->par_desc.dsc_dtype]);
		msgLength += par->par_desc.dsc_length;
		msgLength = FB_ALIGN(msgLength, type_alignments[dtype_short]);
		msgLength += sizeof(SSHORT);
	}

	paramCount *= 2;

	blr.add(blr_version5);
	blr.add(blr_begin);
	blr.add(blr_message);
	blr.add(0);
	blr.add(paramCount);
	blr.add(paramCount >> 8);

	values.resize(paramCount);
	msg.resize(msgLength);

	msgLength = 0;
	dsc* value = values.begin();

	for (i = 0; i < paramCount / 2; i++)
	{
		const dsql_par* par = params[i];

		if (type_alignments[par->par_desc.dsc_dtype])
			msgLength = FB_ALIGN(msgLength, type_alignments[par->par_desc.dsc_dtype]);
		*value = par->par_desc;
		value->dsc_address = msg.begin() + msgLength;
		msgLength += par->par_desc.dsc_length;

		generateBlr(value, blr);
		value++;

		// Calculate the NULL indicator offset
		if (type_alignments[dtype_short])
			msgLength = FB_ALIGN(msgLength, type_alignments[dtype_short]);
		value->makeShort(0);
		value->dsc_address = msg.begin() + msgLength;
		// set NULL indicator value
		*((SSHORT*) value->dsc_address) = -1;
		msgLength += sizeof(SSHORT);

		// Generate BLR for the NULL indicator
		generateBlr(value, blr);
		value++;
	}

	blr.add(blr_end);
}

void PreparedStatement::generateBlr(const dsc* desc, Firebird::UCharBuffer& blr)
{
	USHORT length = 0;

	switch (desc->dsc_dtype)
	{
	case dtype_text:
		blr.add(blr_text2);
		blr.add(desc->dsc_ttype());
		blr.add(desc->dsc_ttype() >> 8);
		length = desc->dsc_length;
		blr.add(length);
		blr.add(length >> 8);
		break;

	case dtype_varying:
		blr.add(blr_varying2);
		blr.add(desc->dsc_ttype());
		blr.add(desc->dsc_ttype() >> 8);
		fb_assert(desc->dsc_length >= sizeof(USHORT));
		length = desc->dsc_length - sizeof(USHORT);
		blr.add(length);
		blr.add(length >> 8);
		break;

	case dtype_short:
		blr.add(blr_short);
		blr.add(desc->dsc_scale);
		break;

	case dtype_long:
		blr.add(blr_long);
		blr.add(desc->dsc_scale);
		break;

	case dtype_quad:
		blr.add(blr_quad);
		blr.add(desc->dsc_scale);
		break;

	case dtype_int64:
		blr.add(blr_int64);
		blr.add(desc->dsc_scale);
		break;

	case dtype_real:
		blr.add(blr_float);
		break;

	case dtype_double:
		blr.add(blr_double);
		break;

	case dtype_sql_date:
		blr.add(blr_sql_date);
		break;

	case dtype_sql_time:
		blr.add(blr_sql_time);
		break;

	case dtype_timestamp:
		blr.add(blr_timestamp);
		break;

	case dtype_array:
		blr.add(blr_quad);
		blr.add(0);
		break;

	case dtype_blob:
		blr.add(blr_blob2);
		blr.add(desc->dsc_sub_type);
		blr.add(desc->dsc_sub_type >> 8);
		blr.add(desc->getTextType());
		blr.add(desc->getTextType() >> 8);
		break;

	default:
		fb_assert(false);
	}
}


}	// namespace
