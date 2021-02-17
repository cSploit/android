/*
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
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 * Adriano dos Santos Fernandes
 */

#ifndef DSQL_COMPILER_SCRATCH_H
#define DSQL_COMPILER_SCRATCH_H

#include "../jrd/jrd.h"
#include "../dsql/dsql.h"
#include "../dsql/BlrDebugWriter.h"
#include "../common/classes/array.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/stack.h"
#include "../common/classes/alloc.h"

namespace Jrd
{

class BinaryBoolNode;
class CompoundStmtNode;
class DeclareCursorNode;
class DeclareVariableNode;
class ParameterClause;
class RseNode;
class SelectExprNode;
class TypeClause;
class VariableNode;
class WithClause;


// DSQL Compiler scratch block - may be discarded after compilation in the future.
class DsqlCompilerScratch : public BlrDebugWriter
{
public:
	static const unsigned FLAG_IN_AUTO_TRANS_BLOCK	= 0x0001;
	static const unsigned FLAG_RETURNING_INTO		= 0x0002;
	static const unsigned FLAG_METADATA_SAVED		= 0x0004;
	static const unsigned FLAG_PROCEDURE			= 0x0008;
	static const unsigned FLAG_TRIGGER				= 0x0010;
	static const unsigned FLAG_BLOCK				= 0x0020;
	static const unsigned FLAG_RECURSIVE_CTE		= 0x0040;
	static const unsigned FLAG_UPDATE_OR_INSERT		= 0x0080;
	static const unsigned FLAG_MERGE				= 0x0100;
	static const unsigned FLAG_FUNCTION				= 0x0200;
	static const unsigned FLAG_SUB_ROUTINE			= 0x0400;
	static const unsigned FLAG_INTERNAL_REQUEST		= 0x0800;
	static const unsigned FLAG_AMBIGUOUS_STMT		= 0x1000;
	static const unsigned FLAG_DDL					= 0x2000;

public:
	DsqlCompilerScratch(MemoryPool& p, dsql_dbb* aDbb, jrd_tra* aTransaction,
				DsqlCompiledStatement* aStatement)
		: BlrDebugWriter(p),
		  dbb(aDbb),
		  transaction(aTransaction),
		  statement(aStatement),
		  flags(0),
		  ports(p),
		  relation(NULL),
		  procedure(NULL),
		  mainContext(p),
		  context(&mainContext),
		  unionContext(p),
		  derivedContext(p),
		  outerAggContext(NULL),
		  contextNumber(0),
		  derivedContextNumber(0),
		  scopeLevel(0),
		  loopLevel(0),
		  labels(p),
		  cursorNumber(0),
		  cursors(p),
		  inSelectList(0),
		  inWhereClause(0),
		  inGroupByClause(0),
		  inHavingClause(0),
		  inOrderByClause(0),
		  errorHandlers(0),
		  clientDialect(0),
		  inOuterJoin(0),
		  aliasRelationPrefix(p),
		  package(p),
		  currCtes(p),
		  recursiveCtx(0),
		  recursiveCtxId(0),
		  processingWindow(false),
		  checkConstraintTrigger(false),
		  hiddenVarsNumber(0),
		  hiddenVariables(p),
		  variables(p),
		  outputVariables(p),
		  currCteAlias(NULL),
		  ctes(p),
		  cteAliases(p),
		  psql(false),
		  subFunctions(p),
		  subProcedures(p)
	{
		domainValue.clear();
	}

protected:
	// DsqlCompilerScratch should never be destroyed using delete.
	// It dies together with it's pool in release_request().
	~DsqlCompilerScratch()
	{
	}

public:
	virtual bool isVersion4()
	{
		return statement->getBlrVersion() == 4;
	}

	MemoryPool& getPool()
	{
		return PermanentStorage::getPool();
	}

	dsql_dbb* getAttachment()
	{
		return dbb;
	}

	jrd_tra* getTransaction()
	{
		return transaction;
	}

	void setTransaction(jrd_tra* value)
	{
		transaction = value;
	}

	DsqlCompiledStatement* getStatement()
	{
		return statement;
	}

	DsqlCompiledStatement* getStatement() const
	{
		return statement;
	}

	void putDtype(const TypeClause* field, bool useSubType);
	void putType(const TypeClause* type, bool useSubType);
	void putLocalVariables(CompoundStmtNode* parameters, USHORT locals);
	void putLocalVariable(dsql_var* variable, const DeclareVariableNode* hostParam,
		const Firebird::MetaName& collationName);
	dsql_var* makeVariable(dsql_fld*, const char*, const dsql_var::Type type, USHORT,
		USHORT, USHORT);
	dsql_var* resolveVariable(const Firebird::MetaName& varName);
	void genReturn(bool eosFlag = false);

	void genParameters(Firebird::Array<NestConst<ParameterClause> >& parameters,
		Firebird::Array<NestConst<ParameterClause> >& returns);

	// Get rid of any predefined contexts created for a view or trigger definition.
	// Also reset hidden variables.
	void resetContextStack()
	{
		context->clear();
		contextNumber = 0;
		derivedContextNumber = 0;

		hiddenVarsNumber = 0;
		hiddenVariables.clear();
	}

	void resetTriggerContextStack()
	{
		context->clear();
		contextNumber = 0;
	}

	void addCTEs(WithClause* withClause);
	SelectExprNode* findCTE(const Firebird::MetaName& name);
	void clearCTEs();
	void checkUnusedCTEs() const;

	// hvlad: each member of recursive CTE can refer to CTE itself (only once) via
	// CTE name or via alias. We need to substitute this aliases when processing CTE
	// member to resolve field names. Therefore we store all aliases in order of
	// occurrence and later use it in backward order (since our parser is right-to-left).
	// Also we put CTE name after all such aliases to distinguish aliases for
	// different CTE's.
	// We also need to repeat this process if main select expression contains union with
	// recursive CTE
	void addCTEAlias(const Firebird::string& alias)
	{
		thread_db* tdbb = JRD_get_thread_data();
		cteAliases.add(FB_NEW(*tdbb->getDefaultPool()) Firebird::string(*tdbb->getDefaultPool(), alias));
	}

	const Firebird::string* getNextCTEAlias()
	{
		return *(--currCteAlias);
	}

	void resetCTEAlias(const Firebird::string& alias)
	{
#ifdef DEV_BUILD
		const Firebird::string* const* begin = cteAliases.begin();
#endif

		currCteAlias = cteAliases.end() - 1;
		fb_assert(currCteAlias >= begin);

		const Firebird::string* curr = *(currCteAlias);

		while (strcmp(curr->c_str(), alias.c_str()))
		{
			currCteAlias--;
			fb_assert(currCteAlias >= begin);

			curr = *(currCteAlias);
		}
	}

	bool isPsql() const { return psql; }
	void setPsql(bool value) { psql = value; }

	dsql_udf* getSubFunction(const Firebird::MetaName& name)
	{
		dsql_udf* subFunc = NULL;
		subFunctions.get(name, subFunc);
		return subFunc;
	}

	void putSubFunction(dsql_udf* subFunc)
	{
		if (subFunctions.exist(subFunc->udf_name.identifier))
		{
			using namespace Firebird;
			status_exception::raise(
				Arg::Gds(isc_dsql_duplicate_spec) << subFunc->udf_name.identifier);
		}

		subFunctions.put(subFunc->udf_name.identifier, subFunc);
	}

	dsql_prc* getSubProcedure(const Firebird::MetaName& name)
	{
		dsql_prc* subProc = NULL;
		subProcedures.get(name, subProc);
		return subProc;
	}

	void putSubProcedure(dsql_prc* subProc)
	{
		if (subProcedures.exist(subProc->prc_name.identifier))
		{
			using namespace Firebird;
			status_exception::raise(
				Arg::Gds(isc_dsql_duplicate_spec) << subProc->prc_name.identifier);
		}

		subProcedures.put(subProc->prc_name.identifier, subProc);
	}

private:
	SelectExprNode* pass1RecursiveCte(SelectExprNode* input);
	RseNode* pass1RseIsRecursive(RseNode* input);
	bool pass1RelProcIsRecursive(RecordSourceNode* input);
	BoolExprNode* pass1JoinIsRecursive(RecordSourceNode*& input);

	dsql_dbb* dbb;						// DSQL attachment
	jrd_tra* transaction;				// Transaction
	DsqlCompiledStatement* statement;	// Compiled statement

public:
	unsigned flags;						// flags
	Firebird::Array<dsql_msg*> ports;	// Port messages
	dsql_rel* relation;					// relation created by this request (for DDL)
	dsql_prc* procedure;				// procedure created by this request (for DDL)
	DsqlContextStack mainContext;
	DsqlContextStack* context;
	DsqlContextStack unionContext;		// Save contexts for views of unions
	DsqlContextStack derivedContext;	// Save contexts for views of derived tables
	dsql_ctx* outerAggContext;			// agg context for outer ref
	// CVC: I think the two contexts may need a bigger var, too.
	USHORT contextNumber;				// Next available context number
	USHORT derivedContextNumber;		// Next available context number for derived tables
	USHORT scopeLevel;					// Scope level for parsing aliases in subqueries
	USHORT loopLevel;					// Loop level
	Firebird::Stack<Firebird::MetaName*> labels;	// Loop labels
	USHORT cursorNumber;				// Cursor number
	Firebird::Array<DeclareCursorNode*> cursors; // Cursors
	USHORT inSelectList;				// now processing "select list"
	USHORT inWhereClause;				// processing "where clause"
	USHORT inGroupByClause;				// processing "group by clause"
	USHORT inHavingClause;				// processing "having clause"
	USHORT inOrderByClause;				// processing "order by clause"
	USHORT errorHandlers;				// count of active error handlers
	USHORT clientDialect;				// dialect passed into the API call
	USHORT inOuterJoin;					// processing inside outer-join part
	Firebird::string aliasRelationPrefix;	// prefix for every relation-alias.
	Firebird::MetaName package;			// package being defined
	Firebird::Stack<SelectExprNode*> currCtes;	// current processing CTE's
	class dsql_ctx* recursiveCtx;		// context of recursive CTE
	USHORT recursiveCtxId;				// id of recursive union stream context
	bool processingWindow;				// processing window functions
	bool checkConstraintTrigger;		// compiling a check constraint trigger
	dsc domainValue;					// VALUE in the context of domain's check constraint
	USHORT hiddenVarsNumber;			// next hidden variable number
	Firebird::Array<dsql_var*> hiddenVariables;	// hidden variables
	Firebird::Array<dsql_var*> variables;
	Firebird::Array<dsql_var*> outputVariables;
	const Firebird::string* const* currCteAlias;

private:
	Firebird::HalfStaticArray<SelectExprNode*, 4> ctes; // common table expressions
	Firebird::HalfStaticArray<const Firebird::string*, 4> cteAliases; // CTE aliases in recursive members
	bool psql;
	Firebird::GenericMap<Firebird::Left<Firebird::MetaName, dsql_udf*> > subFunctions;
	Firebird::GenericMap<Firebird::Left<Firebird::MetaName, dsql_prc*> > subProcedures;
};

class PsqlChanger
{
public:
	PsqlChanger(DsqlCompilerScratch* aDsqlScratch, bool value)
		: dsqlScratch(aDsqlScratch),
		  oldValue(dsqlScratch->isPsql())
	{
		dsqlScratch->setPsql(value);
	}

	~PsqlChanger()
	{
		dsqlScratch->setPsql(oldValue);
	}

private:
	// copying is prohibited
	PsqlChanger(const PsqlChanger&);
	PsqlChanger& operator =(const PsqlChanger&);

	DsqlCompilerScratch* dsqlScratch;
	const bool oldValue;
};

}	// namespace Jrd

#endif // DSQL_COMPILER_SCRATCH_H
