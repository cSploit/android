/*
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
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 * Adriano dos Santos Fernandes - refactored from pass1.cpp
 */

#ifndef DSQL_VISITORS_H
#define DSQL_VISITORS_H

#include "../dsql/dsql.h"
#include "../common/classes/array.h"
#include "../common/classes/auto.h"
#include "../common/classes/NestConst.h"

namespace Jrd {

class CompilerScratch;
class ExprNode;
class FieldNode;
class MapNode;
class MessageNode;


enum FieldMatchType
{
	FIELD_MATCH_TYPE_EQUAL = 0,
	FIELD_MATCH_TYPE_LOWER = 1,
	FIELD_MATCH_TYPE_LOWER_EQUAL = 2
	/***
	,
	FIELD_MATCH_TYPE_HIGHER = 3,
	FIELD_MATCH_TYPE_HIGHER_EQUAL = 4
	***/
};


// Check for an aggregate expression in an expression. It could be buried in an expression
// tree and therefore call itselfs again. The level parameters (currentLevel & deepestLevel)
// are used to see how deep we are with passing sub-queries (= scope_level).
class AggregateFinder
{
public:
	AggregateFinder(DsqlCompilerScratch* aDsqlScratch, bool aWindow);

	static bool find(DsqlCompilerScratch* dsqlScratch, bool window, ExprNode* node);

	bool visit(ExprNode* node);

public:
	DsqlCompilerScratch* dsqlScratch;
	bool window;
	USHORT currentLevel;
	USHORT deepestLevel;
	bool ignoreSubSelects;
};

// Check the fields inside an aggregate and check if the field scope_level meets the specified
// conditions.
//
// The SQL 2008 standard says:
// <where clause> ::=
//   WHERE <search condition>
// Syntax Rules
// 1) If a <value expression> directly contained in the <search condition> is a
// <set function specification>, then the <where clause> shall be contained in a <having clause>
// or <select list>, the <set function specification> shall contain a column reference, and
// every column reference contained in an aggregated argument of the <set function specification>
// shall be an outer reference.
//    NOTE 160 - outer reference is defined in Subclause 6.7, "<column reference>".
// 2) The <search condition> shall not contain a <window function> without an intervening
// <query expression>.
class Aggregate2Finder
{
public:
	Aggregate2Finder(USHORT aCheckScopeLevel, FieldMatchType aMatchType, bool aWindowOnly);

	static bool find(USHORT checkScopeLevel, FieldMatchType matchType, bool windowOnly,
		ExprNode* node);

	bool visit(ExprNode* node);

public:
	const USHORT checkScopeLevel;
	const FieldMatchType matchType;
	bool windowOnly;
	bool currentScopeLevelEqual;
};

// Check the fields inside an aggregate and check if the field scope_level meets the specified
// conditions.
class FieldFinder
{
public:
	FieldFinder(USHORT aCheckScopeLevel, FieldMatchType aMatchType);

	static bool find(USHORT checkScopeLevel, FieldMatchType matchType, ExprNode* node);

	bool visit(ExprNode* node);

public:
	bool getField() const
	{
		return field;
	}

public:
	const USHORT checkScopeLevel;
	const FieldMatchType matchType;
	bool field;
};

// Validate that an expanded field / context pair is in a specified list. This is used in one
// instance to check that a simple field selected through a grouping rse is a grouping field -
// thus a valid field reference. For the sake of argument, we'll match qualified to unqualified
// reference, but qualified reference must match completely.
// A list element containing a simple CAST for collation purposes is allowed.
class InvalidReferenceFinder
{
public:
	InvalidReferenceFinder(const dsql_ctx* aContext, const ValueListNode* aList);

	static bool find(const dsql_ctx* context, const ValueListNode* list, ExprNode* node);

	bool visit(ExprNode* node);

public:
	const dsql_ctx* const context;
	const ValueListNode* const list;
	bool insideOwnMap;
	bool insideHigherMap;
};

// Called to map fields used in an aggregate-context after all pass1 calls
// (SELECT-, ORDER BY-lists). Walk completly through the given node 'field' and map the fields
// with same scope_level as the given context to the given context with the post_map function.
class FieldRemapper
{
public:
	FieldRemapper(DsqlCompilerScratch* aDsqlScratch, dsql_ctx* aContext, bool aWindow,
				ValueListNode* aPartitionNode = NULL, ValueListNode* aOrderNode = NULL);

	static ExprNode* remap(DsqlCompilerScratch* dsqlScratch, dsql_ctx* context, bool window,
		ExprNode* field, ValueListNode* partitionNode = NULL, ValueListNode* orderNode = NULL)
	{
		// The bool value returned by the visitor is completely discarded in this class.
		return FieldRemapper(dsqlScratch, context, window, partitionNode, orderNode).visit(field);
	}

	ExprNode* visit(ExprNode* node);

public:
	DsqlCompilerScratch* const dsqlScratch;
	dsql_ctx* const context;
	const bool window;
	ValueListNode* partitionNode;
	ValueListNode* orderNode;
	USHORT currentLevel;
};

// Search if a sub select is buried inside a select list from a query expression.
class SubSelectFinder
{
public:
	static bool find(ExprNode* node)
	{
		return SubSelectFinder().visit(node);
	}

	bool visit(ExprNode* node);
};


// Generic node copier.
class NodeCopier
{
public:
	NodeCopier(CompilerScratch* aCsb, StreamType* aRemap)
		: csb(aCsb),
		  remap(aRemap),
		  message(NULL)
	{
	}

	virtual ~NodeCopier()
	{
	}

public:
	// Copy an expression tree remapping field streams. If the map isn't present, don't remap.
	template <typename T>
	T* copy(thread_db* tdbb, const T* input)
	{
		if (!input)
			return NULL;

		T* copy = input->copy(tdbb, *this);
		postCopy(input, copy, copy);

		return copy;
	}

	template <typename T>
	T* copy(thread_db* tdbb, const NestConst<T>& input)
	{
		return copy(tdbb, input.getObject());
	}

	template <typename T>
	static T* copy(thread_db* tdbb, CompilerScratch* csb, const T* input, StreamType* remap)
	{
		return NodeCopier(csb, remap).copy(tdbb, input);
	}

	template <typename T>
	static T* copy(thread_db* tdbb, CompilerScratch* csb, const NestConst<T>& input, StreamType* remap)
	{
		return NodeCopier(csb, remap).copy(tdbb, input.getObject());
	}

	virtual USHORT remapField(USHORT /*stream*/, USHORT fldId)
	{
		return fldId;
	}

	virtual USHORT getFieldId(const FieldNode* input);

private:
	template <typename T1, typename T2> void postCopy(const T1* source, T2* target, ExprNode* /*dummy*/)
	{
		target->nodFlags = source->nodFlags;
	}

	void postCopy(const StmtNode* /*source*/, StmtNode* /*target*/, StmtNode* /*dummy*/)
	{
	}

public:
	CompilerScratch* const csb;
	StreamType* const remap;
	MessageNode* message;
};


} // namespace

#endif // DSQL_VISITORS_H
