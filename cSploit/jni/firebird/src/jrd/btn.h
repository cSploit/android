/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		btn.h
 *	DESCRIPTION:	B-tree management code
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
 *  The Original Code was created by Arno Brinkman
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2003 Arno Brinkman and all contributors
 *  signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_BTN_H
#define JRD_BTN_H

#include "firebird.h"			// needed for get_long
#include "memory_routines.h"	// needed for get_long

#include "../jrd/ods.h"
#include "../common/classes/array.h"

namespace Jrd {

// Flags (3-bits) used for index node
const int BTN_NORMAL_FLAG					= 0;
const int BTN_END_LEVEL_FLAG				= 1;
const int BTN_END_BUCKET_FLAG				= 2;
const int BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG	= 3;
const int BTN_ZERO_LENGTH_FLAG				= 4;
const int BTN_ONE_LENGTH_FLAG				= 5;
//const int BTN_ZERO_PREFIX_ONE_LENGTH_FLAG	= 6;
//const int BTN_GET_MORE_FLAGS	= 7;

// Firebird B-tree nodes
const USHORT BTN_LEAF_SIZE	= 6;
const USHORT BTN_PAGE_SIZE	= 10;

struct IndexNode
{
	UCHAR* nodePointer;			// pointer to where this node can be read from the page
	USHORT prefix;				// size of compressed prefix
	USHORT length;				// length of data in node
	ULONG pageNumber;			// page number
	UCHAR* data;				// Data can be read from here
	RecordNumber recordNumber;	// record number
	bool isEndBucket;
	bool isEndLevel;

	static USHORT computePrefix(const UCHAR* prevString, USHORT prevLength,
								const UCHAR* string, USHORT length);

	static SLONG findPageInDuplicates(const Ods::btree_page* page, UCHAR* pointer,
									  SLONG previousNumber, RecordNumber findRecordNumber);

	bool keyEqual(USHORT length, const UCHAR* data) const
	{
		if (length != this->length + this->prefix)
			return false;

		if (!this->length)
			return true;

		return !memcmp(this->data, data + this->prefix, this->length);
	}

	void setEndBucket()
	{
		this->isEndBucket = true;
		this->isEndLevel = false;
	}

	void setEndLevel()
	{
		this->isEndBucket = false;
		this->isEndLevel = true;
		this->prefix = 0;
		this->length = 0;
		this->pageNumber = 0;
		this->recordNumber.setValue(0);
	}

	void setNode(USHORT prefix = 0, USHORT length = 0,
				 RecordNumber recordNumber = RecordNumber(0), SLONG pageNumber = 0,
				 bool isEndBucket = false, bool isEndLevel = false)
	{
		this->isEndBucket = isEndBucket;
		this->isEndLevel = isEndLevel;
		this->prefix = prefix;
		this->length = length;
		this->recordNumber = recordNumber;
		this->pageNumber = pageNumber;
	}

	USHORT getNodeSize(bool leafNode) const;
	UCHAR* writeNode(UCHAR* pagePointer, bool leafNode, bool withData = true);

	UCHAR* readNode(UCHAR* pagePointer, bool leafNode)
	{
	/**************************************
	 *
	 *	r e a d N o d e
	 *
	 **************************************
	 *
	 * Functional description
	 *	Read a leaf/page node from the page by the
	 *  given pagePointer and the return the
	 *  remaining position after the read.
	 *
	 **************************************/
		nodePointer = pagePointer;

		// Get first byte that contains internal flags and 6 bits from number
		UCHAR* localPointer = pagePointer;
		UCHAR internalFlags = *localPointer++;
		SINT64 number = (internalFlags & 0x1F);
		internalFlags = ((internalFlags & 0xE0) >> 5);

		isEndLevel = (internalFlags == BTN_END_LEVEL_FLAG);
		isEndBucket = (internalFlags == BTN_END_BUCKET_FLAG);

		// If this is a END_LEVEL marker then we're done
		if (isEndLevel)
		{
			prefix = 0;
			length = 0;
			recordNumber.setValue(0);
			pageNumber = 0;
			return localPointer;
		}

		// Get remaining bits for number
		ULONG tmp = *localPointer++;
		number |= (tmp & 0x7F) << 5;
		if (tmp >= 128)
		{
			tmp = *localPointer++;
			number |= (tmp & 0x7F) << 12;
			if (tmp >= 128)
			{
				tmp = *localPointer++;
				number |= (tmp & 0x7F) << 19;
				if (tmp >= 128)
				{
					tmp = *localPointer++;
					number |= (FB_UINT64) (tmp & 0x7F) << 26;
					if (tmp >= 128)
					{
						tmp = *localPointer++;
						number |= (FB_UINT64) (tmp & 0x7F) << 33;
	/*
		Uncomment this if you need more bits in record number
							if (tmp >= 128)
							{
								tmp = *localPointer++;
								number |= (FB_UINT64) (tmp & 0x7F) << 40;
								if (tmp >= 128)
								{
									tmp = *localPointer++;
									number |= (FB_UINT64) (tmp & 0x7F) << 47;
									if (tmp >= 128)
									{
										tmp = *localPointer++;
										number |= (FB_UINT64) (tmp & 0x7F) << 54; // We get 61 bits at this point!
									}
								}
							}
	*/
					}
				}
			}
		}
		recordNumber.setValue(number);

		if (!leafNode)
		{
			// Get page number for non-leaf pages
			tmp = *localPointer++;
			number = (tmp & 0x7F);
			if (tmp >= 128)
			{
				tmp = *localPointer++;
				number |= (tmp & 0x7F) << 7;
				if (tmp >= 128)
				{
					tmp = *localPointer++;
					number |= (tmp & 0x7F) << 14;
					if (tmp >= 128)
					{
						tmp = *localPointer++;
						number |= (tmp & 0x7F) << 21;
						if (tmp >= 128)
						{
							tmp = *localPointer++;
							number |= (tmp & 0x0F) << 28;
						}
					}
				}
			}
			pageNumber = number;
		}

		if (internalFlags == BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG)
		{
			// Prefix is zero
			prefix = 0;
		}
		else
		{
			// Get prefix
			tmp = *localPointer++;
			prefix = (tmp & 0x7F);
			if (tmp & 0x80)
			{
				tmp = *localPointer++;
				prefix |= (tmp & 0x7F) << 7; // We get 14 bits at this point
			}
		}

		if ((internalFlags == BTN_ZERO_LENGTH_FLAG) ||
			(internalFlags == BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG))
		{
			// Length is zero
			length = 0;
		}
		else if (internalFlags == BTN_ONE_LENGTH_FLAG)
		{
			// Length is one
			length = 1;
		}
		else
		{
			// Get length
			tmp = *localPointer++;
			length = (tmp & 0x7F);
			if (tmp & 0x80)
			{
				tmp = *localPointer++;
				length |= (tmp & 0x7F) << 7; // We get 14 bits at this point
			}
		}

		// Get pointer where data starts
		data = localPointer;
		localPointer += length;

		return localPointer;
	}

};

struct IndexJumpNode
{
	UCHAR* nodePointer;	// pointer to where this node can be read from the page
	USHORT prefix;		// length of prefix against previous jump node
	USHORT length;		// length of data in jump node (together with prefix this is prefix for pointing node)
	USHORT offset;		// offset to node in page
	UCHAR* data;		// Data can be read from here

	USHORT getJumpNodeSize() const;
	UCHAR* readJumpNode(UCHAR* pagePointer);
	UCHAR* writeJumpNode(UCHAR* pagePointer);
};

struct dynKey
{
	USHORT keyLength;
	UCHAR* keyData;
};

typedef Firebird::HalfStaticArray<dynKey*, 32> keyList;
typedef Firebird::HalfStaticArray<IndexJumpNode, 32> jumpNodeList;

} // namespace Jrd

#endif // JRD_BTN_H
