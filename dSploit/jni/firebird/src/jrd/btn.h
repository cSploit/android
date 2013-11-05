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

#include "../jrd/common.h"
#include "../jrd/ods.h"
#include "../common/classes/array.h"

// format of expanded index node, used for backwards navigation
struct btree_exp
{
	UCHAR btx_previous_length;		// AB: total size for previous node --length of data for previous node
	UCHAR btx_btr_previous_length;	// length of data for previous node on btree page
	UCHAR btx_data[1];				// expanded data element
};

const int BTX_SIZE				= 2;

// Flags (3-bits) used for index node
const int BTN_NORMAL_FLAG		= 0;
const int BTN_END_LEVEL_FLAG	= 1;
const int BTN_END_BUCKET_FLAG	= 2;
const int BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG	= 3;
const int BTN_ZERO_LENGTH_FLAG	= 4;
const int BTN_ONE_LENGTH_FLAG	= 5;
//const int BTN_ZERO_PREFIX_ONE_LENGTH_FLAG	= 6;
//const int BTN_GET_MORE_FLAGS	= 7;

// format of expanded index buffer
struct exp_index_buf
{
	USHORT exp_length;
	ULONG exp_incarnation;
	btree_exp exp_nodes[1];
};

const size_t EXP_SIZE	= OFFSETA (exp_index_buf*, exp_nodes);

struct dynKey
{
	USHORT keyLength;
	UCHAR* keyData;
};

typedef Firebird::Array<dynKey*> keyList;
typedef Firebird::HalfStaticArray<Ods::IndexJumpNode, 32> jumpNodeList;

namespace BTreeNode {

	USHORT computePrefix(const UCHAR* prevString, USHORT prevLength,
				const UCHAR* string, USHORT length);

	SLONG findPageInDuplicates(const Ods::btree_page* page, UCHAR* pointer,
				SLONG previousNumber, RecordNumber findRecordNumber);

	USHORT getJumpNodeSize(const Ods::IndexJumpNode* jumpNode, UCHAR flags);
	USHORT getNodeSize(const Ods::IndexNode* indexNode, UCHAR flags, bool leafNode = true);
	UCHAR* getPointerFirstNode(Ods::btree_page* page, Ods::IndexJumpInfo* jumpInfo = NULL);

	bool keyEquality(USHORT length, const UCHAR* data, const Ods::IndexNode* indexNode);

#ifdef SCROLLABLE_CURSORS
	UCHAR* lastNode(Ods::btree_page* page, exp_index_buf* expanded_page, btree_exp** expanded_node);
#endif

	UCHAR* nextNode(Ods::IndexNode* node, UCHAR* pointer,
				UCHAR flags, btree_exp** expanded_node);
#ifdef SCROLLABLE_CURSORS
	UCHAR* previousNode(/*Ods::IndexNode* node,*/ UCHAR* pointer,
				/*UCHAR flags,*/ btree_exp** expanded_node);
#endif

	//void quad_put(SLONG value, UCHAR *data);

	UCHAR* readJumpInfo(Ods::IndexJumpInfo* jumpInfo, UCHAR* pagePointer);
	UCHAR* readJumpNode(Ods::IndexJumpNode* jumpNode, UCHAR* pagePointer, UCHAR flags);
	inline UCHAR* readNode(Ods::IndexNode* indexNode, UCHAR* pagePointer, UCHAR flags,
		bool leafNode);

	UCHAR* writeJumpInfo(Ods::btree_page* page, const Ods::IndexJumpInfo* jumpInfo);
	UCHAR* writeJumpNode(Ods::IndexJumpNode* jumpNode, UCHAR* pagePointer, UCHAR flags);
	UCHAR* writeNode(Ods::IndexNode* indexNode, UCHAR* pagePointer, UCHAR flags,
		bool leafNode, bool withData = true);

	void setEndBucket(Ods::IndexNode* indexNode); //, bool leafNode = true);
	void setEndLevel(Ods::IndexNode* indexNode); //, bool leafNode = true);
	void setNode(Ods::IndexNode* indexNode, USHORT prefix = 0, USHORT length = 0,
		RecordNumber recordNumber = RecordNumber(0), SLONG pageNumber = 0,
		bool isEndBucket = false, bool isEndLevel = false);

} // namespace BTreeNode

inline UCHAR* BTreeNode::readNode(Ods::IndexNode* indexNode, UCHAR* pagePointer, UCHAR flags, bool leafNode)
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
	indexNode->nodePointer = pagePointer;
	if (flags & Ods::btr_large_keys)
	{

		// Get first byte that contains internal flags and 6 bits from number
		UCHAR* localPointer = pagePointer;
		UCHAR internalFlags = *localPointer++;
		SINT64 number = (internalFlags & 0x1F);
		internalFlags = ((internalFlags & 0xE0) >> 5);

		indexNode->isEndLevel = (internalFlags == BTN_END_LEVEL_FLAG);
		indexNode->isEndBucket = (internalFlags == BTN_END_BUCKET_FLAG);

		// If this is a END_LEVEL marker then we're done
		if (indexNode->isEndLevel)
		{
			indexNode->prefix = 0;
			indexNode->length = 0;
			indexNode->recordNumber.setValue(0);
			indexNode->pageNumber = 0;
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
		indexNode->recordNumber.setValue(number);

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
/*
	Change number to 64-bit type and enable this for 64-bit support

							number |= (*tmp & 0x7F) << 28;
							if (tmp >= 128)
							{
								tmp = *localPointer++;
								number |= (*tmp & 0x7F) << 35;
								if (tmp >= 128)
								{
									tmp = *localPointer++;
									number |= (*tmp & 0x7F) << 42;
									if (tmp >= 128)
									{
										tmp = *localPointer++;
										number |= (*tmp & 0x7F) << 49;
										if (tmp >= 128)
										{
											tmp = *localPointer++;
											number |= (*tmp & 0x7F) << 56; // We get 63 bits at this point!
										}
									}
								}
							}
*/
						}
					}
				}
			}
			indexNode->pageNumber = number;
		}

		if (internalFlags == BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG)
		{
			// Prefix is zero
			indexNode->prefix = 0;
		}
		else
		{
			// Get prefix
			tmp = *localPointer++;
			indexNode->prefix = (tmp & 0x7F);
			if (tmp & 0x80)
			{
				tmp = *localPointer++;
				indexNode->prefix |= (tmp & 0x7F) << 7; // We get 14 bits at this point
			}
		}

		if ((internalFlags == BTN_ZERO_LENGTH_FLAG) ||
			(internalFlags == BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG))
		{
			// Length is zero
			indexNode->length = 0;
		}
		else if (internalFlags == BTN_ONE_LENGTH_FLAG)
		{
			// Length is one
			indexNode->length = 1;
		}
		else
		{
			// Get length
			tmp = *localPointer++;
			indexNode->length = (tmp & 0x7F);
			if (tmp & 0x80)
			{
				tmp = *localPointer++;
				indexNode->length |= (tmp & 0x7F) << 7; // We get 14 bits at this point
			}
		}

		// Get pointer where data starts
		indexNode->data = localPointer;
		localPointer += indexNode->length;

		return localPointer;
	}
	else
	{
		indexNode->prefix = *pagePointer++;
		indexNode->length = *pagePointer++;
		if (leafNode)
		{
			// Nice sign extension should happen here
			indexNode->recordNumber.setValue(get_long(pagePointer));
			indexNode->isEndLevel = (indexNode->recordNumber.getValue() == Ods::END_LEVEL);
			indexNode->isEndBucket = (indexNode->recordNumber.getValue() == Ods::END_BUCKET);
		}
		else
		{
			indexNode->pageNumber = get_long(pagePointer);
			indexNode->isEndLevel = (indexNode->pageNumber == Ods::END_LEVEL);
			indexNode->isEndBucket = (indexNode->pageNumber == Ods::END_BUCKET);
		}
		pagePointer += sizeof(SLONG);

		indexNode->data = pagePointer;
		pagePointer += indexNode->length;

		// Get recordnumber for non-leaf-nodes and on leaf-nodes when
		// last node is END_BUCKET and duplicate (or NULL).
		if ((flags & Ods::btr_all_record_number) &&
			((!leafNode) ||
			 (leafNode && indexNode->isEndBucket && (indexNode->length == 0))))
		{
			indexNode->recordNumber.setValue(get_long(pagePointer));
			pagePointer += sizeof(SLONG);
		}
	}
	return pagePointer;
}

#endif // JRD_BTN_H
