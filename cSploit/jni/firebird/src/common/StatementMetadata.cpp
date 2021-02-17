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
 *  Copyright (c) 2011 Adriano dos Santos Fernandes <adrianosf at gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *		Alex Peshkoff
 *
 */

#include "firebird.h"
#include "../common/StatementMetadata.h"
#include "memory_routines.h"
#include "../common/StatusHolder.h"
#include "../jrd/inf_pub.h"
#include "../yvalve/gds_proto.h"
#include "../common/utils_proto.h"

namespace Firebird {


static const UCHAR DESCRIBE_VARS[] =
{
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
};

static const unsigned INFO_BUFFER_SIZE = MAX_USHORT;


static int getNumericInfo(const UCHAR** ptr);
static void getStringInfo(const UCHAR** ptr, string* str);


// Build a list of info codes based on a prepare flags bitmask.
// Return "estimated" necessary size for the result buffer.
unsigned StatementMetadata::buildInfoItems(Array<UCHAR>& items, unsigned flags)
{
	items.clear();

	if (flags & IStatement::PREPARE_PREFETCH_TYPE)
		items.add(isc_info_sql_stmt_type);

	if (flags & IStatement::PREPARE_PREFETCH_FLAGS)
		items.add(isc_info_sql_stmt_flags);

	if (flags & IStatement::PREPARE_PREFETCH_INPUT_PARAMETERS)
	{
		items.add(isc_info_sql_bind);
		items.push(DESCRIBE_VARS, sizeof(DESCRIBE_VARS));
	}

	if (flags & IStatement::PREPARE_PREFETCH_OUTPUT_PARAMETERS)
	{
		items.add(isc_info_sql_select);
		items.push(DESCRIBE_VARS, sizeof(DESCRIBE_VARS));
	}

	if (flags & IStatement::PREPARE_PREFETCH_LEGACY_PLAN)
		items.add(isc_info_sql_get_plan);

	if (flags & IStatement::PREPARE_PREFETCH_DETAILED_PLAN)
		items.add(isc_info_sql_explain_plan);

	return INFO_BUFFER_SIZE;
}

// Build a prepare flags bitmask based on a list of info codes.
unsigned StatementMetadata::buildInfoFlags(unsigned itemsLength, const UCHAR* items)
{
	unsigned flags = 0;
	const UCHAR* end = items + itemsLength;
	UCHAR c;

	while (items < end && (c = *items++) != isc_info_end)
	{
		switch (c)
		{
			case isc_info_sql_stmt_type:
				flags |= IStatement::PREPARE_PREFETCH_TYPE;
				break;

			case isc_info_sql_stmt_flags:
				flags |= IStatement::PREPARE_PREFETCH_FLAGS;
				break;

			case isc_info_sql_get_plan:
				flags |= IStatement::PREPARE_PREFETCH_LEGACY_PLAN;
				break;

			case isc_info_sql_explain_plan:
				flags |= IStatement::PREPARE_PREFETCH_DETAILED_PLAN;
				break;

			case isc_info_sql_select:
				flags |= IStatement::PREPARE_PREFETCH_OUTPUT_PARAMETERS;
				break;

			case isc_info_sql_bind:
				flags |= IStatement::PREPARE_PREFETCH_INPUT_PARAMETERS;
				break;
		}
	}

	return flags;
}

// Get statement type.
unsigned StatementMetadata::getType()
{
	if (!type.specified)
	{
		UCHAR info[] = {isc_info_sql_stmt_type};
		UCHAR result[16];

		getAndParse(sizeof(info), info, sizeof(result), result);

		fb_assert(type.specified);
	}

	return type.value;
}

unsigned StatementMetadata::getFlags()
{
	if (!flags.specified)
	{
		UCHAR info[] = {isc_info_sql_stmt_flags};
		UCHAR result[16];

		getAndParse(sizeof(info), info, sizeof(result), result);

		fb_assert(flags.specified);
	}

	return flags.value;
}

// Get statement plan.
const char* StatementMetadata::getPlan(bool detailed)
{
	string* plan = detailed ? &detailedPlan : &legacyPlan;

	if (plan->isEmpty())
	{
		UCHAR info[] = {detailed ? isc_info_sql_explain_plan : isc_info_sql_get_plan};
		UCHAR result[INFO_BUFFER_SIZE];

		getAndParse(sizeof(info), info, sizeof(result), result);
	}

	return plan->nullStr();
}

// Get statement input parameters.
IMessageMetadata* StatementMetadata::getInputMetadata()
{
	if (!inputParameters->fetched)
		fetchParameters(isc_info_sql_bind, inputParameters);

	inputParameters->addRef();
	return inputParameters;
}

// Get statement output parameters.
IMessageMetadata* StatementMetadata::getOutputMetadata()
{
	if (!outputParameters->fetched)
		fetchParameters(isc_info_sql_select, outputParameters);

	outputParameters->addRef();
	return outputParameters;
}

// Get number of records affected by the statement execution.
ISC_UINT64 StatementMetadata::getAffectedRecords()
{
	UCHAR info[] = {isc_info_sql_records};
	UCHAR result[33];

	getAndParse(sizeof(info), info, sizeof(result), result);

	ISC_UINT64 count = 0;

	if (result[0] == isc_info_sql_records)
	{
		const UCHAR* p = result + 3;

		while (*p != isc_info_end)
		{
			UCHAR counter = *p++;
			const SSHORT len = gds__vax_integer(p, 2);
			p += 2;
			if (counter != isc_info_req_select_count)
				count += gds__vax_integer(p, len);
			p += len;
		}
	}

	return count;
}

// Reset the object to its initial state.
void StatementMetadata::clear()
{
	type.specified = false;
	legacyPlan = detailedPlan = "";
	inputParameters->items.clear();
	outputParameters->items.clear();
	inputParameters->fetched = outputParameters->fetched = false;
}

// Parse an info response buffer.
void StatementMetadata::parse(unsigned bufferLength, const UCHAR* buffer)
{
	const UCHAR* bufferEnd = buffer + bufferLength;
	Parameters* parameters = NULL;
	bool finish = false;
	UCHAR c;

	while (!finish && buffer < bufferEnd && (c = *buffer++) != isc_info_end)
	{
		switch (c)
		{
			case isc_info_sql_stmt_type:
				type = getNumericInfo(&buffer);
				break;

			case isc_info_sql_stmt_flags:
				flags = getNumericInfo(&buffer);
				break;

			case isc_info_sql_get_plan:
			case isc_info_sql_explain_plan:
			{
				string* plan = (c == isc_info_sql_explain_plan ? &detailedPlan : &legacyPlan);
				getStringInfo(&buffer, plan);
				break;
			}

			case isc_info_sql_select:
				parameters = outputParameters;
				break;

			case isc_info_sql_bind:
				parameters = inputParameters;
				break;

			case isc_info_sql_num_variables:
			case isc_info_sql_describe_vars:
			{
				if (!parameters)
				{
					finish = true;
					break;
				}

				getNumericInfo(&buffer);	// skip the message index

				if (c == isc_info_sql_num_variables)
					continue;

				parameters->fetched = true;

				Parameters::Item temp(*getDefaultMemoryPool());
				Parameters::Item* param = &temp;
				bool finishDescribe = false;

				// Loop over the variables being described.
				while (!finishDescribe)
				{
					switch ((c = *buffer++))
					{
						case isc_info_sql_describe_end:
							param->finished = true;
							break;

						case isc_info_sql_sqlda_seq:
						{
							unsigned num = getNumericInfo(&buffer);

							while (parameters->items.getCount() < num)
								parameters->items.add();

							param = &parameters->items[num - 1];
							break;
						}

						case isc_info_sql_type:
							param->type = getNumericInfo(&buffer);
							param->nullable = (param->type & 1) != 0;
							param->type &= ~1;
							break;

						case isc_info_sql_sub_type:
							param->subType = getNumericInfo(&buffer);
							break;

						case isc_info_sql_length:
							param->length = getNumericInfo(&buffer);
							break;

						case isc_info_sql_scale:
							param->scale = getNumericInfo(&buffer);
							break;

						case isc_info_sql_field:
							getStringInfo(&buffer, &param->field);
							break;

						case isc_info_sql_relation:
							getStringInfo(&buffer, &param->relation);
							break;

						case isc_info_sql_owner:
							getStringInfo(&buffer, &param->owner);
							break;

						case isc_info_sql_alias:
							getStringInfo(&buffer, &param->alias);
							break;

						case isc_info_truncated:
							parameters->fetched = false;
							// fall into

						default:
							--buffer;
							finishDescribe = true;
							break;
					}
				}

				if (parameters->fetched)
				{
					unsigned off = 0;

					for (unsigned n = 0; n < parameters->items.getCount(); ++n)
					{
						Parameters::Item* param = &parameters->items[n];

						if (!param->finished)
						{
							parameters->fetched = false;
							break;
						}

						off = fb_utils::sqlTypeToDsc(off, param->type, param->length,
							NULL /*dtype*/, NULL /*length*/, &param->offset, &param->nullInd);

						switch(param->type)
						{
						case SQL_VARYING:
						case SQL_TEXT:
							param->charSet = param->subType;
							param->subType = 0;
							break;
						case SQL_BLOB:
							param->charSet = param->scale;
							param->scale = 0;
							break;
						}
					}

					if (parameters->fetched)
						parameters->length = off;
				}

				break;
			}

			default:
				finish = true;
				break;
		}
	}

	// CVC: This routine assumes the input is well formed, hence at least check we didn't read
	// beyond the buffer's end, although I would prefer to make the previous code more robust.
	// fb_assert(buffer <= bufferEnd);
	// ASF: User may pass any (including unknown from new version) info code and we can't
	// understand them. In this case, we leave these code without parse (when they're in the end)
	// or we'll need extra info calls (done by methods of this class) to parse only the info we
	// can understand.

	for (ObjectsArray<Parameters::Item>::iterator i = inputParameters->items.begin();
		 i != inputParameters->items.end() && inputParameters->fetched;
		 ++i)
	{
		inputParameters->fetched = i->finished;
	}

	for (ObjectsArray<Parameters::Item>::iterator i = outputParameters->items.begin();
		 i != outputParameters->items.end() && outputParameters->fetched;
		 ++i)
	{
		outputParameters->fetched = i->finished;
	}
}

// Get a info buffer and parse it.
void StatementMetadata::getAndParse(unsigned itemsLength, const UCHAR* items,
	unsigned bufferLength, UCHAR* buffer)
{
	LocalStatus status;
	statement->getInfo(&status, itemsLength, items, bufferLength, buffer);
	status.check();

	parse(bufferLength, buffer);
}

// Fill an output buffer from the cached data. Return true if succeeded.
bool StatementMetadata::fillFromCache(unsigned itemsLength, const UCHAR* items,
	unsigned bufferLength, UCHAR* buffer)
{
	//// TODO: Respond more things locally. isc_dsql_prepare_m will need.

	if (((itemsLength == 1 && items[0] == isc_info_sql_stmt_type) ||
			(itemsLength == 2 && items[0] == isc_info_sql_stmt_type &&
				(items[1] == isc_info_end || items[1] == 0))) &&
		type.specified)
	{
		if (bufferLength >= 8)
		{
			*buffer++ = isc_info_sql_stmt_type;
			put_vax_short(buffer, 4);
			buffer += 2;
			put_vax_long(buffer, type.value);
			buffer += 4;
			*buffer = isc_info_end;
		}
		else
			*buffer = isc_info_truncated;

		return true;
	}

	return false;
}

// Fetch input or output parameter list.
void StatementMetadata::fetchParameters(UCHAR code, Parameters* parameters)
{
	while (!parameters->fetched)
	{
		unsigned startIndex = 0;

		for (ObjectsArray<Parameters::Item>::iterator i = parameters->items.begin();
			 i != parameters->items.end();
			 ++i)
		{
			if (!i->finished)
				break;

			++startIndex;
		}

		UCHAR items[5 + sizeof(DESCRIBE_VARS)] =
		{
			isc_info_sql_sqlda_start,
			2,
			(startIndex & 0xFF),
			((startIndex >> 8) & 0xFF),
			code
		};
		memcpy(items + 5, DESCRIBE_VARS, sizeof(DESCRIBE_VARS));

		UCHAR buffer[INFO_BUFFER_SIZE];
		getAndParse(sizeof(items), items, sizeof(buffer), buffer);
	}
}


//--------------------------------------


// Pick up a VAX format numeric info item with a 2 byte length.
static int getNumericInfo(const UCHAR** ptr)
{
	const SSHORT len = static_cast<SSHORT>(gds__vax_integer(*ptr, 2));
	*ptr += 2;
	int item = gds__vax_integer(*ptr, len);
	*ptr += len;
	return item;
}

// Pick up a string valued info item.
static void getStringInfo(const UCHAR** ptr, string* str)
{
	const UCHAR* p = *ptr;
	SSHORT len = static_cast<SSHORT>(gds__vax_integer(p, 2));

	// CVC: What else can we do here?
	if (len < 0)
		len = 0;

	*ptr += len + 2;
	p += 2;

	str->assign(p, len);
}


}	// namespace Firebird
