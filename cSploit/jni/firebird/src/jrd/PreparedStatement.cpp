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
#include "../jrd/req.h"
#include "../dsql/dsql.h"
#include "../common/classes/auto.h"
#include "../dsql/sqlda_pub.h"
#include "../dsql/dsql_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/Attachment.h"

using namespace Firebird;


namespace
{
	class ParamCmp
	{
	public:
		static int greaterThan(const Jrd::dsql_par* p1, const Jrd::dsql_par* p2)
		{
			return p1->par_index > p2->par_index;
		}
	};

	void dscToMetaItem(const dsc* desc, MsgMetadata::Item& item)
	{
		item.finished = true;

		switch (desc->dsc_dtype)
		{
			case dtype_text:
				item.type = SQL_TEXT;
				item.charSet = desc->dsc_ttype();
				item.length = desc->dsc_length;
				break;

			case dtype_varying:
				item.type = SQL_VARYING;
				item.charSet = desc->dsc_ttype();
				fb_assert(desc->dsc_length >= sizeof(USHORT));
				item.length = desc->dsc_length - sizeof(USHORT);
				break;

			case dtype_short:
				item.type = SQL_SHORT;
				item.scale = desc->dsc_scale;
				item.length = sizeof(SSHORT);
				break;

			case dtype_long:
				item.type = SQL_LONG;
				item.scale = desc->dsc_scale;
				item.length = sizeof(SLONG);
				break;

			case dtype_quad:
				item.type = SQL_QUAD;
				item.scale = desc->dsc_scale;
				item.length = sizeof(SLONG) * 2;
				break;

			case dtype_int64:
				item.type = SQL_INT64;
				item.scale = desc->dsc_scale;
				item.length = sizeof(SINT64);
				break;

			case dtype_real:
				item.type = SQL_FLOAT;
				item.length = sizeof(float);
				break;

			case dtype_double:
				item.type = SQL_DOUBLE;
				item.length = sizeof(double);
				break;

			case dtype_sql_date:
				item.type = SQL_TYPE_DATE;
				item.length = sizeof(SLONG);
				break;

			case dtype_sql_time:
				item.type = SQL_TYPE_TIME;
				item.length = sizeof(SLONG);
				break;

			case dtype_timestamp:
				item.type = SQL_TIMESTAMP;
				item.length = sizeof(SLONG) * 2;
				break;

			case dtype_array:
				item.type = SQL_ARRAY;
				item.length = sizeof(ISC_QUAD);
				break;

			case dtype_blob:
				item.type = SQL_BLOB;
				item.length = sizeof(ISC_QUAD);
				item.subType = desc->dsc_sub_type;
				item.charSet = desc->getTextType();
				break;

			case dtype_boolean:
				item.type = SQL_BOOLEAN;
				item.length = sizeof(UCHAR);
				break;

			default:
				item.finished = false;
				fb_assert(false);
		}
	}
}


namespace Jrd {


// Move data from result set message to user variables.
void PreparedStatement::Builder::moveFromResultSet(thread_db* tdbb, ResultSet* rs) const
{
	for (Array<OutputSlot>::const_iterator i = outputSlots.begin(); i != outputSlots.end(); ++i)
	{
		switch (i->type)
		{
			case TYPE_SSHORT:
				*(SSHORT*) i->address = rs->getSmallInt(tdbb, i->number);
				break;

			case TYPE_SLONG:
				*(SLONG*) i->address = rs->getInt(tdbb, i->number);
				break;

			case TYPE_SINT64:
				*(SINT64*) i->address = rs->getBigInt(tdbb, i->number);
				break;

			case TYPE_DOUBLE:
				*(double*) i->address = rs->getDouble(tdbb, i->number);
				break;

			case TYPE_STRING:
			{
				AbstractString* str = (AbstractString*) i->address;
				str->replace(0, str->length(), rs->getString(tdbb, i->number));
				break;
			}

			case TYPE_METANAME:
				*(MetaName*) i->address = rs->getMetaName(tdbb, i->number);
				break;

			default:
				fb_assert(false);
		}

		if (i->specifiedAddress && rs->isNull(i->number))
			*i->specifiedAddress = false;
	}
}


// Move data from user variables to the request input message.
void PreparedStatement::Builder::moveToStatement(thread_db* tdbb, PreparedStatement* stmt) const
{
	for (Array<InputSlot>::const_iterator i = inputSlots.begin(); i != inputSlots.end(); ++i)
	{
		if (i->specifiedAddress && !*i->specifiedAddress)
		{
			stmt->setNull(i->number);
			continue;
		}

		switch (i->type)
		{
			case TYPE_SSHORT:
				stmt->setSmallInt(tdbb, i->number, *(SSHORT*) i->address);
				break;

			case TYPE_SLONG:
				stmt->setInt(tdbb, i->number, *(SLONG*) i->address);
				break;

			case TYPE_SINT64:
				stmt->setBigInt(tdbb, i->number, *(SINT64*) i->address);
				break;

			case TYPE_DOUBLE:
				stmt->setDouble(tdbb, i->number, *(double*) i->address);
				break;

			case TYPE_STRING:
				stmt->setString(tdbb, i->number, *(AbstractString*) i->address);
				break;

			case TYPE_METANAME:
				stmt->setMetaName(tdbb, i->number, *(MetaName*) i->address);
				break;

			default:
				fb_assert(false);
		}
	}
}


PreparedStatement::PreparedStatement(thread_db* tdbb, MemoryPool& pool,
			Attachment* attachment, jrd_tra* transaction, const string& text,
			bool isInternalRequest)
	: PermanentStorage(pool),
	  builder(NULL),
	  inValues(pool),
	  outValues(pool),
	  inMetadata(new Firebird::MsgMetadata),
	  outMetadata(new Firebird::MsgMetadata),
	  inMessage(pool),
	  outMessage(pool),
	  resultSet(NULL)
{
	init(tdbb, attachment, transaction, text, isInternalRequest);
}


PreparedStatement::PreparedStatement(thread_db* tdbb, MemoryPool& pool,
			Attachment* attachment, jrd_tra* transaction, const Builder& aBuilder,
			bool isInternalRequest)
	: PermanentStorage(pool),
	  builder(&aBuilder),
	  inValues(pool),
	  outValues(pool),
	  inMetadata(new Firebird::MsgMetadata),
	  outMetadata(new Firebird::MsgMetadata),
	  inMessage(pool),
	  outMessage(pool),
	  resultSet(NULL)
{
	init(tdbb, attachment, transaction, builder->getText(), isInternalRequest);
}


PreparedStatement::~PreparedStatement()
{
	thread_db* tdbb = JRD_get_thread_data();

	DSQL_free_statement(tdbb, request, DSQL_drop);

	if (resultSet)
		resultSet->stmt = NULL;
}


void PreparedStatement::init(thread_db* tdbb, Attachment* attachment, jrd_tra* transaction,
	const Firebird::string& text, bool isInternalRequest)
{
	AutoSetRestore<SSHORT> autoAttCharset(&attachment->att_charset,
		(isInternalRequest ? CS_METADATA : attachment->att_charset));

	request = NULL;
	try
	{
		const Database& dbb = *tdbb->getDatabase();
		const int dialect = isInternalRequest || (dbb.dbb_flags & DBB_DB_SQL_dialect_3) ?
			SQL_DIALECT_V6 : SQL_DIALECT_V5;

		request = DSQL_prepare(tdbb, attachment, transaction, text.length(), text.c_str(), dialect,
			NULL, NULL, isInternalRequest);

		const DsqlCompiledStatement* statement = request->getStatement();

		if (statement->getSendMsg())
			parseDsqlMessage(statement->getSendMsg(), inValues, inMetadata, inMessage);

		if (statement->getReceiveMsg())
			parseDsqlMessage(statement->getReceiveMsg(), outValues, outMetadata, outMessage);
	}
	catch (const Exception&)
	{
		if (request)
		{
			DSQL_free_statement(tdbb, request, DSQL_drop);
		}
		throw;
	}
}


void PreparedStatement::setDesc(thread_db* tdbb, unsigned param, const dsc& value)
{
	fb_assert(param > 0);

	jrd_req* jrdRequest = getRequest()->req_request;

	// Setup tdbb info necessary for blobs.
	AutoSetRestore2<jrd_req*, thread_db> autoRequest(
		tdbb, &thread_db::getRequest, &thread_db::setRequest, jrdRequest);
	AutoSetRestore<jrd_tra*> autoRequestTrans(&jrdRequest->req_transaction,
		tdbb->getTransaction());

	MOV_move(tdbb, const_cast<dsc*>(&value), &inValues[(param - 1) * 2]);

	const dsc* desc = &inValues[(param - 1) * 2 + 1];
	fb_assert(desc->dsc_dtype == dtype_short);
	*reinterpret_cast<SSHORT*>(desc->dsc_address) = 0;
}


void PreparedStatement::execute(thread_db* tdbb, jrd_tra* transaction)
{
	fb_assert(resultSet == NULL);

	if (builder)
		builder->moveToStatement(tdbb, this);

	DSQL_execute(tdbb, &transaction, request, false, inMetadata, inMessage.begin(), NULL, NULL);
}


void PreparedStatement::open(thread_db* tdbb, jrd_tra* transaction)
{
	fb_assert(resultSet == NULL);

	if (builder)
		builder->moveToStatement(tdbb, this);

	DSQL_execute(tdbb, &transaction, request, true, inMetadata, inMessage.begin(), outMetadata, NULL);
}


ResultSet* PreparedStatement::executeQuery(thread_db* tdbb, jrd_tra* transaction)
{
	fb_assert(resultSet == NULL && request->getStatement()->getReceiveMsg());

	if (builder)
		builder->moveToStatement(tdbb, this);

	return FB_NEW(getPool()) ResultSet(tdbb, this, transaction);
}


unsigned PreparedStatement::executeUpdate(thread_db* tdbb, jrd_tra* transaction)
{
	execute(tdbb, transaction);
	return getRequest()->req_request->req_records_updated;
}


int PreparedStatement::getResultCount() const
{
	return outValues.getCount() / 2;
}


void PreparedStatement::parseDsqlMessage(const dsql_msg* dsqlMsg, Array<dsc>& values,
	MsgMetadata* msgMetadata, UCharBuffer& msg)
{
	// hvlad: Parameters in dsqlMsg->msg_parameters almost always linked in descending
	// order by par_index. The only known exception is EXECUTE BLOCK statement.
	// To generate correct metadata we must walk params in ascending par_index order.
	// So store all params in array in an ascending par_index order despite of
	// order in linked list.
	// ASF: Input parameters don't come necessarily in ascending or descending order,
	// so I changed the code to use a SortedArray.
	// AP: removed assertions for correct parameters order - useless with SortedArray.

	SortedArray<const dsql_par*, InlineStorage<const dsql_par*, 16>, const dsql_par*,
		DefaultKeyValue<const dsql_par*>, ParamCmp> params;

	for (size_t i = 0; i < dsqlMsg->msg_parameters.getCount(); ++i)
	{
		dsql_par* par = dsqlMsg->msg_parameters[i];
		if (par->par_index)
			params.add(par);
	}

	size_t paramCount = params.getCount();
	values.resize(paramCount * 2);
	msgMetadata->items.resize(paramCount);

	for (size_t i = 0; i < paramCount; ++i)
		dscToMetaItem(&params[i]->par_desc, msgMetadata->items[i]);

	msgMetadata->makeOffsets();
	msg.resize(msgMetadata->length);

	dsc* value = values.begin();

	for (size_t i = 0; i < paramCount; ++i)
	{
		// value
		*value = params[i]->par_desc;
		value->dsc_address = msg.begin() + msgMetadata->items[i].offset;
		++value;

		// NULL indicator
		value->makeShort(0);
		value->dsc_address = msg.begin() + msgMetadata->items[i].nullInd;
		// set NULL indicator value
		*((SSHORT*) value->dsc_address) = -1;
		++value;
	}
}


}	// namespace
