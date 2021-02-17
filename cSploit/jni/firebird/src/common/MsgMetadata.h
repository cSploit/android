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

#ifndef COMMON_MSG_METADATA_H
#define COMMON_MSG_METADATA_H

#include "firebird/Provider.h"
#include "iberror.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/dsc.h"

namespace Firebird {

class MsgMetadata : public RefCntIface<IMessageMetadata, FB_MESSAGE_METADATA_VERSION>
{
public:
	struct Item
	{
		explicit Item(MemoryPool& pool)
			: field(pool),
			  relation(pool),
			  owner(pool),
			  alias(pool),
			  type(0),
			  subType(0),
			  length(0),
			  scale(0),
			  charSet(0),
			  offset(0),
			  nullInd(0),
			  nullable(false),
			  finished(false)
		{
		}

		Item(MemoryPool& pool, const Item& v)
			: field(pool, v.field),
			  relation(pool, v.relation),
			  owner(pool, v.owner),
			  alias(pool, v.alias),
			  type(v.type),
			  subType(v.subType),
			  length(v.length),
			  scale(v.scale),
			  charSet(v.charSet),
			  offset(v.offset),
			  nullInd(v.nullInd),
			  nullable(v.nullable),
			  finished(v.finished)
		{
		}

		string field;
		string relation;
		string owner;
		string alias;
		unsigned type;
		int subType;
		unsigned length;
		int scale;
		unsigned charSet;
		unsigned offset;
		unsigned nullInd;
		bool nullable;
		bool finished;
	};

public:
	MsgMetadata()
		: items(getPool()),
		  length(0)
	{
	}

	explicit MsgMetadata(IMessageMetadata* from)
		: items(getPool()),
		  length(0)
	{
		assign(from);
	}

	virtual int FB_CARG release();

	virtual unsigned FB_CARG getCount(IStatus* /*status*/)
	{
		return (unsigned) items.getCount();
	}

	virtual const char* FB_CARG getField(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].field.c_str();

		raiseIndexError(status, index, "getField");
		return NULL;
	}

	virtual const char* FB_CARG getRelation(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].relation.c_str();

		raiseIndexError(status, index, "getRelation");
		return NULL;
	}

	virtual const char* FB_CARG getOwner(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].owner.c_str();

		raiseIndexError(status, index, "getOwner");
		return NULL;
	}

	virtual const char* FB_CARG getAlias(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].alias.c_str();

		raiseIndexError(status, index, "getAlias");
		return NULL;
	}

	virtual unsigned FB_CARG getType(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].type;

		raiseIndexError(status, index, "getType");
		return 0;
	}

	virtual FB_BOOLEAN FB_CARG isNullable(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].nullable;

		raiseIndexError(status, index, "isNullable");
		return false;
	}

	virtual int FB_CARG getSubType(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].subType;

		raiseIndexError(status, index, "getSubType");
		return 0;
	}

	virtual unsigned FB_CARG getLength(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].length;

		raiseIndexError(status, index, "getLength");
		return 0;
	}

	virtual int FB_CARG getScale(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].scale;

		raiseIndexError(status, index, "getScale");
		return 0;
	}

	virtual unsigned FB_CARG getCharSet(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].charSet;

		raiseIndexError(status, index, "getCharSet");
		return 0;
	}

	virtual unsigned FB_CARG getOffset(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].offset;

		raiseIndexError(status, index, "getOffset");
		return 0;
	}

	virtual unsigned FB_CARG getNullOffset(IStatus* status, unsigned index)
	{
		if (index < items.getCount())
			return items[index].nullInd;

		raiseIndexError(status, index, "getOffset");
		return 0;
	}

	virtual IMetadataBuilder* FB_CARG getBuilder(IStatus* status);

	virtual unsigned FB_CARG getMessageLength(IStatus* /*status*/)
	{
		return length;
	}

public:
	void addItem(const MetaName& name, bool nullable, const dsc& desc);
	unsigned makeOffsets();

private:
	void raiseIndexError(IStatus* status, unsigned index, const char* method) const
	{
		status->set((Arg::Gds(isc_invalid_index_val) <<
			Arg::Num(index) << (string("IMessageMetadata::") + method)).value());
	}

	void assign(IMessageMetadata* from);

public:
	ObjectsArray<Item> items;
	unsigned length;
};

class AttMetadata : public MsgMetadata
{
public:
	explicit AttMetadata(IAttachment* att)
		: MsgMetadata(),
		  attachment(att)
	{ }

	// re-implement here release() present in MsgMetadata to call correct dtor
	virtual int FB_CARG release();

	RefPtr<IAttachment> attachment;
};

class MetadataBuilder FB_FINAL : public RefCntIface<IMetadataBuilder, FB_METADATA_BUILDER_VERSION>
{
public:
	explicit MetadataBuilder(const MsgMetadata* from);
	MetadataBuilder(unsigned fieldCount);

	virtual int FB_CARG release();

	// IMetadataBuilder implementation
	virtual void FB_CARG setType(IStatus* status, unsigned index, unsigned type);
	virtual void FB_CARG setSubType(IStatus* status, unsigned index, int subType);
	virtual void FB_CARG setLength(IStatus* status, unsigned index, unsigned length);
	virtual void FB_CARG setCharSet(IStatus* status, unsigned index, unsigned charSet);
	virtual void FB_CARG setScale(IStatus* status, unsigned index, unsigned scale);
	virtual void FB_CARG truncate(IStatus* status, unsigned count);
	virtual void FB_CARG remove(IStatus* status, unsigned index);
	virtual void FB_CARG moveNameToIndex(IStatus* status, const char* name, unsigned index);
	virtual unsigned FB_CARG addField(IStatus* status);
	virtual IMessageMetadata* FB_CARG getMetadata(IStatus* status);

private:
	RefPtr<MsgMetadata> msgMetadata;
	Mutex mtx;

	void metadataError(const char* functionName);
	void indexError(unsigned index, const char* functionName);
};

}	// namespace Firebird

#endif	// COMMON_MSG_METADATA_H
