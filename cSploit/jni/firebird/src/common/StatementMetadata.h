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
 *		Alex Peshkov
 *
 */

#ifndef COMMON_STATEMENT_METADATA_H
#define COMMON_STATEMENT_METADATA_H

#include "firebird/Provider.h"
#include "iberror.h"
#include "../common/classes/Nullable.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/objects_array.h"
#include "../common/MsgMetadata.h"

namespace Firebird {


// Make new metadata support work together with old info-based buffers.
class StatementMetadata : public PermanentStorage
{
public:
	class Parameters : public AttMetadata
	{
	public:
		explicit Parameters(IAttachment* att)
			: AttMetadata(att),
			  fetched(false)
		{
		}

		bool fetched;
	};

	StatementMetadata(MemoryPool& pool, IStatement* aStatement, IAttachment* att)
		: PermanentStorage(pool),
		  statement(aStatement),
		  legacyPlan(pool),
		  detailedPlan(pool),
		  inputParameters(new Parameters(att)),
		  outputParameters(new Parameters(att))
	{
	}

	static unsigned buildInfoItems(Array<UCHAR>& items, unsigned flags);
	static unsigned buildInfoFlags(unsigned itemsLength, const UCHAR* items);

	unsigned getType();
	unsigned getFlags();
	const char* getPlan(bool detailed);
	IMessageMetadata* getInputMetadata();
	IMessageMetadata* getOutputMetadata();
	ISC_UINT64 getAffectedRecords();

	void clear();
	void parse(unsigned bufferLength, const UCHAR* buffer);
	void getAndParse(unsigned itemsLength, const UCHAR* items, unsigned bufferLength, UCHAR* buffer);
	bool fillFromCache(unsigned itemsLength, const UCHAR* items, unsigned bufferLength, UCHAR* buffer);

private:
	void fetchParameters(UCHAR code, Parameters* parameters);

private:
	IStatement* statement;
	Nullable<unsigned> type, flags;
	string legacyPlan, detailedPlan;
	RefPtr<Parameters> inputParameters, outputParameters;
};


}	// namespace Firebird

#endif	// COMMON_STATEMENT_METADATA_H
