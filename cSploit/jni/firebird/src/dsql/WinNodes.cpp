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
 *  Copyright (c) 2010 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../dsql/WinNodes.h"
#include "../dsql/make_proto.h"
#include "../dsql/pass1_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/recsrc/RecordSource.h"

using namespace Firebird;
using namespace Jrd;

namespace Jrd {


static RegisterNode<WinFuncNode> regWinFuncNode(blr_agg_function);

WinFuncNode::Factory* WinFuncNode::factories = NULL;

WinFuncNode::WinFuncNode(MemoryPool& pool, const AggInfo& aAggInfo, ValueExprNode* aArg)
	: AggNode(pool, aAggInfo, false, false, aArg)
{
}

DmlNode* WinFuncNode::parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR /*blrOp*/)
{
	MetaName name;
	PAR_name(csb, name);

	WinFuncNode* node = NULL;

	for (const Factory* factory = factories; factory; factory = factory->next)
	{
		if (name == factory->name)
		{
			node = factory->newInstance(pool);
			break;
		}
	}

	if (!node)
		PAR_error(csb, Arg::Gds(isc_funnotdef) << name);

	UCHAR count = csb->csb_blr_reader.getByte();

	if (count != node->jrdChildNodes.getCount())
		PAR_error(csb, Arg::Gds(isc_funmismat) << name);

	node->parseArgs(tdbb, csb, count);

	return node;
}


//--------------------


static WinFuncNode::Register<DenseRankWinNode> denseRankWinInfo("DENSE_RANK");

DenseRankWinNode::DenseRankWinNode(MemoryPool& pool)
	: WinFuncNode(pool, denseRankWinInfo)
{
	fb_assert(dsqlChildNodes.getCount() == 1 && jrdChildNodes.getCount() == 1);
	dsqlChildNodes.clear();
	jrdChildNodes.clear();
}

void DenseRankWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void DenseRankWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* DenseRankWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW(*tdbb->getDefaultPool()) DenseRankWinNode(*tdbb->getDefaultPool());
}

void DenseRankWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

void DenseRankWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* DenseRankWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* request) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	++impure->vlu_misc.vlu_int64;
	return &impure->vlu_desc;
}

AggNode* DenseRankWinNode::dsqlCopy(DsqlCompilerScratch* /*dsqlScratch*/) /*const*/
{
	return FB_NEW(getPool()) DenseRankWinNode(getPool());
}


//--------------------


static WinFuncNode::Register<RankWinNode> rankWinInfo("RANK");

RankWinNode::RankWinNode(MemoryPool& pool)
	: WinFuncNode(pool, rankWinInfo),
	  tempImpure(0)
{
	fb_assert(dsqlChildNodes.getCount() == 1 && jrdChildNodes.getCount() == 1);
	dsqlChildNodes.clear();
	jrdChildNodes.clear();
}

void RankWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void RankWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* RankWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW(*tdbb->getDefaultPool()) RankWinNode(*tdbb->getDefaultPool());
}

AggNode* RankWinNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	AggNode::pass2(tdbb, csb);
	tempImpure = CMP_impure(csb, sizeof(impure_value_ex));
	return this;
}

void RankWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(1, 0);
	impure->vlux_count = 0;
}

void RankWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* request, dsc* /*desc*/) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	++impure->vlux_count;
}

dsc* RankWinNode::aggExecute(thread_db* tdbb, jrd_req* request) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	dsc temp;
	temp.makeInt64(0, &impure->vlu_misc.vlu_int64);

	impure_value_ex* impureTemp = request->getImpure<impure_value_ex>(tempImpure);
	EVL_make_value(tdbb, &temp, impureTemp);

	impure->vlu_misc.vlu_int64 += impure->vlux_count;
	impure->vlux_count = 0;

	return &impureTemp->vlu_desc;
}

AggNode* RankWinNode::dsqlCopy(DsqlCompilerScratch* /*dsqlScratch*/) /*const*/
{
	return FB_NEW(getPool()) RankWinNode(getPool());
}


//--------------------


static WinFuncNode::Register<RowNumberWinNode> rowNumberWinInfo("ROW_NUMBER");

RowNumberWinNode::RowNumberWinNode(MemoryPool& pool)
	: WinFuncNode(pool, rowNumberWinInfo)
{
	fb_assert(dsqlChildNodes.getCount() == 1 && jrdChildNodes.getCount() == 1);
	dsqlChildNodes.clear();
	jrdChildNodes.clear();
}

void RowNumberWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void RowNumberWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* RowNumberWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW(*tdbb->getDefaultPool()) RowNumberWinNode(*tdbb->getDefaultPool());
}

void RowNumberWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

void RowNumberWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* RowNumberWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* request) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	return &impure->vlu_desc;
}

dsc* RowNumberWinNode::winPass(thread_db* /*tdbb*/, jrd_req* request, SlidingWindow* /*window*/) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	++impure->vlu_misc.vlu_int64;
	return &impure->vlu_desc;
}

AggNode* RowNumberWinNode::dsqlCopy(DsqlCompilerScratch* /*dsqlScratch*/) /*const*/
{
	return FB_NEW(getPool()) RowNumberWinNode(getPool());
}


//--------------------


static WinFuncNode::Register<FirstValueWinNode> firstValueWinInfo("FIRST_VALUE");

FirstValueWinNode::FirstValueWinNode(MemoryPool& pool, ValueExprNode* aArg)
	: WinFuncNode(pool, firstValueWinInfo, aArg)
{
}

void FirstValueWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
}

void FirstValueWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void FirstValueWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

ValueExprNode* FirstValueWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	FirstValueWinNode* node = FB_NEW(*tdbb->getDefaultPool()) FirstValueWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	return node;
}

void FirstValueWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

void FirstValueWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* FirstValueWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
{
	return NULL;
}

dsc* FirstValueWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	SINT64 records = impure->vlu_misc.vlu_int64++;

	if (!window->move(-records))
	{
		window->move(0);	// Come back to our row.
		return NULL;
	}

	dsc* desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}

AggNode* FirstValueWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW(getPool()) FirstValueWinNode(getPool(), doDsqlPass(dsqlScratch, arg));
}


//--------------------


static WinFuncNode::Register<LastValueWinNode> lastValueWinInfo("LAST_VALUE");

LastValueWinNode::LastValueWinNode(MemoryPool& pool, ValueExprNode* aArg)
	: WinFuncNode(pool, lastValueWinInfo, aArg)
{
}

void LastValueWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
}

void LastValueWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void LastValueWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

ValueExprNode* LastValueWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	LastValueWinNode* node = FB_NEW(*tdbb->getDefaultPool()) LastValueWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	return node;
}

void LastValueWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);
}

void LastValueWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* LastValueWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
{
	return NULL;
}

dsc* LastValueWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	if (!window->move(0))
		return NULL;

	dsc* desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}

AggNode* LastValueWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW(getPool()) LastValueWinNode(getPool(), doDsqlPass(dsqlScratch, arg));
}


//--------------------


static WinFuncNode::Register<NthValueWinNode> nthValueWinInfo("NTH_VALUE");

NthValueWinNode::NthValueWinNode(MemoryPool& pool, ValueExprNode* aArg, ValueExprNode* aRow,
			ValueExprNode* aFrom)
	: WinFuncNode(pool, nthValueWinInfo, aArg),
	  row(aRow),
	  from(aFrom)
{
	addChildNode(row, row);
	addChildNode(from, from);
}

void NthValueWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
	row = PAR_parse_value(tdbb, csb);
	from = PAR_parse_value(tdbb, csb);
}

void NthValueWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void NthValueWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

ValueExprNode* NthValueWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	NthValueWinNode* node = FB_NEW(*tdbb->getDefaultPool()) NthValueWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	node->row = copier.copy(tdbb, row);
	node->from = copier.copy(tdbb, from);
	return node;
}

void NthValueWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

void NthValueWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* NthValueWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
{
	return NULL;
}

dsc* NthValueWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	window->move(0);	// Come back to our row because row may reference columns.

	dsc* desc = EVL_expr(tdbb, request, row);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	SINT64 records = MOV_get_int64(desc, 0);
	if (records <= 0)
	{
		status_exception::raise(Arg::Gds(isc_sysf_argnmustbe_positive) <<
			Arg::Num(2) << Arg::Str(aggInfo.name));
	}

	desc = EVL_expr(tdbb, request, from);
	const SLONG fromPos = desc ? MOV_get_long(desc, 0) : FROM_FIRST;

	if (fromPos == FROM_FIRST)
	{
		if (records > ++impure->vlu_misc.vlu_int64)
			return NULL;

		records -= impure->vlu_misc.vlu_int64;
	}
	else
		records = impure->vlu_misc.vlu_int64 - records + 1;

	if (!window->move(records))
	{
		window->move(0);	// Come back to our row.
		return NULL;
	}

	desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}

AggNode* NthValueWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW(getPool()) NthValueWinNode(getPool(),
		doDsqlPass(dsqlScratch, arg), doDsqlPass(dsqlScratch, row), doDsqlPass(dsqlScratch, from));
}


//--------------------


// A direction of -1 is LAG, and 1 is LEAD.
LagLeadWinNode::LagLeadWinNode(MemoryPool& pool, const AggInfo& aAggInfo, int aDirection,
			ValueExprNode* aArg, ValueExprNode* aRows, ValueExprNode* aOutExpr)
	: WinFuncNode(pool, aAggInfo, aArg),
	  direction(aDirection),
	  rows(aRows),
	  outExpr(aOutExpr)
{
	fb_assert(direction == -1 || direction == 1);

	addChildNode(rows, rows);
	addChildNode(outExpr, outExpr);
}

void LagLeadWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
	rows = PAR_parse_value(tdbb, csb);
	outExpr = PAR_parse_value(tdbb, csb);
}

void LagLeadWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void LagLeadWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

void LagLeadWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);
}

void LagLeadWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* LagLeadWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
{
	return NULL;
}

dsc* LagLeadWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	window->move(0);	// Come back to our row because rows may reference columns.

	dsc* desc = EVL_expr(tdbb, request, rows);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	SINT64 records = MOV_get_int64(desc, 0);
	if (records < 0)
	{
		status_exception::raise(Arg::Gds(isc_sysf_argnmustbe_nonneg) <<
			Arg::Num(2) << Arg::Str(aggInfo.name));
	}

	if (!window->move(records * direction))
	{
		window->move(0);	// Come back to our row because outExpr may reference columns.

		desc = EVL_expr(tdbb, request, outExpr);
		if (!desc || (request->req_flags & req_null))
			return NULL;

		return desc;
	}

	desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}


//--------------------


static WinFuncNode::Register<LagWinNode> lagWinInfo("LAG");

LagWinNode::LagWinNode(MemoryPool& pool, ValueExprNode* aArg, ValueExprNode* aRows,
			ValueExprNode* aOutExpr)
	: LagLeadWinNode(pool, lagWinInfo, -1, aArg, aRows, aOutExpr)
{
}

ValueExprNode* LagWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	LagWinNode* node = FB_NEW(*tdbb->getDefaultPool()) LagWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	node->rows = copier.copy(tdbb, rows);
	node->outExpr = copier.copy(tdbb, outExpr);
	return node;
}

AggNode* LagWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW(getPool()) LagWinNode(getPool(),
		doDsqlPass(dsqlScratch, arg),
		doDsqlPass(dsqlScratch, rows),
		doDsqlPass(dsqlScratch, outExpr));
}


//--------------------


static WinFuncNode::Register<LeadWinNode> leadWinInfo("LEAD");

LeadWinNode::LeadWinNode(MemoryPool& pool, ValueExprNode* aArg, ValueExprNode* aRows,
			ValueExprNode* aOutExpr)
	: LagLeadWinNode(pool, leadWinInfo, 1, aArg, aRows, aOutExpr)
{
}

ValueExprNode* LeadWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	LeadWinNode* node = FB_NEW(*tdbb->getDefaultPool()) LeadWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	node->rows = copier.copy(tdbb, rows);
	node->outExpr = copier.copy(tdbb, outExpr);
	return node;
}

AggNode* LeadWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW(getPool()) LeadWinNode(getPool(),
		doDsqlPass(dsqlScratch, arg),
		doDsqlPass(dsqlScratch, rows),
		doDsqlPass(dsqlScratch, outExpr));
}


}	// namespace Jrd
