/*
 *	PROGRAM:	Server Code
 *	MODULE:		rpb_chain.h
 *	DESCRIPTION:	Keeps track of record_param's, updated_in_place by
 *	        		single transcation
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef JRD_RPB_CHAIN_H
#define JRD_RPB_CHAIN_H

#include "../jrd/gdsassert.h"
#include <string.h>

#include "../common/classes/array.h"
#include "../jrd/jrd.h"
#include "../jrd/req.h"

// req.h defines struct record_param

namespace Jrd {

class traRpbListElement
{
public:
	record_param* lr_rpb;
	int level;
	traRpbListElement(record_param* r, USHORT l)
		: lr_rpb(r), level(l)
	{}
	traRpbListElement() {}

	static inline bool greaterThan(const traRpbListElement& i1, const traRpbListElement& i2)
	{
		return i1.lr_rpb->rpb_relation->rel_id != i2.lr_rpb->rpb_relation->rel_id ?
			   i1.lr_rpb->rpb_relation->rel_id > i2.lr_rpb->rpb_relation->rel_id :
			   i1.lr_rpb->rpb_number != i2.lr_rpb->rpb_number ?
			   i1.lr_rpb->rpb_number > i2.lr_rpb->rpb_number :
			   i1.level > i2.level;
	}

	static inline const traRpbListElement& generate(const void* /*sender*/, const traRpbListElement& item)
	{
		return item;
	}
};


typedef Firebird::SortedArray<traRpbListElement,
	Firebird::InlineStorage<traRpbListElement, 16>, traRpbListElement,
	traRpbListElement, traRpbListElement> traRpbArray;
class traRpbList : public traRpbArray
{
public:
	explicit traRpbList(Firebird::MemoryPool& p)
		: traRpbArray(p)
	{}
	int PushRpb(record_param* value);
	bool PopRpb(record_param* value, int Level);
};

} //namespace Jrd

#endif	//JRD_RPB_CHAIN_H

