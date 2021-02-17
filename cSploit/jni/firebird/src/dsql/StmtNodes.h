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

#ifndef DSQL_STMT_NODES_H
#define DSQL_STMT_NODES_H

#include "../common/classes/MetaName.h"
#include "../jrd/blr.h"
#include "../jrd/Function.h"
#include "../jrd/extds/ExtDS.h"
#include "../dsql/Nodes.h"
#include "../dsql/DdlNodes.h"

namespace Jrd {

class CompoundStmtNode;
class ExecBlockNode;
class PlanNode;
class RelationSourceNode;
class SelectNode;
class GeneratorItem;

typedef Firebird::Pair<
	Firebird::NonPooled<NestConst<ValueListNode>, NestConst<ValueListNode> > > ReturningClause;


class ExceptionItem : public Firebird::PermanentStorage
{
public:
	enum Type
	{
		SQL_CODE = 1,
		GDS_CODE = 2,
		XCP_CODE = 3,
		XCP_DEFAULT = 4
	};

	ExceptionItem(MemoryPool& pool, const ExceptionItem& o)
		: PermanentStorage(pool),
		  type(o.type),
		  code(o.code),
		  name(pool, o.name),
		  secName(pool, o.secName)
	{
	}

	explicit ExceptionItem(MemoryPool& pool)
		: PermanentStorage(pool),
		  code(0),
		  name(pool),
		  secName(pool)
	{
	}

	ExceptionItem& operator =(const ExceptionItem& o)
	{
		code = o.code;
		name = o.name;
		secName = o.secName;
		return *this;
	}

	Type type;
	SLONG code;
	// ASF: There are some inconsistencies in the type of 'name'. Metanames have maximum of 31 chars,
	// while there are system exceptions with 32 chars. The parser always expects metanames, but
	// I'm following the legacy code and making this a string.
	Firebird::string name;
	Firebird::MetaName secName;
};

typedef Firebird::ObjectsArray<ExceptionItem> ExceptionArray;


struct ValidateInfo
{
	NestConst<BoolExprNode> boolean;
	NestConst<ValueExprNode> value;
};


class AssignmentNode : public TypedNode<StmtNode, StmtNode::TYPE_ASSIGNMENT>
{
public:
	explicit AssignmentNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_ASSIGNMENT>(pool),
		  asgnFrom(NULL),
		  asgnTo(NULL),
		  missing(NULL),
		  missing2(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual AssignmentNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual AssignmentNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual AssignmentNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual AssignmentNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<ValueExprNode> asgnFrom;
	NestConst<ValueExprNode> asgnTo;
	NestConst<ValueExprNode> missing;
	NestConst<ValueExprNode> missing2;
};


class BlockNode : public TypedNode<StmtNode, StmtNode::TYPE_BLOCK>
{
public:
	explicit BlockNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_BLOCK>(pool),
		  action(NULL),
		  handlers(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual BlockNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual BlockNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static bool testAndFixupError(thread_db* tdbb, jrd_req* request, const ExceptionArray& conditions);

public:
	NestConst<StmtNode> action;
	NestConst<CompoundStmtNode> handlers;
};


class CommitRollbackNode : public TransactionNode
{
public:
	enum Command
	{
		CMD_COMMIT,
		CMD_ROLLBACK
	};

public:
	explicit CommitRollbackNode(MemoryPool& pool, Command aCommand, bool aRetain)
		: TransactionNode(pool),
		  command(aCommand),
		  retain(aRetain)
	{
	}

public:
	virtual void print(Firebird::string& text) const
	{
		text = "CommitRollbackNode";
	}

	virtual CommitRollbackNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		switch (command)
		{
			case CMD_COMMIT:
				dsqlScratch->getStatement()->setType(retain ?
					DsqlCompiledStatement::TYPE_COMMIT_RETAIN : DsqlCompiledStatement::TYPE_COMMIT);
				break;

			case CMD_ROLLBACK:
				dsqlScratch->getStatement()->setType(retain ?
					DsqlCompiledStatement::TYPE_ROLLBACK_RETAIN : DsqlCompiledStatement::TYPE_ROLLBACK);
				break;
		}

		return this;
	}

	virtual void execute(thread_db* tdbb, dsql_req* request, jrd_tra** transaction) const
	{
		if (retain)
		{
			switch (command)
			{
				case CMD_COMMIT:
					JRD_commit_retaining(tdbb, request->req_transaction);
					break;

				case CMD_ROLLBACK:
					JRD_rollback_retaining(tdbb, request->req_transaction);
					break;
			}
		}
		else
		{
			switch (command)
			{
				case CMD_COMMIT:
					JRD_commit_transaction(tdbb, request->req_transaction);
					break;

				case CMD_ROLLBACK:
					JRD_rollback_transaction(tdbb, request->req_transaction);
					break;
			}

			*transaction = NULL;
		}
	}

private:
	Command command;
	bool retain;
};


class CompoundStmtNode : public TypedNode<StmtNode, StmtNode::TYPE_COMPOUND_STMT>	// blr_begin
{
public:
	explicit CompoundStmtNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_COMPOUND_STMT>(pool),
		  statements(pool),
		  onlyAssignments(false)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual CompoundStmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual CompoundStmtNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual CompoundStmtNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual CompoundStmtNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	Firebird::Array<NestConst<StmtNode> > statements;
	bool onlyAssignments;
};


class ContinueLeaveNode : public TypedNode<StmtNode, StmtNode::TYPE_CONTINUE_LEAVE>
{
public:
	explicit ContinueLeaveNode(MemoryPool& pool, UCHAR aBlrOp)
		: TypedNode<StmtNode, StmtNode::TYPE_CONTINUE_LEAVE>(pool),
		  blrOp(aBlrOp),
		  labelNumber(0),
		  dsqlLabelName(NULL)
	{
		fb_assert(blrOp == blr_continue_loop || blrOp == blr_leave);
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ContinueLeaveNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual ContinueLeaveNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual ContinueLeaveNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	UCHAR blrOp;
	USHORT labelNumber;
	Firebird::MetaName* dsqlLabelName;
};


class CursorStmtNode : public TypedNode<StmtNode, StmtNode::TYPE_CURSOR_STMT>
{
public:
	explicit CursorStmtNode(MemoryPool& pool, UCHAR aCursorOp, const Firebird::MetaName& aDsqlName = "",
				ValueListNode* aDsqlIntoStmt = NULL)
		: TypedNode<StmtNode, StmtNode::TYPE_CURSOR_STMT>(pool),
		  dsqlName(pool, aDsqlName),
		  dsqlIntoStmt(aDsqlIntoStmt),
		  cursorOp(aCursorOp),
		  cursorNumber(0),
		  scrollOp(0),
		  scrollExpr(NULL),
		  intoStmt(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual CursorStmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual CursorStmtNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual CursorStmtNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	Firebird::MetaName dsqlName;
	ValueListNode* dsqlIntoStmt;
	UCHAR cursorOp;
	USHORT cursorNumber;
	UCHAR scrollOp;
	NestConst<ValueExprNode> scrollExpr;
	NestConst<StmtNode> intoStmt;
};


class DeclareCursorNode : public TypedNode<StmtNode, StmtNode::TYPE_DECLARE_CURSOR>
{
public:
	static const USHORT CUR_TYPE_NONE = 0;
	static const USHORT CUR_TYPE_EXPLICIT = 1;
	static const USHORT CUR_TYPE_FOR = 2;
	static const USHORT CUR_TYPE_ALL = (CUR_TYPE_EXPLICIT | CUR_TYPE_FOR);

	explicit DeclareCursorNode(MemoryPool& pool, const Firebird::MetaName& aDsqlName = NULL,
				USHORT aDsqlCursorType = CUR_TYPE_NONE)
		: TypedNode<StmtNode, StmtNode::TYPE_DECLARE_CURSOR>(pool),
		  dsqlCursorType(aDsqlCursorType),
		  dsqlScroll(false),
		  dsqlName(aDsqlName),
		  dsqlSelect(NULL),
		  rse(NULL),
		  refs(NULL),
		  cursorNumber(0),
		  cursor(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual DeclareCursorNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual DeclareCursorNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual DeclareCursorNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	USHORT dsqlCursorType;
	bool dsqlScroll;
	Firebird::MetaName dsqlName;
	NestConst<SelectNode> dsqlSelect;
	NestConst<RseNode> rse;
	NestConst<ValueListNode> refs;
	USHORT cursorNumber;
	NestConst<Cursor> cursor;
};


class DeclareSubFuncNode : public TypedNode<StmtNode, StmtNode::TYPE_DECLARE_SUBFUNC>
{
public:
	explicit DeclareSubFuncNode(MemoryPool& pool, const Firebird::MetaName& aName)
		: TypedNode<StmtNode, StmtNode::TYPE_DECLARE_SUBFUNC>(pool),
		  name(pool, aName),
		  dsqlDeterministic(false),
		  dsqlBlock(NULL),
		  blockScratch(NULL),
		  dsqlFunction(NULL),
		  blrStart(NULL),
		  blrLength(0),
		  subCsb(NULL),
		  routine(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual DeclareSubFuncNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual DeclareSubFuncNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual DeclareSubFuncNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static void parseParameters(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb,
		Firebird::Array<NestConst<Parameter> >& paramArray, USHORT* defaultCount = NULL);

	void genParameters(DsqlCompilerScratch* dsqlScratch,
		Firebird::Array<NestConst<ParameterClause> >& paramArray);

public:
	Firebird::MetaName name;
	bool dsqlDeterministic;
	NestConst<ExecBlockNode> dsqlBlock;
	DsqlCompilerScratch* blockScratch;
	dsql_udf* dsqlFunction;
	const UCHAR* blrStart;
	ULONG blrLength;
	CompilerScratch* subCsb;
	Function* routine;
};


class DeclareSubProcNode : public TypedNode<StmtNode, StmtNode::TYPE_DECLARE_SUBPROC>
{
public:
	explicit DeclareSubProcNode(MemoryPool& pool, const Firebird::MetaName& aName)
		: TypedNode<StmtNode, StmtNode::TYPE_DECLARE_SUBPROC>(pool),
		  name(pool, aName),
		  dsqlBlock(NULL),
		  blockScratch(NULL),
		  dsqlProcedure(NULL),
		  blrStart(NULL),
		  blrLength(0),
		  subCsb(NULL),
		  routine(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual DeclareSubProcNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual DeclareSubProcNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual DeclareSubProcNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static void parseParameters(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb,
		Firebird::Array<NestConst<Parameter> >& paramArray, USHORT* defaultCount = NULL);

	void genParameters(DsqlCompilerScratch* dsqlScratch,
		Firebird::Array<NestConst<ParameterClause> >& paramArray);

public:
	Firebird::MetaName name;
	NestConst<ExecBlockNode> dsqlBlock;
	DsqlCompilerScratch* blockScratch;
	dsql_prc* dsqlProcedure;
	const UCHAR* blrStart;
	ULONG blrLength;
	CompilerScratch* subCsb;
	jrd_prc* routine;
};


class DeclareVariableNode : public TypedNode<StmtNode, StmtNode::TYPE_DECLARE_VARIABLE>
{
public:
	explicit DeclareVariableNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_DECLARE_VARIABLE>(pool),
		  dsqlDef(NULL),
		  varId(0)
	{
		varDesc.clear();
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual DeclareVariableNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual DeclareVariableNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual DeclareVariableNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual DeclareVariableNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<ParameterClause> dsqlDef;
	USHORT varId;
	dsc varDesc;
};


class EraseNode : public TypedNode<StmtNode, StmtNode::TYPE_ERASE>
{
public:
	explicit EraseNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_ERASE>(pool),
		  dsqlRelation(NULL),
		  dsqlBoolean(NULL),
		  dsqlPlan(NULL),
		  dsqlOrder(NULL),
		  dsqlRows(NULL),
		  dsqlCursorName(pool),
		  dsqlReturning(NULL),
		  dsqlRse(NULL),
		  dsqlContext(NULL),
		  statement(NULL),
		  subStatement(NULL),
		  stream(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual EraseNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual EraseNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static void pass1Erase(thread_db* tdbb, CompilerScratch* csb, EraseNode* node);
	const StmtNode* erase(thread_db* tdbb, jrd_req* request, WhichTrigger whichTrig) const;

public:
	NestConst<RelationSourceNode> dsqlRelation;
	NestConst<BoolExprNode> dsqlBoolean;
	NestConst<PlanNode> dsqlPlan;
	NestConst<ValueListNode> dsqlOrder;
	NestConst<RowsClause> dsqlRows;
	Firebird::MetaName dsqlCursorName;
	NestConst<ReturningClause> dsqlReturning;
	NestConst<RseNode> dsqlRse;
	dsql_ctx* dsqlContext;
	NestConst<StmtNode> statement;
	NestConst<StmtNode> subStatement;
	StreamType stream;
};


class ErrorHandlerNode : public TypedNode<StmtNode, StmtNode::TYPE_ERROR_HANDLER>
{
public:
	explicit ErrorHandlerNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_ERROR_HANDLER>(pool),
		  action(NULL),
		  conditions(pool)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ErrorHandlerNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual ErrorHandlerNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ErrorHandlerNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<StmtNode> action;
	ExceptionArray conditions;
};


class ExecProcedureNode : public TypedNode<StmtNode, StmtNode::TYPE_EXEC_PROCEDURE>
{
public:
	explicit ExecProcedureNode(MemoryPool& pool,
				const Firebird::QualifiedName& aDsqlName = Firebird::QualifiedName(),
				ValueListNode* aInputs = NULL, ValueListNode* aOutputs = NULL)
		: TypedNode<StmtNode, StmtNode::TYPE_EXEC_PROCEDURE>(pool),
		  dsqlName(pool, aDsqlName),
		  dsqlProcedure(NULL),
		  inputSources(aInputs),
		  inputTargets(NULL),
		  inputMessage(NULL),
		  outputSources(aOutputs),
		  outputTargets(NULL),
		  outputMessage(NULL),
		  procedure(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ExecProcedureNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual ExecProcedureNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ExecProcedureNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	ValueListNode* explodeOutputs(DsqlCompilerScratch* dsqlScratch, const dsql_prc* procedure);
	void executeProcedure(thread_db* tdbb, jrd_req* request) const;

public:
	Firebird::QualifiedName dsqlName;
	dsql_prc* dsqlProcedure;
	NestConst<ValueListNode> inputSources;
	NestConst<ValueListNode> inputTargets;
	NestConst<MessageNode> inputMessage;
	NestConst<ValueListNode> outputSources;
	NestConst<ValueListNode> outputTargets;
	NestConst<MessageNode> outputMessage;
	NestConst<jrd_prc> procedure;
};


class ExecStatementNode : public TypedNode<StmtNode, StmtNode::TYPE_EXEC_STATEMENT>
{
public:
	explicit ExecStatementNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_EXEC_STATEMENT>(pool),
		  dsqlLabelName(NULL),
		  dsqlLabelNumber(0),
		  sql(NULL),
		  dataSource(NULL),
		  userName(NULL),
		  password(NULL),
		  role(NULL),
		  innerStmt(NULL),
		  inputs(NULL),
		  outputs(NULL),
		  useCallerPrivs(false),
		  traScope(EDS::TraScope(0)),	// not defined
		  inputNames(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual ExecStatementNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ExecStatementNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static void genOptionalExpr(DsqlCompilerScratch* dsqlScratch, const UCHAR code, ValueExprNode* node);

	void getString(thread_db* tdbb, jrd_req* request, const ValueExprNode* node,
		Firebird::string& str, bool useAttCS = false) const;

public:
	Firebird::MetaName* dsqlLabelName;
	USHORT dsqlLabelNumber;
	NestConst<ValueExprNode> sql;
	NestConst<ValueExprNode> dataSource;
	NestConst<ValueExprNode> userName;
	NestConst<ValueExprNode> password;
	NestConst<ValueExprNode> role;
	NestConst<StmtNode> innerStmt;
	NestConst<ValueListNode> inputs;
	NestConst<ValueListNode> outputs;
	bool useCallerPrivs;
	EDS::TraScope traScope;
	EDS::ParamNames* inputNames;
};


class IfNode : public TypedNode<StmtNode, StmtNode::TYPE_IF>
{
public:
	explicit IfNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_IF>(pool),
		  condition(NULL),
		  trueAction(NULL),
		  falseAction(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual IfNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual IfNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual IfNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<BoolExprNode> condition;
	NestConst<StmtNode> trueAction;
	NestConst<StmtNode> falseAction;
};


class InAutonomousTransactionNode : public TypedNode<StmtNode, StmtNode::TYPE_IN_AUTO_TRANS>
{
	struct Impure
	{
		TraNumber traNumber;
		SLONG savNumber;
	};

public:
	explicit InAutonomousTransactionNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_IN_AUTO_TRANS>(pool),
		  action(NULL),
		  impureOffset(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual InAutonomousTransactionNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual InAutonomousTransactionNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual InAutonomousTransactionNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<StmtNode> action;
	SLONG impureOffset;
};


class InitVariableNode : public TypedNode<StmtNode, StmtNode::TYPE_INIT_VARIABLE>
{
public:
	explicit InitVariableNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_INIT_VARIABLE>(pool),
		  varId(0),
		  varDecl(NULL),
		  varInfo(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual InitVariableNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual InitVariableNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual InitVariableNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual InitVariableNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	USHORT varId;
	NestConst<DeclareVariableNode> varDecl;
	NestConst<ItemInfo> varInfo;
};


class ExecBlockNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_EXEC_BLOCK>
{
public:
	explicit ExecBlockNode(MemoryPool& pool)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_EXEC_BLOCK>(pool),
		  parameters(pool),
		  returns(pool),
		  localDeclList(NULL),
		  body(NULL)
	{
	}

	virtual void print(Firebird::string& text) const;
	virtual ExecBlockNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

private:
	static void revertParametersOrder(Firebird::Array<dsql_par*>& parameters);

public:
	Firebird::Array<NestConst<ParameterClause> > parameters;
	Firebird::Array<NestConst<ParameterClause> > returns;
	NestConst<CompoundStmtNode> localDeclList;
	NestConst<StmtNode> body;
};


class ExceptionNode : public TypedNode<StmtNode, StmtNode::TYPE_EXCEPTION>
{
public:
	ExceptionNode(MemoryPool& pool, const Firebird::MetaName& name,
				ValueExprNode* aMessageExpr = NULL, ValueListNode* aParameters = NULL)
		: TypedNode<StmtNode, StmtNode::TYPE_EXCEPTION>(pool),
		  messageExpr(aMessageExpr),
		  parameters(aParameters)
	{
		exception = FB_NEW(pool) ExceptionItem(pool);
		exception->type = ExceptionItem::XCP_CODE;
		exception->name = name.c_str();
	}

	explicit ExceptionNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_EXCEPTION>(pool),
		  messageExpr(NULL),
		  parameters(NULL),
		  exception(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual ExceptionNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ExceptionNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	void setError(thread_db* tdbb) const;

public:
	NestConst<ValueExprNode> messageExpr;
	NestConst<ValueListNode> parameters;
	NestConst<ExceptionItem> exception;
};


class ExitNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_EXIT>
{
public:
	explicit ExitNode(MemoryPool& pool)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_EXIT>(pool)
	{
	}

public:
	virtual void print(Firebird::string& text) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
};


class ForNode : public TypedNode<StmtNode, StmtNode::TYPE_FOR>
{
public:
	explicit ForNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_FOR>(pool),
		  dsqlSelect(NULL),
		  dsqlInto(NULL),
		  dsqlCursor(NULL),
		  dsqlLabelName(NULL),
		  dsqlLabelNumber(0),
		  dsqlForceSingular(false),
		  stall(NULL),
		  rse(NULL),
		  statement(NULL),
		  cursor(NULL),
		  parBlrBeginCnt(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ForNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual StmtNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual StmtNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<SelectNode> dsqlSelect;
	NestConst<ValueListNode> dsqlInto;
	DeclareCursorNode* dsqlCursor;
	Firebird::MetaName* dsqlLabelName;
	USHORT dsqlLabelNumber;
	bool dsqlForceSingular;
	NestConst<StmtNode> stall;
	NestConst<RseNode> rse;
	NestConst<StmtNode> statement;
	NestConst<Cursor> cursor;
	int parBlrBeginCnt;
};


class HandlerNode : public TypedNode<StmtNode, StmtNode::TYPE_HANDLER>
{
public:
	explicit HandlerNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_HANDLER>(pool),
		  statement(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual HandlerNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual HandlerNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual HandlerNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<StmtNode> statement;
};


class LabelNode : public TypedNode<StmtNode, StmtNode::TYPE_LABEL>
{
public:
	explicit LabelNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_LABEL>(pool),
		  statement(NULL),
		  labelNumber(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual LabelNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual LabelNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual LabelNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<StmtNode> statement;
	USHORT labelNumber;
};


class LineColumnNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_LINE_COLUMN>
{
public:
	explicit LineColumnNode(MemoryPool& pool, ULONG aLine, ULONG aColumn, StmtNode* aStatement)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_LINE_COLUMN>(pool),
		  statement(aStatement)
	{
		line = aLine;
		column = aColumn;
	}

public:
	virtual void print(Firebird::string& text) const;
	virtual LineColumnNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

private:
	NestConst<StmtNode> statement;
};


class LoopNode : public TypedNode<StmtNode, StmtNode::TYPE_LOOP>
{
public:
	explicit LoopNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_LOOP>(pool),
		  dsqlLabelName(NULL),
		  dsqlLabelNumber(0),
		  dsqlExpr(NULL),
		  statement(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual LoopNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual LoopNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual LoopNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	Firebird::MetaName* dsqlLabelName;
	USHORT dsqlLabelNumber;
	NestConst<BoolExprNode> dsqlExpr;
	NestConst<StmtNode> statement;
};


class MergeNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_MERGE>
{
public:
	struct Matched
	{
		explicit Matched(MemoryPool& pool)
			: assignments(NULL),
			  condition(NULL)
		{
		}

		NestConst<CompoundStmtNode> assignments;
		NestConst<BoolExprNode> condition;
	};

	struct NotMatched
	{
		explicit NotMatched(MemoryPool& pool)
			: fields(pool),
			  values(NULL),
			  condition(NULL)
		{
		}

		Firebird::Array<NestConst<FieldNode> > fields;
		NestConst<ValueListNode> values;
		NestConst<BoolExprNode> condition;
	};

	explicit MergeNode(MemoryPool& pool)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_MERGE>(pool),
		  relation(NULL),
		  usingClause(NULL),
		  condition(NULL),
		  whenMatched(pool),
		  whenNotMatched(pool),
		  returning(NULL)
	{
	}

	virtual void print(Firebird::string& text) const;
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

public:
	NestConst<RelationSourceNode> relation;
	NestConst<RecordSourceNode> usingClause;
	NestConst<BoolExprNode> condition;
	Firebird::ObjectsArray<Matched> whenMatched;
	Firebird::ObjectsArray<NotMatched> whenNotMatched;
	NestConst<ReturningClause> returning;
};


class MessageNode : public TypedNode<StmtNode, StmtNode::TYPE_MESSAGE>
{
public:
	explicit MessageNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_MESSAGE>(pool),
		  messageNumber(0),
		  format(NULL),
		  impureFlags(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	void setup(thread_db* tdbb, CompilerScratch* csb, USHORT message, USHORT count);

	virtual USHORT setupDesc(thread_db* tdbb, CompilerScratch* csb, USHORT index,
		dsc* desc, ItemInfo* itemInfo);

	virtual void print(Firebird::string& text) const;
	virtual MessageNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual MessageNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual MessageNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual MessageNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	USHORT messageNumber;
	NestConst<Format> format;
	ULONG impureFlags;
};


class ModifyNode : public TypedNode<StmtNode, StmtNode::TYPE_MODIFY>
{
public:
	explicit ModifyNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_MODIFY>(pool),
		  dsqlRelation(NULL),
		  dsqlBoolean(NULL),
		  dsqlPlan(NULL),
		  dsqlOrder(NULL),
		  dsqlRows(NULL),
		  dsqlCursorName(pool),
		  dsqlReturning(NULL),
		  dsqlRseFlags(0),
		  dsqlRse(NULL),
		  dsqlContext(NULL),
		  statement(NULL),
		  statement2(NULL),
		  subMod(NULL),
		  validations(pool),
		  mapView(NULL),
		  orgStream(0),
		  newStream(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	StmtNode* internalDsqlPass(DsqlCompilerScratch* dsqlScratch, bool updateOrInsert);
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual ModifyNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ModifyNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static void pass1Modify(thread_db* tdbb, CompilerScratch* csb, ModifyNode* node);
	const StmtNode* modify(thread_db* tdbb, jrd_req* request, WhichTrigger whichTrig) const;

public:
	NestConst<RecordSourceNode> dsqlRelation;
	NestConst<BoolExprNode> dsqlBoolean;
	NestConst<PlanNode> dsqlPlan;
	NestConst<ValueListNode> dsqlOrder;
	NestConst<RowsClause> dsqlRows;
	Firebird::MetaName dsqlCursorName;
	NestConst<ReturningClause> dsqlReturning;
	USHORT dsqlRseFlags;
	NestConst<RecordSourceNode> dsqlRse;
	dsql_ctx* dsqlContext;
	NestConst<StmtNode> statement;
	NestConst<StmtNode> statement2;
	NestConst<StmtNode> subMod;
	Firebird::Array<ValidateInfo> validations;
	NestConst<StmtNode> mapView;
	StreamType orgStream;
	StreamType newStream;
};


class PostEventNode : public TypedNode<StmtNode, StmtNode::TYPE_POST_EVENT>
{
public:
	explicit PostEventNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_POST_EVENT>(pool),
		  event(NULL),
		  argument(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual PostEventNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual PostEventNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual PostEventNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<ValueExprNode> event;
	NestConst<ValueExprNode> argument;
};


class ReceiveNode : public TypedNode<StmtNode, StmtNode::TYPE_RECEIVE>
{
public:
	explicit ReceiveNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_RECEIVE>(pool),
		  statement(NULL),
		  message(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ReceiveNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual ReceiveNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ReceiveNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<StmtNode> statement;
	NestConst<MessageNode> message;
};


class StoreNode : public TypedNode<StmtNode, StmtNode::TYPE_STORE>
{
public:
	explicit StoreNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_STORE>(pool),
		  dsqlRelation(NULL),
		  dsqlFields(pool),
		  dsqlValues(NULL),
		  dsqlReturning(NULL),
		  dsqlRse(NULL),
		  statement(NULL),
		  statement2(NULL),
		  subStore(NULL),
		  validations(pool),
		  relationSource(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	StmtNode* internalDsqlPass(DsqlCompilerScratch* dsqlScratch, bool updateOrInsert);
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual StoreNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual StoreNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

private:
	static bool pass1Store(thread_db* tdbb, CompilerScratch* csb, StoreNode* node);
	void makeDefaults(thread_db* tdbb, CompilerScratch* csb);
	const StmtNode* store(thread_db* tdbb, jrd_req* request, WhichTrigger whichTrig) const;

public:
	NestConst<RecordSourceNode> dsqlRelation;
	Firebird::Array<NestConst<FieldNode> > dsqlFields;
	NestConst<ValueListNode> dsqlValues;
	NestConst<ReturningClause> dsqlReturning;
	NestConst<RecordSourceNode> dsqlRse;
	NestConst<StmtNode> statement;
	NestConst<StmtNode> statement2;
	NestConst<StmtNode> subStore;
	Firebird::Array<ValidateInfo> validations;
	NestConst<RelationSourceNode> relationSource;
};


class UserSavepointNode : public TypedNode<StmtNode, StmtNode::TYPE_USER_SAVEPOINT>
{
public:
	enum Command
	{
		CMD_NOTHING = -1,
		CMD_SET = blr_savepoint_set,
		CMD_RELEASE = blr_savepoint_release,
		CMD_RELEASE_ONLY = blr_savepoint_release_single,
		CMD_ROLLBACK = blr_savepoint_undo
	};

public:
	explicit UserSavepointNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_USER_SAVEPOINT>(pool),
		  command(CMD_NOTHING),
		  name(pool)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual UserSavepointNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual UserSavepointNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual UserSavepointNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	Command command;
	Firebird::MetaName name;
};


class SelectNode : public TypedNode<StmtNode, StmtNode::TYPE_SELECT>
{
public:
	explicit SelectNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_SELECT>(pool),
		  dsqlExpr(NULL),
		  dsqlForUpdate(false),
		  dsqlWithLock(false),
		  dsqlRse(NULL),
		  statements(pool)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual SelectNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual SelectNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual SelectNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<SelectExprNode> dsqlExpr;
	bool dsqlForUpdate;
	bool dsqlWithLock;
	NestConst<RseNode> dsqlRse;
	Firebird::Array<NestConst<StmtNode> > statements;
};


// This is only for GPRE's cmp_set_generator().
class SetGeneratorNode : public TypedNode<StmtNode, StmtNode::TYPE_SET_GENERATOR>
{
public:
	SetGeneratorNode(MemoryPool& pool, const Firebird::MetaName& name, ValueExprNode* aValue = NULL)
		: TypedNode<StmtNode, StmtNode::TYPE_SET_GENERATOR>(pool),
		  generator(pool, name), value(aValue)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;

	// DSQL support is implemented in CreateAlterSequenceNode.
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch)
	{
		fb_assert(false);
	}

	virtual SetGeneratorNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual SetGeneratorNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	GeneratorItem generator;
	NestConst<ValueExprNode> value;
};


class StallNode : public TypedNode<StmtNode, StmtNode::TYPE_STALL>
{
public:
	explicit StallNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_STALL>(pool)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual StallNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual StallNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual StallNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;
};


class SuspendNode : public TypedNode<StmtNode, StmtNode::TYPE_SUSPEND>
{
public:
	explicit SuspendNode(MemoryPool& pool)
		: TypedNode<StmtNode, StmtNode::TYPE_SUSPEND>(pool),
		  message(NULL),
		  statement(NULL)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual SuspendNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual SuspendNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual SuspendNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	NestConst<MessageNode> message;
	NestConst<StmtNode> statement;
};


class ReturnNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_RETURN>
{
public:
	explicit ReturnNode(MemoryPool& pool, ValueExprNode* val = NULL)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_RETURN>(pool),
		  value(val)
	{
	}

	virtual void print(Firebird::string& text) const;
	virtual ReturnNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

public:
	NestConst<ValueExprNode> value;
};


class SavePointNode : public TypedNode<StmtNode, StmtNode::TYPE_SAVEPOINT>
{
public:
	explicit SavePointNode(MemoryPool& pool, UCHAR aBlrOp)
		: TypedNode<StmtNode, StmtNode::TYPE_SAVEPOINT>(pool),
		  blrOp(aBlrOp)
	{
		fb_assert(blrOp == blr_start_savepoint || blrOp == blr_end_savepoint);
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual SavePointNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual SavePointNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual SavePointNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const;

public:
	UCHAR blrOp;
};


class SetTransactionNode : public TransactionNode
{
public:
	struct RestrictionOption : Firebird::PermanentStorage
	{
		RestrictionOption(MemoryPool& p, Firebird::ObjectsArray<Firebird::MetaName>* aTables,
					unsigned aLockMode)
			: PermanentStorage(p),
			  tables(aTables),
			  lockMode(aLockMode)
		{
		}

		Firebird::ObjectsArray<Firebird::MetaName>* tables;
		unsigned lockMode;
	};

	enum
	{
		ISO_LEVEL_CONCURRENCY,
		ISO_LEVEL_CONSISTENCY,
		ISO_LEVEL_READ_COMMITTED_REC_VERSION,
		ISO_LEVEL_READ_COMMITTED_NO_REC_VERSION
	};

	static const unsigned LOCK_MODE_SHARED 		= 0x1;
	static const unsigned LOCK_MODE_PROTECTED	= 0x2;
	static const unsigned LOCK_MODE_READ		= 0x4;
	static const unsigned LOCK_MODE_WRITE		= 0x8;

public:
	explicit SetTransactionNode(MemoryPool& pool)
		: TransactionNode(pool),
		  reserveList(pool),
		  tpb(pool)
	{
	}

public:
	virtual void print(Firebird::string& text) const
	{
		text = "SetTransactionNode";
	}

	virtual SetTransactionNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void execute(thread_db* tdbb, dsql_req* request, jrd_tra** transaction) const;

private:
	void genTableLock(DsqlCompilerScratch* dsqlScratch, const RestrictionOption& tblLock,
		USHORT lockLevel);

public:
	Nullable<bool> readOnly;
	Nullable<bool> wait;
	Nullable<unsigned> isoLevel;
	Nullable<bool> noAutoUndo;
	Nullable<bool> ignoreLimbo;
	Nullable<bool> restartRequests;
	Nullable<USHORT> lockTimeout;
	Firebird::Array<RestrictionOption*> reserveList;
	Firebird::UCharBuffer tpb;
};


// This node should better be session management node,
// but as long as we do not have other session management and
// node is rather similiar internally to transaction management
// let it for now be transaction management node.
class SetRoleNode : public TransactionNode
{
public:
	explicit SetRoleNode(MemoryPool& pool)
		: TransactionNode(pool),
		  trusted(true),
		  roleName(pool)
	{
	}

	SetRoleNode(MemoryPool& pool, Firebird::MetaName* name)
		: TransactionNode(pool),
		  trusted(false),
		  roleName(pool, *name)
	{
	}

public:
	virtual void print(Firebird::string& text) const
	{
		text = "SetRoleNode";
	}

	virtual SetRoleNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void execute(thread_db* tdbb, dsql_req* request, jrd_tra** transaction) const;

public:
	bool trusted;
	Firebird::MetaName roleName;
};


class UpdateOrInsertNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_UPDATE_OR_INSERT>
{
public:
	explicit UpdateOrInsertNode(MemoryPool& pool)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_UPDATE_OR_INSERT>(pool),
		  relation(NULL),
		  fields(pool),
		  values(NULL),
		  matching(pool),
		  returning(NULL)
	{
	}

	virtual void print(Firebird::string& text) const;
	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

public:
	NestConst<RelationSourceNode> relation;
	Firebird::Array<NestConst<FieldNode> > fields;
	NestConst<ValueListNode> values;
	Firebird::Array<NestConst<FieldNode> > matching;
	NestConst<ReturningClause> returning;
};


} // namespace

#endif // DSQL_STMT_NODES_H
