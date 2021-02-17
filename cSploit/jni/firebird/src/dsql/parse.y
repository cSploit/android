%{
/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		parse.y
 *	DESCRIPTION:	Dynamic SQL parser
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
 *
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 * 2001.05.20 Neil McCalden: Allow a udf to be used in a 'group by' clause.
 * 2001.05.30 Claudio Valderrama: DROP TABLE and DROP VIEW lead now to two
 *   different node types so DDL can tell which is which.
 * 2001.06.13 Claudio Valderrama: SUBSTRING is being surfaced.
 * 2001.06.30 Claudio valderrama: Feed (line,column) for each node. See node.h.
 * 2001.07.10 Claudio Valderrama: Better (line,column) report and "--" for comments.
 * 2001.07.28 John Bellardo: Changes to support parsing LIMIT and FIRST
 * 2001.08.03 John Bellardo: Finalized syntax for LIMIT, change LIMIT to SKIP
 * 2001.08.05 Claudio Valderrama: closed Bug #448062 and other spaces that appear
 *   in rdb$*_source fields when altering domains plus one unexpected null pointer.
 * 2001.08.12 Claudio Valderrama: adjust SUBSTRING's starting pos argument here
 *   and not in gen.c; this closes Bug #450301.
 * 2001.10.01 Claudio Valderrama: enable explicit GRANT...to ROLE role_name.
 * 2001.10.06 Claudio Valderrama: Honor explicit USER keyword in GRANTs and REVOKEs.
 * 2002.07.05 Mark O'Donohue: change keyword DEBUG to KW_DEBUG to avoid
 *			clashes with normal DEBUG macro.
 * 2002.07.30 Arno Brinkman:
 * 2002.07.30 	Let IN predicate handle value_expressions
 * 2002.07.30 	tokens CASE, NULLIF, COALESCE added
 * 2002.07.30 	See block < CASE expression > what is added to value as case_expression
 * 2002.07.30 	function is split up into aggregate_function, numeric_value_function, string_value_function, generate_value_function
 * 2002.07.30 	new group_by_function and added to grp_column_elem
 * 2002.07.30 	cast removed from function and added as cast_specification to value
 * 2002.08.04 Claudio Valderrama: allow declaring and defining variables at the same time
 * 2002.08.04 Dmitry Yemanov: ALTER VIEW
 * 2002.08.06 Arno Brinkman: ordinal added to grp_column_elem for using positions in group by
 * 2002.08.07 Dmitry Yemanov: INT64/LARGEINT are replaced with BIGINT and available in dialect 3 only
 * 2002.08.31 Dmitry Yemanov: allowed user-defined index names for PK/FK/UK constraints
 * 2002.09.01 Dmitry Yemanov: RECREATE VIEW
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *							exception handling in SPs/triggers,
 *							implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.12.03 Dmitry Yemanov: Implemented ORDER BY clause in subqueries.
 * 2002.12.18 Dmitry Yemanov: Added support for SQL-compliant labels and LEAVE statement
 * 2002.12.28 Dmitry Yemanov: Added support for parametrized events.
 * 2003.01.14 Dmitry Yemanov: Fixed bug with cursors in triggers.
 * 2003.01.15 Dmitry Yemanov: Added support for runtime trigger action checks.
 * 2003.02.10 Mike Nordell  : Undefined Microsoft introduced macros to get a clean compile.
 * 2003.05.24 Nickolay Samofatov: Make SKIP and FIRST non-reserved keywords
 * 2003.06.13 Nickolay Samofatov: Make INSERTING/UPDATING/DELETING non-reserved keywords
 * 2003.07.01 Blas Rodriguez Somoza: Change DEBUG and IN to avoid conflicts in win32 build/bison
 * 2003.08.11 Arno Brinkman: Changed GROUP BY to support all expressions and added "AS" support
 *						   with table alias. Also removed group_by_function and ordinal.
 * 2003.08.14 Arno Brinkman: Added support for derived tables.
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL.
 * 2004.01.16 Vlad Horsun: added support for default parameters and
 *   EXECUTE BLOCK statement
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "dyn_consts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "gen/iberror.h"
#include "../dsql/dsql.h"
#include "../jrd/ibase.h"
#include "../jrd/flags.h"
#include "../jrd/jrd.h"
#include "../jrd/DataTypeUtil.h"
#include "../dsql/errd_proto.h"
#include "../dsql/make_proto.h"
#include "../yvalve/keywords.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/err_proto.h"
#include "../common/intlobj_new.h"
#include "../jrd/Attachment.h"
#include "../common/StatusArg.h"

// This is needed here to provide backward compatibility when working with SSPI plugin
#include "../auth/trusted/AuthSspi.h"

// since UNIX isn't standard, we have to define
// stuff which is in <limits.h> (which isn't available on all UNIXes...

const long SHRT_POS_MAX			= 32767;
const long SHRT_UNSIGNED_MAX	= 65535;
const long SHRT_NEG_MAX			= 32768;
const int POSITIVE	= 0;
const int NEGATIVE	= 1;
const int UNSIGNED	= 2;

//const int MIN_CACHE_BUFFERS	= 250;
//const int DEF_CACHE_BUFFERS	= 1000;

#define YYSTYPE YYSTYPE
#if defined(DEBUG) || defined(DEV_BUILD)
#define YYDEBUG		1
#endif

#define YYREDUCEPOSNFUNC yyReducePosn
#define YYREDUCEPOSNFUNCARG NULL


// ASF: Inherited attributes (aka rule parameters) are executed even when in trial mode, but action
// rules ({}) are executed only when in full parse mode. NOTRIAL should be used to avoid segfaults
// due to accessing invalid pointers in parameters (not yet returned from action rules).
#define NOTRIAL(x) (yytrial ? NULL : (x))


inline unsigned trigger_type_suffix(const unsigned slot1, const unsigned slot2, const unsigned slot3)
{
	return ((slot1 << 1) | (slot2 << 3) | (slot3 << 5));
}


#include "../dsql/chars.h"

using namespace Jrd;
using namespace Firebird;

%}


// token declarations

// Tokens are organized chronologically by date added.
// See yvalve/keywords.cpp for a list organized alphabetically

// Tokens in v4.0 -- not separated into v3 and v4 tokens

%token <metaNamePtr> ACTIVE
%token <metaNamePtr> ADD
%token <metaNamePtr> AFTER
%token <metaNamePtr> ALL
%token <metaNamePtr> ALTER
%token <metaNamePtr> AND
%token <metaNamePtr> ANY
%token <metaNamePtr> AS
%token <metaNamePtr> ASC
%token <metaNamePtr> AT
%token <metaNamePtr> AVG
%token <metaNamePtr> AUTO
%token <metaNamePtr> BEFORE
%token <metaNamePtr> BEGIN
%token <metaNamePtr> BETWEEN
%token <metaNamePtr> BLOB
%token <metaNamePtr> BY
%token <metaNamePtr> CAST
%token <metaNamePtr> CHARACTER
%token <metaNamePtr> CHECK
%token <metaNamePtr> COLLATE
%token <metaNamePtr> COMMIT
%token <metaNamePtr> COMMITTED
%token <metaNamePtr> COMPUTED
%token <metaNamePtr> CONCATENATE
%token <metaNamePtr> CONDITIONAL
%token <metaNamePtr> CONSTRAINT
%token <metaNamePtr> CONTAINING
%token <metaNamePtr> COUNT
%token <metaNamePtr> CREATE
%token <metaNamePtr> CSTRING
%token <metaNamePtr> CURRENT
%token <metaNamePtr> CURSOR
%token <metaNamePtr> DATABASE
%token <metaNamePtr> DATE
%token <metaNamePtr> DB_KEY
%token <metaNamePtr> DECIMAL
%token <metaNamePtr> DECLARE
%token <metaNamePtr> DEFAULT
%token <metaNamePtr> KW_DELETE
%token <metaNamePtr> DESC
%token <metaNamePtr> DISTINCT
%token <metaNamePtr> DO
%token <metaNamePtr> KW_DOMAIN
%token <metaNamePtr> DROP
%token <metaNamePtr> ELSE
%token <metaNamePtr> END
%token <metaNamePtr> ENTRY_POINT
%token <metaNamePtr> ESCAPE
%token <metaNamePtr> EXCEPTION
%token <metaNamePtr> EXECUTE
%token <metaNamePtr> EXISTS
%token <metaNamePtr> EXIT
%token <metaNamePtr> EXTERNAL
%token <metaNamePtr> FILTER
%token <metaNamePtr> FOR
%token <metaNamePtr> FOREIGN
%token <metaNamePtr> FROM
%token <metaNamePtr> FULL
%token <metaNamePtr> FUNCTION
%token <metaNamePtr> GDSCODE
%token <metaNamePtr> GEQ
%token <metaNamePtr> GENERATOR
%token <metaNamePtr> GEN_ID
%token <metaNamePtr> GRANT
%token <metaNamePtr> GROUP
%token <metaNamePtr> HAVING
%token <metaNamePtr> IF
%token <metaNamePtr> KW_IN
%token <metaNamePtr> INACTIVE
%token <metaNamePtr> INNER
%token <metaNamePtr> INPUT_TYPE
%token <metaNamePtr> INDEX
%token <metaNamePtr> INSERT
%token <metaNamePtr> INTEGER
%token <metaNamePtr> INTO
%token <metaNamePtr> IS
%token <metaNamePtr> ISOLATION
%token <metaNamePtr> JOIN
%token <metaNamePtr> KEY
%token <metaNamePtr> KW_CHAR
%token <metaNamePtr> KW_DEC
%token <metaNamePtr> KW_DOUBLE
%token <metaNamePtr> KW_FILE
%token <metaNamePtr> KW_FLOAT
%token <metaNamePtr> KW_INT
%token <metaNamePtr> KW_LONG
%token <metaNamePtr> KW_NULL
%token <metaNamePtr> KW_NUMERIC
%token <metaNamePtr> KW_UPPER
%token <metaNamePtr> KW_VALUE
%token <metaNamePtr> LENGTH
%token <metaNamePtr> LEFT
%token <metaNamePtr> LEQ
%token <metaNamePtr> LEVEL
%token <metaNamePtr> LIKE
%token <metaNamePtr> MANUAL
%token <metaNamePtr> MAXIMUM
%token <metaNamePtr> MERGE
%token <metaNamePtr> MINIMUM
%token <metaNamePtr> MODULE_NAME
%token <metaNamePtr> NAMES
%token <metaNamePtr> NATIONAL
%token <metaNamePtr> NATURAL
%token <metaNamePtr> NCHAR
%token <metaNamePtr> NEQ
%token <metaNamePtr> NO
%token <metaNamePtr> NOT
%token <metaNamePtr> NOT_GTR
%token <metaNamePtr> NOT_LSS
%token <metaNamePtr> OF
%token <metaNamePtr> ON
%token <metaNamePtr> ONLY
%token <metaNamePtr> OPTION
%token <metaNamePtr> OR
%token <metaNamePtr> ORDER
%token <metaNamePtr> OUTER
%token <metaNamePtr> OUTPUT_TYPE
%token <metaNamePtr> KW_OVERFLOW
%token <metaNamePtr> PAGE
%token <metaNamePtr> PAGES
%token <metaNamePtr> KW_PAGE_SIZE
%token <metaNamePtr> PARAMETER
%token <metaNamePtr> PASSWORD
%token <metaNamePtr> PLAN
%token <metaNamePtr> POSITION
%token <metaNamePtr> POST_EVENT
%token <metaNamePtr> PRECISION
%token <metaNamePtr> PRIMARY
%token <metaNamePtr> PRIVILEGES
%token <metaNamePtr> PROCEDURE
%token <metaNamePtr> PROTECTED
%token <metaNamePtr> READ
%token <metaNamePtr> REAL
%token <metaNamePtr> REFERENCES
%token <metaNamePtr> RESERVING
%token <metaNamePtr> RETAIN
%token <metaNamePtr> RETURNING_VALUES
%token <metaNamePtr> RETURNS
%token <metaNamePtr> REVOKE
%token <metaNamePtr> RIGHT
%token <metaNamePtr> ROLLBACK
%token <metaNamePtr> SEGMENT
%token <metaNamePtr> SELECT
%token <metaNamePtr> SET
%token <metaNamePtr> SHADOW
%token <metaNamePtr> KW_SHARED
%token <metaNamePtr> SINGULAR
%token <metaNamePtr> KW_SIZE
%token <metaNamePtr> SMALLINT
%token <metaNamePtr> SNAPSHOT
%token <metaNamePtr> SOME
%token <metaNamePtr> SORT
%token <metaNamePtr> SQLCODE
%token <metaNamePtr> STABILITY
%token <metaNamePtr> STARTING
%token <metaNamePtr> STATISTICS
%token <metaNamePtr> SUB_TYPE
%token <metaNamePtr> SUSPEND
%token <metaNamePtr> SUM
%token <metaNamePtr> TABLE
%token <metaNamePtr> THEN
%token <metaNamePtr> TO
%token <metaNamePtr> TRANSACTION
%token <metaNamePtr> TRIGGER
%token <metaNamePtr> UNCOMMITTED
%token <metaNamePtr> UNION
%token <metaNamePtr> UNIQUE
%token <metaNamePtr> UPDATE
%token <metaNamePtr> USER
%token <metaNamePtr> VALUES
%token <metaNamePtr> VARCHAR
%token <metaNamePtr> VARIABLE
%token <metaNamePtr> VARYING
%token <metaNamePtr> VERSION
%token <metaNamePtr> VIEW
%token <metaNamePtr> WAIT
%token <metaNamePtr> WHEN
%token <metaNamePtr> WHERE
%token <metaNamePtr> WHILE
%token <metaNamePtr> WITH
%token <metaNamePtr> WORK
%token <metaNamePtr> WRITE

%token <stringPtr> FLOAT_NUMBER
%token <metaNamePtr> SYMBOL
%token <int32Val> NUMBER

%token <intlStringPtr> STRING
%token <metaNamePtr> INTRODUCER

// New tokens added v5.0

%token <metaNamePtr> ACTION
%token <metaNamePtr> ADMIN
%token <metaNamePtr> CASCADE
%token <metaNamePtr> FREE_IT			// ISC SQL extension
%token <metaNamePtr> RESTRICT
%token <metaNamePtr> ROLE

// New tokens added v6.0

%token <metaNamePtr> COLUMN
%token <metaNamePtr> KW_TYPE
%token <metaNamePtr> EXTRACT
%token <metaNamePtr> YEAR
%token <metaNamePtr> MONTH
%token <metaNamePtr> DAY
%token <metaNamePtr> HOUR
%token <metaNamePtr> MINUTE
%token <metaNamePtr> SECOND
%token <metaNamePtr> WEEKDAY			// ISC SQL extension
%token <metaNamePtr> YEARDAY			// ISC SQL extension
%token <metaNamePtr> TIME
%token <metaNamePtr> TIMESTAMP
%token <metaNamePtr> CURRENT_DATE
%token <metaNamePtr> CURRENT_TIME
%token <metaNamePtr> CURRENT_TIMESTAMP

// special aggregate token types returned by lex in v6.0

%token <scaledNumber> NUMBER64BIT SCALEDINT

// CVC: Special Firebird additions.

%token <metaNamePtr> CURRENT_USER
%token <metaNamePtr> CURRENT_ROLE
%token <metaNamePtr> KW_BREAK
%token <metaNamePtr> SUBSTRING
%token <metaNamePtr> RECREATE
%token <metaNamePtr> KW_DESCRIPTOR
%token <metaNamePtr> FIRST
%token <metaNamePtr> SKIP

// tokens added for Firebird 1.5

%token <metaNamePtr> CURRENT_CONNECTION
%token <metaNamePtr> CURRENT_TRANSACTION
%token <metaNamePtr> BIGINT
%token <metaNamePtr> CASE
%token <metaNamePtr> NULLIF
%token <metaNamePtr> COALESCE
%token <metaNamePtr> USING
%token <metaNamePtr> NULLS
%token <metaNamePtr> LAST
%token <metaNamePtr> ROW_COUNT
%token <metaNamePtr> LOCK
%token <metaNamePtr> SAVEPOINT
%token <metaNamePtr> RELEASE
%token <metaNamePtr> STATEMENT
%token <metaNamePtr> LEAVE
%token <metaNamePtr> INSERTING
%token <metaNamePtr> UPDATING
%token <metaNamePtr> DELETING

// tokens added for Firebird 2.0

%token <metaNamePtr> BACKUP
%token <metaNamePtr> KW_DIFFERENCE
%token <metaNamePtr> OPEN
%token <metaNamePtr> CLOSE
%token <metaNamePtr> FETCH
%token <metaNamePtr> ROWS
%token <metaNamePtr> BLOCK
%token <metaNamePtr> IIF
%token <metaNamePtr> SCALAR_ARRAY
%token <metaNamePtr> CROSS
%token <metaNamePtr> NEXT
%token <metaNamePtr> SEQUENCE
%token <metaNamePtr> RESTART
%token <metaNamePtr> BOTH
%token <metaNamePtr> COLLATION
%token <metaNamePtr> COMMENT
%token <metaNamePtr> BIT_LENGTH
%token <metaNamePtr> CHAR_LENGTH
%token <metaNamePtr> CHARACTER_LENGTH
%token <metaNamePtr> LEADING
%token <metaNamePtr> KW_LOWER
%token <metaNamePtr> OCTET_LENGTH
%token <metaNamePtr> TRAILING
%token <metaNamePtr> TRIM
%token <metaNamePtr> RETURNING
%token <metaNamePtr> KW_IGNORE
%token <metaNamePtr> LIMBO
%token <metaNamePtr> UNDO
%token <metaNamePtr> REQUESTS
%token <metaNamePtr> TIMEOUT

// tokens added for Firebird 2.1

%token <metaNamePtr> ABS
%token <metaNamePtr> ACCENT
%token <metaNamePtr> ACOS
%token <metaNamePtr> ALWAYS
%token <metaNamePtr> ASCII_CHAR
%token <metaNamePtr> ASCII_VAL
%token <metaNamePtr> ASIN
%token <metaNamePtr> ATAN
%token <metaNamePtr> ATAN2
%token <metaNamePtr> BIN_AND
%token <metaNamePtr> BIN_OR
%token <metaNamePtr> BIN_SHL
%token <metaNamePtr> BIN_SHR
%token <metaNamePtr> BIN_XOR
%token <metaNamePtr> CEIL
%token <metaNamePtr> CONNECT
%token <metaNamePtr> COS
%token <metaNamePtr> COSH
%token <metaNamePtr> COT
%token <metaNamePtr> DATEADD
%token <metaNamePtr> DATEDIFF
%token <metaNamePtr> DECODE
%token <metaNamePtr> DISCONNECT
%token <metaNamePtr> EXP
%token <metaNamePtr> FLOOR
%token <metaNamePtr> GEN_UUID
%token <metaNamePtr> GENERATED
%token <metaNamePtr> GLOBAL
%token <metaNamePtr> HASH
%token <metaNamePtr> INSENSITIVE
%token <metaNamePtr> LIST
%token <metaNamePtr> LN
%token <metaNamePtr> LOG
%token <metaNamePtr> LOG10
%token <metaNamePtr> LPAD
%token <metaNamePtr> MATCHED
%token <metaNamePtr> MATCHING
%token <metaNamePtr> MAXVALUE
%token <metaNamePtr> MILLISECOND
%token <metaNamePtr> MINVALUE
%token <metaNamePtr> MOD
%token <metaNamePtr> OVERLAY
%token <metaNamePtr> PAD
%token <metaNamePtr> PI
%token <metaNamePtr> PLACING
%token <metaNamePtr> POWER
%token <metaNamePtr> PRESERVE
%token <metaNamePtr> RAND
%token <metaNamePtr> RECURSIVE
%token <metaNamePtr> REPLACE
%token <metaNamePtr> REVERSE
%token <metaNamePtr> ROUND
%token <metaNamePtr> RPAD
%token <metaNamePtr> SENSITIVE
%token <metaNamePtr> SIGN
%token <metaNamePtr> SIN
%token <metaNamePtr> SINH
%token <metaNamePtr> SPACE
%token <metaNamePtr> SQRT
%token <metaNamePtr> START
%token <metaNamePtr> TAN
%token <metaNamePtr> TANH
%token <metaNamePtr> TEMPORARY
%token <metaNamePtr> TRUNC
%token <metaNamePtr> WEEK

// tokens added for Firebird 2.5

%token <metaNamePtr> AUTONOMOUS
%token <metaNamePtr> CHAR_TO_UUID
%token <metaNamePtr> FIRSTNAME
%token <metaNamePtr> GRANTED
%token <metaNamePtr> LASTNAME
%token <metaNamePtr> MIDDLENAME
%token <metaNamePtr> MAPPING
%token <metaNamePtr> OS_NAME
%token <metaNamePtr> SIMILAR
%token <metaNamePtr> UUID_TO_CHAR
// new execute statement
%token <metaNamePtr> CALLER
%token <metaNamePtr> COMMON
%token <metaNamePtr> DATA
%token <metaNamePtr> SOURCE
%token <metaNamePtr> TWO_PHASE
%token <metaNamePtr> BIND_PARAM
%token <metaNamePtr> BIN_NOT

// tokens added for Firebird 3.0

%token <metaNamePtr> BODY
%token <metaNamePtr> CONTINUE
%token <metaNamePtr> DDL
%token <metaNamePtr> DECRYPT
%token <metaNamePtr> ENCRYPT
%token <metaNamePtr> ENGINE
%token <metaNamePtr> NAME
%token <metaNamePtr> OVER
%token <metaNamePtr> PACKAGE
%token <metaNamePtr> PARTITION
%token <metaNamePtr> RDB_GET_CONTEXT
%token <metaNamePtr> RDB_SET_CONTEXT
%token <metaNamePtr> SCROLL
%token <metaNamePtr> PRIOR
%token <metaNamePtr> KW_ABSOLUTE
%token <metaNamePtr> KW_RELATIVE
%token <metaNamePtr> ACOSH
%token <metaNamePtr> ASINH
%token <metaNamePtr> ATANH
%token <metaNamePtr> RETURN
%token <metaNamePtr> DETERMINISTIC
%token <metaNamePtr> IDENTITY
%token <metaNamePtr> DENSE_RANK
%token <metaNamePtr> FIRST_VALUE
%token <metaNamePtr> NTH_VALUE
%token <metaNamePtr> LAST_VALUE
%token <metaNamePtr> LAG
%token <metaNamePtr> LEAD
%token <metaNamePtr> RANK
%token <metaNamePtr> ROW_NUMBER
%token <metaNamePtr> SQLSTATE
%token <metaNamePtr> KW_BOOLEAN
%token <metaNamePtr> KW_FALSE
%token <metaNamePtr> KW_TRUE
%token <metaNamePtr> UNKNOWN
%token <metaNamePtr> USAGE
%token <metaNamePtr> RDB_RECORD_VERSION
%token <metaNamePtr> LINGER
%token <metaNamePtr> TAGS
%token <metaNamePtr> PLUGIN
%token <metaNamePtr> SERVERWIDE
%token <metaNamePtr> INCREMENT
%token <metaNamePtr> TRUSTED

// precedence declarations for expression evaluation

%left	OR
%left	AND
%left	NOT
%left	'=' '<' '>' IS BETWEEN LIKE CONTAINING STARTING SIMILAR KW_IN NEQ GEQ LEQ NOT_GTR NOT_LSS
%left	'+' '-'
%left	'*' '/'
%left	UMINUS UPLUS
%left	CONCATENATE
%left	COLLATE

// Fix the dangling IF-THEN-ELSE problem
%nonassoc THEN
%nonassoc ELSE

/* The same issue exists with ALTER COLUMN now that keywords can be used
   in order to change their names.  The syntax which shows the issue is:
	 ALTER COLUMN where column is part of the alter statement
	   or
	 ALTER COLUMN where column is the name of the column in the relation
*/
%nonassoc ALTER
%nonassoc COLUMN

%union
{
	BaseNullable<int> nullableIntVal;
	BaseNullable<bool> nullableBoolVal;
	bool boolVal;
	int intVal;
	unsigned uintVal;
	SLONG int32Val;
	BaseNullable<SLONG> nullableInt32Val;
	SINT64 int64Val;
	FB_UINT64 uint64Val;
	BaseNullable<SINT64> nullableInt64Val;
	BaseNullable<BaseNullable<SINT64> > nullableNullableInt64Val;
	BaseNullable<FB_UINT64> nullableUint64Val;
	Jrd::ScaledNumber scaledNumber;
	UCHAR blrOp;
	Jrd::OrderNode::NullsPlacement nullsPlacement;
	Jrd::ComparativeBoolNode::DsqlFlag cmpBoolFlag;
	Jrd::dsql_fld* legacyField;
	Jrd::ReturningClause* returningClause;
	Firebird::MetaName* metaNamePtr;
	Firebird::ObjectsArray<Firebird::MetaName>* metaNameArray;
	Firebird::PathName* pathNamePtr;
	Firebird::string* stringPtr;
	Jrd::IntlString* intlStringPtr;
	Jrd::DbFileClause* dbFileClause;
	Firebird::Array<NestConst<Jrd::DbFileClause> >* dbFilesClause;
	Jrd::ExternalClause* externalClause;
	Firebird::Array<NestConst<Jrd::ParameterClause> >* parametersClause;
	Jrd::Node* node;
	Jrd::ExprNode* exprNode;
	Jrd::ValueExprNode* valueExprNode;
	Jrd::BoolExprNode* boolExprNode;
	Jrd::RecordSourceNode* recSourceNode;
	Jrd::RelationSourceNode* relSourceNode;
	Jrd::ValueListNode* valueListNode;
	Jrd::RecSourceListNode* recSourceListNode;
	Jrd::RseNode* rseNode;
	Jrd::PlanNode* planNode;
	Jrd::PlanNode::AccessType* accessType;
	Jrd::StmtNode* stmtNode;
	Jrd::DdlNode* ddlNode;
	Jrd::SelectExprNode* selectExprNode;
	Jrd::WithClause* withClause;
	Jrd::RowsClause* rowsClause;
	Jrd::FieldNode* fieldNode;
	Jrd::DecodeNode* decodeNode;
	Firebird::Array<Jrd::FieldNode*>* fieldArray;
	Firebird::Array<NestConst<Jrd::FieldNode> >* nestFieldArray;
	Jrd::TransactionNode* traNode;
	Firebird::Array<Jrd::PrivilegeClause>* privilegeArray;
	Jrd::GranteeClause* granteeClause;
	Firebird::Array<Jrd::GranteeClause>* granteeArray;
	Jrd::GrantRevokeNode* grantRevokeNode;
	Jrd::CreateCollationNode* createCollationNode;
	Jrd::CreateDomainNode* createDomainNode;
	Jrd::AlterDomainNode* alterDomainNode;
	Jrd::CreateAlterFunctionNode* createAlterFunctionNode;
	Jrd::CreateAlterProcedureNode* createAlterProcedureNode;
	Jrd::CreateAlterTriggerNode* createAlterTriggerNode;
	Jrd::CreateAlterPackageNode* createAlterPackageNode;
	Jrd::CreateFilterNode::NameNumber* filterNameNumber;
	Jrd::CreateAlterExceptionNode* createAlterExceptionNode;
	Jrd::CreateAlterSequenceNode* createAlterSequenceNode;
	Jrd::CreateShadowNode* createShadowNode;
	Firebird::Array<Jrd::CreateAlterPackageNode::Item>* packageItems;
	Jrd::ExceptionArray* exceptionArray;
	Jrd::CreateAlterPackageNode::Item packageItem;
	Jrd::CreatePackageBodyNode* createPackageBodyNode;
	Jrd::BoolSourceClause* boolSourceClause;
	Jrd::ValueSourceClause* valueSourceClause;
	Jrd::RelationNode* relationNode;
	Jrd::RelationNode::AddColumnClause* addColumnClause;
	Jrd::RelationNode::RefActionClause* refActionClause;
	Jrd::RelationNode::IndexConstraintClause* indexConstraintClause;
	Jrd::CreateRelationNode* createRelationNode;
	Jrd::CreateAlterViewNode* createAlterViewNode;
	Jrd::CreateIndexNode* createIndexNode;
	Jrd::AlterDatabaseNode* alterDatabaseNode;
	Jrd::ExecBlockNode* execBlockNode;
	Jrd::StoreNode* storeNode;
	Jrd::UpdateOrInsertNode* updInsNode;
	Jrd::AggNode* aggNode;
	Jrd::SysFuncCallNode* sysFuncCallNode;
	Jrd::ValueIfNode* valueIfNode;
	Jrd::CompoundStmtNode* compoundStmtNode;
	Jrd::CursorStmtNode* cursorStmtNode;
	Jrd::DeclareCursorNode* declCursorNode;
	Jrd::ErrorHandlerNode* errorHandlerNode;
	Jrd::ExecStatementNode* execStatementNode;
	Jrd::MergeNode* mergeNode;
	Jrd::MergeNode::NotMatched* mergeNotMatchedClause;
	Jrd::MergeNode::Matched* mergeMatchedClause;
	Jrd::SelectNode* selectNode;
	Jrd::SetTransactionNode* setTransactionNode;
	Jrd::SetTransactionNode::RestrictionOption* setTransactionRestrictionClause;
	Jrd::DeclareSubProcNode* declareSubProcNode;
	Jrd::DeclareSubFuncNode* declareSubFuncNode;
	Jrd::dsql_req* dsqlReq;
	Jrd::CreateAlterUserNode* createAlterUserNode;
	Jrd::MappingNode* mappingNode;
	Jrd::MappingNode::OP mappingOp;
	Jrd::SetRoleNode* setRoleNode;
}

%include types.y

%%

// list of possible statements

top
	: statement			{ DSQL_parse = $1; }
	| statement ';'		{ DSQL_parse = $1; }
	;

%type <dsqlReq> statement
statement
	: dml_statement		{ $$ = newNode<DsqlDmlRequest>($1); }
	| ddl_statement		{ $$ = newNode<DsqlDdlRequest>($1); }
	| tra_statement		{ $$ = newNode<DsqlTransactionRequest>($1); }
	;

%type <stmtNode> dml_statement
dml_statement
	: delete									{ $$ = $1; }
	| insert									{ $$ = $1; }
	| merge										{ $$ = $1; }
	| exec_procedure							{ $$ = $1; }
	| exec_block								{ $$ = $1; }
	| savepoint									{ $$ = $1; }
	| select									{ $$ = $1; }
	| update									{ $$ = $1; }
	| update_or_insert							{ $$ = $1; }
	;

%type <ddlNode> ddl_statement
ddl_statement
	: alter										{ $$ = $1; }
	| comment									{ $$ = $1; }
	| create									{ $$ = $1; }
	| create_or_alter							{ $$ = $1; }
	| declare									{ $$ = $1; }
	| drop										{ $$ = $1; }
	| grant										{ $$ = $1; }
	| recreate									{ $$ = $1; }
	| revoke									{ $$ = $1; }
	| set_statistics							{ $$ = $1; }
	;

%type <traNode> tra_statement
tra_statement
	: set_transaction							{ $$ = $1; }
	| commit									{ $$ = $1; }
	| rollback									{ $$ = $1; }
	| set_role									{ $$ = $1; }
	;


// GRANT statement

%type <grantRevokeNode> grant
grant
	: GRANT
			{ $$ = newNode<GrantRevokeNode>(true); }
		grant0($2)
			{ $$ = $2; }
	;

%type grant0(<grantRevokeNode>)
grant0($node)
	: privileges(NOTRIAL(&$node->privileges)) ON table_noise symbol_table_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_relation, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| execute_privilege(NOTRIAL(&$node->privileges)) ON PROCEDURE symbol_procedure_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_procedure, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| execute_privilege(NOTRIAL(&$node->privileges)) ON FUNCTION symbol_UDF_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_udf, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| execute_privilege(NOTRIAL(&$node->privileges)) ON PACKAGE symbol_package_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_package_header, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| usage_privilege(NOTRIAL(&$node->privileges)) ON KW_DOMAIN symbol_domain_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_field, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| usage_privilege(NOTRIAL(&$node->privileges)) ON EXCEPTION symbol_exception_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_exception, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| usage_privilege(NOTRIAL(&$node->privileges)) ON GENERATOR symbol_generator_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_generator, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| usage_privilege(NOTRIAL(&$node->privileges)) ON SEQUENCE symbol_generator_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_generator, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| usage_privilege(NOTRIAL(&$node->privileges)) ON CHARACTER SET symbol_character_set_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_charset, *$5);
			$node->grantAdminOption = $8;
			$node->grantor = $9;
		}
	| usage_privilege(NOTRIAL(&$node->privileges)) ON COLLATION symbol_collation_name
			TO non_role_grantee_list(NOTRIAL(&$node->users)) grant_option granted_by
		{
			$node->object = newNode<GranteeClause>(obj_collation, *$4);
			$node->grantAdminOption = $7;
			$node->grantor = $8;
		}
	| role_name_list(NOTRIAL(&$node->roles)) TO role_grantee_list(NOTRIAL(&$node->users))
			role_admin_option granted_by
		{
			$node->grantAdminOption = $4;
			$node->grantor = $5;
		}
	;

table_noise
	: // nothing
	| TABLE
	;

%type privileges(<privilegeArray>)
privileges($privilegeArray)
	: ALL				{ $privilegeArray->add(PrivilegeClause('A', NULL)); }
	| ALL PRIVILEGES	{ $privilegeArray->add(PrivilegeClause('A', NULL)); }
	| privilege_list($privilegeArray)
	;

%type privilege_list(<privilegeArray>)
privilege_list($privilegeArray)
	: privilege($privilegeArray)
	| privilege_list ',' privilege($privilegeArray)
	;

%type execute_privilege(<privilegeArray>)
execute_privilege($privilegeArray)
	: EXECUTE						{ $privilegeArray->add(PrivilegeClause('X', NULL)); }
	;

%type usage_privilege(<privilegeArray>)
usage_privilege($privilegeArray)
	: USAGE							{ $privilegeArray->add(PrivilegeClause('G', NULL)); }
	;

%type privilege(<privilegeArray>)
privilege($privilegeArray)
	: SELECT						{ $privilegeArray->add(PrivilegeClause('S', NULL)); }
	| INSERT						{ $privilegeArray->add(PrivilegeClause('I', NULL)); }
	| KW_DELETE						{ $privilegeArray->add(PrivilegeClause('D', NULL)); }
	| UPDATE column_parens_opt		{ $privilegeArray->add(PrivilegeClause('U', $2)); }
	| REFERENCES column_parens_opt	{ $privilegeArray->add(PrivilegeClause('R', $2)); }
	;

%type <boolVal> grant_option
grant_option
	: /* nothing */			{ $$ = false; }
	| WITH GRANT OPTION		{ $$ = true; }
	;

%type <boolVal> role_admin_option
role_admin_option
	: /* nothing */			{ $$ = false; }
	| WITH ADMIN OPTION		{ $$ = true; }
	;

%type <metaNamePtr> granted_by
granted_by
	: /* nothing */				{ $$ = NULL; }
	| granted_by_text grantor	{ $$ = $2; }
	;

granted_by_text
	: GRANTED BY
	| AS
	;

%type <metaNamePtr> grantor
grantor
	: symbol_user_name
	| USER symbol_user_name		{ $$ = $2; }
	;


// REVOKE statement

%type <grantRevokeNode> revoke
revoke
	: REVOKE
			{ $$ = newNode<GrantRevokeNode>(false); }
		revoke0($2)
			{ $$ = $2; }
	;

%type revoke0(<grantRevokeNode>)
revoke0($node)
	: rev_grant_option privileges(NOTRIAL(&$node->privileges)) ON table_noise symbol_table_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_relation, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}

	| rev_grant_option execute_privilege(NOTRIAL(&$node->privileges)) ON PROCEDURE symbol_procedure_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_procedure, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option execute_privilege(NOTRIAL(&$node->privileges)) ON FUNCTION symbol_UDF_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_udf, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option execute_privilege(NOTRIAL(&$node->privileges)) ON PACKAGE symbol_package_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_package_header, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option usage_privilege(NOTRIAL(&$node->privileges)) ON KW_DOMAIN symbol_domain_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_field, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option usage_privilege(NOTRIAL(&$node->privileges)) ON EXCEPTION symbol_exception_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_exception, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option usage_privilege(NOTRIAL(&$node->privileges)) ON GENERATOR symbol_generator_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_generator, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option usage_privilege(NOTRIAL(&$node->privileges)) ON SEQUENCE symbol_generator_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_generator, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_grant_option usage_privilege(NOTRIAL(&$node->privileges)) ON CHARACTER SET symbol_character_set_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_charset, *$6);
			$node->grantAdminOption = $1;
			$node->grantor = $9;
		}
	| rev_grant_option usage_privilege(NOTRIAL(&$node->privileges)) ON COLLATION symbol_collation_name
			FROM non_role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->object = newNode<GranteeClause>(obj_collation, *$5);
			$node->grantAdminOption = $1;
			$node->grantor = $8;
		}
	| rev_admin_option role_name_list(NOTRIAL(&$node->roles))
			FROM role_grantee_list(NOTRIAL(&$node->users)) granted_by
		{
			$node->grantAdminOption = $1;
			$node->grantor = $5;
		}
	| ALL ON ALL FROM non_role_grantee_list(NOTRIAL(&$node->users))
	;

%type <boolVal> rev_grant_option
rev_grant_option
	: /* nothing */			{ $$ = false; }
	| GRANT OPTION FOR		{ $$ = true; }
	;

%type <boolVal> rev_admin_option
rev_admin_option
	: /* nothing */			{ $$ = false; }
	| ADMIN OPTION FOR		{ $$ = true; }
	;

%type non_role_grantee_list(<granteeArray>)
non_role_grantee_list($granteeArray)
	: grantee($granteeArray)
	| user_grantee($granteeArray)
	| non_role_grantee_list ',' grantee($granteeArray)
	| non_role_grantee_list ',' user_grantee($granteeArray)
	;

%type grantee(<granteeArray>)
grantee($granteeArray)
	: PROCEDURE symbol_procedure_name
		{ $granteeArray->add(GranteeClause(obj_procedure, *$2)); }
	| FUNCTION symbol_UDF_name
		{ $granteeArray->add(GranteeClause(obj_udf, *$2)); }
	| PACKAGE symbol_package_name
		{ $granteeArray->add(GranteeClause(obj_package_header, *$2)); }
	| TRIGGER symbol_trigger_name
		{ $granteeArray->add(GranteeClause(obj_trigger, *$2)); }
	| VIEW symbol_view_name
		{ $granteeArray->add(GranteeClause(obj_view, *$2)); }
	| ROLE symbol_role_name
		{ $granteeArray->add(GranteeClause(obj_sql_role, *$2)); }
	;

// CVC: In the future we can deprecate the first implicit form since we'll support
// explicit grant/revoke for both USER and ROLE keywords & object types.

%type user_grantee(<granteeArray>)
user_grantee($granteeArray)
	: symbol_user_name
		{ $granteeArray->add(GranteeClause(obj_user_or_role, *$1)); }
	| USER symbol_user_name
		{ $granteeArray->add(GranteeClause(obj_user, *$2)); }
	| GROUP symbol_user_name
		{ $granteeArray->add(GranteeClause(obj_user_group, *$2)); }
	;

%type role_name_list(<granteeArray>)
role_name_list($granteeArray)
	: role_name($granteeArray)
	| role_name_list ',' role_name($granteeArray)
	;

%type role_name(<granteeArray>)
role_name($granteeArray)
	: symbol_role_name
		{ $granteeArray->add(GranteeClause(obj_sql_role, *$1)); }
	;

%type role_grantee_list(<granteeArray>)
role_grantee_list($granteeArray)
	: role_grantee($granteeArray)
	| role_grantee_list ',' role_grantee($granteeArray)
	;

%type role_grantee(<granteeArray>)
role_grantee($granteeArray)
	: grantor	{ $granteeArray->add(GranteeClause(obj_user, *$1)); }
	;


// DECLARE operations

%type <ddlNode> declare
declare
	: DECLARE declare_clause	{ $$ = $2;}
	;

%type <ddlNode> declare_clause
declare_clause
	: FILTER filter_decl_clause				{ $$ = $2; }
	| EXTERNAL FUNCTION udf_decl_clause		{ $$ = $3; }
	;

%type <createAlterFunctionNode> udf_decl_clause
udf_decl_clause
	: symbol_UDF_name
			{ $$ = newNode<CreateAlterFunctionNode>(*$1); }
		arg_desc_list1(NOTRIAL(&$2->parameters))
		RETURNS return_value1($2)
		ENTRY_POINT utf_string MODULE_NAME utf_string
			{
				$$ = $2;
				$$->external = newNode<ExternalClause>();
				$$->external->name = *$7;
				$$->external->udfModule = *$9;
			}
	;

%type <legacyField> udf_data_type
udf_data_type
	: simple_type
	| BLOB
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_blob;
			$$->length = sizeof(bid);
		}
	| CSTRING '(' pos_short_integer ')' charset_clause
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_cstring;
			$$->charLength = (USHORT) $3;
			if ($5)
				$$->charSet = *$5;
		}
	;

%type arg_desc_list1(<parametersClause>)
arg_desc_list1($parameters)
	:
	| arg_desc_list($parameters)
	| '(' arg_desc_list($parameters) ')'
	;

%type arg_desc_list(<parametersClause>)
arg_desc_list($parameters)
	: arg_desc($parameters)
	| arg_desc_list ',' arg_desc($parameters)
	;

%type arg_desc(<parametersClause>)
arg_desc($parameters)
	: udf_data_type param_mechanism
		{
			$parameters->add(newNode<ParameterClause>($1, MetaName()));
			$parameters->back()->udfMechanism = $2;
		}
	;

%type <nullableIntVal> param_mechanism
param_mechanism
	: /* nothing */		{ $$ = Nullable<int>::empty(); }	// Beware: This means FUN_reference or FUN_blob_struct.
	| BY KW_DESCRIPTOR	{ $$ = Nullable<int>::val(FUN_descriptor); }
	| BY SCALAR_ARRAY	{ $$ = Nullable<int>::val(FUN_scalar_array); }
	| KW_NULL			{ $$ = Nullable<int>::val(FUN_ref_with_null); }
	;

%type return_value1(<createAlterFunctionNode>)
return_value1($function)
	: return_value($function)
	| '(' return_value($function) ')'
	;

%type return_value(<createAlterFunctionNode>)
return_value($function)
	: udf_data_type return_mechanism
		{
			$function->returnType = newNode<ParameterClause>($1, MetaName());
			$function->returnType->udfMechanism = $2;
		}
	| PARAMETER pos_short_integer
		{ $function->udfReturnPos = $2; }
	;

%type <int32Val> return_mechanism
return_mechanism
	: /* nothing */				{ $$ = FUN_reference; }
	| BY KW_VALUE				{ $$ = FUN_value; }
	| BY KW_DESCRIPTOR			{ $$ = FUN_descriptor; }
	// FUN_refrence with FREE_IT is -ve
	| FREE_IT					{ $$ = -1 * FUN_reference; }
	| BY KW_DESCRIPTOR FREE_IT	{ $$ = -1 * FUN_descriptor; }
	;


%type <ddlNode> filter_decl_clause
filter_decl_clause
	: symbol_filter_name
		INPUT_TYPE blob_filter_subtype
		OUTPUT_TYPE blob_filter_subtype
		ENTRY_POINT utf_string MODULE_NAME utf_string
			{
				CreateFilterNode* node = newNode<CreateFilterNode>(*$1);
				node->inputFilter = $3;
				node->outputFilter = $5;
				node->entryPoint = *$7;
				node->moduleName = *$9;
				$$ = node;
			}
	;

%type <filterNameNumber> blob_filter_subtype
blob_filter_subtype
	: symbol_blob_subtype_name
		{ $$ = newNode<CreateFilterNode::NameNumber>(*$1); }
	| signed_short_integer
		{ $$ = newNode<CreateFilterNode::NameNumber>($1); }
	;

// CREATE metadata operations

%type <ddlNode> create
create
	: CREATE create_clause		{ $$ = $2; }
	;

%type <ddlNode> create_clause
create_clause
	: EXCEPTION exception_clause				{ $$ = $2; }
	| unique_opt order_direction INDEX symbol_index_name ON simple_table_name
			{
				CreateIndexNode* node = newNode<CreateIndexNode>(*$4);
				node->unique = $1;
				node->descending = $2;
				node->relation = $6;
				$$ = node;
			}
		index_definition(static_cast<CreateIndexNode*>($7))
			{
				$$ = $7;
			}
	| FUNCTION function_clause					{ $$ = $2; }
	| PROCEDURE procedure_clause				{ $$ = $2; }
	| TABLE table_clause						{ $$ = $2; }
	| GLOBAL TEMPORARY TABLE gtt_table_clause	{ $$ = $4; }
	| TRIGGER trigger_clause					{ $$ = $2; }
	| VIEW view_clause							{ $$ = $2; }
	| GENERATOR generator_clause				{ $$ = $2; }
	| SEQUENCE generator_clause					{ $$ = $2; }
	| DATABASE db_clause						{ $$ = $2; }
	| KW_DOMAIN domain_clause					{ $$ = $2; }
	| SHADOW shadow_clause						{ $$ = $2; }
	| ROLE role_clause							{ $$ = $2; }
	| COLLATION collation_clause				{ $$ = $2; }
	| USER create_user_clause					{ $$ = $2; }
	| PACKAGE package_clause					{ $$ = $2; }
	| PACKAGE BODY package_body_clause			{ $$ = $3; }
	| MAPPING create_map_clause(false)			{ $$ = $2; }
	| GLOBAL MAPPING create_map_clause(true)	{ $$ = $3; }
	;


%type <ddlNode> recreate
recreate
	: RECREATE recreate_clause		{ $$ = $2; }
	;

%type <ddlNode> recreate_clause
recreate_clause
	: PROCEDURE procedure_clause
		{ $$ = newNode<RecreateProcedureNode>($2); }
	| FUNCTION function_clause
		{ $$ = newNode<RecreateFunctionNode>($2); }
	| TABLE table_clause
		{ $$ = newNode<RecreateTableNode>($2); }
	| GLOBAL TEMPORARY TABLE gtt_table_clause
		{ $$ = newNode<RecreateTableNode>($4); }
	| VIEW view_clause
		{ $$ = newNode<RecreateViewNode>($2); }
	| TRIGGER trigger_clause
		{ $$ = newNode<RecreateTriggerNode>($2); }
	| PACKAGE package_clause
		{ $$ = newNode<RecreatePackageNode>($2); }
	| PACKAGE BODY package_body_clause
		{ $$ = newNode<RecreatePackageBodyNode>($3); }
	| EXCEPTION exception_clause
		{ $$ = newNode<RecreateExceptionNode>($2); }
	| GENERATOR generator_clause
		{ $$ = newNode<RecreateSequenceNode>($2); }
	| SEQUENCE generator_clause
		{ $$ = newNode<RecreateSequenceNode>($2); }
	;

%type <ddlNode> create_or_alter
create_or_alter
	: CREATE OR ALTER replace_clause		{ $$ = $4; }
	;

%type <ddlNode> replace_clause
replace_clause
	: PROCEDURE replace_procedure_clause		{ $$ = $2; }
	| FUNCTION replace_function_clause			{ $$ = $2; }
	| TRIGGER replace_trigger_clause			{ $$ = $2; }
	| PACKAGE replace_package_clause			{ $$ = $2; }
	| VIEW replace_view_clause					{ $$ = $2; }
	| EXCEPTION replace_exception_clause		{ $$ = $2; }
	| GENERATOR replace_sequence_clause			{ $$ = $2; }
	| SEQUENCE replace_sequence_clause			{ $$ = $2; }
	| USER replace_user_clause					{ $$ = $2; }
	| MAPPING replace_map_clause(false)			{ $$ = $2; }
	| GLOBAL MAPPING replace_map_clause(true)	{ $$ = $3; }
	;


// CREATE EXCEPTION
// ASF: The charset from sql_string is discarded because the database column uses NONE.

%type <createAlterExceptionNode> exception_clause
exception_clause
	: symbol_exception_name sql_string
		{ $$ = newNode<CreateAlterExceptionNode>(*$1, $2->getString()); }
	;

%type <createAlterExceptionNode> replace_exception_clause
replace_exception_clause
	: symbol_exception_name sql_string
		{
			CreateAlterExceptionNode* node = newNode<CreateAlterExceptionNode>(*$1, $2->getString());
			node->alter = true;
			$$ = node;
		}
	;

%type <createAlterExceptionNode> alter_exception_clause
alter_exception_clause
	: symbol_exception_name sql_string
		{
			CreateAlterExceptionNode* node = newNode<CreateAlterExceptionNode>(*$1, $2->getString());
			node->create = false;
			node->alter = true;
			$$ = node;
		}
	;


// CREATE INDEX

%type <boolVal> unique_opt
unique_opt
	: /* nothing */		{ $$ = false; }
	| UNIQUE			{ $$ = true; }
	;

%type index_definition(<createIndexNode>)
index_definition($createIndexNode)
	: column_list
		{ $createIndexNode->columns = $1; }
	| column_parens
		{ $createIndexNode->columns = $1; }
	| computed_by '(' value ')'
		{
 			$createIndexNode->computed = newNode<ValueSourceClause>();
			$createIndexNode->computed->value = $3;
			$createIndexNode->computed->source = makeParseStr(YYPOSNARG(2), YYPOSNARG(4));
		}
	;


// CREATE SHADOW
%type <createShadowNode> shadow_clause
shadow_clause
	: pos_short_integer manual_auto conditional utf_string first_file_length
	 		{
	 			$$ = newNode<CreateShadowNode>($1);
		 		$$->manual = $2;
		 		$$->conditional = $3;
		 		$$->files.add(newNode<DbFileClause>(*$4));
		 		$$->files.front()->length = $5;
	 		}
		sec_shadow_files(NOTRIAL(&$6->files))
		 	{ $$ = $6; }
	;

%type <boolVal>	manual_auto
manual_auto
	: /* nothing */		{ $$ = false; }
	| MANUAL			{ $$ = true; }
	| AUTO				{ $$ = false; }
	;

%type <boolVal>	conditional
conditional
	: /* nothing */		{ $$ = false; }
	| CONDITIONAL		{ $$ = true; }
	;

%type <int32Val> first_file_length
first_file_length
	: /* nothing */								{ $$ = 0; }
	| LENGTH equals long_integer page_noise		{ $$ = $3; }
	;

%type sec_shadow_files(<dbFilesClause>)
sec_shadow_files($dbFilesClause)
	: // nothing
	| db_file_list($dbFilesClause)
	;

%type db_file_list(<dbFilesClause>)
db_file_list($dbFilesClause)
	: db_file				{ $dbFilesClause->add($1); }
	| db_file_list db_file	{ $dbFilesClause->add($2); }
	;


// CREATE DOMAIN

%type <createDomainNode> domain_clause
domain_clause
	: symbol_column_name as_opt data_type domain_default_opt
			{
				$3->fld_name = *$1;
				$<createDomainNode>$ = newNode<CreateDomainNode>(
					newNode<ParameterClause>($3, MetaName(), $4));
			}
		domain_constraints_opt($5) collate_clause
			{
				$$ = $5;
				if ($7)
					$$->nameType->type->collate = *$7;
			}
	;

%type domain_constraints_opt(<createDomainNode>)
domain_constraints_opt($createDomainNode)
	: // nothing
	| domain_constraints($createDomainNode)
	;

%type domain_constraints(<createDomainNode>)
domain_constraints($createDomainNode)
	: domain_constraint($createDomainNode)
	| domain_constraints domain_constraint($createDomainNode)
	;

%type domain_constraint(<createDomainNode>)
domain_constraint($createDomainNode)
	: null_constraint
		{ setClause($createDomainNode->notNull, "NOT NULL"); }
	| check_constraint
		{ setClause($createDomainNode->check, "DOMAIN CHECK CONSTRAINT", $1); }
	;

as_opt
	: // nothing
	| AS
	;

%type <valueSourceClause> domain_default
domain_default
	: DEFAULT default_value
		{
			ValueSourceClause* clause = newNode<ValueSourceClause>();
			clause->value = $2;
			clause->source = makeParseStr(YYPOSNARG(1), YYPOSNARG(2));
			$$ = clause;
		}
	;

%type <valueSourceClause> domain_default_opt
domain_default_opt
	: /* nothing */		{ $$ = NULL; }
	| domain_default
	;

null_constraint
	: NOT KW_NULL
	;

%type <boolSourceClause> check_constraint
check_constraint
	: CHECK '(' search_condition ')'
		{
			BoolSourceClause* clause = newNode<BoolSourceClause>();
			clause->value = $3;
			clause->source = makeParseStr(YYPOSNARG(1), YYPOSNARG(4));
			$$ = clause;
		}
	;


// CREATE SEQUENCE/GENERATOR

%type <createAlterSequenceNode> generator_clause
generator_clause
	: symbol_generator_name start_with_opt step_option
		{ $$ = newNode<CreateAlterSequenceNode>(*$1, $2, $3); }
	;

%type <nullableInt64Val> start_with_opt
start_with_opt
	: /* nothing */		{ $$ = Nullable<SINT64>::empty(); }
	| start_with		{ $$ = Nullable<SINT64>::val($1); }
	;

%type <int64Val> start_with
start_with
	: START WITH sequence_value		{ $$ = $3; }
	;

%type <nullableInt32Val> step_option
step_option
	: /* nothing */ 							{ $$ = Nullable<SLONG>::empty(); }
	| INCREMENT by_noise signed_long_integer	{ $$ = Nullable<SLONG>::val($3); }

by_noise
	: // nothing
	| BY

%type <createAlterSequenceNode> replace_sequence_clause
replace_sequence_clause
	: symbol_generator_name replace_sequence_options step_option
		{
			CreateAlterSequenceNode* node = newNode<CreateAlterSequenceNode>(*$1, $2, $3);
			node->alter = true;
			node->restartSpecified = true;
			$$ = node;
		}
	;

%type <nullableInt64Val> replace_sequence_options
replace_sequence_options
	: RESTART						{ $$ = Nullable<SINT64>::empty(); }
	| START WITH sequence_value		{ $$ = Nullable<SINT64>::val($3); }
	;

%type <createAlterSequenceNode> alter_sequence_clause
alter_sequence_clause
	: symbol_generator_name restart_value_opt2 step_option
		{
			if (!$2.specified && !$3.specified)
				yyerrorIncompleteCmd();
			CreateAlterSequenceNode* node = newNode<CreateAlterSequenceNode>(*$1, $2.value, $3);
			node->create = false;
			node->alter = true;
			node->restartSpecified = $2.specified;
			$$ = node;
		}
	;

%type <nullableInt64Val> restart_value_opt
restart_value_opt
	: RESTART						{ $$ = Nullable<SINT64>::empty(); }
	| RESTART WITH sequence_value	{ $$ = Nullable<SINT64>::val($3); }
	;

%type <nullableNullableInt64Val> restart_value_opt2
restart_value_opt2
	: /* nothing */			{ $$ = Nullable<BaseNullable<SINT64> >::empty(); }
	| restart_value_opt		{ $$ = Nullable<BaseNullable<SINT64> > ::val($1); }
	;


%type <createAlterSequenceNode> set_generator_clause
set_generator_clause
	: SET GENERATOR symbol_generator_name TO sequence_value
		{
			CreateAlterSequenceNode* node = newNode<CreateAlterSequenceNode>(*$3,
				Nullable<SINT64>::val($5), Nullable<SLONG>::empty());
			node->create = false;
			node->alter = true;
			node->legacy = true;
			node->restartSpecified = true;
			$$ = node;
		}
	;

%type <int64Val> sequence_value
sequence_value
	: signed_long_integer	{ $$ = $1; }
	| NUMBER64BIT
		{
			SINT64 signedNumber = (SINT64) $1.number;

			if (!$1.hex && $1.number > MAX_SINT64)
			{
				ERRD_post(
					Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					Arg::Gds(isc_arith_except) <<
					Arg::Gds(isc_numeric_out_of_range));
			}

			$$ = signedNumber;
		}
	| '-' NUMBER64BIT
		{
			SINT64 signedNumber = (SINT64) $2.number;

			if ($2.hex && signedNumber == MIN_SINT64)
				ERRD_post(Arg::Gds(isc_exception_integer_overflow));

			$$ = -signedNumber;
		}
	;


// CREATE ROLE

%type <ddlNode> role_clause
role_clause
	: symbol_role_name		{ $$ = newNode<CreateRoleNode>(*$1); }
	;


// CREATE COLLATION

%type <createCollationNode> collation_clause
collation_clause
	: symbol_collation_name FOR symbol_character_set_name
			{ $<createCollationNode>$ = newNode<CreateCollationNode>(*$1, *$3); }
		collation_sequence_definition($4)
		collation_attribute_list_opt($4) collation_specific_attribute_opt($4)
			{ $$ = $4; }
	;

%type collation_sequence_definition(<createCollationNode>)
collation_sequence_definition($createCollation)
	: // nothing
	| FROM symbol_collation_name
		{ $createCollation->fromName = *$2; }
	| FROM EXTERNAL '(' utf_string ')'
		{ $createCollation->fromExternal = *$4; }
	;

%type collation_attribute_list_opt(<createCollationNode>)
collation_attribute_list_opt($createCollation)
	: // nothing
	| collation_attribute_list($createCollation)
	;

%type collation_attribute_list(<createCollationNode>)
collation_attribute_list($createCollation)
	: collation_attribute($createCollation)
	| collation_attribute_list collation_attribute($createCollation)
	;

%type collation_attribute(<createCollationNode>)
collation_attribute($createCollation)
	: collation_pad_attribute($createCollation)
	| collation_case_attribute($createCollation)
	| collation_accent_attribute($createCollation)
	;

%type collation_pad_attribute(<createCollationNode>)
collation_pad_attribute($createCollation)
	: NO PAD		{ $createCollation->unsetAttribute(TEXTTYPE_ATTR_PAD_SPACE); }
	| PAD SPACE		{ $createCollation->setAttribute(TEXTTYPE_ATTR_PAD_SPACE); }
	;

%type collation_case_attribute(<createCollationNode>)
collation_case_attribute($createCollation)
	: CASE SENSITIVE		{ $createCollation->unsetAttribute(TEXTTYPE_ATTR_CASE_INSENSITIVE); }
	| CASE INSENSITIVE		{ $createCollation->setAttribute(TEXTTYPE_ATTR_CASE_INSENSITIVE); }
	;

%type collation_accent_attribute(<createCollationNode>)
collation_accent_attribute($createCollation)
	: ACCENT SENSITIVE		{ $createCollation->unsetAttribute(TEXTTYPE_ATTR_ACCENT_INSENSITIVE); }
	| ACCENT INSENSITIVE	{ $createCollation->setAttribute(TEXTTYPE_ATTR_ACCENT_INSENSITIVE); }
	;

%type collation_specific_attribute_opt(<createCollationNode>)
collation_specific_attribute_opt($createCollation)
	: // nothing
	| utf_string
		{
			const string& s = *$1;
			$createCollation->specificAttributes.clear();
			$createCollation->specificAttributes.add((const UCHAR*) s.begin(), s.length());
		}
	;

// ALTER CHARACTER SET

%type <ddlNode> alter_charset_clause
alter_charset_clause
	: symbol_character_set_name SET DEFAULT COLLATION symbol_collation_name
		{ $$ = newNode<AlterCharSetNode>(*$1, *$5); }
	;

// CREATE DATABASE
// ASF: CREATE DATABASE command is divided in three pieces: name, initial options and
// remote options.
// Initial options are basic properties of a database and should be handled here and
// in preparse.cpp.
// Remote options always come after initial options, so they don't need to be parsed
// in preparse.cpp. They are interpreted only in the server, using this grammar.
// Although LENGTH is defined as an initial option, it's also used in the server.

%type <alterDatabaseNode> db_clause
db_clause
	: db_name
			{
				$$ = newNode<AlterDatabaseNode>();
				$$->create = true;
			}
		db_initial_desc1($2) db_rem_desc1($2)
			{ $$ = $2; }
	;

equals
	: // nothing
	| '='
	;

%type <stringPtr> db_name
db_name
	: utf_string
	;

%type db_initial_desc1(<alterDatabaseNode>)
db_initial_desc1($alterDatabaseNode)
	: // nothing
	| db_initial_desc($alterDatabaseNode)
	;

%type db_initial_desc(<alterDatabaseNode>)
db_initial_desc($alterDatabaseNode)
	: db_initial_option($alterDatabaseNode)
	| db_initial_desc db_initial_option($alterDatabaseNode)
	;

// With the exception of LENGTH, all clauses here are handled only at the client.
%type db_initial_option(<alterDatabaseNode>)
db_initial_option($alterDatabaseNode)
	: KW_PAGE_SIZE equals pos_short_integer
	| USER utf_string
	| PASSWORD utf_string
	| SET NAMES utf_string
	| LENGTH equals long_integer page_noise
		{ $alterDatabaseNode->createLength = $3; }
	;

%type db_rem_desc1(<alterDatabaseNode>)
db_rem_desc1($alterDatabaseNode)
	: // nothing
	| db_rem_desc($alterDatabaseNode)
	;

%type db_rem_desc(<alterDatabaseNode>)
db_rem_desc($alterDatabaseNode)
	: db_rem_option($alterDatabaseNode)
	| db_rem_desc db_rem_option($alterDatabaseNode)
	;

%type db_rem_option(<alterDatabaseNode>)
db_rem_option($alterDatabaseNode)
	: db_file
		{ $alterDatabaseNode->files.add($1); }
	| DEFAULT CHARACTER SET symbol_character_set_name
		{ $alterDatabaseNode->setDefaultCharSet = *$4; }
	| DEFAULT CHARACTER SET symbol_character_set_name COLLATION symbol_collation_name
		{
			$alterDatabaseNode->setDefaultCharSet = *$4;
			$alterDatabaseNode->setDefaultCollation = *$6;
		}
	| KW_DIFFERENCE KW_FILE utf_string
		{ $alterDatabaseNode->differenceFile = *$3; }
	;

%type <dbFileClause> db_file
db_file
	: KW_FILE utf_string
			{
				DbFileClause* clause = newNode<DbFileClause>(*$2);
				$$ = clause;
			}
		file_desc1($3)
			{ $$ = $3; }
	;

%type file_desc1(<dbFileClause>)
file_desc1($dbFileClause)
	: // nothing
	| file_desc($dbFileClause)
	;

%type file_desc(<dbFileClause>)
file_desc($dbFileClause)
	: file_clause($dbFileClause)
	| file_desc file_clause($dbFileClause)
	;

%type file_clause(<dbFileClause>)
file_clause($dbFileClause)
	: STARTING file_clause_noise long_integer
		{ $dbFileClause->start = $3; }
	| LENGTH equals long_integer page_noise
		{ $dbFileClause->length = $3; }
	;

file_clause_noise
	: // nothing
	| AT
	| AT PAGE
	;

page_noise
	: // nothing
	| PAGE
	| PAGES
	;


// CREATE TABLE

%type <createRelationNode> table_clause
table_clause
	: simple_table_name external_file
			{ $<createRelationNode>$ = newNode<CreateRelationNode>($1, $2); }
		'(' table_elements($3) ')'
			{ $$ = $3; }
	;

%type <createRelationNode> gtt_table_clause
gtt_table_clause
	: simple_table_name
			{ $<createRelationNode>$ = newNode<CreateRelationNode>($1); }
		'(' table_elements($2) ')' gtt_scope
			{
				$$ = $2;
				$$->relationType = static_cast<rel_t>($6);
			}
	;

%type <intVal> gtt_scope
gtt_scope
	: /* nothing */				{ $$ = rel_global_temp_delete; }
	| ON COMMIT KW_DELETE ROWS	{ $$ = rel_global_temp_delete; }
	| ON COMMIT PRESERVE ROWS	{ $$ = rel_global_temp_preserve; }
	;

%type <stringPtr> external_file
external_file
	: /* nothing */					{ $$ = NULL; }
	| EXTERNAL KW_FILE utf_string	{ $$ = $3; }
	| EXTERNAL utf_string			{ $$ = $2; }
	;

%type table_elements(<createRelationNode>)
table_elements($createRelationNode)
	: table_element($createRelationNode)
	| table_elements ',' table_element($createRelationNode)
	;

%type table_element(<createRelationNode>)
table_element($createRelationNode)
	: column_def($createRelationNode)
	| table_constraint_definition($createRelationNode)
	;


// column definition

%type column_def(<relationNode>)
column_def($relationNode)
	: symbol_column_name data_type_or_domain domain_default_opt
			{
				RelationNode::AddColumnClause* clause = $<addColumnClause>$ =
					newNode<RelationNode::AddColumnClause>();
				clause->field = $2;
				clause->field->fld_name = *$1;
				clause->defaultValue = $3;
				$relationNode->clauses.add(clause);
			}
		column_constraint_clause(NOTRIAL($<addColumnClause>4)) collate_clause
			{
				if ($6)
					$<addColumnClause>4->collate = *$6;
			}
	| symbol_column_name data_type_or_domain identity_clause
			{
				RelationNode::AddColumnClause* clause = $<addColumnClause>$ =
					newNode<RelationNode::AddColumnClause>();
				clause->field = $2;
				clause->field->fld_name = *$1;
				clause->identity = true;
				clause->identityStart = $3;
				$relationNode->clauses.add(clause);
			}
		column_constraint_clause(NOTRIAL($<addColumnClause>4)) collate_clause
			{
				if ($6)
					$<addColumnClause>4->collate = *$6;
			}
	| symbol_column_name non_array_type def_computed
		{
			RelationNode::AddColumnClause* clause = newNode<RelationNode::AddColumnClause>();
			clause->field = $2;
			clause->field->fld_name = *$1;
			clause->computed = $3;
			$relationNode->clauses.add(clause);
			clause->field->flags |= FLD_computed;
		}
	| symbol_column_name def_computed
		{
			RelationNode::AddColumnClause* clause = newNode<RelationNode::AddColumnClause>();
			clause->field = newNode<dsql_fld>();
			clause->field->fld_name = *$1;
			clause->computed = $2;
			$relationNode->clauses.add(clause);
			clause->field->flags |= FLD_computed;
		}
	;

%type <int64Val> identity_clause
identity_clause
	: GENERATED BY DEFAULT AS IDENTITY identity_clause_options	{ $$ = $6; }
	;

%type <int64Val> identity_clause_options
identity_clause_options
	: /* nothing */			{ $$ = 0; }
	| '(' start_with ')'	{ $$ = $2; }
	;

// value does allow parens around it, but there is a problem getting the source text.

%type <valueSourceClause> def_computed
def_computed
	: computed_clause '(' value ')'
		{
			ValueSourceClause* clause = newNode<ValueSourceClause>();
			clause->value = $3;
			clause->source = makeParseStr(YYPOSNARG(2), YYPOSNARG(4));
			$$ = clause;
		}
	;

computed_clause
	: computed_by
	| generated_always_clause
	;

generated_always_clause
	: GENERATED ALWAYS AS
	;

computed_by
	: COMPUTED BY
	| COMPUTED
	;

%type <legacyField> data_type_or_domain
data_type_or_domain
	: data_type
	| symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$1;
		}
	;

%type <metaNamePtr> collate_clause
collate_clause
	:								{ $$ = NULL; }
	| COLLATE symbol_collation_name	{ $$ = $2; }
	;


%type <legacyField> data_type_descriptor
data_type_descriptor
	: data_type
	| KW_TYPE OF symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$3;
		}
	| KW_TYPE OF COLUMN symbol_column_name '.' symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfTable = *$4;
			$$->typeOfName = *$6;
		}
	| symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$1;
			$$->fullDomain = true;
		}
	;


%type <valueExprNode> default_value
default_value
	: constant						{ $$ = $1; }
	| current_user					{ $$ = $1; }
	| current_role					{ $$ = $1; }
	| internal_info					{ $$ = $1; }
	| null_value					{ $$ = $1; }
	| datetime_value_expression		{ $$ = $1; }
	;

%type column_constraint_clause(<addColumnClause>)
column_constraint_clause($addColumnClause)
	: // nothing
	| column_constraint_list($addColumnClause)
	;

%type column_constraint_list(<addColumnClause>)
column_constraint_list($addColumnClause)
	: column_constraint_def($addColumnClause)
	| column_constraint_list column_constraint_def($addColumnClause)
	;

%type column_constraint_def(<addColumnClause>)
column_constraint_def($addColumnClause)
	: constraint_name_opt column_constraint($addColumnClause)
		{
			if ($1)
				$addColumnClause->constraints.back()->name = *$1;
		}
	;

%type column_constraint(<addColumnClause>)
column_constraint($addColumnClause)
	: null_constraint
		{
			RelationNode::AddConstraintClause& constraint = $addColumnClause->constraints.add();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_NOT_NULL;
		}
	| check_constraint
		{
			RelationNode::AddConstraintClause& constraint = $addColumnClause->constraints.add();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_CHECK;
			constraint.check = $1;
		}
	| REFERENCES symbol_table_name column_parens_opt
			referential_trigger_action constraint_index_opt
		{
			RelationNode::AddConstraintClause& constraint = $addColumnClause->constraints.add();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_FK;

			constraint.columns.add($addColumnClause->field->fld_name);
			constraint.refRelation = *$2;
			constraint.refAction = $4;

			const ValueListNode* refColumns = $3;
			if (refColumns)
			{
				const NestConst<ValueExprNode>* ptr = refColumns->items.begin();

				for (const NestConst<ValueExprNode>* const end = refColumns->items.end(); ptr != end; ++ptr)
					constraint.refColumns.add((*ptr)->as<FieldNode>()->dsqlName);
			}

			constraint.index = $5;
		}
	| UNIQUE constraint_index_opt
		{
			RelationNode::AddConstraintClause& constraint = $addColumnClause->constraints.add();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_UNIQUE;
			constraint.index = $2;
		}
	| PRIMARY KEY constraint_index_opt
		{
			RelationNode::AddConstraintClause& constraint = $addColumnClause->constraints.add();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_PK;
			constraint.index = $3;
		}
	;


// table constraints

%type table_constraint_definition(<relationNode>)
table_constraint_definition($relationNode)
	: constraint_name_opt table_constraint($relationNode)
		{
			if ($1)
			{
				static_cast<RelationNode::AddConstraintClause*>(
					$relationNode->clauses.back().getObject())->name = *$1;
			}
		}
	;

%type <metaNamePtr> constraint_name_opt
constraint_name_opt
	: /* nothing */							{ $$ = NULL; }
	| CONSTRAINT symbol_constraint_name		{ $$ = $2; }
	;

%type table_constraint(<relationNode>)
table_constraint($relationNode)
	: UNIQUE column_parens constraint_index_opt
		{
			RelationNode::AddConstraintClause& constraint = *newNode<RelationNode::AddConstraintClause>();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_UNIQUE;

			const ValueListNode* columns = $2;
			const NestConst<ValueExprNode>* ptr = columns->items.begin();

			for (const NestConst<ValueExprNode>* const end = columns->items.end(); ptr != end; ++ptr)
				constraint.columns.add((*ptr)->as<FieldNode>()->dsqlName);

			constraint.index = $3;

			$relationNode->clauses.add(&constraint);
		}
	| PRIMARY KEY column_parens constraint_index_opt
		{
			RelationNode::AddConstraintClause& constraint = *newNode<RelationNode::AddConstraintClause>();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_PK;

			const ValueListNode* columns = $3;
			const NestConst<ValueExprNode>* ptr = columns->items.begin();

			for (const NestConst<ValueExprNode>* const end = columns->items.end(); ptr != end; ++ptr)
				constraint.columns.add((*ptr)->as<FieldNode>()->dsqlName);

			constraint.index = $4;

			$relationNode->clauses.add(&constraint);
		}
	| FOREIGN KEY column_parens REFERENCES symbol_table_name column_parens_opt
		referential_trigger_action constraint_index_opt
		{
			RelationNode::AddConstraintClause& constraint = *newNode<RelationNode::AddConstraintClause>();
			constraint.constraintType = RelationNode::AddConstraintClause::CTYPE_FK;

			const ValueListNode* columns = $3;
			const NestConst<ValueExprNode>* ptr = columns->items.begin();

			for (const NestConst<ValueExprNode>* const end = columns->items.end(); ptr != end; ++ptr)
				constraint.columns.add((*ptr)->as<FieldNode>()->dsqlName);

			constraint.refRelation = *$5;
			constraint.refAction = $7;

			const ValueListNode* refColumns = $6;
			if (refColumns)
			{
				const NestConst<ValueExprNode>* ptr = refColumns->items.begin();

				for (const NestConst<ValueExprNode>* const end = refColumns->items.end(); ptr != end; ++ptr)
					constraint.refColumns.add((*ptr)->as<FieldNode>()->dsqlName);
			}

			constraint.index = $8;

			$relationNode->clauses.add(&constraint);
		}
	| check_constraint
		{
			RelationNode::AddConstraintClause* constraint = newNode<RelationNode::AddConstraintClause>();
			constraint->constraintType = RelationNode::AddConstraintClause::CTYPE_CHECK;
			constraint->check = $1;
			$relationNode->clauses.add(constraint);
		}
	;

%type <indexConstraintClause> constraint_index_opt
constraint_index_opt
	: // nothing
		{ $$ = newNode<RelationNode::IndexConstraintClause>(); }
	| USING order_direction INDEX symbol_index_name
		{
			RelationNode::IndexConstraintClause* clause = $$ =
				newNode<RelationNode::IndexConstraintClause>();
			clause->descending = $2;
			clause->name = *$4;
		}
	/***
	| NO INDEX
		{ $$ = NULL; }
	***/
	;

%type <refActionClause> referential_trigger_action
referential_trigger_action
	: /* nothing */				{ $$ = NULL; }
	| update_rule				{ $$ = newNode<RelationNode::RefActionClause>($1, 0); }
	| delete_rule				{ $$ = newNode<RelationNode::RefActionClause>(0, $1); }
	| delete_rule update_rule	{ $$ = newNode<RelationNode::RefActionClause>($2, $1); }
	| update_rule delete_rule	{ $$ = newNode<RelationNode::RefActionClause>($1, $2); }
	;

%type <uintVal> update_rule
update_rule
	: ON UPDATE referential_action		{ $$ = $3;}
	;

%type <uintVal> delete_rule
delete_rule
	: ON KW_DELETE referential_action	{ $$ = $3;}
	;

%type <uintVal>	referential_action
referential_action
	: CASCADE		{ $$ = RelationNode::RefActionClause::ACTION_CASCADE; }
	| SET DEFAULT	{ $$ = RelationNode::RefActionClause::ACTION_SET_DEFAULT; }
	| SET KW_NULL	{ $$ = RelationNode::RefActionClause::ACTION_SET_NULL; }
	| NO ACTION		{ $$ = RelationNode::RefActionClause::ACTION_NONE; }
	;


// PROCEDURE


%type <createAlterProcedureNode> procedure_clause
procedure_clause
	: psql_procedure_clause
	| external_procedure_clause
	;

%type <createAlterProcedureNode> psql_procedure_clause
psql_procedure_clause
	: procedure_clause_start AS local_declaration_list full_proc_block
		{
			$$ = $1;
			$$->source = makeParseStr(YYPOSNARG(3), YYPOSNARG(4));
			$$->localDeclList = $3;
			$$->body = $4;
		}
	;

%type <createAlterProcedureNode> external_procedure_clause
external_procedure_clause
	: procedure_clause_start external_clause external_body_clause_opt
		{
			$$ = $1;
			$$->external = $2;
			if ($3)
				$$->source = *$3;
		}
	;

%type <createAlterProcedureNode> procedure_clause_start
procedure_clause_start
	: symbol_procedure_name
			{ $$ = newNode<CreateAlterProcedureNode>(*$1); }
		input_parameters(NOTRIAL(&$2->parameters)) output_parameters(NOTRIAL(&$2->returns))
			{ $$ = $2; }
	;

%type <createAlterProcedureNode> alter_procedure_clause
alter_procedure_clause
	: procedure_clause
		{
			$$ = $1;
			$$->alter = true;
			$$->create = false;
		}
	;

%type <createAlterProcedureNode> replace_procedure_clause
replace_procedure_clause
	: procedure_clause
		{
			$$ = $1;
			$$->alter = true;
		}
	;

%type input_parameters(<parametersClause>)
input_parameters($parameters)
	:
	| '(' ')'
	| '(' input_proc_parameters($parameters) ')'
	;

%type output_parameters(<parametersClause>)
output_parameters($parameters)
	:
	| RETURNS '(' output_proc_parameters($parameters) ')'
	;

%type input_proc_parameters(<parametersClause>)
input_proc_parameters($parameters)
	: input_proc_parameter($parameters)
	| input_proc_parameters ',' input_proc_parameter($parameters)
	;

%type input_proc_parameter(<parametersClause>)
input_proc_parameter($parameters)
	: column_domain_or_non_array_type collate_clause default_par_opt
		{ $parameters->add(newNode<ParameterClause>($1, optName($2), $3)); }
	;

%type output_proc_parameters(<parametersClause>)
output_proc_parameters($parameters)
	: output_proc_parameter
	| output_proc_parameters ',' output_proc_parameter($parameters)
	;

%type output_proc_parameter(<parametersClause>)
output_proc_parameter($parameters)
	: column_domain_or_non_array_type collate_clause
		{ $parameters->add(newNode<ParameterClause>($1, optName($2))); }
	;

%type <legacyField> column_domain_or_non_array_type
column_domain_or_non_array_type
	: symbol_column_name domain_or_non_array_type
		{
			$$ = $2;
			$$->fld_name = *$1;
		}
	;

%type <valueSourceClause> default_par_opt
default_par_opt
	: // nothing
		{ $$ = NULL; }
	| DEFAULT default_value
		{
			ValueSourceClause* clause = newNode<ValueSourceClause>();
			clause->value = $2;
			clause->source = makeParseStr(YYPOSNARG(1), YYPOSNARG(2));
			$$ = clause;
		}
	| '=' default_value
		{
			ValueSourceClause* clause = newNode<ValueSourceClause>();
			clause->value = $2;
			clause->source = makeParseStr(YYPOSNARG(1), YYPOSNARG(2));
			$$ = clause;
		}
	;


// FUNCTION

%type <createAlterFunctionNode> function_clause
function_clause
	: psql_function_clause
	| external_function_clause;

%type <createAlterFunctionNode> psql_function_clause
psql_function_clause
	: function_clause_start AS local_declaration_list full_proc_block
		{
			$$ = $1;
			$$->source = makeParseStr(YYPOSNARG(3), YYPOSNARG(4));
			$$->localDeclList = $3;
			$$->body = $4;
		}
	;

%type <createAlterFunctionNode> external_function_clause
external_function_clause
	: function_clause_start external_clause external_body_clause_opt
		{
			$$ = $1;
			$$->external = $2;
			if ($3)
				$$->source = *$3;
		}
	;

%type <createAlterFunctionNode> function_clause_start
function_clause_start
	: symbol_UDF_name
			{ $$ = newNode<CreateAlterFunctionNode>(*$1); }
		input_parameters(NOTRIAL(&$2->parameters))
		RETURNS domain_or_non_array_type collate_clause deterministic_opt
			{
				$$ = $2;
				$$->returnType = newNode<ParameterClause>($5, optName($6));
				$$->deterministic = $7;
			}
	;

%type <boolVal> deterministic_opt
deterministic_opt
	:					{ $$ = false; }
	| NOT DETERMINISTIC	{ $$ = false; }
	| DETERMINISTIC		{ $$ = true; }
	;

%type <externalClause> external_clause
external_clause
	: EXTERNAL NAME utf_string ENGINE valid_symbol_name
		{
			$$ = newNode<ExternalClause>();
			$$->name = *$3;
			$$->engine = *$5;
		}
	| EXTERNAL ENGINE valid_symbol_name
		{
			$$ = newNode<ExternalClause>();
			$$->engine = *$3;
		}
	;

%type <stringPtr> external_body_clause_opt
external_body_clause_opt
	: /* nothing */		{ $$ = NULL; }
	| AS utf_string		{ $$ = $2; }
	;

%type <createAlterFunctionNode> alter_function_clause
alter_function_clause
	: function_clause
		{
			$$ = $1;
			$$->alter = true;
			$$->create = false;
		}
	;

%type <createAlterFunctionNode> replace_function_clause
replace_function_clause
	: function_clause
		{
			$$ = $1;
			$$->alter = true;
		}
	;


// PACKAGE

%type <createAlterPackageNode> package_clause
package_clause
	: symbol_package_name AS BEGIN package_items_opt END
		{
			CreateAlterPackageNode* node = newNode<CreateAlterPackageNode>(*$1);
			node->source = makeParseStr(YYPOSNARG(3), YYPOSNARG(5));
			node->items = $4;
			$$ = node;
		}
	;

%type <packageItems> package_items_opt
package_items_opt
	: package_items
	|
		{ $$ = newNode<Array<CreateAlterPackageNode::Item> >(); }
	;

%type <packageItems> package_items
package_items
	: package_item
		{
			$$ = newNode<Array<CreateAlterPackageNode::Item> >();
			$$->add($1);
		}
	| package_items package_item
		{
			$$ = $1;
			$$->add($2);
		}
	;

%type <packageItem> package_item
package_item
	: FUNCTION function_clause_start ';'
		{ $$ = CreateAlterPackageNode::Item::create($2); }
	| PROCEDURE procedure_clause_start ';'
		{ $$ = CreateAlterPackageNode::Item::create($2); }
	;

%type <createAlterPackageNode> alter_package_clause
alter_package_clause
	: package_clause
		{
			$$ = $1;
			$$->alter = true;
			$$->create = false;
		}
	;

%type <createAlterPackageNode> replace_package_clause
replace_package_clause
	: package_clause
		{
			$$ = $1;
			$$->alter = true;
		}
	;


// PACKAGE BODY

%type <createPackageBodyNode> package_body_clause
package_body_clause
	: symbol_package_name AS BEGIN package_items package_body_items_opt END
		{
			CreatePackageBodyNode* node = newNode<CreatePackageBodyNode>(*$1);
			node->source = makeParseStr(YYPOSNARG(3), YYPOSNARG(6));
			node->declaredItems = $4;
			node->items = $5;
			$$ = node;
		}
	| symbol_package_name AS BEGIN package_body_items_opt END
		{
			CreatePackageBodyNode* node = newNode<CreatePackageBodyNode>(*$1);
			node->source = makeParseStr(YYPOSNARG(3), YYPOSNARG(5));
			node->items = $4;
			$$ = node;
		}
	;

%type <packageItems> package_body_items_opt
package_body_items_opt
	: /* nothing */			{ $$ = newNode<Array<CreateAlterPackageNode::Item> >(); }
	| package_body_items
	;

%type <packageItems> package_body_items
package_body_items
	: package_body_item
		{
			$$ = newNode<Array<CreateAlterPackageNode::Item> >();
			$$->add($1);
		}
	| package_body_items package_body_item
		{
			$$ = $1;
			$$->add($2);
		}
	;

%type <packageItem> package_body_item
package_body_item
	: FUNCTION psql_function_clause
		{ $$ = CreateAlterPackageNode::Item::create($2); }
	| FUNCTION external_function_clause ';'
		{ $$ = CreateAlterPackageNode::Item::create($2); }
	| PROCEDURE psql_procedure_clause
		{ $$ = CreateAlterPackageNode::Item::create($2); }
	| PROCEDURE external_procedure_clause ';'
		{ $$ = CreateAlterPackageNode::Item::create($2); }
	;


%type <compoundStmtNode> local_declaration_list
local_declaration_list
	: /* nothing */			{ $$ = NULL; }
	| local_declarations
	;

%type <compoundStmtNode> local_declarations
local_declarations
	: local_declaration
		{
			$$ = newNode<CompoundStmtNode>();
			$$->statements.add($1);
		}
	| local_declarations local_declaration
		{
			$1->statements.add($2);
			$$ = $1;
		}
	;

%type <stmtNode> local_declaration
local_declaration
	: DECLARE var_decl_opt local_declaration_item ';'
		{
			$$ = $3;
			$$->line = YYPOSNARG(1).firstLine;
			$$->column = YYPOSNARG(1).firstColumn;
		}
	| DECLARE PROCEDURE symbol_procedure_name
			{ $<execBlockNode>$ = newNode<ExecBlockNode>(); }
			input_parameters(NOTRIAL(&$<execBlockNode>4->parameters))
			output_parameters(NOTRIAL(&$<execBlockNode>4->returns)) AS
			local_declaration_list
			full_proc_block
		{
			DeclareSubProcNode* node = newNode<DeclareSubProcNode>(*$3);
			node->dsqlBlock = $<execBlockNode>4;
			node->dsqlBlock->localDeclList = $8;
			node->dsqlBlock->body = $9;

			for (size_t i = 0; i < node->dsqlBlock->parameters.getCount(); ++i)
				node->dsqlBlock->parameters[i]->parameterExpr = make_parameter();

			$$ = node;
		}
	| DECLARE FUNCTION symbol_UDF_name
			{ $<execBlockNode>$ = newNode<ExecBlockNode>(); }
			input_parameters(NOTRIAL(&$<execBlockNode>4->parameters))
			RETURNS domain_or_non_array_type collate_clause deterministic_opt AS
			local_declaration_list
			full_proc_block
		{
			DeclareSubFuncNode* node = newNode<DeclareSubFuncNode>(*$3);
			node->dsqlDeterministic = $9;
			node->dsqlBlock = $<execBlockNode>4;
			node->dsqlBlock->localDeclList = $11;
			node->dsqlBlock->body = $12;

			for (size_t i = 0; i < node->dsqlBlock->parameters.getCount(); ++i)
				node->dsqlBlock->parameters[i]->parameterExpr = make_parameter();

			node->dsqlBlock->returns.add(newNode<ParameterClause>($<legacyField>7, optName($8)));

			$$ = node;
		}
	;

%type <stmtNode> local_declaration_item
local_declaration_item
	: var_declaration_item
	| cursor_declaration_item
	;

%type <stmtNode> var_declaration_item
var_declaration_item
	: column_domain_or_non_array_type collate_clause default_par_opt
		{
			DeclareVariableNode* node = newNode<DeclareVariableNode>();
			node->dsqlDef = newNode<ParameterClause>($1, optName($2), $3);
			$$ = node;
		}
	;

var_decl_opt
	: // nothing
	| VARIABLE
	;

%type <stmtNode> cursor_declaration_item
cursor_declaration_item
	: symbol_cursor_name scroll_opt CURSOR FOR '(' select ')'
		{
			DeclareCursorNode* node = newNode<DeclareCursorNode>(*$1,
				DeclareCursorNode::CUR_TYPE_EXPLICIT);
			node->dsqlScroll = $2;
			node->dsqlSelect = $6;
			$$ = node;
		}
	;

%type <boolVal> scroll_opt
scroll_opt
	: /* nothing */	{ $$ = false; }
	| NO SCROLL		{ $$ = false; }
	| SCROLL		{ $$ = true; }
	;

%type <stmtNode> proc_block
proc_block
	: proc_statement
	| full_proc_block
	;

%type <stmtNode> full_proc_block
full_proc_block
	: BEGIN full_proc_block_body END
		{ $$ = newNode<LineColumnNode>(YYPOSNARG(1).firstLine, YYPOSNARG(1).firstColumn, $2); }
	;

%type <stmtNode> full_proc_block_body
full_proc_block_body
	: // nothing
		{ $$ = newNode<CompoundStmtNode>(); }
	| proc_statements
		{
			BlockNode* node = newNode<BlockNode>();
			node->action = $1;
			$$ = node;
		}
	| proc_statements excp_hndl_statements
		{
			BlockNode* node = newNode<BlockNode>();
			node->action = $1;
			node->handlers = $2;
			$$ = node;
		}
	;

%type <compoundStmtNode> proc_statements
proc_statements
	: proc_block
		{
			$$ = newNode<CompoundStmtNode>();
			$$->statements.add($1);
		}
	| proc_statements proc_block
		{
			$1->statements.add($2);
			$$ = $1;
		}
	;

%type <stmtNode> proc_statement
proc_statement
	: simple_proc_statement ';'
		{ $$ = newNode<LineColumnNode>(YYPOSNARG(1).firstLine, YYPOSNARG(1).firstColumn, $1); }
	| complex_proc_statement
		{ $$ = newNode<LineColumnNode>(YYPOSNARG(1).firstLine, YYPOSNARG(1).firstColumn, $1); }
	;

%type <stmtNode> simple_proc_statement
simple_proc_statement
	: assignment
	| insert			{ $$ = $1; }
	| merge				{ $$ = $1; }
	| update
	| update_or_insert	{ $$ = $1; }
	| delete
	| singleton_select
	| exec_procedure
	| exec_sql			{ $$ = $1; }
	| exec_into			{ $$ = $1; }
	| exec_function
	| excp_statement	{ $$ = $1; }
	| raise_statement
	| post_event
	| cursor_statement
	| breakleave
	| continue
	| SUSPEND			{ $$ = newNode<SuspendNode>(); }
	| EXIT				{ $$ = newNode<ExitNode>(); }
	| RETURN value		{ $$ = newNode<ReturnNode>($2); }
	;

%type <stmtNode> complex_proc_statement
complex_proc_statement
	: in_autonomous_transaction
	| if_then_else
	| while
	| for_select
	| for_exec_into					{ $$ = $1; }
	;

%type <stmtNode> in_autonomous_transaction
in_autonomous_transaction
	: KW_IN AUTONOMOUS TRANSACTION DO proc_block
		{
			InAutonomousTransactionNode* node = newNode<InAutonomousTransactionNode>();
			node->action = $5;
			$$ = node;
		}
	;

%type <stmtNode> excp_statement
excp_statement
	: EXCEPTION symbol_exception_name
		{ $$ = newNode<ExceptionNode>(*$2); }
	| EXCEPTION symbol_exception_name value
		{ $$ = newNode<ExceptionNode>(*$2, $3); }
	| EXCEPTION symbol_exception_name USING '(' value_list ')'
		{ $$ = newNode<ExceptionNode>(*$2, (ValueExprNode*) NULL, $5); }
	;

%type <stmtNode> raise_statement
raise_statement
	: EXCEPTION		{ $$ = newNode<ExceptionNode>(); }
	;

%type <stmtNode> for_select
for_select
	: label_def_opt FOR select INTO variable_list cursor_def DO proc_block
		{
			ForNode* node = newNode<ForNode>();
			node->dsqlLabelName = $1;
			node->dsqlSelect = $3;
			node->dsqlInto = $5;
			node->dsqlCursor = $6;
			node->statement = $8;
			$$ = node;
		}
	;

%type <execStatementNode> exec_sql
exec_sql
	: EXECUTE STATEMENT
			{ $<execStatementNode>$ = newNode<ExecStatementNode>(); }
			exec_stmt_inputs($<execStatementNode>3) exec_stmt_options($<execStatementNode>3)
		{
			$$ = $<execStatementNode>3;
		}
	;

%type <execStatementNode> exec_into
exec_into
	: exec_sql INTO variable_list
		{
			$$ = $<execStatementNode>1;
			$$->outputs = $3;
		}
	;

%type <execStatementNode> for_exec_into
for_exec_into
	: label_def_opt FOR exec_into DO proc_block
		{
			$$ = $<execStatementNode>3;
			$$->dsqlLabelName = $1;
			$$->innerStmt = $5;
		}
	;

%type exec_stmt_inputs(<execStatementNode>)
exec_stmt_inputs($execStatementNode)
	: value
		{ $execStatementNode->sql = $1; }
	| '(' value ')' '(' named_params_list($execStatementNode) ')'
		{ $execStatementNode->sql = $2; }
	| '(' value ')' '(' not_named_params_list($execStatementNode) ')'
		{ $execStatementNode->sql = $2; }
	;

%type named_params_list(<execStatementNode>)
named_params_list($execStatementNode)
	: named_param($execStatementNode)
	| named_params_list ',' named_param($execStatementNode)
	;

%type named_param(<execStatementNode>)
named_param($execStatementNode)
	: symbol_variable_name BIND_PARAM value
		{
			if (!$execStatementNode->inputNames)
				$execStatementNode->inputNames = FB_NEW(getPool()) EDS::ParamNames(getPool());

			$execStatementNode->inputNames->add($1);

			if (!$execStatementNode->inputs)
				$execStatementNode->inputs = newNode<ValueListNode>($3);
			else
				$execStatementNode->inputs->add($3);
		}
	;

%type not_named_params_list(<execStatementNode>)
not_named_params_list($execStatementNode)
	: not_named_param($execStatementNode)
	| not_named_params_list ',' not_named_param($execStatementNode)
	;

%type not_named_param(<execStatementNode>)
not_named_param($execStatementNode)
	: value
		{
			if (!$execStatementNode->inputs)
				$execStatementNode->inputs = newNode<ValueListNode>($1);
			else
				$execStatementNode->inputs->add($1);
		}
	;

%type exec_stmt_options(<execStatementNode>)
exec_stmt_options($execStatementNode)
	: // nothing
	| exec_stmt_options_list($execStatementNode)
	;

%type exec_stmt_options_list(<execStatementNode>)
exec_stmt_options_list($execStatementNode)
	: exec_stmt_options_list exec_stmt_option($execStatementNode)
	| exec_stmt_option($execStatementNode)
	;

%type exec_stmt_option(<execStatementNode>)
exec_stmt_option($execStatementNode)
	: ON EXTERNAL DATA SOURCE value
		{ setClause($execStatementNode->dataSource, "EXTERNAL DATA SOURCE", $5); }
	| ON EXTERNAL value
		{ setClause($execStatementNode->dataSource, "EXTERNAL DATA SOURCE", $3); }
	| AS USER value
		{ setClause($execStatementNode->userName, "USER", $3); }
	| PASSWORD value
		{ setClause($execStatementNode->password, "PASSWORD", $2); }
	| ROLE value
		{ setClause($execStatementNode->role, "ROLE", $2); }
	| WITH AUTONOMOUS TRANSACTION
		{ setClause($execStatementNode->traScope, "TRANSACTION", EDS::traAutonomous); }
	| WITH COMMON TRANSACTION
		{ setClause($execStatementNode->traScope, "TRANSACTION", EDS::traCommon); }
	| WITH CALLER PRIVILEGES
		{ setClause($execStatementNode->useCallerPrivs, "CALLER PRIVILEGES"); }
	/*
	| WITH TWO_PHASE TRANSACTION
		{ setClause($execStatementNode->traScope, "TRANSACTION", EDS::traTwoPhase); }
	*/
	;

%type <stmtNode> if_then_else
if_then_else
	: IF '(' search_condition ')' THEN proc_block ELSE proc_block
		{
			IfNode* node = newNode<IfNode>();
			node->condition = $3;
			node->trueAction = $6;
			node->falseAction = $8;
			$$ = node;
		}
	| IF '(' search_condition ')' THEN proc_block
		{
			IfNode* node = newNode<IfNode>();
			node->condition = $3;
			node->trueAction = $6;
			$$ = node;
		}
	;

%type <stmtNode> post_event
post_event
	: POST_EVENT value event_argument_opt
		{
			PostEventNode* node = newNode<PostEventNode>();
			node->event = $2;
			node->argument = $3;
			$$ = node;
		}
	;

%type <valueExprNode> event_argument_opt
event_argument_opt
	: /* nothing */	{ $$ = NULL; }
	///| ',' value		{ $$ = $2; }
	;

%type <stmtNode> singleton_select
singleton_select
	: select INTO variable_list
		{
			ForNode* node = newNode<ForNode>();
			node->dsqlSelect = $1;
			node->dsqlInto = $3;
			$$ = node;
		}
	;

%type <valueExprNode> variable
variable
	: ':' symbol_variable_name
		{
			VariableNode* node = newNode<VariableNode>();
			node->dsqlName = *$2;
			$$ = node;
		}
	;

%type <valueListNode> variable_list
variable_list
	: variable							{ $$ = newNode<ValueListNode>($1); }
	| column_name						{ $$ = newNode<ValueListNode>($1); }
	| variable_list ',' column_name		{ $$ = $1->add($3); }
	| variable_list ',' variable		{ $$ = $1->add($3); }
	;

%type <stmtNode> while
while
	: label_def_opt WHILE '(' search_condition ')' DO proc_block
		{
			LoopNode* node = newNode<LoopNode>();
			node->dsqlLabelName = $1;
			node->dsqlExpr = $4;
			node->statement = $7;
			$$ = node;
		}
	;

%type <metaNamePtr> label_def_opt
label_def_opt
	: /* nothing */				{ $$ = NULL; }
	| symbol_label_name ':'		{ $$ = $1; }
	;

%type <stmtNode> breakleave
breakleave
	: KW_BREAK
		{ $$ = newNode<ContinueLeaveNode>(blr_leave); }
	| LEAVE label_use_opt
		{
			ContinueLeaveNode* node = newNode<ContinueLeaveNode>(blr_leave);
			node->dsqlLabelName = $2;
			$$ = node;
		}
	;

%type <stmtNode> continue
continue
	: CONTINUE label_use_opt
		{
			ContinueLeaveNode* node = newNode<ContinueLeaveNode>(blr_continue_loop);
			node->dsqlLabelName = $2;
			$$ = node;
		}
	;

%type <metaNamePtr> label_use_opt
label_use_opt
	: /* nothing */				{ $$ = NULL; }
	| symbol_label_name
	;

%type <declCursorNode> cursor_def
cursor_def
	: // nothing
		{ $$ = NULL; }
	| AS CURSOR symbol_cursor_name
		{ $$ = newNode<DeclareCursorNode>(*$3, DeclareCursorNode::CUR_TYPE_FOR); }
	;

%type <compoundStmtNode> excp_hndl_statements
excp_hndl_statements
	: excp_hndl_statement
		{
			$$ = newNode<CompoundStmtNode>();
			$$->statements.add($1);
		}
	| excp_hndl_statements excp_hndl_statement
		{
			$1->statements.add($2);
			$$ = $1;
		}
	;

%type <stmtNode> excp_hndl_statement
excp_hndl_statement
	: WHEN
			{ $<errorHandlerNode>$ = newNode<ErrorHandlerNode>(); }
		errors(NOTRIAL(&$<errorHandlerNode>2->conditions)) DO proc_block
			{
				ErrorHandlerNode* node = $<errorHandlerNode>2;
				node->action = $5;
				$$ = node;
			}
	;

%type errors(<exceptionArray>)
errors($exceptionArray)
	: err($exceptionArray)
	| errors ',' err($exceptionArray)
	;

%type err(<exceptionArray>)
err($exceptionArray)
	: SQLCODE signed_short_integer
		{
			ExceptionItem& item = $exceptionArray->add();
			item.type = ExceptionItem::SQL_CODE;
			item.code = $2;
		}
	| GDSCODE symbol_gdscode_name
		{
			ExceptionItem& item = $exceptionArray->add();
			item.type = ExceptionItem::GDS_CODE;
			item.name = $2->c_str();
		}
	| EXCEPTION symbol_exception_name
		{
			ExceptionItem& item = $exceptionArray->add();
			item.type = ExceptionItem::XCP_CODE;
			item.name = $2->c_str();
		}
	| ANY
		{
			ExceptionItem& item = $exceptionArray->add();
			item.type = ExceptionItem::XCP_DEFAULT;
		}
	;

%type <stmtNode> cursor_statement
cursor_statement
	: open_cursor
	| fetch_cursor
	| close_cursor
	;

%type <stmtNode> open_cursor
open_cursor
	: OPEN symbol_cursor_name
		{ $$ = newNode<CursorStmtNode>(blr_cursor_open, *$2); }
	;

%type <stmtNode> close_cursor
close_cursor
	: CLOSE symbol_cursor_name
		{ $$ = newNode<CursorStmtNode>(blr_cursor_close, *$2); }
	;

%type <stmtNode> fetch_cursor
fetch_cursor
	: FETCH
			{ $<cursorStmtNode>$ = newNode<CursorStmtNode>(blr_cursor_fetch_scroll); }
			fetch_scroll($<cursorStmtNode>2) FROM symbol_cursor_name INTO variable_list
		{
			CursorStmtNode* cursorStmt = $<cursorStmtNode>2;
			cursorStmt->dsqlName = *$5;
			cursorStmt->dsqlIntoStmt = $7;
			$$ = cursorStmt;
		}
	| FETCH symbol_cursor_name INTO variable_list
		{ $$ = newNode<CursorStmtNode>(blr_cursor_fetch, *$2, $4); }
	;

%type fetch_scroll(<cursorStmtNode>)
fetch_scroll($cursorStmtNode)
	: FIRST
		{ $cursorStmtNode->scrollOp = blr_scroll_bof; }
	| LAST
		{ $cursorStmtNode->scrollOp = blr_scroll_eof; }
	| PRIOR
		{ $cursorStmtNode->scrollOp = blr_scroll_backward; }
	| NEXT
		{ $cursorStmtNode->scrollOp = blr_scroll_forward; }
	| KW_ABSOLUTE value
		{
			$cursorStmtNode->scrollOp = blr_scroll_absolute;
			$cursorStmtNode->scrollExpr = $2;
		}
	| KW_RELATIVE value
		{
			$cursorStmtNode->scrollOp = blr_scroll_relative;
			$cursorStmtNode->scrollExpr = $2;
		}
	;


// EXECUTE PROCEDURE

%type <stmtNode> exec_procedure
exec_procedure
	: EXECUTE PROCEDURE symbol_procedure_name proc_inputs proc_outputs_opt
		{ $$ = newNode<ExecProcedureNode>(QualifiedName(*$3), $4, $5); }
	| EXECUTE PROCEDURE symbol_package_name '.' symbol_procedure_name proc_inputs proc_outputs_opt
		{ $$ = newNode<ExecProcedureNode>(QualifiedName(*$5, *$3), $6, $7); }
	;

%type <valueListNode> proc_inputs
proc_inputs
	: /* nothing */			{ $$ = NULL; }
	| value_list			{ $$ = $1; }
	| '(' value_list ')'	{ $$ = $2; }
	;

%type <valueListNode> proc_outputs_opt
proc_outputs_opt
	: /* nothing */								{ $$ = NULL; }
	| RETURNING_VALUES variable_list			{ $$ = $2; }
	| RETURNING_VALUES '(' variable_list ')'	{ $$ = $3; }
	;

// EXECUTE BLOCK

%type <execBlockNode> exec_block
exec_block
	: EXECUTE BLOCK
			{ $<execBlockNode>$ = newNode<ExecBlockNode>(); }
			block_input_params(NOTRIAL(&$3->parameters))
			output_parameters(NOTRIAL(&$3->returns)) AS
			local_declaration_list
			full_proc_block
		{
			ExecBlockNode* node = $3;
			node->localDeclList = $7;
			node->body = $8;
			$$ = node;
		}
	;

%type block_input_params(<parametersClause>)
block_input_params($parameters)
	: // nothing
	| '(' block_parameters($parameters) ')'
	;

%type block_parameters(<parametersClause>)
block_parameters($parameters)
	: block_parameter($parameters)
	| block_parameters ',' block_parameter($parameters)
	;

%type block_parameter(<parametersClause>)
block_parameter($parameters)
	: column_domain_or_non_array_type collate_clause '=' parameter
		{ $parameters->add(newNode<ParameterClause>($1, optName($2), (ValueSourceClause*) NULL, $4)); }
	;

// CREATE VIEW

%type <createAlterViewNode> view_clause
view_clause
	: simple_table_name column_parens_opt AS select_expr check_opt
		{
			CreateAlterViewNode* node = newNode<CreateAlterViewNode>($1, $2, $4);
			node->source = makeParseStr(YYPOSNARG(4), YYPOSNARG(5));
			node->withCheckOption = $5;
			$$ = node;
		}
	;

%type <ddlNode> replace_view_clause
replace_view_clause
	: view_clause
		{
			$1->alter = true;
			$$ = $1;
		}
	;

%type <ddlNode>	alter_view_clause
alter_view_clause
	: view_clause
		{
			$1->alter = true;
			$1->create = false;
			$$ = $1;
		}
	;

%type <boolVal>	check_opt
check_opt
	: /* nothing */			{ $$ = false; }
	| WITH CHECK OPTION		{ $$ = true; }
	;


// CREATE TRIGGER

%type <createAlterTriggerNode> trigger_clause
trigger_clause
	: symbol_trigger_name trigger_active trigger_type trigger_position
			AS local_declaration_list full_proc_block
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
			$$->source = makeParseStr(YYPOSNARG(5), YYPOSNARG(7));
			$$->localDeclList = $6;
			$$->body = $7;
		}
	| symbol_trigger_name trigger_active trigger_type trigger_position
			external_clause external_body_clause_opt
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
			$$->external = $5;
			if ($6)
				$$->source = *$6;
		}
	| symbol_trigger_name trigger_active trigger_type trigger_position ON symbol_table_name
			AS local_declaration_list full_proc_block
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
			$$->relationName = *$6;
			$$->source = makeParseStr(YYPOSNARG(7), YYPOSNARG(9));
			$$->localDeclList = $8;
			$$->body = $9;
		}
	| symbol_trigger_name trigger_active trigger_type trigger_position ON symbol_table_name
			external_clause external_body_clause_opt
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
			$$->relationName = *$6;
			$$->external = $7;
			if ($8)
				$$->source = *$8;
		}
	| symbol_trigger_name FOR symbol_table_name trigger_active trigger_type trigger_position
			AS local_declaration_list full_proc_block
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->active = $4;
			$$->type = $5;
			$$->position = $6;
			$$->relationName = *$3;
			$$->source = makeParseStr(YYPOSNARG(7), YYPOSNARG(9));
			$$->localDeclList = $8;
			$$->body = $9;
		}
	| symbol_trigger_name FOR symbol_table_name trigger_active trigger_type trigger_position
			external_clause external_body_clause_opt
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->active = $4;
			$$->type = $5;
			$$->position = $6;
			$$->relationName = *$3;
			$$->external = $7;
			if ($8)
				$$->source = *$8;
		}
	;

%type <createAlterTriggerNode> replace_trigger_clause
replace_trigger_clause
	: trigger_clause
		{
			$$ = $1;
			$$->alter = true;
		}
	;

%type <nullableBoolVal> trigger_active
trigger_active
	: ACTIVE
		{ $$ = Nullable<bool>::val(true); }
	| INACTIVE
		{ $$ = Nullable<bool>::val(false); }
	|
		{ $$ = Nullable<bool>::empty(); }
	;

%type <uint64Val> trigger_type
trigger_type
	: trigger_type_prefix trigger_type_suffix	{ $$ = $1 + $2 - 1; }
	| ON trigger_db_type						{ $$ = $2; }
	| trigger_type_prefix trigger_ddl_type		{ $$ = $1 + $2; }
	;

%type <uint64Val> trigger_db_type
trigger_db_type
	: CONNECT				{ $$ = TRIGGER_TYPE_DB | DB_TRIGGER_CONNECT; }
	| DISCONNECT			{ $$ = TRIGGER_TYPE_DB | DB_TRIGGER_DISCONNECT; }
	| TRANSACTION START		{ $$ = TRIGGER_TYPE_DB | DB_TRIGGER_TRANS_START; }
	| TRANSACTION COMMIT	{ $$ = TRIGGER_TYPE_DB | DB_TRIGGER_TRANS_COMMIT; }
	| TRANSACTION ROLLBACK	{ $$ = TRIGGER_TYPE_DB | DB_TRIGGER_TRANS_ROLLBACK; }
	;

%type <uint64Val> trigger_ddl_type
trigger_ddl_type
	: trigger_ddl_type_items
	| ANY DDL STATEMENT
		{
			$$ = TRIGGER_TYPE_DDL | DDL_TRIGGER_ANY;
		}
	;

%type <uint64Val> trigger_ddl_type_items
trigger_ddl_type_items
	: CREATE TABLE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_TABLE); }
	| ALTER TABLE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_TABLE); }
	| DROP TABLE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_TABLE); }
	| CREATE PROCEDURE		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_PROCEDURE); }
	| ALTER PROCEDURE		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_PROCEDURE); }
	| DROP PROCEDURE		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_PROCEDURE); }
	| CREATE FUNCTION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_FUNCTION); }
	| ALTER FUNCTION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_FUNCTION); }
	| DROP FUNCTION			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_FUNCTION); }
	| CREATE TRIGGER		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_TRIGGER); }
	| ALTER TRIGGER			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_TRIGGER); }
	| DROP TRIGGER			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_TRIGGER); }
	| CREATE EXCEPTION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_EXCEPTION); }
	| ALTER EXCEPTION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_EXCEPTION); }
	| DROP EXCEPTION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_EXCEPTION); }
	| CREATE VIEW			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_VIEW); }
	| ALTER VIEW			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_VIEW); }
	| DROP VIEW				{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_VIEW); }
	| CREATE KW_DOMAIN		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_DOMAIN); }
	| ALTER KW_DOMAIN		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_DOMAIN); }
	| DROP KW_DOMAIN		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_DOMAIN); }
	| CREATE ROLE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_ROLE); }
	| ALTER ROLE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_ROLE); }
	| DROP ROLE				{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_ROLE); }
	| CREATE SEQUENCE		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_SEQUENCE); }
	| ALTER SEQUENCE		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_SEQUENCE); }
	| DROP SEQUENCE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_SEQUENCE); }
	| CREATE USER			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_USER); }
	| ALTER USER			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_USER); }
	| DROP USER				{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_USER); }
	| CREATE INDEX			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_INDEX); }
	| ALTER INDEX			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_INDEX); }
	| DROP INDEX			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_INDEX); }
	| CREATE COLLATION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_COLLATION); }
	| DROP COLLATION		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_COLLATION); }
	| ALTER CHARACTER SET	{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_CHARACTER_SET); }
	| CREATE PACKAGE		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_PACKAGE); }
	| ALTER PACKAGE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_PACKAGE); }
	| DROP PACKAGE			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_PACKAGE); }
	| CREATE PACKAGE BODY	{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_PACKAGE_BODY); }
	| DROP PACKAGE BODY		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_PACKAGE_BODY); }
	| CREATE MAPPING		{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_CREATE_MAPPING); }
	| ALTER MAPPING			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_ALTER_MAPPING); }
	| DROP MAPPING			{ $$ = TRIGGER_TYPE_DDL | (1LL << DDL_TRIGGER_DROP_MAPPING); }
	| trigger_ddl_type OR
		trigger_ddl_type	{ $$ = $1 | $3; }
	;

%type <uint64Val> trigger_type_prefix
trigger_type_prefix
	: BEFORE	{ $$ = 0; }
	| AFTER		{ $$ = 1; }
	;

%type <uint64Val> trigger_type_suffix
trigger_type_suffix
	: INSERT							{ $$ = trigger_type_suffix(1, 0, 0); }
	| UPDATE							{ $$ = trigger_type_suffix(2, 0, 0); }
	| KW_DELETE							{ $$ = trigger_type_suffix(3, 0, 0); }
	| INSERT OR UPDATE					{ $$ = trigger_type_suffix(1, 2, 0); }
	| INSERT OR KW_DELETE				{ $$ = trigger_type_suffix(1, 3, 0); }
	| UPDATE OR INSERT					{ $$ = trigger_type_suffix(2, 1, 0); }
	| UPDATE OR KW_DELETE				{ $$ = trigger_type_suffix(2, 3, 0); }
	| KW_DELETE OR INSERT				{ $$ = trigger_type_suffix(3, 1, 0); }
	| KW_DELETE OR UPDATE				{ $$ = trigger_type_suffix(3, 2, 0); }
	| INSERT OR UPDATE OR KW_DELETE		{ $$ = trigger_type_suffix(1, 2, 3); }
	| INSERT OR KW_DELETE OR UPDATE		{ $$ = trigger_type_suffix(1, 3, 2); }
	| UPDATE OR INSERT OR KW_DELETE		{ $$ = trigger_type_suffix(2, 1, 3); }
	| UPDATE OR KW_DELETE OR INSERT		{ $$ = trigger_type_suffix(2, 3, 1); }
	| KW_DELETE OR INSERT OR UPDATE		{ $$ = trigger_type_suffix(3, 1, 2); }
	| KW_DELETE OR UPDATE OR INSERT		{ $$ = trigger_type_suffix(3, 2, 1); }
	;

%type <nullableIntVal> trigger_position
trigger_position
	: /* nothing */					{ $$ = Nullable<int>::empty(); }
	| POSITION nonneg_short_integer	{ $$ = Nullable<int>::val($2); }
	;

// ALTER statement

%type <ddlNode> alter
alter
	: ALTER alter_clause	{ $$ = $2; }
	| set_generator_clause	{ $$ = $1; }
	;

%type <ddlNode> alter_clause
alter_clause
	: EXCEPTION alter_exception_clause		{ $$ = $2; }
	| TABLE simple_table_name
			{ $$ = newNode<AlterRelationNode>($2); }
		alter_ops($<relationNode>3)
			{ $$ = $<relationNode>3; }
	| VIEW alter_view_clause				{ $$ = $2; }
	| TRIGGER alter_trigger_clause			{ $$ = $2; }
	| PROCEDURE alter_procedure_clause		{ $$ = $2; }
	| PACKAGE alter_package_clause			{ $$ = $2; }
	| DATABASE
			{ $<alterDatabaseNode>$ = newNode<AlterDatabaseNode>(); }
		alter_db($<alterDatabaseNode>2)
			{ $$ = $<alterDatabaseNode>2; }
	| KW_DOMAIN alter_domain				{ $$ = $2; }
	| INDEX alter_index_clause				{ $$ = $2; }
	| EXTERNAL FUNCTION alter_udf_clause	{ $$ = $3; }
	| FUNCTION alter_function_clause		{ $$ = $2; }
	| ROLE alter_role_clause				{ $$ = $2; }
	| USER alter_user_clause				{ $$ = $2; }
	| CURRENT USER alter_cur_user_clause	{ $$ = $3; }
	| CHARACTER SET alter_charset_clause	{ $$ = $3; }
	| GENERATOR alter_sequence_clause		{ $$ = $2; }
	| SEQUENCE alter_sequence_clause		{ $$ = $2; }
	| MAPPING alter_map_clause(false)		{ $$ = $2; }
	| GLOBAL MAPPING alter_map_clause(true)	{ $$ = $3; }
	;

%type <alterDomainNode> alter_domain
alter_domain
	: keyword_or_column
			{ $<alterDomainNode>$ = newNode<AlterDomainNode>(*$1); }
		alter_domain_ops($2)
			{ $$ = $2; }
	;

%type alter_domain_ops(<alterDomainNode>)
alter_domain_ops($alterDomainNode)
	: alter_domain_op($alterDomainNode)
	| alter_domain_ops alter_domain_op($alterDomainNode)
	;

%type alter_domain_op(<alterDomainNode>)
alter_domain_op($alterDomainNode)
	: SET domain_default
		{ setClause($alterDomainNode->setDefault, "DOMAIN DEFAULT", $2); }
	| ADD CONSTRAINT check_constraint
		{ setClause($alterDomainNode->setConstraint, "DOMAIN CONSTRAINT", $3); }
	| ADD check_constraint
		{ setClause($alterDomainNode->setConstraint, "DOMAIN CONSTRAINT", $2); }
	| DROP DEFAULT
		{ setClause($alterDomainNode->dropDefault, "DOMAIN DROP DEFAULT"); }
	| DROP CONSTRAINT
		{ setClause($alterDomainNode->dropConstraint, "DOMAIN DROP CONSTRAINT"); }
	| KW_NULL
		{ setClause($alterDomainNode->notNullFlag, "[NOT] NULL", false); }
	| NOT KW_NULL
		{ setClause($alterDomainNode->notNullFlag, "[NOT] NULL", true); }
	| TO symbol_column_name
		{ setClause($alterDomainNode->renameTo, "DOMAIN NAME", *$2); }
	| KW_TYPE non_array_type
		{
			//// FIXME: ALTER DOMAIN doesn't support collations, and altered domain's
			//// collation is always lost.
			dsql_fld* type = $2;
			type->collate = "";
			setClause($alterDomainNode->type, "DOMAIN TYPE", type);
		}
	;

%type alter_ops(<relationNode>)
alter_ops($relationNode)
	: alter_op($relationNode)
	| alter_ops ',' alter_op($relationNode)
	;

%type alter_op(<relationNode>)
alter_op($relationNode)
	: DROP symbol_column_name drop_behaviour
		{
			RelationNode::DropColumnClause* clause = newNode<RelationNode::DropColumnClause>();
			clause->name = *$2;
			clause->cascade = $3;
			$relationNode->clauses.add(clause);
		}
	| DROP CONSTRAINT symbol_constraint_name
		{
			RelationNode::DropConstraintClause* clause = newNode<RelationNode::DropConstraintClause>();
			clause->name = *$3;
			$relationNode->clauses.add(clause);
		}
	| ADD column_def($relationNode)
	| ADD table_constraint_definition($relationNode)
	| col_opt alter_column_name POSITION pos_short_integer
		{
			RelationNode::AlterColPosClause* clause = newNode<RelationNode::AlterColPosClause>();
			clause->name = *$2;
			clause->newPos = $4;
			$relationNode->clauses.add(clause);
		}
	| col_opt alter_column_name TO symbol_column_name
		{
			RelationNode::AlterColNameClause* clause = newNode<RelationNode::AlterColNameClause>();
			clause->fromName = *$2;
			clause->toName = *$4;
			$relationNode->clauses.add(clause);
		}
	| col_opt alter_column_name KW_NULL
		{
			RelationNode::AlterColNullClause* clause = newNode<RelationNode::AlterColNullClause>();
			clause->name = *$2;
			clause->notNullFlag = false;
			$relationNode->clauses.add(clause);
		}
	| col_opt alter_column_name NOT KW_NULL
		{
			RelationNode::AlterColNullClause* clause = newNode<RelationNode::AlterColNullClause>();
			clause->name = *$2;
			clause->notNullFlag = true;
			$relationNode->clauses.add(clause);
		}
	| col_opt symbol_column_name KW_TYPE alter_data_type_or_domain
		{
			RelationNode::AlterColTypeClause* clause = newNode<RelationNode::AlterColTypeClause>();
			clause->field = $4;
			clause->field->fld_name = *$2;
			$relationNode->clauses.add(clause);
		}
	| col_opt symbol_column_name KW_TYPE non_array_type def_computed
		{
			RelationNode::AlterColTypeClause* clause = newNode<RelationNode::AlterColTypeClause>();
			clause->field = newNode<dsql_fld>();
			clause->field->fld_name = *$2;
			clause->computed = $5;
			$relationNode->clauses.add(clause);
			clause->field->flags |= FLD_computed;
		}
	| col_opt symbol_column_name def_computed
		{
			RelationNode::AlterColTypeClause* clause = newNode<RelationNode::AlterColTypeClause>();
			clause->field = newNode<dsql_fld>();
			clause->field->fld_name = *$2;
			clause->computed = $3;
			$relationNode->clauses.add(clause);
			clause->field->flags |= FLD_computed;
		}
	| col_opt symbol_column_name SET domain_default
		{
			RelationNode::AlterColTypeClause* clause = newNode<RelationNode::AlterColTypeClause>();
			clause->field = newNode<dsql_fld>();
			clause->field->fld_name = *$2;
			clause->defaultValue = $4;
			$relationNode->clauses.add(clause);
		}
	| col_opt symbol_column_name DROP DEFAULT
		{
			RelationNode::AlterColTypeClause* clause = newNode<RelationNode::AlterColTypeClause>();
			clause->field = newNode<dsql_fld>();
			clause->field->fld_name = *$2;
			clause->dropDefault = true;
			$relationNode->clauses.add(clause);
		}
	| col_opt symbol_column_name restart_value_opt
		{
			RelationNode::AlterColTypeClause* clause = newNode<RelationNode::AlterColTypeClause>();
			clause->field = newNode<dsql_fld>();
			clause->field->fld_name = *$2;
			clause->identityRestart = true;
			clause->identityRestartValue = $3;
			$relationNode->clauses.add(clause);
		}
	;

%type <metaNamePtr> alter_column_name
alter_column_name
	: keyword_or_column
	;

// below are reserved words that could be used as column identifiers
// in the previous versions

%type <metaNamePtr> keyword_or_column
keyword_or_column
	: valid_symbol_name
	| ADMIN					// added in IB 5.0
	| COLUMN				// added in IB 6.0
	| EXTRACT
	| YEAR
	| MONTH
	| DAY
	| HOUR
	| MINUTE
	| SECOND
	| TIME
	| TIMESTAMP
	| CURRENT_DATE
	| CURRENT_TIME
	| CURRENT_TIMESTAMP
	| CURRENT_USER			// added in FB 1.0
	| CURRENT_ROLE
	| RECREATE
	| CURRENT_CONNECTION	// added in FB 1.5
	| CURRENT_TRANSACTION
	| BIGINT
	| CASE
	| RELEASE
	| ROW_COUNT
	| SAVEPOINT
	| OPEN					// added in FB 2.0
	| CLOSE
	| FETCH
	| ROWS
	| USING
	| CROSS
	| BIT_LENGTH
	| BOTH
	| CHAR_LENGTH
	| CHARACTER_LENGTH
	| COMMENT
	| LEADING
	| KW_LOWER
	| OCTET_LENGTH
	| TRAILING
	| TRIM
	| CONNECT				// added in FB 2.1
	| DISCONNECT
	| GLOBAL
	| INSENSITIVE
	| RECURSIVE
	| SENSITIVE
	| START
	| SIMILAR				// added in FB 2.5
	| OVER					// added in FB 3.0
	| SCROLL
	| RETURN
	| DETERMINISTIC
	| SQLSTATE
	;

col_opt
	: ALTER
	| ALTER COLUMN
	;

%type <legacyField> alter_data_type_or_domain
alter_data_type_or_domain
	: non_array_type
	| symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$1;
		}
	;

%type <boolVal> drop_behaviour
drop_behaviour
	:				{ $$ = false; }
	| RESTRICT		{ $$ = false; }
	| CASCADE		{ $$ = true; }
	;

%type <ddlNode>	alter_index_clause
alter_index_clause
	: symbol_index_name ACTIVE		{ $$ = newNode<AlterIndexNode>(*$1, true); }
	| symbol_index_name INACTIVE	{ $$ = newNode<AlterIndexNode>(*$1, false); }
	;

%type <ddlNode>	alter_udf_clause
alter_udf_clause
	: symbol_UDF_name entry_op module_op
		{
			AlterExternalFunctionNode* node = newNode<AlterExternalFunctionNode>(*$1);
			if ($2)
				node->clauses.name = *$2;
			if ($3)
				node->clauses.udfModule = *$3;
			$$ = node;
		}
	;

%type <stringPtr> entry_op
entry_op
	: /* nothing */				{ $$ = NULL; }
	| ENTRY_POINT utf_string	{ $$ = $2; }
	;

%type <stringPtr> module_op
module_op
	: /* nothing */				{ $$ = NULL; }
	| MODULE_NAME utf_string	{ $$ = $2; }
	;

%type <mappingNode>	alter_role_clause
alter_role_clause
	: symbol_role_name alter_role_enable AUTO ADMIN MAPPING
		{
			$$ = newNode<MappingNode>(MappingNode::MAP_RPL, "AutoAdminImplementationMapping");
			$$->op = $2 ? MappingNode::MAP_RPL : MappingNode::MAP_DROP;
			$$->from = newNode<IntlString>(FB_DOMAIN_ANY_RID_ADMINS);
			$$->fromType = newNode<MetaName>(FB_PREDEFINED_GROUP);
			$$->mode = 'P';
			$$->plugin = newNode<MetaName>("Win_Sspi");
			$$->role = true;
			$$->to = $1;
			$$->validateAdmin();
		}
	;

%type <boolVal>	alter_role_enable
alter_role_enable
	: SET		{ $$ = true; }
	| DROP		{ $$ = false; }
	;


// ALTER DATABASE

%type alter_db(<alterDatabaseNode>)
alter_db($alterDatabaseNode)
	: db_alter_clause($alterDatabaseNode)
	| alter_db db_alter_clause($alterDatabaseNode)
	;

%type db_alter_clause(<alterDatabaseNode>)
db_alter_clause($alterDatabaseNode)
	: ADD db_file_list(NOTRIAL(&$alterDatabaseNode->files))
	| ADD KW_DIFFERENCE KW_FILE utf_string
		{ $alterDatabaseNode->differenceFile = *$4; }
	| DROP KW_DIFFERENCE KW_FILE
		{ $alterDatabaseNode->clauses |= AlterDatabaseNode::CLAUSE_DROP_DIFFERENCE; }
	| BEGIN BACKUP
		{ $alterDatabaseNode->clauses |= AlterDatabaseNode::CLAUSE_BEGIN_BACKUP; }
	| END BACKUP
		{ $alterDatabaseNode->clauses |= AlterDatabaseNode::CLAUSE_END_BACKUP; }
	| SET DEFAULT CHARACTER SET symbol_character_set_name
		{ $alterDatabaseNode->setDefaultCharSet = *$5; }
	| ENCRYPT WITH valid_symbol_name
		{ $alterDatabaseNode->cryptPlugin = *$3; }
	| DECRYPT
		{ $alterDatabaseNode->clauses |= AlterDatabaseNode::CLAUSE_DECRYPT; }
	| SET LINGER TO long_integer
		{ $alterDatabaseNode->linger = $4; }
	| DROP LINGER
		{ $alterDatabaseNode->linger = 0; }
	;


// ALTER TRIGGER

%type <createAlterTriggerNode> alter_trigger_clause
alter_trigger_clause
	: symbol_trigger_name trigger_active trigger_type_opt trigger_position
			AS local_declaration_list full_proc_block
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->alter = true;
			$$->create = false;
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
			$$->source = makeParseStr(YYPOSNARG(5), YYPOSNARG(7));
			$$->localDeclList = $6;
			$$->body = $7;
		}
	| symbol_trigger_name trigger_active trigger_type_opt trigger_position
			external_clause external_body_clause_opt
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->alter = true;
			$$->create = false;
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
			$$->external = $5;
			if ($6)
				$$->source = *$6;
		}
	| symbol_trigger_name trigger_active trigger_type_opt trigger_position
		{
			$$ = newNode<CreateAlterTriggerNode>(*$1);
			$$->alter = true;
			$$->create = false;
			$$->active = $2;
			$$->type = $3;
			$$->position = $4;
		}
	;

%type <nullableUint64Val> trigger_type_opt
trigger_type_opt	// we do not allow alter database triggers, hence we do not use trigger_type here
	: trigger_type_prefix trigger_type_suffix
		{ $$ = Nullable<FB_UINT64>::val($1 + $2 - 1); }
	|
		{ $$ = Nullable<FB_UINT64>::empty(); }
	;


// DROP metadata operations

%type <ddlNode> drop
drop
	: DROP drop_clause	{ $$ = $2; }
	;

%type <ddlNode> drop_clause
drop_clause
	: EXCEPTION symbol_exception_name
		{ $$ = newNode<DropExceptionNode>(*$2); }
	| INDEX symbol_index_name
		{ $$ = newNode<DropIndexNode>(*$2); }
	| PROCEDURE symbol_procedure_name
		{ $$ = newNode<DropProcedureNode>(*$2); }
	| TABLE symbol_table_name
		{ $$ = newNode<DropRelationNode>(*$2, false); }
	| TRIGGER symbol_trigger_name
		{ $$ = newNode<DropTriggerNode>(*$2); }
	| VIEW symbol_view_name
		{ $$ = newNode<DropRelationNode>(*$2, true); }
	| FILTER symbol_filter_name
		{ $$ = newNode<DropFilterNode>(*$2); }
	| KW_DOMAIN symbol_domain_name
		{ $$ = newNode<DropDomainNode>(*$2); }
	| EXTERNAL FUNCTION symbol_UDF_name
		{ $$ = newNode<DropFunctionNode>(*$3); }
	| FUNCTION symbol_UDF_name
		{ $$ = newNode<DropFunctionNode>(*$2); }
	| SHADOW pos_short_integer
		{ $$ = newNode<DropShadowNode>($2); }
	| ROLE symbol_role_name
		{ $$ = newNode<DropRoleNode>(*$2); }
	| GENERATOR symbol_generator_name
		{ $$ = newNode<DropSequenceNode>(*$2); }
	| SEQUENCE symbol_generator_name
		{ $$ = newNode<DropSequenceNode>(*$2); }
	| COLLATION symbol_collation_name
		{ $$ = newNode<DropCollationNode>(*$2); }
	| USER symbol_user_name
		{ $$ = newNode<DropUserNode>(*$2); }
	| PACKAGE symbol_package_name
		{ $$ = newNode<DropPackageNode>(*$2); }
	| PACKAGE BODY symbol_package_name
		{ $$ = newNode<DropPackageBodyNode>(*$3); }
	| MAPPING drop_map_clause(false)
		{ $$ = $2; }
	| GLOBAL MAPPING drop_map_clause(true)
		{ $$ = $3; }
	;


// these are the allowable datatypes

%type <legacyField> data_type
data_type
	: non_array_type
	| array_type
	;

%type <legacyField> domain_or_non_array_type
domain_or_non_array_type
	: domain_or_non_array_type_name
	| domain_or_non_array_type_name NOT KW_NULL
		{
			$$ = $1;
			$$->notNull = true;
		}
	;

%type <legacyField> domain_or_non_array_type_name
domain_or_non_array_type_name
	: non_array_type
	| domain_type
	;

%type <legacyField> domain_type
domain_type
	: KW_TYPE OF symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$3;
		}
	| KW_TYPE OF COLUMN symbol_column_name '.' symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$6;
			$$->typeOfTable = *$4;
		}
	| symbol_column_name
		{
			$$ = newNode<dsql_fld>();
			$$->typeOfName = *$1;
			$$->fullDomain = true;
		}
	;

%type <legacyField> non_array_type
non_array_type
	: simple_type
	| blob_type
	;

%type <legacyField> array_type
array_type
	: non_charset_simple_type '[' array_spec ']'
		{
			$1->ranges = $3;
			$1->dimensions = $1->ranges->items.getCount() / 2;
			$1->elementDtype = $1->dtype;
			$$ = $1;
		}
	| character_type '[' array_spec ']' charset_clause
		{
			$1->ranges = $3;
			$1->dimensions = $1->ranges->items.getCount() / 2;
			$1->elementDtype = $1->dtype;
			if ($5)
				$1->charSet = *$5;
			$$ = $1;
		}
	;

%type <valueListNode> array_spec
array_spec
	: array_range
	| array_spec ',' array_range	{ $$ = $1->add($3->items[0])->add($3->items[1]); }
	;

%type <valueListNode> array_range
array_range
	: signed_long_integer
		{
			if ($1 < 1)
		 		$$ = newNode<ValueListNode>(MAKE_const_slong($1))->add(MAKE_const_slong(1));
			else
		 		$$ = newNode<ValueListNode>(MAKE_const_slong(1))->add(MAKE_const_slong($1));
		}
	| signed_long_integer ':' signed_long_integer
 		{ $$ = newNode<ValueListNode>(MAKE_const_slong($1))->add(MAKE_const_slong($3)); }
	;

%type <legacyField> simple_type
simple_type
	: non_charset_simple_type
	| character_type charset_clause
		{
			$$ = $1;
			if ($2)
				$$->charSet = *$2;
		}
	;

%type <legacyField> non_charset_simple_type
non_charset_simple_type
	: national_character_type
	| numeric_type
	| float_type
	| BIGINT
		{
			$$ = newNode<dsql_fld>();

			if (client_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post (Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					Arg::Gds(isc_sql_dialect_datatype_unsupport) << Arg::Num(client_dialect) <<
																	Arg::Str("BIGINT"));
			}

			if (db_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post (Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					Arg::Gds(isc_sql_db_dialect_dtype_unsupport) << Arg::Num(db_dialect) <<
																Arg::Str("BIGINT"));
			}

			$$->dtype = dtype_int64;
			$$->length = sizeof(SINT64);
		}
	| integer_keyword
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_long;
			$$->length = sizeof(SLONG);
		}
	| SMALLINT
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_short;
			$$->length = sizeof(SSHORT);
		}
	| DATE
		{
			$$ = newNode<dsql_fld>();
			stmt_ambiguous = true;

			if (client_dialect <= SQL_DIALECT_V5)
			{
				// Post warning saying that DATE is equivalent to TIMESTAMP
				ERRD_post_warning(Arg::Warning(isc_sqlwarn) << Arg::Num(301) <<
								  Arg::Warning(isc_dtype_renamed));
				$$->dtype = dtype_timestamp;
				$$->length = sizeof(GDS_TIMESTAMP);
			}
			else if (client_dialect == SQL_DIALECT_V6_TRANSITION)
				yyabandon(-104, isc_transitional_date);
			else
			{
				$$->dtype = dtype_sql_date;
				$$->length = sizeof(ULONG);
			}
		}
	| TIME
		{
			$$ = newNode<dsql_fld>();

			if (client_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_dialect_datatype_unsupport) << Arg::Num(client_dialect) <<
																		  Arg::Str("TIME"));
			}
			if (db_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_db_dialect_dtype_unsupport) << Arg::Num(db_dialect) <<
																		  Arg::Str("TIME"));
			}
			$$->dtype = dtype_sql_time;
			$$->length = sizeof(SLONG);
		}
	| TIMESTAMP
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_timestamp;
			$$->length = sizeof(GDS_TIMESTAMP);
		}
	| KW_BOOLEAN
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_boolean;
			$$->length = sizeof(UCHAR);
		}
	;

integer_keyword
	: INTEGER
	| KW_INT
	;


// allow a blob to be specified with any combination of segment length and subtype

%type <legacyField> blob_type
blob_type
	: BLOB { $$ = newNode<dsql_fld>(); } blob_subtype(NOTRIAL($2)) blob_segsize charset_clause
		{
			$$ = $2;
			$$->dtype = dtype_blob;
			$$->length = sizeof(ISC_QUAD);
			$$->segLength = $4;
			if ($5)
				$$->charSet = *$5;
		}
	| BLOB '(' unsigned_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_blob;
			$$->length = sizeof(ISC_QUAD);
			$$->segLength = (USHORT) $3;
			$$->subType = 0;
		}
	| BLOB '(' unsigned_short_integer ',' signed_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_blob;
			$$->length = sizeof(ISC_QUAD);
			$$->segLength = (USHORT) $3;
			$$->subType = (USHORT) $5;
		}
	| BLOB '(' ',' signed_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_blob;
			$$->length = sizeof(ISC_QUAD);
			$$->segLength = 80;
			$$->subType = (USHORT) $4;
		}
	;

%type <uintVal> blob_segsize
blob_segsize
	: /* nothing */								{ $$ = (USHORT) 80; }
	| SEGMENT KW_SIZE unsigned_short_integer	{ $$ = (USHORT) $3; }
	;

%type blob_subtype(<legacyField>)
blob_subtype($field)
	: // nothing
		{ $field->subType = (USHORT) 0; }
	| SUB_TYPE signed_short_integer
		{ $field->subType = (USHORT) $2; }
	| SUB_TYPE symbol_blob_subtype_name
		{ $field->subTypeName = *$2; }
	;

%type <metaNamePtr> charset_clause
charset_clause
	: /* nothing */								{ $$ = NULL; }
	| CHARACTER SET symbol_character_set_name	{ $$ = $3; }
	;


// character type


%type <legacyField> national_character_type
national_character_type
	: national_character_keyword '(' pos_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_text;
			$$->charLength = (USHORT) $3;
			$$->flags |= FLD_national;
		}
	| national_character_keyword
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_text;
			$$->charLength = 1;
			$$->flags |= FLD_national;
		}
	| national_character_keyword VARYING '(' pos_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_varying;
			$$->charLength = (USHORT) $4;
			$$->flags |= FLD_national;
		}
	;

%type <legacyField> character_type
character_type
	: character_keyword '(' pos_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_text;
			$$->charLength = (USHORT) $3;
		}
	| character_keyword
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_text;
			$$->charLength = 1;
		}
	| varying_keyword '(' pos_short_integer ')'
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_varying;
			$$->charLength = (USHORT) $3;
		}
	;

varying_keyword
	: VARCHAR
	| CHARACTER VARYING
	| KW_CHAR VARYING
	;

character_keyword
	: CHARACTER
	| KW_CHAR
	;

national_character_keyword
	: NCHAR
	| NATIONAL CHARACTER
	| NATIONAL KW_CHAR
	;


// numeric type

%type <legacyField> numeric_type
numeric_type
	: KW_NUMERIC prec_scale
		{
			$$ = $2;
			$$->subType = dsc_num_type_numeric;
		}
	| decimal_keyword prec_scale
		{
			$$ = $2;
			$$->subType = dsc_num_type_decimal;

			if ($$->dtype == dtype_short)
			{
				$$->dtype = dtype_long;
				$$->length = sizeof(SLONG);
			}
		}
	;

%type <legacyField> prec_scale
prec_scale
	: // nothing
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_long;
			$$->length = sizeof(SLONG);
			$$->precision = 9;
		}
	| '(' signed_long_integer ')'
		{
			$$ = newNode<dsql_fld>();

			if ($2 < 1 || $2 > 18)
				yyabandon(-842, isc_precision_err);	// Precision must be between 1 and 18.

			if ($2 > 9)
			{
				if ( ( (client_dialect <= SQL_DIALECT_V5) && (db_dialect > SQL_DIALECT_V5) ) ||
					( (client_dialect > SQL_DIALECT_V5) && (db_dialect <= SQL_DIALECT_V5) ) )
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-817) <<
							  Arg::Gds(isc_ddl_not_allowed_by_db_sql_dial) << Arg::Num(db_dialect));
				}

				if (client_dialect <= SQL_DIALECT_V5)
				{
					$$->dtype = dtype_double;
					$$->length = sizeof(double);
				}
				else
				{
					if (client_dialect == SQL_DIALECT_V6_TRANSITION)
					{
						ERRD_post_warning(Arg::Warning(isc_dsql_warn_precision_ambiguous));
						ERRD_post_warning(Arg::Warning(isc_dsql_warn_precision_ambiguous1));
						ERRD_post_warning(Arg::Warning(isc_dsql_warn_precision_ambiguous2));
					}

					$$->dtype = dtype_int64;
					$$->length = sizeof(SINT64);
				}
			}
			else
			{
				if ($2 < 5)
				{
					$$->dtype = dtype_short;
					$$->length = sizeof(SSHORT);
				}
				else
				{
					$$->dtype = dtype_long;
					$$->length = sizeof(SLONG);
				}
			}

			$$->precision = (USHORT) $2;
		}
	| '(' signed_long_integer ',' signed_long_integer ')'
		{
			$$ = newNode<dsql_fld>();

			if ($2 < 1 || $2 > 18)
				yyabandon (-842, isc_precision_err);	// Precision should be between 1 and 18

			if ($4 > $2 || $4 < 0)
				yyabandon (-842, isc_scale_nogt);	// Scale must be between 0 and precision

			if ($2 > 9)
			{
				if ( ( (client_dialect <= SQL_DIALECT_V5) && (db_dialect > SQL_DIALECT_V5) ) ||
					( (client_dialect > SQL_DIALECT_V5) && (db_dialect <= SQL_DIALECT_V5) ) )
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-817) <<
							  Arg::Gds(isc_ddl_not_allowed_by_db_sql_dial) << Arg::Num(db_dialect));
				}

				if (client_dialect <= SQL_DIALECT_V5)
				{
					$$->dtype = dtype_double;
					$$->length = sizeof(double);
				}
				else
				{
					if (client_dialect == SQL_DIALECT_V6_TRANSITION)
					{
						ERRD_post_warning(Arg::Warning(isc_dsql_warn_precision_ambiguous));
						ERRD_post_warning(Arg::Warning(isc_dsql_warn_precision_ambiguous1));
						ERRD_post_warning(Arg::Warning(isc_dsql_warn_precision_ambiguous2));
					}
					// client_dialect >= SQL_DIALECT_V6
					$$->dtype = dtype_int64;
					$$->length = sizeof(SINT64);
				}
			}
			else
			{
				if ($2 < 5)
				{
					$$->dtype = dtype_short;
					$$->length = sizeof(SSHORT);
				}
				else
				{
					$$->dtype = dtype_long;
					$$->length = sizeof(SLONG);
				}
			}

			$$->precision = (USHORT) $2;
			$$->scale = - (SSHORT) $4;
		}
	;

decimal_keyword
	: DECIMAL
	| KW_DEC
	;


// floating point type

%type <legacyField> float_type
float_type
	: KW_FLOAT precision_opt
		{
			$$ = newNode<dsql_fld>();

			if ($2 > 7)
			{
				$$->dtype = dtype_double;
				$$->length = sizeof(double);
			}
			else
			{
				$$->dtype = dtype_real;
				$$->length = sizeof(float);
			}
		}
	| KW_LONG KW_FLOAT precision_opt
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_double;
			$$->length = sizeof(double);
		}
	| REAL
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_real;
			$$->length = sizeof(float);
		}
	| KW_DOUBLE PRECISION
		{
			$$ = newNode<dsql_fld>();
			$$->dtype = dtype_double;
			$$->length = sizeof(double);
		}
	;

%type <int32Val> precision_opt
precision_opt
	: /* nothing */					{ $$ = 0; }
	| '(' nonneg_short_integer ')'	{ $$ = $2; }
	;


// transaction statements

%type <stmtNode> savepoint
savepoint
	: set_savepoint
	| release_savepoint
	| undo_savepoint
	;

%type <stmtNode> set_savepoint
set_savepoint
	: SAVEPOINT symbol_savepoint_name
		{
			UserSavepointNode* node = newNode<UserSavepointNode>();
			node->command = UserSavepointNode::CMD_SET;
			node->name = *$2;
			$$ = node;
		}
	;

%type <stmtNode> release_savepoint
release_savepoint
	: RELEASE SAVEPOINT symbol_savepoint_name release_only_opt
		{
			UserSavepointNode* node = newNode<UserSavepointNode>();
			node->command = ($4 ? UserSavepointNode::CMD_RELEASE_ONLY : UserSavepointNode::CMD_RELEASE);
			node->name = *$3;
			$$ = node;
		}
	;

%type <boolVal> release_only_opt
release_only_opt
	: /* nothing */		{ $$ = false; }
	| ONLY				{ $$ = true; }
	;

%type <stmtNode> undo_savepoint
undo_savepoint
	: ROLLBACK optional_work TO optional_savepoint symbol_savepoint_name
		{
			UserSavepointNode* node = newNode<UserSavepointNode>();
			node->command = UserSavepointNode::CMD_ROLLBACK;
			node->name = *$5;
			$$ = node;
		}
	;

optional_savepoint
	: // nothing
	| SAVEPOINT
	;

%type <traNode> commit
commit
	: COMMIT optional_work optional_retain
		{ $$ = newNode<CommitRollbackNode>(CommitRollbackNode::CMD_COMMIT, $3); }
	;

%type <traNode> rollback
rollback
	: ROLLBACK optional_work optional_retain
		{ $$ = newNode<CommitRollbackNode>(CommitRollbackNode::CMD_ROLLBACK, $3); }
	;

optional_work
	: // nothing
	| WORK
	;

%type <boolVal>	optional_retain
optional_retain
	: /* nothing */		 	{ $$ = false; }
	| RETAIN opt_snapshot	{ $$ = true; }
	;

opt_snapshot
	: // nothing
	| SNAPSHOT
	;

%type <setTransactionNode> set_transaction
set_transaction
	: SET TRANSACTION
			{ $$ = newNode<SetTransactionNode>(); }
		tran_option_list_opt($3)
			{ $$ = $3; }
	;

%type <setRoleNode> set_role
set_role
	: SET ROLE valid_symbol_name
		{ $$ = newNode<SetRoleNode>($3); }
	| SET TRUSTED ROLE
		{ $$ = newNode<SetRoleNode>(); }
	;

%type tran_option_list_opt(<setTransactionNode>)
tran_option_list_opt($setTransactionNode)
	: // nothing
	| tran_option_list($setTransactionNode)
	;

%type tran_option_list(<setTransactionNode>)
tran_option_list($setTransactionNode)
	: tran_option($setTransactionNode)
	| tran_option_list tran_option($setTransactionNode)
	;

%type tran_option(<setTransactionNode>)
tran_option($setTransactionNode)
	// access mode
	: READ ONLY
		{ setClause($setTransactionNode->readOnly, "READ {ONLY | WRITE}", true); }
	| READ WRITE
		{ setClause($setTransactionNode->readOnly, "READ {ONLY | WRITE}", false); }
	// wait mode
	| WAIT
		{ setClause($setTransactionNode->wait, "[NO] WAIT", true); }
	| NO WAIT
		{ setClause($setTransactionNode->wait, "[NO] WAIT", false); }
	// isolation mode
	| isolation_mode
		{ setClause($setTransactionNode->isoLevel, "ISOLATION LEVEL", $1); }
	// misc options
	| NO AUTO UNDO
		{ setClause($setTransactionNode->noAutoUndo, "NO AUTO UNDO", true); }
	| KW_IGNORE LIMBO
		{ setClause($setTransactionNode->ignoreLimbo, "IGNORE LIMBO", true); }
	| RESTART REQUESTS
		{ setClause($setTransactionNode->restartRequests, "RESTART REQUESTS", true); }
	// timeout
	| LOCK TIMEOUT nonneg_short_integer
		{ setClause($setTransactionNode->lockTimeout, "LOCK TIMEOUT", (USHORT) $3); }
	// reserve options
	| RESERVING
			{ checkDuplicateClause($setTransactionNode->reserveList, "RESERVING"); }
		restr_list($setTransactionNode)
	;

%type <uintVal>	isolation_mode
isolation_mode
	: ISOLATION LEVEL iso_mode	{ $$ = $3;}
	| iso_mode
	;

%type <uintVal>	iso_mode
iso_mode
	: snap_shot
	| READ UNCOMMITTED version_mode		{ $$ = $3; }
	| READ COMMITTED version_mode		{ $$ = $3; }
	;

%type <uintVal>	snap_shot
snap_shot
	: SNAPSHOT					{ $$ = SetTransactionNode::ISO_LEVEL_CONCURRENCY; }
	| SNAPSHOT TABLE			{ $$ = SetTransactionNode::ISO_LEVEL_CONSISTENCY; }
	| SNAPSHOT TABLE STABILITY	{ $$ = SetTransactionNode::ISO_LEVEL_CONSISTENCY; }
	;

%type <uintVal>	version_mode
version_mode
	: /* nothing */	{ $$ = SetTransactionNode::ISO_LEVEL_READ_COMMITTED_NO_REC_VERSION; }
	| VERSION		{ $$ = SetTransactionNode::ISO_LEVEL_READ_COMMITTED_REC_VERSION; }
	| NO VERSION	{ $$ = SetTransactionNode::ISO_LEVEL_READ_COMMITTED_NO_REC_VERSION; }
	;

%type <uintVal> lock_type
lock_type
	: /* nothing */		{ $$ = 0; }
	| KW_SHARED			{ $$ = SetTransactionNode::LOCK_MODE_SHARED; }
	| PROTECTED			{ $$ = SetTransactionNode::LOCK_MODE_PROTECTED; }
	;

%type <uintVal> lock_mode
lock_mode
	: READ		{ $$ = SetTransactionNode::LOCK_MODE_READ; }
	| WRITE		{ $$ = SetTransactionNode::LOCK_MODE_WRITE; }
	;

%type restr_list(<setTransactionNode>)
restr_list($setTransactionNode)
	: restr_option
		{ $setTransactionNode->reserveList.add($1); }
	| restr_list ',' restr_option
		{ $setTransactionNode->reserveList.add($3); }
	;

%type <setTransactionRestrictionClause> restr_option
restr_option
	: table_list table_lock
		{ $$ = newNode<SetTransactionNode::RestrictionOption>($1, $2); }
	;

%type <uintVal> table_lock
table_lock
	: /* nothing */				{ $$ = 0; }
	| FOR lock_type lock_mode	{ $$ = $2 | $3; }
	;

%type <metaNameArray> table_list
table_list
	: symbol_table_name
		{
			ObjectsArray<MetaName>* node = newNode<ObjectsArray<MetaName> >();
			node->add(*$1);
			$$ = node;
		}
	| table_list ',' symbol_table_name
		{
			ObjectsArray<MetaName>* node = $1;
			node->add(*$3);
			$$ = node;
		}
	;


%type <ddlNode>	set_statistics
set_statistics
	: SET STATISTICS INDEX symbol_index_name
		{ $$ = newNode<SetStatisticsNode>(*$4); }
	;

%type <ddlNode> comment
comment
	: COMMENT ON ddl_type0 IS ddl_desc
		{ $$ = newNode<CommentOnNode>($3, "", "", *$5); }
	| COMMENT ON ddl_type1 symbol_ddl_name IS ddl_desc
		{ $$ = newNode<CommentOnNode>($3, *$4, "", *$6); }
	| COMMENT ON ddl_type2 symbol_ddl_name ddl_subname IS ddl_desc
		{ $$ = newNode<CommentOnNode>($3, *$4, *$5, *$7); }
	| COMMENT ON USER symbol_user_name IS ddl_desc
		{
			CreateAlterUserNode* node =
				newNode<CreateAlterUserNode>(CreateAlterUserNode::USER_MOD, *$4);
			node->comment = $6;
			$$ = node;
		}
	;

%type <intVal> ddl_type0
ddl_type0
	: DATABASE
		{ $$ = obj_database; }
	;

%type <intVal> ddl_type1
ddl_type1
	: KW_DOMAIN				{ $$ = obj_field; }
	| TABLE					{ $$ = obj_relation; }
	| VIEW					{ $$ = obj_view; }
	| PROCEDURE				{ $$ = obj_procedure; }
	| TRIGGER				{ $$ = obj_trigger; }
	| EXTERNAL FUNCTION		{ $$ = obj_udf; }
	| FUNCTION				{ $$ = obj_udf; }
	| FILTER				{ $$ = obj_blob_filter; }
	| EXCEPTION				{ $$ = obj_exception; }
	| GENERATOR				{ $$ = obj_generator; }
	| SEQUENCE				{ $$ = obj_generator; }
	| INDEX					{ $$ = obj_index; }
	| ROLE					{ $$ = obj_sql_role; }
	| CHARACTER SET			{ $$ = obj_charset; }
	| COLLATION				{ $$ = obj_collation; }
	| PACKAGE				{ $$ = obj_package_header; }
	/***
	| SECURITY CLASS		{ $$ = ddl_sec_class; }
	***/
	;

%type <intVal> ddl_type2
ddl_type2
	: COLUMN					{ $$ = obj_relation; }
	| ddl_param_opt PARAMETER	{ $$ = $1; }
	;

%type <intVal> ddl_param_opt
ddl_param_opt
	:			{ $$ = obj_parameter; }
	| PROCEDURE	{ $$ = obj_procedure; }
	| FUNCTION	{ $$ = obj_udf; }
	;

%type <metaNamePtr> ddl_subname
ddl_subname
	: '.' symbol_ddl_name	{ $$ = $2; }
	;

%type <stringPtr> ddl_desc
ddl_desc
    : utf_string	{ $$ = $1; }
	| KW_NULL		{ $$ = newString(""); }
	;


// SELECT statement

%type <selectNode> select
select
	: select_expr for_update_clause lock_clause
		{
			SelectNode* node = newNode<SelectNode>();
			node->dsqlExpr = $1;
			node->dsqlForUpdate = $2;
			node->dsqlWithLock = $3;
			$$ = node;
		}
	;

%type <boolVal>	for_update_clause
for_update_clause
	: /* nothing */					{ $$ = false; }
	| FOR UPDATE for_update_list	{ $$ = true; /* for_update_list is ignored */ }
	;

%type <valueListNode> for_update_list
for_update_list
	: /* nothing */		{ $$ = NULL; }
	| OF column_list	{ $$ = $2; }
	;

%type <boolVal>	lock_clause
lock_clause
	: /* nothing */	{ $$ = false; }
	| WITH LOCK		{ $$ = true; }
	;


// SELECT expression

%type <selectExprNode> select_expr
select_expr
	: with_clause select_expr_body order_clause rows_clause
		{
			SelectExprNode* node = $$ = newNode<SelectExprNode>();
			node->querySpec = $2;
			node->orderClause = $3;
			node->rowsClause = $4;
			node->withClause = $1;
		}
	;

%type <withClause> with_clause
with_clause
	: // nothing
		{ $$ = NULL; }
	| WITH RECURSIVE with_list
		{
			$$ = $3;
			$$->recursive = true;
		}
	| WITH with_list
		{ $$ = $2; }
	;

%type <withClause> with_list
with_list
	: with_item
		{
			$$ = newNode<WithClause>();
			$$->add($1);
		}
	| with_item ',' with_list
		{
			$$ = $3;
			$$->add($1);
		}
	;

%type <selectExprNode> with_item
with_item
	: symbol_table_alias_name derived_column_list AS '(' select_expr ')'
		{
			$$ = $5;
			$$->dsqlFlags |= RecordSourceNode::DFLAG_DERIVED;
			$$->alias = $1->c_str();
			$$->columns = $2;
		}
	;

%type <selectExprNode> column_select
column_select
	: select_expr
		{
			$$ = $1;
			$$->dsqlFlags |= RecordSourceNode::DFLAG_VALUE;
		}
	;

%type <valueExprNode> column_singleton
column_singleton
	: column_select
		{
			$1->dsqlFlags |= RecordSourceNode::DFLAG_SINGLETON;
			$$ = newNode<SubQueryNode>(blr_via, $1);
		}
	;

%type <recSourceNode> select_expr_body
select_expr_body
	: query_term
		{ $$ = $1; }
	| select_expr_body UNION distinct_noise query_term
		{
			UnionSourceNode* node = $1->as<UnionSourceNode>();
			if (node && !node->dsqlAll)
				node->dsqlClauses->add($4);
			else
			{
				node = newNode<UnionSourceNode>();
				node->dsqlClauses = newNode<RecSourceListNode>($1)->add($4);
			}
			$$ = node;
		}
	| select_expr_body UNION ALL query_term
		{
			UnionSourceNode* node = $1->as<UnionSourceNode>();
			if (node && node->dsqlAll)
				node->dsqlClauses->add($4);
			else
			{
				node = newNode<UnionSourceNode>();
				node->dsqlAll = true;
				node->dsqlClauses = newNode<RecSourceListNode>($1)->add($4);
			}
			$$ = node;
		}
	;

%type <rseNode> query_term
query_term
	: query_spec
	;

%type <rseNode> query_spec
query_spec
	: SELECT limit_clause
			 distinct_clause
			 select_list
			 from_clause
			 where_clause
			 group_clause
			 having_clause
			 plan_clause
		{
			RseNode* rse = newNode<RseNode>();
			rse->dsqlFirst = $2 ? $2->items[1] : NULL;
			rse->dsqlSkip = $2 ? $2->items[0] : NULL;
			rse->dsqlDistinct = $3;
			rse->dsqlSelectList = $4;
			rse->dsqlFrom = $5;
			rse->dsqlWhere = $6;
			rse->dsqlGroup = $7;
			rse->dsqlHaving = $8;
			rse->rse_plan = $9;
			$$ = rse;
		}
	;

%type <valueListNode> limit_clause
limit_clause
	: /* nothing */				{ $$ = NULL; }
	| first_clause skip_clause	{ $$ = newNode<ValueListNode>($2)->add($1); }
	| first_clause				{ $$ = newNode<ValueListNode>(1u)->add($1); }
	| skip_clause				{ $$ = newNode<ValueListNode>($1)->add(NULL); }
	;

%type <valueExprNode> first_clause
first_clause
	: FIRST long_integer	{ $$ = MAKE_const_slong($2); }
	| FIRST '(' value ')'	{ $$ = $3; }
	| FIRST parameter		{ $$ = $2; }
	;

%type <valueExprNode> skip_clause
skip_clause
	: SKIP long_integer		{ $$ = MAKE_const_slong($2); }
	| SKIP '(' value ')'	{ $$ = $3; }
	| SKIP parameter		{ $$ = $2; }
	;

%type <valueListNode> distinct_clause
distinct_clause
	: DISTINCT		{ $$ = newNode<ValueListNode>(0); }
	| all_noise		{ $$ = NULL; }
	;

%type <valueListNode> select_list
select_list
	: select_items	{ $$ = $1; }
	| '*'			{ $$ = NULL; }
	;

%type <valueListNode> select_items
select_items
	: select_item					{ $$ = newNode<ValueListNode>($1); }
	| select_items ',' select_item	{ $$ = $1->add($3); }
	;

%type <valueExprNode> select_item
select_item
	: value_opt_alias
	| symbol_table_alias_name '.' '*'
		{
			FieldNode* fieldNode = newNode<FieldNode>();
			fieldNode->dsqlQualifier = *$1;
			$$ = fieldNode;
		}
	;

%type <valueExprNode> value_opt_alias
value_opt_alias
	: value
	| value as_noise symbol_item_alias_name		{ $$ = newNode<DsqlAliasNode>(*$3, $1); }
	;

as_noise
	:
	| AS
	;

// FROM clause

%type <recSourceListNode> from_clause
from_clause
	: FROM from_list	{ $$ = $2; }
	;

%type <recSourceListNode> from_list
from_list
	: table_reference					{ $$ = newNode<RecSourceListNode>($1); }
	| from_list ',' table_reference		{ $$ = $1->add($3); }
	;

%type <recSourceNode> table_reference
table_reference
	: joined_table
	| table_primary
	;

%type <recSourceNode> table_primary
table_primary
	: table_proc
	| derived_table			{ $$ = $1; }
	| '(' joined_table ')'	{ $$ = $2; }
	;

%type <selectExprNode> derived_table
derived_table
	: '(' select_expr ')' as_noise correlation_name derived_column_list
		{
			$$ = $2;
			$$->dsqlFlags |= RecordSourceNode::DFLAG_DERIVED;
			if ($5)
				$$->alias = $5->c_str();
			$$->columns = $6;
		}
	;

%type <metaNamePtr> correlation_name
correlation_name
	: /* nothing */				{ $$ = NULL; }
	| symbol_table_alias_name
	;

%type <metaNameArray> derived_column_list
derived_column_list
	: /* nothing */			{ $$ = NULL; }
	| '(' alias_list ')'	{ $$ = $2; }
	;

%type <metaNameArray> alias_list
alias_list
	: symbol_item_alias_name
		{
			ObjectsArray<MetaName>* node = newNode<ObjectsArray<MetaName> >();
			node->add(*$1);
			$$ = node;
		}
	| alias_list ',' symbol_item_alias_name
		{
			ObjectsArray<MetaName>* node = $1;
			node->add(*$3);
			$$ = node;
		}
	;

%type <recSourceNode> joined_table
joined_table
	: cross_join
	| natural_join
	| qualified_join
	;

%type <recSourceNode> cross_join
cross_join
	: table_reference CROSS JOIN table_primary
		{
			RseNode* rse = newNode<RseNode>();
			rse->dsqlExplicitJoin = true;
			rse->rse_jointype = blr_inner;
			rse->dsqlFrom = newNode<RecSourceListNode>($1)->add($4);
			$$ = rse;
		}
	;

%type <recSourceNode> natural_join
natural_join
	: table_reference NATURAL join_type JOIN table_primary
		{
			RseNode* rse = newNode<RseNode>();
			rse->dsqlExplicitJoin = true;
			rse->rse_jointype = $3;
			rse->dsqlFrom = newNode<RecSourceListNode>($1)->add($5);
			rse->dsqlJoinUsing = newNode<ValueListNode>(0u);	// using list with size 0 -> natural
			$$ = rse;
		}
	;

%type <recSourceNode> qualified_join
qualified_join
	: table_reference join_type JOIN table_reference join_condition
		{
			RseNode* rse = newNode<RseNode>();
			rse->dsqlExplicitJoin = true;
			rse->rse_jointype = $2;
			rse->dsqlFrom = newNode<RecSourceListNode>($1);
			rse->dsqlFrom->add($4);
			rse->dsqlWhere = $5;
			$$ = rse;
		}
	| table_reference join_type JOIN table_reference named_columns_join
		{
			RseNode* rse = newNode<RseNode>();
			rse->dsqlExplicitJoin = true;
			rse->rse_jointype = $2;
			rse->dsqlFrom = newNode<RecSourceListNode>($1);
			rse->dsqlFrom->add($4);
			rse->dsqlJoinUsing = $5;
			$$ = rse;
		}
	;

%type <boolExprNode> join_condition
join_condition
	: ON search_condition	{ $$ = $2; }
	;

%type <valueListNode> named_columns_join
named_columns_join
	: USING '(' column_list ')'		{ $$ = $3; }
	;

%type <recSourceNode> table_proc
table_proc
	: symbol_procedure_name table_proc_inputs as_noise symbol_table_alias_name
		{
			ProcedureSourceNode* node = newNode<ProcedureSourceNode>(QualifiedName(*$1));
			node->sourceList = $2;
			node->alias = $4->c_str();
			$$ = node;
		}
	| symbol_procedure_name table_proc_inputs
		{
			ProcedureSourceNode* node = newNode<ProcedureSourceNode>(QualifiedName(*$1));
			node->sourceList = $2;
			$$ = node;
		}
	| symbol_package_name '.' symbol_procedure_name table_proc_inputs as_noise symbol_table_alias_name
		{
			ProcedureSourceNode* node = newNode<ProcedureSourceNode>(
				QualifiedName(*$3, *$1));
			node->sourceList = $4;
			node->alias = $6->c_str();
			$$ = node;
		}
	| symbol_package_name '.' symbol_procedure_name table_proc_inputs
		{
			ProcedureSourceNode* node = newNode<ProcedureSourceNode>(
				QualifiedName(*$3, *$1));
			node->sourceList = $4;
			$$ = node;
		}
	;

%type <valueListNode> table_proc_inputs
table_proc_inputs
	: /* nothing */			{ $$ = NULL; }
	| '(' value_list ')'	{ $$ = $2; }
	;

%type <relSourceNode> table_name
table_name
	: simple_table_name
	| symbol_table_name as_noise symbol_table_alias_name
		{
			RelationSourceNode* node = newNode<RelationSourceNode>(*$1);
			node->alias = $3->c_str();
			$$ = node;
		}
	;

%type <relSourceNode> simple_table_name
simple_table_name
	: symbol_table_name
		{ $$ = newNode<RelationSourceNode>(*$1); }
	;

%type <blrOp> join_type
join_type
	: /* nothing */		{ $$ = blr_inner; }
	| INNER				{ $$ = blr_inner; }
	| LEFT outer_noise	{ $$ = blr_left; }
	| RIGHT outer_noise	{ $$ = blr_right; }
	| FULL outer_noise	{ $$ = blr_full; }

	;

outer_noise
	:
	| OUTER
	;


// other clauses in the select expression

%type <valueListNode> group_clause
group_clause
	: /* nothing */				{ $$ = NULL; }
	| GROUP BY group_by_list	{ $$ = $3; }
	;

%type <valueListNode> group_by_list
group_by_list
	: group_by_item						{ $$ = newNode<ValueListNode>($1); }
	| group_by_list ',' group_by_item	{ $$ = $1->add($3); }
	;

// Except aggregate-functions are all expressions supported in group_by_item,
// they are caught inside pass1.cpp
%type <valueExprNode> group_by_item
group_by_item
	: value
	;

%type <boolExprNode> having_clause
having_clause
	: /* nothing */				{ $$ = NULL; }
	| HAVING search_condition	{ $$ = $2; }
	;

%type <boolExprNode> where_clause
where_clause
	: /* nothing */				{ $$ = NULL; }
	| WHERE search_condition	{ $$ = $2; }
	;


// PLAN clause to specify an access plan for a query

%type <planNode> plan_clause
plan_clause
	: /* nothing */			{ $$ = NULL; }
	| PLAN plan_expression	{ $$ = $2; }
	;

%type <planNode> plan_expression
plan_expression
	: plan_type { $$ = newNode<PlanNode>(PlanNode::TYPE_RETRIEVE); } '(' plan_item_list($2) ')'
		{ $$ = $2; }
	;

plan_type
	: // nothing
	| JOIN
	| SORT MERGE
	| MERGE
	| HASH
	| SORT
	;

%type plan_item_list(<planNode>)
plan_item_list($planNode)
	: plan_item						{ $planNode->subNodes.add($1); }
	| plan_item_list ',' plan_item	{ $planNode->subNodes.insert(0, $3); }
	;

%type <planNode> plan_item
plan_item
	: table_or_alias_list access_type
		{
			$$ = newNode<PlanNode>(PlanNode::TYPE_RETRIEVE);
			$$->dsqlNames = $1;
			$$->accessType = $2;
		}
	| plan_expression
	;

%type <metaNameArray> table_or_alias_list
table_or_alias_list
	: symbol_table_name
		{
			ObjectsArray<MetaName>* node = newNode<ObjectsArray<MetaName> >();
			node->add(*$1);
			$$ = node;
		}
	| table_or_alias_list symbol_table_name
		{
			ObjectsArray<MetaName>* node = $1;
			node->add(*$2);
			$$ = node;
		}
	;

%type <accessType> access_type
access_type
	: NATURAL
		{ $$ = newNode<PlanNode::AccessType>(PlanNode::AccessType::TYPE_SEQUENTIAL); }
	| INDEX { $$ = newNode<PlanNode::AccessType>(PlanNode::AccessType::TYPE_INDICES); }
			'(' index_list($2) ')'
		{ $$ = $2; }
	| ORDER { $$ = newNode<PlanNode::AccessType>(PlanNode::AccessType::TYPE_NAVIGATIONAL); }
			symbol_index_name extra_indices_opt($2)
		{
			$$ = $2;
			$$->items.insert(0).indexName = *$3;
		}
	;

%type index_list(<accessType>)
index_list($accessType)
	: symbol_index_name
		{
			PlanNode::AccessItem& item = $accessType->items.add();
			item.indexName = *$1;
		}
	| index_list ', ' symbol_index_name
		{
			PlanNode::AccessItem& item = $accessType->items.insert(0);
			item.indexName = *$3;
		}
	;

%type extra_indices_opt(<accessType>)
extra_indices_opt($accessType)
	: // nothing
	| INDEX '(' index_list($accessType) ')'
	;

// ORDER BY clause

%type <valueListNode> order_clause
order_clause
	: /* nothing */			{ $$ = NULL; }
	| ORDER BY order_list	{ $$ = $3; }
	;

%type <valueListNode> order_list
order_list
	: order_item					{ $$ = newNode<ValueListNode>($1); }
	| order_list ',' order_item		{ $$ = $1->add($3); }
	;

%type <valueExprNode> order_item
order_item
	: value order_direction nulls_clause
		{
			OrderNode* node = newNode<OrderNode>($1);
			node->descending = $2;
			node->nullsPlacement = $3;
			$$ = node;
		}
	;

%type <boolVal>	order_direction
order_direction
	: /* nothing */		{ $$ = false; }
	| ASC				{ $$ = false; }
	| DESC				{ $$ = true; }
	;

%type <nullsPlacement> nulls_clause
nulls_clause
	: /* nothing */				{ $$ = OrderNode::NULLS_DEFAULT; }
	| NULLS nulls_placement		{ $$ = $2; }
	;

%type <nullsPlacement> nulls_placement
nulls_placement
	: FIRST	{ $$ = OrderNode::NULLS_FIRST; }
	| LAST	{ $$ = OrderNode::NULLS_LAST; }
	;

// ROWS clause

%type <rowsClause> rows_clause
rows_clause
	: // nothing
		{ $$ = NULL; }
	// equivalent to FIRST value
	| ROWS value
		{
			$$ = newNode<RowsClause>();
			$$->length = $2;
		}
	// equivalent to FIRST (upper_value - lower_value + 1) SKIP (lower_value - 1)
	| ROWS value TO value
		{
			$$ = newNode<RowsClause>();
			$$->skip = newNode<ArithmeticNode>(blr_subtract, true, $2, MAKE_const_slong(1));
			$$->length = newNode<ArithmeticNode>(blr_add, true,
				newNode<ArithmeticNode>(blr_subtract, true, $4, $2), MAKE_const_slong(1));
		}
	;


// INSERT statement
// IBO hack: replace column_parens_opt by ins_column_parens_opt.
%type <storeNode> insert
insert
	: insert_start ins_column_parens_opt(NOTRIAL(&$1->dsqlFields)) VALUES '(' value_list ')'
			returning_clause
		{
			StoreNode* node = $$ = $1;
			node->dsqlValues = $5;
			node->dsqlReturning = $7;
		}
	| insert_start ins_column_parens_opt(NOTRIAL(&$1->dsqlFields)) select_expr returning_clause
		{
			StoreNode* node = $$ = $1;
			node->dsqlRse = $3;
			node->dsqlReturning = $4;
			$$ = node;
		}
	| insert_start DEFAULT VALUES returning_clause
		{
			StoreNode* node = $$ = $1;
			node->dsqlReturning = $4;
			$$ = node;
		}
	;

%type <storeNode> insert_start
insert_start
	: INSERT INTO simple_table_name
		{
			StoreNode* node = newNode<StoreNode>();
			node->dsqlRelation = $3;
			$$ = node;
		}
	;


// MERGE statement
%type <mergeNode> merge
merge
	: MERGE INTO table_name USING table_reference ON search_condition
			{
				MergeNode* node = $$ = newNode<MergeNode>();
				node->relation = $3;
				node->usingClause = $5;
				node->condition = $7;
			}
		merge_when_clause($8) returning_clause
			{
				MergeNode* node = $$ = $8;
				node->returning = $10;
			}
	;

%type merge_when_clause(<mergeNode>)
merge_when_clause($mergeNode)
	: merge_when_matched_clause($mergeNode)
	| merge_when_not_matched_clause($mergeNode)
	| merge_when_clause merge_when_matched_clause($mergeNode)
	| merge_when_clause merge_when_not_matched_clause($mergeNode)
	;

%type merge_when_matched_clause(<mergeNode>)
merge_when_matched_clause($mergeNode)
	: WHEN MATCHED
			{ $<mergeMatchedClause>$ = &$mergeNode->whenMatched.add(); }
		merge_update_specification(NOTRIAL($<mergeMatchedClause>3))
	;

%type merge_when_not_matched_clause(<mergeNode>)
merge_when_not_matched_clause($mergeNode)
	: WHEN NOT MATCHED
			{ $<mergeNotMatchedClause>$ = &$mergeNode->whenNotMatched.add(); }
		merge_insert_specification(NOTRIAL($<mergeNotMatchedClause>4))
	;

%type merge_update_specification(<mergeMatchedClause>)
merge_update_specification($mergeMatchedClause)
	: THEN UPDATE SET assignments
		{ $mergeMatchedClause->assignments = $4; }
	| AND search_condition THEN UPDATE SET assignments
		{
			$mergeMatchedClause->condition = $2;
			$mergeMatchedClause->assignments = $6;
		}
	| THEN KW_DELETE
	| AND search_condition THEN KW_DELETE
		{ $mergeMatchedClause->condition = $2; }
	;

%type merge_insert_specification(<mergeNotMatchedClause>)
merge_insert_specification($mergeNotMatchedClause)
	: THEN INSERT ins_column_parens_opt(NOTRIAL(&$mergeNotMatchedClause->fields))
			VALUES '(' value_list ')'
		{ $mergeNotMatchedClause->values = $6; }
	| AND search_condition THEN INSERT ins_column_parens_opt(NOTRIAL(&$mergeNotMatchedClause->fields))
			VALUES '(' value_list ')'
		{
			$mergeNotMatchedClause->values = $8;
			$mergeNotMatchedClause->condition = $2;
		}
	;


// DELETE statement

%type <stmtNode> delete
delete
	: delete_searched
	| delete_positioned
	;

%type <stmtNode> delete_searched
delete_searched
	: KW_DELETE FROM table_name where_clause plan_clause order_clause rows_clause returning_clause
		{
			EraseNode* node = newNode<EraseNode>();
			node->dsqlRelation = $3;
			node->dsqlBoolean = $4;
			node->dsqlPlan = $5;
			node->dsqlOrder = $6;
			node->dsqlRows = $7;
			node->dsqlReturning = $8;
			$$ = node;
		}
	;

%type <stmtNode> delete_positioned
delete_positioned
	: KW_DELETE FROM table_name cursor_clause returning_clause
		{
			EraseNode* node = newNode<EraseNode>();
			node->dsqlRelation = $3;
			node->dsqlCursorName = *$4;
			node->dsqlReturning = $5;
			$$ = node;
		}
	;


// UPDATE statement

%type <stmtNode> update
update
	: update_searched
	| update_positioned
	;

%type <stmtNode> update_searched
update_searched
	: UPDATE table_name SET assignments where_clause plan_clause
			order_clause rows_clause returning_clause
		{
			ModifyNode* node = newNode<ModifyNode>();
			node->dsqlRelation = $2;
			node->statement = $4;
			node->dsqlBoolean = $5;
			node->dsqlPlan = $6;
			node->dsqlOrder = $7;
			node->dsqlRows = $8;
			node->dsqlReturning = $9;
			$$ = node;
		}
	;

%type <stmtNode> update_positioned
update_positioned
	: UPDATE table_name SET assignments cursor_clause returning_clause
		{
			ModifyNode* node = newNode<ModifyNode>();
			node->dsqlRelation = $2;
			node->statement = $4;
			node->dsqlCursorName = *$5;
			node->dsqlReturning = $6;
			$$ = node;
		}
	;


// UPDATE OR INSERT statement

%type <updInsNode> update_or_insert
update_or_insert
	: UPDATE OR INSERT INTO simple_table_name
			{
				UpdateOrInsertNode* node = $$ = newNode<UpdateOrInsertNode>();
				node->relation = $5;
			}
		ins_column_parens_opt(NOTRIAL(&$6->fields)) VALUES '(' value_list ')'
				update_or_insert_matching_opt(NOTRIAL(&$6->matching)) returning_clause
			{
				UpdateOrInsertNode* node = $$ = $6;
				node->values = $10;
				node->returning = $13;
			}
	;

%type update_or_insert_matching_opt(<nestFieldArray>)
update_or_insert_matching_opt($fieldArray)
	: // nothing
	| MATCHING ins_column_parens($fieldArray)
	;


%type <returningClause> returning_clause
returning_clause
	: /* nothing */
		{ $$ = NULL; }
	| RETURNING value_opt_alias_list
		{
			$$ = FB_NEW(getPool()) ReturningClause(getPool());
			$$->first = $2;
		}
	| RETURNING value_opt_alias_list INTO variable_list
		{
			$$ = FB_NEW(getPool()) ReturningClause(getPool());
			$$->first = $2;
			$$->second = $4;
		}
	;

%type <valueListNode> value_opt_alias_list
value_opt_alias_list
	: value_opt_alias							{ $$ = newNode<ValueListNode>($1); }
	| value_opt_alias_list ',' value_opt_alias	{ $$ = $1->add($3); }
	;

%type <metaNamePtr> cursor_clause
cursor_clause
	: WHERE CURRENT OF symbol_cursor_name	{ $$ = newNode<MetaName>(*$4); }
	;


// Assignments

%type <compoundStmtNode> assignments
assignments
	: assignment
		{
			$$ = newNode<CompoundStmtNode>();
			$$->statements.add($1);
		}
	| assignments ',' assignment
		{
			$1->statements.add($3);
			$$ = $1;
		}
	;

%type <stmtNode> assignment
assignment
	: update_column_name '=' value
		{
			AssignmentNode* node = newNode<AssignmentNode>();
			node->asgnTo = $1;
			node->asgnFrom = $3;
			$$ = node;
		}
	;

%type <stmtNode> exec_function
exec_function
	: udf
		{
			AssignmentNode* node = newNode<AssignmentNode>();
			node->asgnTo = newNode<NullNode>();
			node->asgnFrom = $1;
			$$ = node;
		}
	| non_aggregate_function
		{
			AssignmentNode* node = newNode<AssignmentNode>();
			node->asgnTo = newNode<NullNode>();
			node->asgnFrom = $1;
			$$ = node;
		}
	;


// column specifications

%type <valueListNode> column_parens_opt
column_parens_opt
	: /* nothing */		{ $$ = NULL; }
	| column_parens
	;

%type <valueListNode> column_parens
column_parens
	: '(' column_list ')'	{ $$ = $2; }
	;

%type <valueListNode> column_list
column_list
	: simple_column_name					{ $$ = newNode<ValueListNode>($1); }
	| column_list ',' simple_column_name	{ $$ = $1->add($3); }
	;

// begin IBO hack
%type ins_column_parens_opt(<nestFieldArray>)
ins_column_parens_opt($fieldArray)
	: // nothing
	| ins_column_parens($fieldArray)
	;

%type ins_column_parens(<nestFieldArray>)
ins_column_parens($fieldArray)
	: '(' ins_column_list($fieldArray) ')'
	;

%type ins_column_list(<nestFieldArray>)
ins_column_list($fieldArray)
	: update_column_name						{ $fieldArray->add($1); }
	| ins_column_list ',' update_column_name	{ $fieldArray->add($3); }
	;
// end IBO hack

%type <fieldNode> column_name
column_name
	: simple_column_name
	| symbol_table_alias_name '.' symbol_column_name
		{
			FieldNode* fieldNode = newNode<FieldNode>();
			fieldNode->dsqlQualifier = *$1;
			fieldNode->dsqlName = *$3;
			$$ = fieldNode;
		}
	;

%type <fieldNode> simple_column_name
simple_column_name
	: symbol_column_name
		{
			FieldNode* fieldNode = newNode<FieldNode>();
			fieldNode->dsqlName = *$1;
			$$ = fieldNode;
		}
	;

%type <fieldNode> update_column_name
update_column_name
	: simple_column_name
	// CVC: This option should be deprecated! The only allowed syntax should be
	// Update...set column = expr, without qualifier for the column.
	| symbol_table_alias_name '.' symbol_column_name
		{
			FieldNode* fieldNode = newNode<FieldNode>();
			fieldNode->dsqlQualifier = *$1;
			fieldNode->dsqlName = *$3;
			$$ = fieldNode;
		}
	;

// boolean expressions

%type <boolExprNode> search_condition
search_condition
	: value		{ $$ = valueToBool($1); }
	;

%type <boolExprNode> boolean_value_expression
boolean_value_expression
	: predicate
	| value OR value
		{ $$ = newNode<BinaryBoolNode>(blr_or, valueToBool($1), valueToBool($3)); }
	| value AND value
		{ $$ = newNode<BinaryBoolNode>(blr_and, valueToBool($1), valueToBool($3)); }
	| NOT value
		{ $$ = newNode<NotBoolNode>(valueToBool($2)); }
	| '(' boolean_value_expression ')'
		{ $$ = $2; }
	| value IS boolean_literal
		{ $$ = newNode<ComparativeBoolNode>(blr_equiv, newNode<BoolAsValueNode>(valueToBool($1)), $3); }
	| value IS NOT boolean_literal
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_equiv,
				newNode<BoolAsValueNode>(valueToBool($1)), $4);
			$$ = newNode<NotBoolNode>(node);
		}
	;

%type <boolExprNode> predicate
predicate
	: comparison_predicate
	| distinct_predicate
	| between_predicate
	| binary_pattern_predicate
	| ternary_pattern_predicate
	| in_predicate
	| null_predicate
	| quantified_predicate
	| exists_predicate
	| singular_predicate
	;


// comparisons

%type <boolExprNode> comparison_predicate
comparison_predicate
	: value comparison_operator value %prec '='
		{ $$ = newNode<ComparativeBoolNode>($2, $1, $3); }
	;

%type <blrOp> comparison_operator
comparison_operator
	: '='		{ $$ = blr_eql; }
	| '<'		{ $$ = blr_lss; }
	| '>'		{ $$ = blr_gtr; }
	| GEQ		{ $$ = blr_geq; }
	| LEQ		{ $$ = blr_leq; }
	| NOT_GTR	{ $$ = blr_leq; }
	| NOT_LSS	{ $$ = blr_geq; }
	| NEQ		{ $$ = blr_neq; }

// quantified comparisons

%type <boolExprNode> quantified_predicate
quantified_predicate
	: value comparison_operator quantified_flag '(' column_select ')'
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>($2, $1);
			node->dsqlFlag = $3;
			node->dsqlSpecialArg = $5;
			$$ = node;
		}
	;

%type <cmpBoolFlag> quantified_flag
quantified_flag
	: ALL	{ $$ = ComparativeBoolNode::DFLAG_ANSI_ALL; }
	| SOME	{ $$ = ComparativeBoolNode::DFLAG_ANSI_ANY; }
	| ANY	{ $$ = ComparativeBoolNode::DFLAG_ANSI_ANY; }
	;


// other predicates

%type <boolExprNode> distinct_predicate
distinct_predicate
	: value IS DISTINCT FROM value %prec IS
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_equiv, $1, $5);
			$$ = newNode<NotBoolNode>(node);
		}
	| value IS NOT DISTINCT FROM value %prec IS
		{ $$ = newNode<ComparativeBoolNode>(blr_equiv, $1, $6); }
	;

%type <boolExprNode> between_predicate
between_predicate
	: value BETWEEN value_between AND value_between %prec BETWEEN
		{ $$ = newNode<ComparativeBoolNode>(blr_between, $1, $3, $5); }
	| value NOT BETWEEN value_between AND value_between %prec BETWEEN
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_between, $1, $4, $6);
			$$ = newNode<NotBoolNode>(node);
		}
	;

// Special value for BETWEEN, to avoid conflicts with boolean expressions.
%type <valueExprNode> value_between
value_between
	: value_primary
	| '(' boolean_value_expression ')'		{ $$ = newNode<BoolAsValueNode>($2); }
	;

%type <boolExprNode> binary_pattern_predicate
binary_pattern_predicate
	: value binary_pattern_operator value %prec CONTAINING
		{ $$ = newNode<ComparativeBoolNode>($2, $1, $3); }
	| value NOT binary_pattern_operator value %prec CONTAINING
		{
			ComparativeBoolNode* cmpNode = newNode<ComparativeBoolNode>($3, $1, $4);
			$$ = newNode<NotBoolNode>(cmpNode);
		}
	;

%type <blrOp> binary_pattern_operator
binary_pattern_operator
	: CONTAINING	{ $$ = blr_containing; }
	| STARTING		{ $$ = blr_starting; }
	| STARTING WITH	{ $$ = blr_starting; }
	;

%type <boolExprNode> ternary_pattern_predicate
ternary_pattern_predicate
	: value LIKE value %prec LIKE
		{ $$ = newNode<ComparativeBoolNode>(blr_like, $1, $3); }
	| value LIKE value ESCAPE value %prec LIKE
		{ $$ = newNode<ComparativeBoolNode>(blr_like, $1, $3, $5); }
	| value NOT LIKE value %prec LIKE
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_like, $1, $4);
			$$ = newNode<NotBoolNode>(node);
		}
	| value NOT LIKE value ESCAPE value %prec LIKE
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_like, $1, $4, $6);
			$$ = newNode<NotBoolNode>(node);
		}
	| value SIMILAR TO value %prec SIMILAR
		{ $$ = newNode<ComparativeBoolNode>(blr_similar, $1, $4); }
	| value SIMILAR TO value ESCAPE value %prec SIMILAR
		{ $$ = newNode<ComparativeBoolNode>(blr_similar, $1, $4, $6); }
	| value NOT SIMILAR TO value %prec SIMILAR
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_similar, $1, $5);
			$$ = newNode<NotBoolNode>(node);
		}
	| value NOT SIMILAR TO value ESCAPE value %prec SIMILAR
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_similar, $1, $5, $7);
			$$ = newNode<NotBoolNode>(node);
		}
	;

%type <boolExprNode> in_predicate
in_predicate
	: value KW_IN in_predicate_value
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_eql, $1);
			node->dsqlFlag = ComparativeBoolNode::DFLAG_ANSI_ANY;
			node->dsqlSpecialArg = $3;
			$$ = node;
		}
	| value NOT KW_IN in_predicate_value
		{
			ComparativeBoolNode* node = newNode<ComparativeBoolNode>(blr_eql, $1);
			node->dsqlFlag = ComparativeBoolNode::DFLAG_ANSI_ANY;
			node->dsqlSpecialArg = $4;
			$$ = newNode<NotBoolNode>(node);
		}
	;

%type <boolExprNode> exists_predicate
exists_predicate
	: EXISTS '(' select_expr ')'
		{ $$ = newNode<RseBoolNode>(blr_any, $3); }
	;

%type <boolExprNode> singular_predicate
singular_predicate
	: SINGULAR '(' select_expr ')'
		{ $$ = newNode<RseBoolNode>(blr_unique, $3); }
	;

%type <boolExprNode> null_predicate
null_predicate
	: value IS KW_NULL
		{ $$ = newNode<MissingBoolNode>($1); }
	| value IS UNKNOWN
		{ $$ = newNode<MissingBoolNode>($1, true); }
	| value IS NOT KW_NULL
		{ $$ = newNode<NotBoolNode>(newNode<MissingBoolNode>($1)); }
	| value IS NOT UNKNOWN
		{ $$ = newNode<NotBoolNode>(newNode<MissingBoolNode>($1, true)); }
	;


// set values

%type <exprNode> in_predicate_value
in_predicate_value
	: table_subquery		{ $$ = $1; }
	| '(' value_list ')'	{ $$ = $2; }
	;

%type <selectExprNode> table_subquery
table_subquery
	: '(' column_select ')'		{ $$ = $2; }
	;

// USER control SQL interface

%type <createAlterUserNode> create_user_clause
create_user_clause
	: symbol_user_name passwd_clause
 		{
			$$ = newNode<CreateAlterUserNode>(CreateAlterUserNode::USER_ADD, *$1);
			$$->password = $2;
		}
	user_fixed_opts(NOTRIAL($3))
	user_var_opts(NOTRIAL($3))
		{
			$$ = $3;
		}
	;

%type <createAlterUserNode> alter_user_clause
alter_user_clause
	: symbol_user_name set_noise passwd_opt
		{
			$$ = newNode<CreateAlterUserNode>(CreateAlterUserNode::USER_MOD, *$1);
			$$->password = $3;
		}
	user_fixed_opts(NOTRIAL($4))
	user_var_opts(NOTRIAL($4))
		{
			$$ = $4;
		}
	;

%type <createAlterUserNode> alter_cur_user_clause
alter_cur_user_clause
	: set_noise passwd_opt
		{
			$$ = newNode<CreateAlterUserNode>(CreateAlterUserNode::USER_MOD, "");
			$$->password = $2;
		}
	user_fixed_opts(NOTRIAL($3))
	user_var_opts(NOTRIAL($3))
		{
			$$ = $3;
		}
	;

%type <createAlterUserNode> replace_user_clause
replace_user_clause
	: symbol_user_name set_noise passwd_opt
		{
			$$ = newNode<CreateAlterUserNode>(CreateAlterUserNode::USER_RPL, *$1);
			$$->password = $3;
		}
	user_fixed_opts(NOTRIAL($4))
	user_var_opts(NOTRIAL($4))
		{
			$$ = $4;
		}
	;

set_noise
	: // nothing
	| SET
	;

%type <stringPtr> passwd_opt
passwd_opt
	: /* nothing */			{ $$ = NULL; }
	| passwd_clause
	;

%type <stringPtr> passwd_clause
passwd_clause
	: PASSWORD utf_string	{ $$ = $2; }
	;

%type user_fixed_opts(<createAlterUserNode>)
user_fixed_opts($node)
	: // nothing
	| user_fixed_list($node)
	;

%type user_fixed_list(<createAlterUserNode>)
user_fixed_list($node)
	: user_fixed_opt($node)
	| user_fixed_list user_fixed_opt($node)
	;

%type user_fixed_opt(<createAlterUserNode>)
user_fixed_opt($node)
	: FIRSTNAME utf_string	{ setClause($node->firstName, "FIRSTNAME", $2); }
	| MIDDLENAME utf_string	{ setClause($node->middleName, "MIDDLENAME", $2); }
	| LASTNAME utf_string	{ setClause($node->lastName, "LASTNAME", $2); }
	| GRANT ADMIN ROLE		{ setClause($node->adminRole, "ADMIN ROLE", Nullable<bool>(true)); }
	| REVOKE ADMIN ROLE		{ setClause($node->adminRole, "ADMIN ROLE", Nullable<bool>(false)); }
	| ACTIVE				{ setClause($node->active, "ACTIVE/INACTIVE", Nullable<bool>(true)); }
	| INACTIVE				{ setClause($node->active, "ACTIVE/INACTIVE", Nullable<bool>(false)); }
	;

%type user_var_opts(<createAlterUserNode>)
user_var_opts($node)
	: // nothing
	| TAGS '(' user_var_list($node) ')'
	;

%type user_var_list(<createAlterUserNode>)
user_var_list($node)
	: user_var_option($node)
	| user_var_list ',' user_var_option($node)
	;

%type user_var_option(<createAlterUserNode>)
user_var_option($node)
	: valid_symbol_name '=' utf_string
		{
			$node->addProperty($1, $3);
		}
	| DROP valid_symbol_name
		{
			$node->addProperty($2);
		}
	;


// logons mapping

%type <mappingNode> create_map_clause(<boolVal>)
create_map_clause($global)
	: map_clause(MappingNode::MAP_ADD)
	 		{
				$$ = $1;
				$$->global = $global;
			}
		map_to(NOTRIAL($2))
			{
				$$ = $2;
			}
	;

%type <mappingNode> alter_map_clause(<boolVal>)
alter_map_clause($global)
	: map_clause(MappingNode::MAP_MOD)
	 		{
				$$ = $1;
				$$->global = $global;
			}
		map_to(NOTRIAL($2))
			{
				$$ = $2;
			}
	;

%type <mappingNode> replace_map_clause(<boolVal>)
replace_map_clause($global)
	: map_clause(MappingNode::MAP_RPL)
	 		{
				$$ = $1;
				$$->global = $global;
			}
		map_to(NOTRIAL($2))
			{
				$$ = $2;
			}
	;

%type <mappingNode> drop_map_clause(<boolVal>)
drop_map_clause($global)
	: map_name
 		{
 			MappingNode* node = newNode<MappingNode>(MappingNode::MAP_DROP, *$1);
			node->global = $global;
			$$ = node;
		}
	;

%type <mappingNode> map_clause(<mappingOp>)
map_clause($op)
	: map_name
 			{
				$$ = newNode<MappingNode>($op, *$1);
			}
		USING map_using(NOTRIAL($2))
		FROM map_from(NOTRIAL($2))
			{
				$$ = $2;
			}
	;

%type <metaNamePtr> map_name
map_name
	: valid_symbol_name
		{ $$ = $1; }
	;

%type map_from(<mappingNode>)
map_from($node)
	: map_from_symbol_name map_logoninfo
		{
			$node->fromType = $1;
			$node->from = $2;
		}
	| ANY map_from_symbol_name
		{
			$node->fromType = $2;
			$node->from = newNode<IntlString>("*");
		}
	;

%type <metaNamePtr> map_from_symbol_name
map_from_symbol_name
	: valid_symbol_name
	| USER					{ $$ = newNode<MetaName>("USER"); }
	| GROUP					{ $$ = newNode<MetaName>("GROUP"); }
	;

%type <intlStringPtr> map_logoninfo
map_logoninfo
	: STRING
	| valid_symbol_name		{ $$ = newNode<IntlString>($1->c_str()); }
	;

%type map_using(<mappingNode>)
map_using($node)
	: PLUGIN valid_symbol_name map_in
		{
			$node->mode = 'P';
			$node->plugin = $2;
			$node->db = $3;
		}
	| ANY PLUGIN map_in
		{
			$node->mode = 'P';
			$node->db = $3;
		}
	| ANY PLUGIN SERVERWIDE
		{
			$node->mode = 'S';
		}
	| MAPPING map_in
		{
			$node->mode = 'M';
			$node->db = $2;
		}
	| '*' map_in
		{
			$node->mode = '*';
			$node->db = $2;
		}
	;

%type <metaNamePtr> map_in
map_in
	: /* nothing */				{ $$ = NULL; }
	| KW_IN valid_symbol_name	{ $$ = $2; }
	;

%type map_to(<mappingNode>)
map_to($node)
	: TO map_role valid_symbol_name
		{
			$node->role = $2;
			$node->to = $3;
		}
	| TO map_role
		{
			$node->role = $2;
		}
	;

%type <boolVal> map_role
map_role
	: ROLE		{ $$ = true; }
	| USER		{ $$ = false; }
	;


// value types

%type <valueExprNode> value
value
	: value_primary
	| boolean_value_expression
		{ $$ = newNode<BoolAsValueNode>($1); }
	;

%type <valueExprNode> value_primary
value_primary
	: nonparenthesized_value
	| '(' value_primary ')'	{ $$ = $2; }
	;

%type <valueExprNode> nonparenthesized_value
nonparenthesized_value
	: column_name
		{ $$ = $1; }
	| array_element
	| function
		{ $$ = $1; }
	| u_constant
	| boolean_literal
	| parameter
	| variable
	| cast_specification
	| case_expression
	| next_value_expression
		{ $$ = $1; }
	| udf
		{ $$ = $1; }
	| '-' value_primary %prec UMINUS
		{ $$ = newNode<NegateNode>($2); }
	| '+' value_primary %prec UPLUS
		{ $$ = $2; }
	| value_primary '+' value_primary
		{ $$ = newNode<ArithmeticNode>(blr_add, (client_dialect < SQL_DIALECT_V6_TRANSITION), $1, $3); }
	| value_primary CONCATENATE value_primary
		{ $$ = newNode<ConcatenateNode>($1, $3); }
	| value_primary COLLATE symbol_collation_name
		{ $$ = newNode<CollateNode>($1, *$3); }
	| value_primary '-' value_primary
		{ $$ = newNode<ArithmeticNode>(blr_subtract, (client_dialect < SQL_DIALECT_V6_TRANSITION), $1, $3); }
	| value_primary '*' value_primary
		{ $$ = newNode<ArithmeticNode>(blr_multiply, (client_dialect < SQL_DIALECT_V6_TRANSITION), $1, $3); }
	| value_primary '/' value_primary
		{ $$ = newNode<ArithmeticNode>(blr_divide, (client_dialect < SQL_DIALECT_V6_TRANSITION), $1, $3); }
	| '(' column_singleton ')'
		{ $$ = $2; }
	| current_user
		{ $$ = $1; }
	| current_role
		{ $$ = $1; }
	| internal_info
		{ $$ = $1; }
	| recordKeyType
		{ $$ = newNode<RecordKeyNode>($1); }
	| symbol_table_alias_name '.' recordKeyType
		{ $$ = newNode<RecordKeyNode>($3, *$1); }
	| KW_VALUE
		{ $$ = newNode<DomainValidationNode>(); }
	| datetime_value_expression
		{ $$ = $1; }
	| null_value
		{ $$ = $1; }
	;

%type <blrOp> recordKeyType
recordKeyType
	: DB_KEY				{ $$ = blr_dbkey; }
	| RDB_RECORD_VERSION	{ $$ = blr_record_version2; }
	;

%type <valueExprNode> datetime_value_expression
datetime_value_expression
	: CURRENT_DATE
		{
			if (client_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_dialect_datatype_unsupport) << Arg::Num(client_dialect) <<
						  												  Arg::Str("DATE"));
			}

			if (db_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_db_dialect_dtype_unsupport) << Arg::Num(db_dialect) <<
						  												  Arg::Str("DATE"));
			}

			$$ = newNode<CurrentDateNode>();
		}
	| CURRENT_TIME time_precision_opt
		{
			if (client_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_dialect_datatype_unsupport) << Arg::Num(client_dialect) <<
						  												  Arg::Str("TIME"));
			}

			if (db_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_db_dialect_dtype_unsupport) << Arg::Num(db_dialect) <<
						  												  Arg::Str("TIME"));
			}

			$$ = newNode<CurrentTimeNode>($2);
		}
	| CURRENT_TIMESTAMP timestamp_precision_opt
		{ $$ = newNode<CurrentTimeStampNode>($2); }
	;

%type <uintVal>	time_precision_opt
time_precision_opt
	: /* nothing */						{ $$ = DEFAULT_TIME_PRECISION; }
	| '(' nonneg_short_integer ')'		{ $$ = $2; }
	;

%type <uintVal>	timestamp_precision_opt
timestamp_precision_opt
	: /* nothing */					{ $$ = DEFAULT_TIMESTAMP_PRECISION; }
	| '(' nonneg_short_integer ')'	{ $$ = $2; }
	;

%type <valueExprNode> array_element
array_element
	: column_name '[' value_list ']'
		{
			ArrayNode* node = newNode<ArrayNode>($1);
			node->field->dsqlIndices = $3;
			$$ = node;
		}
	;

%type <valueListNode> value_list_opt
value_list_opt
	: /* nothing */		{ $$ = newNode<ValueListNode>(0); }
	| value_list		{ $$ = $1; }
	;

%type <valueListNode> value_list
value_list
	: value						{ $$ = newNode<ValueListNode>($1); }
	| value_list ',' value		{ $$ = $1->add($3); }
	;

%type <valueExprNode> constant
constant
	: u_constant
	| '-' u_numeric_constant	{ $$ = newNode<NegateNode>($2); }
	| boolean_literal
	;

%type <valueExprNode> u_numeric_constant
u_numeric_constant
	: NUMBER
		{ $$ = MAKE_const_slong($1); }
	| FLOAT_NUMBER
		{ $$ = MAKE_constant($1->c_str(), CONSTANT_DOUBLE); }
	| NUMBER64BIT
		{
			SINT64 signedNumber = (SINT64) $1.number;

			if ($1.hex && signedNumber < 0)
				$$ = newNode<NegateNode>(MAKE_const_sint64(-signedNumber, $1.scale));
			else
				$$ = MAKE_const_sint64(signedNumber, $1.scale);
		}
	| SCALEDINT
		{ $$ = MAKE_const_sint64((SINT64) $1.number, $1.scale); }
	;

%type <valueExprNode> u_constant
u_constant
	: u_numeric_constant
	| sql_string
		{ $$ = MAKE_str_constant($1, lex.att_charset); }
	| DATE STRING
		{
			if (client_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_dialect_datatype_unsupport) << Arg::Num(client_dialect) <<
						  												  Arg::Str("DATE"));
			}
			if (db_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_db_dialect_dtype_unsupport) << Arg::Num(db_dialect) <<
						  												  Arg::Str("DATE"));
			}
			$$ = MAKE_constant($2->getString().c_str(), CONSTANT_DATE);
		}
	| TIME STRING
		{
			if (client_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_dialect_datatype_unsupport) << Arg::Num(client_dialect) <<
						  												  Arg::Str("TIME"));
			}
			if (db_dialect < SQL_DIALECT_V6_TRANSITION)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_sql_db_dialect_dtype_unsupport) << Arg::Num(db_dialect) <<
						  												  Arg::Str("TIME"));
			}
			$$ = MAKE_constant($2->getString().c_str(), CONSTANT_TIME);
		}
	| TIMESTAMP STRING
		{ $$ = MAKE_constant($2->getString().c_str(), CONSTANT_TIMESTAMP); }
		;

%type <valueExprNode> boolean_literal
boolean_literal
	: KW_FALSE	{ $$ = MAKE_constant("", CONSTANT_BOOLEAN); }
	| KW_TRUE	{ $$ = MAKE_constant("1", CONSTANT_BOOLEAN); }
	;

%type <valueExprNode> parameter
parameter
	: '?'	{ $$ = make_parameter(); }
	;

%type <valueExprNode> current_user
current_user
	: USER			{ $$ = newNode<CurrentUserNode>(); }
	| CURRENT_USER	{ $$ = newNode<CurrentUserNode>(); }
	;

%type <valueExprNode> current_role
current_role
	: CURRENT_ROLE	{ $$ = newNode<CurrentRoleNode>(); }
	;

%type <valueExprNode> internal_info
internal_info
	: CURRENT_CONNECTION
		{ $$ = newNode<InternalInfoNode>(MAKE_const_slong(INFO_TYPE_CONNECTION_ID)); }
	| CURRENT_TRANSACTION
		{ $$ = newNode<InternalInfoNode>(MAKE_const_slong(INFO_TYPE_TRANSACTION_ID)); }
	| GDSCODE
		{ $$ = newNode<InternalInfoNode>(MAKE_const_slong(INFO_TYPE_GDSCODE)); }
	| SQLCODE
		{ $$ = newNode<InternalInfoNode>(MAKE_const_slong(INFO_TYPE_SQLCODE)); }
	| SQLSTATE
		{ $$ = newNode<InternalInfoNode>(MAKE_const_slong(INFO_TYPE_SQLSTATE)); }
	| ROW_COUNT
		{ $$ = newNode<InternalInfoNode>(MAKE_const_slong(INFO_TYPE_ROWS_AFFECTED)); }
	;

%type <intlStringPtr> sql_string
sql_string
	: STRING					// string in current charset
	| INTRODUCER STRING			// string in specific charset
		{
			$$ = $2;
			$$->setCharSet(*$1);

			StrMark* mark = strMarks.get($2);

			if (mark)	// hex string is not in strMarks
				mark->introduced = true;
		}
	;

%type <stringPtr> utf_string
utf_string
	: sql_string
		{ $$ = newString($1->toUtf8(scratch)); }
	;

%type <int32Val> signed_short_integer
signed_short_integer
	: nonneg_short_integer
	| '-' neg_short_integer
		{ $$ = -$2; }
	;

%type <int32Val> nonneg_short_integer
nonneg_short_integer
	: NUMBER
		{
			if ($1 > SHRT_POS_MAX)
				yyabandon(-842, isc_expec_short);	// Short integer expected

			$$ = $1;
		}
	;

%type <int32Val> neg_short_integer
neg_short_integer
	: NUMBER
		{
			if ($1 > SHRT_NEG_MAX)
				yyabandon(-842, isc_expec_short);	// Short integer expected

			$$ = $1;
		}
	;

%type <int32Val> pos_short_integer
pos_short_integer
	: nonneg_short_integer
		{
			if ($1 == 0)
				yyabandon(-842, isc_expec_positive);	// Positive number expected

			$$ = $1;
		}
	;

%type <int32Val> unsigned_short_integer
unsigned_short_integer
	: NUMBER
		{
			if ($1 > SHRT_UNSIGNED_MAX)
				yyabandon(-842, isc_expec_ushort);	// Unsigned short integer expected

			$$ = $1;
		}
	;

%type <int32Val> signed_long_integer
signed_long_integer
	: long_integer
	| '-' long_integer		{ $$ = -$2; }
	;

%type <int32Val> long_integer
long_integer
	: NUMBER	{ $$ = $1;}
	;


// functions

%type <valueExprNode> function
function
	: aggregate_function		{ $$ = $1; }
	| non_aggregate_function
	| over_clause
	;

%type <valueExprNode> non_aggregate_function
non_aggregate_function
	: numeric_value_function
	| string_value_function
	| system_function_expression
	;

%type <aggNode> aggregate_function
aggregate_function
	: COUNT '(' '*' ')'
		{ $$ = newNode<CountAggNode>(false, (client_dialect < SQL_DIALECT_V6_TRANSITION)); }
	| COUNT '(' all_noise value ')'
		{ $$ = newNode<CountAggNode>(false, (client_dialect < SQL_DIALECT_V6_TRANSITION), $4); }
	| COUNT '(' DISTINCT value ')'
		{ $$ = newNode<CountAggNode>(true, (client_dialect < SQL_DIALECT_V6_TRANSITION), $4); }
	| SUM '(' all_noise value ')'
		{
			$$ = newNode<SumAggNode>(false,
				(client_dialect < SQL_DIALECT_V6_TRANSITION), $4);
		}
	| SUM '(' DISTINCT value ')'
		{
			$$ = newNode<SumAggNode>(true,
				(client_dialect < SQL_DIALECT_V6_TRANSITION), $4);
		}
	| AVG '(' all_noise value ')'
		{
			$$ = newNode<AvgAggNode>(false,
				(client_dialect < SQL_DIALECT_V6_TRANSITION), $4);
		}
	| AVG '(' DISTINCT value ')'
		{
			$$ = newNode<AvgAggNode>(true,
				(client_dialect < SQL_DIALECT_V6_TRANSITION), $4);
		}
	| MINIMUM '(' all_noise value ')'
		{ $$ = newNode<MaxMinAggNode>(MaxMinAggNode::TYPE_MIN, $4); }
	| MINIMUM '(' DISTINCT value ')'
		{ $$ = newNode<MaxMinAggNode>(MaxMinAggNode::TYPE_MIN, $4); }
	| MAXIMUM '(' all_noise value ')'
		{ $$ = newNode<MaxMinAggNode>(MaxMinAggNode::TYPE_MAX, $4); }
	| MAXIMUM '(' DISTINCT value ')'
		{ $$ = newNode<MaxMinAggNode>(MaxMinAggNode::TYPE_MAX, $4); }
	| LIST '(' all_noise value delimiter_opt ')'
		{ $$ = newNode<ListAggNode>(false, $4, $5); }
	| LIST '(' DISTINCT value delimiter_opt ')'
		{ $$ = newNode<ListAggNode>(true, $4, $5); }
	;

%type <aggNode> window_function
window_function
	: DENSE_RANK '(' ')'
		{ $$ = newNode<DenseRankWinNode>(); }
	| RANK '(' ')'
		{ $$ = newNode<RankWinNode>(); }
	| ROW_NUMBER '(' ')'
		{ $$ = newNode<RowNumberWinNode>(); }
	| FIRST_VALUE '(' value ')'
		{ $$ = newNode<FirstValueWinNode>($3); }
	| LAST_VALUE '(' value ')'
		{ $$ = newNode<LastValueWinNode>($3); }
	| NTH_VALUE '(' value ',' value ')' nth_from
		{ $$ = newNode<NthValueWinNode>($3, $5, $7); }
	| LAG '(' value ',' value ',' value ')'
		{ $$ = newNode<LagWinNode>($3, $5, $7); }
	| LAG '(' value ',' value ')'
		{ $$ = newNode<LagWinNode>($3, $5, newNode<NullNode>()); }
	| LAG '(' value ')'
		{ $$ = newNode<LagWinNode>($3, MAKE_const_slong(1), newNode<NullNode>()); }
	| LEAD '(' value ',' value ',' value ')'
		{ $$ = newNode<LeadWinNode>($3, $5, $7); }
	| LEAD '(' value ',' value ')'
		{ $$ = newNode<LeadWinNode>($3, $5, newNode<NullNode>()); }
	| LEAD '(' value ')'
		{ $$ = newNode<LeadWinNode>($3, MAKE_const_slong(1), newNode<NullNode>()); }
	;

%type <valueExprNode> nth_from
nth_from
	: /* nothing */	{ $$ = MAKE_const_slong(NthValueWinNode::FROM_FIRST); }
	| FROM FIRST	{ $$ = MAKE_const_slong(NthValueWinNode::FROM_FIRST); }
	| FROM LAST		{ $$ = MAKE_const_slong(NthValueWinNode::FROM_LAST); }
	;

%type <aggNode> aggregate_window_function
aggregate_window_function
	: aggregate_function
	| window_function
	;

%type <valueExprNode> over_clause
over_clause
	: aggregate_window_function OVER '(' window_partition_opt order_clause ')'
		{ $$ = newNode<OverNode>($1, $4, $5); }
	;

%type <valueListNode> window_partition_opt
window_partition_opt
	: /* nothing */				{ $$ = NULL; }
	| PARTITION BY value_list	{ $$ = $3; }
	;

%type <valueExprNode> delimiter_opt
delimiter_opt
	: /* nothing */		{ $$ = MAKE_str_constant(newIntlString(","), lex.att_charset); }
	| ',' value			{ $$ = $2; }
	;

%type <valueExprNode> numeric_value_function
numeric_value_function
	: extract_expression
	| length_expression
	;

%type <valueExprNode> extract_expression
extract_expression
	: EXTRACT '(' timestamp_part FROM value ')'		{ $$ = newNode<ExtractNode>($3, $5); }
	;

%type <valueExprNode> length_expression
length_expression
	: bit_length_expression
	| char_length_expression
	| octet_length_expression
	;

%type <valueExprNode> bit_length_expression
bit_length_expression
	: BIT_LENGTH '(' value ')'			{ $$ = newNode<StrLenNode>(blr_strlen_bit, $3); }
	;

%type <valueExprNode> char_length_expression
char_length_expression
	: CHAR_LENGTH '(' value ')'			{ $$ = newNode<StrLenNode>(blr_strlen_char, $3); }
	| CHARACTER_LENGTH '(' value ')'	{ $$ = newNode<StrLenNode>(blr_strlen_char, $3); }
	;

%type <valueExprNode> octet_length_expression
octet_length_expression
	: OCTET_LENGTH '(' value ')'		{ $$ = newNode<StrLenNode>(blr_strlen_octet, $3); }
	;

%type <valueExprNode> system_function_expression
system_function_expression
	: system_function_std_syntax '(' value_list_opt ')'
		{ $$ = newNode<SysFuncCallNode>(*$1, $3); }
	| system_function_special_syntax
		{ $$ = $1; }
	;

%type <metaNamePtr> system_function_std_syntax
system_function_std_syntax
	: ABS
	| ACOS
	| ACOSH
	| ASCII_CHAR
	| ASCII_VAL
	| ASIN
	| ASINH
	| ATAN
	| ATAN2
	| ATANH
	| BIN_AND
	| BIN_NOT
	| BIN_OR
	| BIN_SHL
	| BIN_SHR
	| BIN_XOR
	| CEIL
	| CHAR_TO_UUID
	| COS
	| COSH
	| COT
	| EXP
	| FLOOR
	| GEN_UUID
	| HASH
	| LEFT
	| LN
	| LOG
	| LOG10
	| LPAD
	| MAXVALUE
	| MINVALUE
	| MOD
	| PI
	| POWER
	| RAND
	| RDB_GET_CONTEXT
	| RDB_SET_CONTEXT
	| REPLACE
	| REVERSE
	| RIGHT
	| ROUND
	| RPAD
	| SIGN
	| SIN
	| SINH
	| SQRT
	| TAN
	| TANH
	| TRUNC
	| UUID_TO_CHAR
	;

%type <sysFuncCallNode> system_function_special_syntax
system_function_special_syntax
	: DATEADD '(' value timestamp_part TO value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1,
				newNode<ValueListNode>($3)->add(MAKE_const_slong($4))->add($6));
			$$->dsqlSpecialSyntax = true;
		}
	| DATEADD '(' timestamp_part ',' value ',' value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1,
				newNode<ValueListNode>($5)->add(MAKE_const_slong($3))->add($7));
			$$->dsqlSpecialSyntax = true;
		}
	| DATEDIFF '(' timestamp_part FROM value TO value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1,
				newNode<ValueListNode>(MAKE_const_slong($3))->add($5)->add($7));
			$$->dsqlSpecialSyntax = true;
		}
	| DATEDIFF '(' timestamp_part ',' value ',' value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1,
				newNode<ValueListNode>(MAKE_const_slong($3))->add($5)->add($7));
			$$->dsqlSpecialSyntax = true;
		}
	| OVERLAY '(' value PLACING value FROM value FOR value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1,
				newNode<ValueListNode>($3)->add($5)->add($7)->add($9));
			$$->dsqlSpecialSyntax = true;
		}
	| OVERLAY '(' value PLACING value FROM value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1,
				newNode<ValueListNode>($3)->add($5)->add($7));
			$$->dsqlSpecialSyntax = true;
		}
	| POSITION '(' value KW_IN value ')'
		{
			$$ = newNode<SysFuncCallNode>(*$1, newNode<ValueListNode>($3)->add($5));
			$$->dsqlSpecialSyntax = true;
		}
	| POSITION '(' value_list_opt  ')'
		{ $$ = newNode<SysFuncCallNode>(*$1, $3); }
	;

%type <valueExprNode> string_value_function
string_value_function
	: substring_function
	| trim_function
	| KW_UPPER '(' value ')'
		{ $$ = newNode<StrCaseNode>(blr_upcase, $3); }
	| KW_LOWER '(' value ')'
		{ $$ = newNode<StrCaseNode>(blr_lowcase, $3); }
	;

%type <valueExprNode> substring_function
substring_function
	: SUBSTRING '(' value FROM value string_length_opt ')'
		{
			// SQL spec requires numbering to start with 1,
			// hence we decrement the first parameter to make it
			// compatible with the engine's implementation
			ArithmeticNode* subtractNode = newNode<ArithmeticNode>(
				blr_subtract, true, $5, MAKE_const_slong(1));

			$$ = newNode<SubstringNode>($3, subtractNode, $6);
		}
	| SUBSTRING '(' value SIMILAR value ESCAPE value ')'
		{ $$ = newNode<SubstringSimilarNode>($3, $5, $7); }
	;

%type <valueExprNode> string_length_opt
string_length_opt
	: /* nothing */		{ $$ = NULL; }
	| FOR value 		{ $$ = $2; }
	;

%type <valueExprNode> trim_function
trim_function
	: TRIM '(' trim_specification value FROM value ')'
		{ $$ = newNode<TrimNode>($3, $6, $4); }
	| TRIM '(' value FROM value ')'
		{ $$ = newNode<TrimNode>(blr_trim_both, $5, $3); }
	| TRIM '(' trim_specification FROM value ')'
		{ $$ = newNode<TrimNode>($3, $5); }
	| TRIM '(' value ')'
		{ $$ = newNode<TrimNode>(blr_trim_both, $3); }
	;

%type <blrOp> trim_specification
trim_specification
	: BOTH		{ $$ = blr_trim_both; }
	| TRAILING	{ $$ = blr_trim_trailing; }
	| LEADING	{ $$ = blr_trim_leading; }
	;

%type <valueExprNode> udf
udf
	: symbol_UDF_call_name '(' value_list ')'
		{ $$ = newNode<UdfCallNode>(QualifiedName(*$1, ""), $3); }
	| symbol_UDF_call_name '(' ')'
		{ $$ = newNode<UdfCallNode>(QualifiedName(*$1, ""), newNode<ValueListNode>(0)); }
	| symbol_package_name '.' symbol_UDF_name '(' value_list ')'
		{ $$ = newNode<UdfCallNode>(QualifiedName(*$3, *$1), $5); }
	| symbol_package_name '.' symbol_UDF_name '(' ')'
		{ $$ = newNode<UdfCallNode>(QualifiedName(*$3, *$1), newNode<ValueListNode>(0)); }
	;

%type <valueExprNode> cast_specification
cast_specification
	: CAST '(' value AS data_type_descriptor ')'
		{ $$ = newNode<CastNode>($3, $5); }
	;

// case expressions

%type <valueExprNode> case_expression
case_expression
	: case_abbreviation
	| case_specification
	;

%type <valueExprNode> case_abbreviation
case_abbreviation
	: NULLIF '(' value ',' value ')'
		{
			ComparativeBoolNode* condition = newNode<ComparativeBoolNode>(blr_eql, $3, $5);
			$$ = newNode<ValueIfNode>(condition, newNode<NullNode>(), $3);
		}
	| IIF '(' search_condition ',' value ',' value ')'
		{ $$ = newNode<ValueIfNode>($3, $5, $7); }
	| COALESCE '(' value ',' value_list ')'
		{ $$ = newNode<CoalesceNode>($5->addFront($3)); }
	| DECODE '(' value ',' decode_pairs ')'
		{
			ValueListNode* list = $5;
			ValueListNode* conditions = newNode<ValueListNode>(list->items.getCount() / 2);
			ValueListNode* values = newNode<ValueListNode>(list->items.getCount() / 2);

			for (size_t i = 0; i < list->items.getCount(); i += 2)
			{
				conditions->items[i / 2] = list->items[i];
				values->items[i / 2] = list->items[i + 1];
			}

			$$ = newNode<DecodeNode>($3, conditions, values);
		}
	| DECODE '(' value ',' decode_pairs ',' value ')'
		{
			ValueListNode* list = $5;
			ValueListNode* conditions = newNode<ValueListNode>(list->items.getCount() / 2);
			ValueListNode* values = newNode<ValueListNode>(list->items.getCount() / 2 + 1);

			for (size_t i = 0; i < list->items.getCount(); i += 2)
			{
				conditions->items[i / 2] = list->items[i];
				values->items[i / 2] = list->items[i + 1];
			}

			values->items[list->items.getCount() / 2] = $7;

			$$ = newNode<DecodeNode>($3, conditions, values);
		}
	;

%type <valueExprNode> case_specification
case_specification
	: simple_case		{ $$ = $1; }
	| searched_case		{ $$ = $1; }
	;

%type <decodeNode> simple_case
simple_case
	: CASE case_operand
			{ $$ = newNode<DecodeNode>($2, newNode<ValueListNode>(0u), newNode<ValueListNode>(0u)); }
		simple_when_clause(NOTRIAL($3->conditions), NOTRIAL($3->values))
		else_case_result_opt END
			{
				DecodeNode* node = $$ = $3;
				node->label = "CASE";
				if ($5)
					node->values->add($5);
			}
	;

%type simple_when_clause(<valueListNode>, <valueListNode>)
simple_when_clause($conditions, $values)
	: WHEN when_operand THEN case_result
		{
			$conditions->add($2);
			$values->add($4);
		}
	| simple_when_clause WHEN when_operand THEN case_result
		{
			$conditions->add($3);
			$values->add($5);
		}
	;

%type <valueExprNode> else_case_result_opt
else_case_result_opt
	: /* nothing */		{ $$ = NULL; }
	| ELSE case_result	{ $$ = $2; }

%type <valueExprNode> searched_case
searched_case
	: CASE searched_when_clause END
		{ $$ = $2; }
	| CASE searched_when_clause ELSE case_result END
		{
			ValueIfNode* last = $2;
			ValueIfNode* next;

			while ((next = last->falseValue->as<ValueIfNode>()))
				last = next;

			fb_assert(last->falseValue->is<NullNode>());

			last->falseValue = $4;
			$$ = $2;
		}
	;

%type <valueIfNode> searched_when_clause
searched_when_clause
	: WHEN search_condition THEN case_result
		{ $$ = newNode<ValueIfNode>($2, $4, newNode<NullNode>()); }
	| searched_when_clause WHEN search_condition THEN case_result
		{
			ValueIfNode* cond = newNode<ValueIfNode>($3, $5, newNode<NullNode>());
			ValueIfNode* last = $1;
			ValueIfNode* next;

			while ((next = last->falseValue->as<ValueIfNode>()))
				last = next;

			fb_assert(last->falseValue->is<NullNode>());

			last->falseValue = cond;
			$$ = $1;
		}
	;

%type <valueExprNode> when_operand
when_operand
	: value
	;

%type <valueExprNode> case_operand
case_operand
	: value
	;

%type <valueExprNode> case_result
case_result
	: value
	;

%type <valueListNode> decode_pairs
decode_pairs
	: value ',' value
		{ $$ = newNode<ValueListNode>(0u)->add($1)->add($3); }
	| decode_pairs ',' value ',' value
		{ $$ = $1->add($3)->add($5); }
	;

// next value expression

%type <valueExprNode> next_value_expression
next_value_expression
	: NEXT KW_VALUE FOR symbol_generator_name
		{
			$$ = newNode<GenIdNode>((client_dialect < SQL_DIALECT_V6_TRANSITION),
				*$4, ((Jrd::ValueExprNode*) NULL), true);
		}
	| GEN_ID '(' symbol_generator_name ',' value ')'
		{ $$ = newNode<GenIdNode>((client_dialect < SQL_DIALECT_V6_TRANSITION), *$3, $5, false); }
	;


%type <blrOp> timestamp_part
timestamp_part
	: YEAR			{ $$ = blr_extract_year; }
	| MONTH			{ $$ = blr_extract_month; }
	| DAY			{ $$ = blr_extract_day; }
	| HOUR			{ $$ = blr_extract_hour; }
	| MINUTE		{ $$ = blr_extract_minute; }
	| SECOND		{ $$ = blr_extract_second; }
	| MILLISECOND	{ $$ = blr_extract_millisecond; }
	| WEEK			{ $$ = blr_extract_week; }
	| WEEKDAY		{ $$ = blr_extract_weekday; }
	| YEARDAY		{ $$ = blr_extract_yearday; }
	;

all_noise
	:
	| ALL
	;

distinct_noise
	:
	| DISTINCT
	;

%type <valueExprNode> null_value
null_value
	: KW_NULL
		{ $$ = newNode<NullNode>(); }
	| UNKNOWN
		{
			dsql_fld* field = newNode<dsql_fld>();
			field->dtype = dtype_boolean;
			field->length = sizeof(UCHAR);

			CastNode* castNode = newNode<CastNode>(newNode<NullNode>(), field);
			castNode->dsqlAlias = "CONSTANT";
			$$ = castNode;
		}
	;


// Performs special mapping of keywords into symbols

%type <metaNamePtr> symbol_UDF_call_name
symbol_UDF_call_name
	: SYMBOL
	;

%type <metaNamePtr> symbol_UDF_name
symbol_UDF_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_blob_subtype_name
symbol_blob_subtype_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_character_set_name
symbol_character_set_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_collation_name
symbol_collation_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_column_name
symbol_column_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_constraint_name
symbol_constraint_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_cursor_name
symbol_cursor_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_domain_name
symbol_domain_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_exception_name
symbol_exception_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_filter_name
symbol_filter_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_gdscode_name
symbol_gdscode_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_generator_name
symbol_generator_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_index_name
symbol_index_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_item_alias_name
symbol_item_alias_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_label_name
symbol_label_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_ddl_name
symbol_ddl_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_procedure_name
symbol_procedure_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_role_name
symbol_role_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_table_alias_name
symbol_table_alias_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_table_name
symbol_table_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_trigger_name
symbol_trigger_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_user_name
symbol_user_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_variable_name
symbol_variable_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_view_name
symbol_view_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_savepoint_name
symbol_savepoint_name
	: valid_symbol_name
	;

%type <metaNamePtr> symbol_package_name
symbol_package_name
	: valid_symbol_name
	;

// symbols

%type <metaNamePtr> valid_symbol_name
valid_symbol_name
	: SYMBOL
	| non_reserved_word
	;

// list of non-reserved words

%type <metaNamePtr> non_reserved_word
non_reserved_word
	: ACTION				// added in IB 5.0/
	| CASCADE
	| FREE_IT
	| RESTRICT
	| ROLE
	| KW_TYPE				// added in IB 6.0
	| KW_BREAK				// added in FB 1.0
	| KW_DESCRIPTOR
	| SUBSTRING
	| COALESCE				// added in FB 1.5
	| LAST
	| LEAVE
	| LOCK
	| NULLIF
	| NULLS
	| STATEMENT
	| INSERTING
	| UPDATING
	| DELETING
	| FIRST
	| SKIP
	| BLOCK					// added in FB 2.0
	| BACKUP
	| KW_DIFFERENCE
	| IIF
	| SCALAR_ARRAY
	| WEEKDAY
	| YEARDAY
	| SEQUENCE
	| NEXT
	| RESTART
	| COLLATION
	| RETURNING
	| KW_IGNORE
	| LIMBO
	| UNDO
	| REQUESTS
	| TIMEOUT
	| ABS					// added in FB 2.1
	| ACCENT
	| ACOS
	| ALWAYS
	| ASCII_CHAR
	| ASCII_VAL
	| ASIN
	| ATAN
	| ATAN2
	| BIN_AND
	| BIN_OR
	| BIN_SHL
	| BIN_SHR
	| BIN_XOR
	| CEIL
	| COS
	| COSH
	| COT
	| DATEADD
	| DATEDIFF
	| DECODE
	| EXP
	| FLOOR
	| GEN_UUID
	| GENERATED
	| HASH
	| LIST
	| LN
	| LOG
	| LOG10
	| LPAD
	| MATCHED
	| MATCHING
	| MAXVALUE
	| MILLISECOND
	| MINVALUE
	| MOD
	| OVERLAY
	| PAD
	| PI
	| PLACING
	| POWER
	| PRESERVE
	| RAND
	| REPLACE
	| REVERSE
	| ROUND
	| RPAD
	| SIGN
	| SIN
	| SINH
	| SPACE
	| SQRT
	| TAN
	| TANH
	| TEMPORARY
	| TRUNC
	| WEEK
	| AUTONOMOUS			// added in FB 2.5
	| CHAR_TO_UUID
	| FIRSTNAME
	| MIDDLENAME
	| LASTNAME
	| MAPPING
	| OS_NAME
	| UUID_TO_CHAR
	| GRANTED
	| CALLER				// new execute statement
	| COMMON
	| DATA
	| SOURCE
	| TWO_PHASE
	| BIN_NOT
	| ACTIVE				// old keywords, that were reserved pre-Firebird.2.5
//	| ADD					// words commented it this list remain reserved due to conflicts
	| AFTER
	| ASC
	| AUTO
	| BEFORE
	| COMMITTED
	| COMPUTED
	| CONDITIONAL
	| CONTAINING
	| CSTRING
	| DATABASE
//	| DB_KEY
	| DESC
	| DO
	| KW_DOMAIN
	| ENTRY_POINT
	| EXCEPTION
	| EXIT
	| KW_FILE
//	| GDSCODE
	| GENERATOR
	| GEN_ID
	| IF
	| INACTIVE
//	| INDEX
	| INPUT_TYPE
	| ISOLATION
	| KEY
	| LENGTH
	| LEVEL
//	| KW_LONG
	| MANUAL
	| MODULE_NAME
	| NAMES
	| OPTION
	| OUTPUT_TYPE
	| KW_OVERFLOW
	| PAGE
	| PAGES
	| KW_PAGE_SIZE
	| PASSWORD
//	| PLAN
//	| POST_EVENT
	| PRIVILEGES
	| PROTECTED
	| READ
	| RESERVING
	| RETAIN
//	| RETURNING_VALUES
	| SEGMENT
	| SHADOW
	| KW_SHARED
	| SINGULAR
	| KW_SIZE
	| SNAPSHOT
	| SORT
//	| SQLCODE
	| STABILITY
	| STARTING
	| STATISTICS
	| SUB_TYPE
	| SUSPEND
	| TRANSACTION
	| UNCOMMITTED
//	| VARIABLE
//	| VIEW
	| WAIT
//	| WEEK
//	| WHILE
	| WORK
	| WRITE				// end of old keywords, that were reserved pre-Firebird.2.5
	| KW_ABSOLUTE		// added in FB 3.0
	| ACOSH
	| ASINH
	| ATANH
	| BODY
	| CONTINUE
	| DDL
	| DECRYPT
	| ENCRYPT
	| ENGINE
	| IDENTITY
	| NAME
	| PACKAGE
	| PARTITION
	| PRIOR
	| RDB_GET_CONTEXT
	| RDB_SET_CONTEXT
	| KW_RELATIVE
	| DENSE_RANK
	| FIRST_VALUE
	| NTH_VALUE
	| LAST_VALUE
	| LAG
	| LEAD
	| RANK
	| ROW_NUMBER
	| USAGE
	| LINGER
	| TAGS
	| PLUGIN
	| SERVERWIDE
	| INCREMENT
	| TRUSTED
	;

%%

