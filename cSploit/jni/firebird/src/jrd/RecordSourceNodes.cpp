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
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "../jrd/align.h"
#include "../jrd/RecordSourceNodes.h"
#include "../jrd/DataTypeUtil.h"
#include "../jrd/Optimizer.h"
#include "../jrd/recsrc/RecordSource.h"
#include "../dsql/BoolNodes.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"
#include "../dsql/dsql.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cmp_proto.h"
#include "../common/dsc_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/par_proto.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/pass1_proto.h"

using namespace Firebird;
using namespace Jrd;


static RecordSourceNode* dsqlPassRelProc(DsqlCompilerScratch* dsqlScratch, RecordSourceNode* source);
static MapNode* parseMap(thread_db* tdbb, CompilerScratch* csb, StreamType stream);
static int strcmpSpace(const char* p, const char* q);
static void processSource(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
	RecordSourceNode* source, BoolExprNode** boolean, RecordSourceNodeStack& stack);
static void processMap(thread_db* tdbb, CompilerScratch* csb, MapNode* map, Format** inputFormat);
static void genDeliverUnmapped(thread_db* tdbb, BoolExprNodeStack* deliverStack, MapNode* map,
	BoolExprNodeStack* parentStack, StreamType shellStream);
static void markIndices(CompilerScratch::csb_repeat* csbTail, SSHORT relationId);
static ValueExprNode* resolveUsingField(DsqlCompilerScratch* dsqlScratch, const MetaName& name,
	ValueListNode* list, const FieldNode* flawedNode, const TEXT* side, dsql_ctx*& ctx);
static void sortIndicesBySelectivity(CompilerScratch::csb_repeat* csbTail);

namespace
{
	class AutoActivateResetStreams : public AutoStorage
	{
	public:
		AutoActivateResetStreams(CompilerScratch* csb, const RseNode* rse)
			: m_csb(csb), m_streams(getPool()), m_flags(getPool())
		{
			rse->computeRseStreams(m_streams);

			if (m_streams.getCount() >= MAX_STREAMS)
				ERR_post(Arg::Gds(isc_too_many_contexts));

			m_flags.resize(m_streams.getCount());

			for (size_t i = 0; i < m_streams.getCount(); i++)
			{
				const StreamType stream = m_streams[i];
				m_flags[i] = m_csb->csb_rpt[stream].csb_flags;
				m_csb->csb_rpt[stream].csb_flags |= (csb_active | csb_sub_stream);
			}
		}

		~AutoActivateResetStreams()
		{
			for (size_t i = 0; i < m_streams.getCount(); i++)
			{
				const StreamType stream = m_streams[i];
				m_csb->csb_rpt[stream].csb_flags = m_flags[i];
			}
		}

	private:
		CompilerScratch* m_csb;
		StreamList m_streams;
		HalfStaticArray<USHORT, OPT_STATIC_ITEMS> m_flags;
	};
}


//--------------------


SortNode* SortNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	SortNode* newSort = FB_NEW(*tdbb->getDefaultPool()) SortNode(*tdbb->getDefaultPool());
	newSort->unique = unique;

	for (const NestConst<ValueExprNode>* i = expressions.begin(); i != expressions.end(); ++i)
		newSort->expressions.add(copier.copy(tdbb, *i));

	newSort->descending = descending;
	newSort->nullOrder = nullOrder;

	return newSort;
}

SortNode* SortNode::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	for (NestConst<ValueExprNode>* i = expressions.begin(); i != expressions.end(); ++i)
		DmlNode::doPass1(tdbb, csb, i->getAddress());

	return this;
}

SortNode* SortNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	for (NestConst<ValueExprNode>* i = expressions.begin(); i != expressions.end(); ++i)
		(*i)->nodFlags |= ExprNode::FLAG_VALUE;

	for (NestConst<ValueExprNode>* i = expressions.begin(); i != expressions.end(); ++i)
		ExprNode::doPass2(tdbb, csb, i->getAddress());

	return this;
}

bool SortNode::computable(CompilerScratch* csb, StreamType stream, bool allowOnlyCurrentStream)
{
	for (NestConst<ValueExprNode>* i = expressions.begin(); i != expressions.end(); ++i)
	{
		if (!(*i)->computable(csb, stream, allowOnlyCurrentStream))
			return false;
	}

	return true;
}

void SortNode::findDependentFromStreams(const OptimizerRetrieval* optRet,
	SortedStreamList* streamList)
{
	for (NestConst<ValueExprNode>* i = expressions.begin(); i != expressions.end(); ++i)
		(*i)->findDependentFromStreams(optRet, streamList);
}


//--------------------


MapNode* MapNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	MapNode* newMap = FB_NEW(*tdbb->getDefaultPool()) MapNode(*tdbb->getDefaultPool());

	const NestConst<ValueExprNode>* target = targetList.begin();

	for (const NestConst<ValueExprNode>* source = sourceList.begin();
		 source != sourceList.end();
		 ++source, ++target)
	{
		newMap->sourceList.add(copier.copy(tdbb, *source));
		newMap->targetList.add(copier.copy(tdbb, *target));
	}

	return newMap;
}

MapNode* MapNode::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	NestConst<ValueExprNode>* target = targetList.begin();

	for (NestConst<ValueExprNode>* source = sourceList.begin();
		 source != sourceList.end();
		 ++source, ++target)
	{
		DmlNode::doPass1(tdbb, csb, source->getAddress());
		DmlNode::doPass1(tdbb, csb, target->getAddress());
	}

	return this;
}

MapNode* MapNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	NestConst<ValueExprNode>* target = targetList.begin();

	for (NestConst<ValueExprNode>* source = sourceList.begin();
		 source != sourceList.end();
		 ++source, ++target)
	{
		ExprNode::doPass2(tdbb, csb, source->getAddress());
		ExprNode::doPass2(tdbb, csb, target->getAddress());
	}

	return this;
}

void MapNode::aggPostRse(thread_db* tdbb, CompilerScratch* csb)
{
	for (NestConst<ValueExprNode>* source = sourceList.begin();
		 source != sourceList.end();
		 ++source)
	{
		AggNode* aggNode = (*source)->as<AggNode>();
		if (aggNode)
			aggNode->aggPostRse(tdbb, csb);
	}
}


//--------------------


PlanNode* PlanNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	PlanNode* node = FB_NEW(getPool()) PlanNode(getPool(), type);

	if (accessType)
	{
		node->accessType = FB_NEW(getPool()) AccessType(getPool(), accessType->type);
		node->accessType->items = accessType->items;
	}

	node->relationNode = relationNode;

	for (NestConst<PlanNode>* i = subNodes.begin(); i != subNodes.end(); ++i)
		node->subNodes.add((*i)->dsqlPass(dsqlScratch));

	if (dsqlNames)
	{
		node->dsqlNames = FB_NEW(getPool()) ObjectsArray<MetaName>(getPool());
		*node->dsqlNames = *dsqlNames;

		dsql_ctx* context = dsqlPassAliasList(dsqlScratch);

		if (context->ctx_relation)
		{
			RelationSourceNode* relNode = FB_NEW(getPool()) RelationSourceNode(getPool());
			relNode->dsqlContext = context;
			node->dsqlRecordSourceNode = relNode;
		}
		else if (context->ctx_procedure)
		{
			// ASF: Note that usage of procedure name in a PLAN clause causes errors when
			// parsing the BLR.
			ProcedureSourceNode* procNode = FB_NEW(getPool()) ProcedureSourceNode(getPool());
			procNode->dsqlContext = context;
			node->dsqlRecordSourceNode = procNode;
		}

		// ASF: I think it's a error to let node->dsqlRecordSourceNode be NULL here, but it happens
		// at least since v2.5. See gen.cpp/gen_plan for more information.
		///fb_assert(node->dsqlRecordSourceNode);
	}

	return node;
}

// The passed alias list fully specifies a relation. The first alias represents a relation
// specified in the from list at this scope levels. Subsequent contexts, if there are any,
// represent base relations in a view stack. They are used to fully specify a base relation of
// a view. The aliases used in the view stack are those used in the view definition.
dsql_ctx* PlanNode::dsqlPassAliasList(DsqlCompilerScratch* dsqlScratch)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	ObjectsArray<MetaName>::iterator arg = dsqlNames->begin();
	ObjectsArray<MetaName>::iterator end = dsqlNames->end();

	// Loop through every alias and find the context for that alias.
	// All aliases should have a corresponding context.
	int aliasCount = dsqlNames->getCount();
	USHORT savedScopeLevel = dsqlScratch->scopeLevel;
	dsql_ctx* context = NULL;
	while (aliasCount > 0)
	{
		if (context)
		{
			if (context->ctx_rse && !context->ctx_relation && !context->ctx_procedure)
			{
				// Derived table
				dsqlScratch->scopeLevel++;
				context = dsqlPassAlias(dsqlScratch, context->ctx_childs_derived_table, *arg);
			}
			else if (context->ctx_relation)
			{
				// This must be a VIEW
				ObjectsArray<MetaName>::iterator startArg = arg;
				dsql_rel* relation = context->ctx_relation;

				// find the base table using the specified alias list, skipping the first one
				// since we already matched it to the context.
				for (; arg != end; ++arg, --aliasCount)
				{
					relation = METD_get_view_relation(dsqlScratch->getTransaction(),
						dsqlScratch, relation->rel_name.c_str(), arg->c_str());

					if (!relation)
						break;
				}

				// Found base relation
				if (aliasCount == 0 && relation)
				{
					// AB: Pretty ugly huh?
					// make up a dummy context to hold the resultant relation.
					dsql_ctx* newContext = FB_NEW(getPool()) dsql_ctx(getPool());
					newContext->ctx_context = context->ctx_context;
					newContext->ctx_relation = relation;

					// Concatenate all the contexts to form the alias name.
					// Calculate the length leaving room for spaces.
					USHORT aliasLength = dsqlNames->getCount();
					ObjectsArray<MetaName>::iterator aliasArg = startArg;
					for (; aliasArg != end; ++aliasArg)
						aliasLength += aliasArg->length();

					newContext->ctx_alias.reserve(aliasLength);

					for (aliasArg = startArg; aliasArg != end; ++aliasArg)
					{
						if (aliasArg != startArg)
							newContext->ctx_alias.append(" ", 1);
						newContext->ctx_alias.append(aliasArg->c_str(), aliasArg->length());
					}

					context = newContext;
				}
				else
					context = NULL;
			}
			else
				context = NULL;
		}
		else
			context = dsqlPassAlias(dsqlScratch, *dsqlScratch->context, *arg);

		if (!context)
			break;

		++arg;
		--aliasCount;
	}

	dsqlScratch->scopeLevel = savedScopeLevel;

	if (!context)
	{
		// there is no alias or table named %s at this scope level.
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_dsql_no_relation_alias) << *arg);
	}

	return context;
}

// The passed relation or alias represents a context which was previously specified in the from
// list. Find and return the proper context.
dsql_ctx* PlanNode::dsqlPassAlias(DsqlCompilerScratch* dsqlScratch, DsqlContextStack& stack,
	const MetaName& alias)
{
	dsql_ctx* relation_context = NULL;

	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	// look through all contexts at this scope level
	// to find one that has a relation name or alias
	// name which matches the identifier passed.
	for (DsqlContextStack::iterator itr(stack); itr.hasData(); ++itr)
	{
		dsql_ctx* context = itr.object();
		if (context->ctx_scope_level != dsqlScratch->scopeLevel)
			continue;

		// check for matching alias.
		if (context->ctx_internal_alias.hasData())
		{
			if (context->ctx_internal_alias == alias.c_str())
				return context;

			continue;
		}

		// If an unnamed derived table and empty alias.
		if (context->ctx_rse && !context->ctx_relation && !context->ctx_procedure && alias.isEmpty())
			relation_context = context;

		// Check for matching relation name; aliases take priority so
		// save the context in case there is an alias of the same name.
		// Also to check that there is no self-join in the query.
		if (context->ctx_relation && context->ctx_relation->rel_name == alias)
		{
			if (relation_context)
			{
				// the table %s is referenced twice; use aliases to differentiate
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  Arg::Gds(isc_dsql_self_join) << alias);
			}

			relation_context = context;
		}
	}

	return relation_context;
}


//--------------------


RecSourceListNode::RecSourceListNode(MemoryPool& pool, unsigned count)
	: TypedNode<ListExprNode, ExprNode::TYPE_REC_SOURCE_LIST>(pool),
	  items(pool)
{
	items.resize(count);

	for (unsigned i = 0; i < count; ++i)
		addDsqlChildNode((items[i] = NULL));
}

RecSourceListNode::RecSourceListNode(MemoryPool& pool, RecordSourceNode* arg1)
	: TypedNode<ListExprNode, ExprNode::TYPE_REC_SOURCE_LIST>(pool),
	  items(pool)
{
	items.resize(1);
	addDsqlChildNode((items[0] = arg1));
}

RecSourceListNode* RecSourceListNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	RecSourceListNode* node = FB_NEW(getPool()) RecSourceListNode(getPool(), items.getCount());

	NestConst<RecordSourceNode>* dst = node->items.begin();

	for (NestConst<RecordSourceNode>* src = items.begin(); src != items.end(); ++src, ++dst)
		*dst = doDsqlPass(dsqlScratch, *src);

	return node;
}


//--------------------


// Parse a relation reference.
RelationSourceNode* RelationSourceNode::parse(thread_db* tdbb, CompilerScratch* csb,
	const SSHORT blrOp, bool parseContext)
{
	SET_TDBB(tdbb);

	// Make a relation reference node

	RelationSourceNode* node = FB_NEW(*tdbb->getDefaultPool()) RelationSourceNode(
		*tdbb->getDefaultPool());

	// Find relation either by id or by name
	string* aliasString = NULL;
	MetaName name;

	switch (blrOp)
	{
		case blr_rid:
		case blr_rid2:
		{
			const SSHORT id = csb->csb_blr_reader.getWord();

			if (blrOp == blr_rid2)
			{
				aliasString = FB_NEW(csb->csb_pool) string(csb->csb_pool);
				PAR_name(csb, *aliasString);
			}

			if (!(node->relation = MET_lookup_relation_id(tdbb, id, false)))
				name.printf("id %d", id);

			break;
		}

		case blr_relation:
		case blr_relation2:
		{
			PAR_name(csb, name);

			if (blrOp == blr_relation2)
			{
				aliasString = FB_NEW(csb->csb_pool) string(csb->csb_pool);
				PAR_name(csb, *aliasString);
			}

			node->relation = MET_lookup_relation(tdbb, name);
			break;
		}

		default:
			fb_assert(false);
	}

	if (!node->relation)
		PAR_error(csb, Arg::Gds(isc_relnotdef) << Arg::Str(name), false);

	// if an alias was passed, store with the relation

	if (aliasString)
		node->alias = *aliasString;

	// Scan the relation if it hasn't already been scanned for meta data

	if ((!(node->relation->rel_flags & REL_scanned) || (node->relation->rel_flags & REL_being_scanned)) &&
		((node->relation->rel_flags & REL_force_scan) || !(csb->csb_g_flags & csb_internal)))
	{
		node->relation->rel_flags &= ~REL_force_scan;
		MET_scan_relation(tdbb, node->relation);
	}
	else if (node->relation->rel_flags & REL_sys_triggers)
		MET_parse_sys_trigger(tdbb, node->relation);

	// generate a stream for the relation reference, assuming it is a real reference

	if (parseContext)
	{
		node->stream = PAR_context(csb, &node->context);
		fb_assert(node->stream <= MAX_STREAMS);

		csb->csb_rpt[node->stream].csb_relation = node->relation;
		csb->csb_rpt[node->stream].csb_alias = aliasString;

		if (csb->csb_g_flags & csb_get_dependencies)
			PAR_dependency(tdbb, csb, node->stream, (SSHORT) -1, "");
	}
	else
		delete aliasString;

	return node;
}

RecordSourceNode* RelationSourceNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	return dsqlPassRelProc(dsqlScratch, this);
}

bool RelationSourceNode::dsqlMatch(const ExprNode* other, bool /*ignoreMapCast*/) const
{
	const RelationSourceNode* o = other->as<RelationSourceNode>();
	return o && dsqlContext == o->dsqlContext;
}

// Generate blr for a relation reference.
void RelationSourceNode::genBlr(DsqlCompilerScratch* dsqlScratch)
{
	const dsql_rel* relation = dsqlContext->ctx_relation;

	// if this is a trigger or procedure, don't want relation id used

	if (DDL_ids(dsqlScratch))
	{
		dsqlScratch->appendUChar(dsqlContext->ctx_alias.hasData() ? blr_rid2 : blr_rid);
		dsqlScratch->appendUShort(relation->rel_id);
	}
	else
	{
		dsqlScratch->appendUChar(dsqlContext->ctx_alias.hasData() ? blr_relation2 : blr_relation);
		dsqlScratch->appendMetaString(relation->rel_name.c_str());
	}

	if (dsqlContext->ctx_alias.hasData())
		dsqlScratch->appendMetaString(dsqlContext->ctx_alias.c_str());

	GEN_stuff_context(dsqlScratch, dsqlContext);
}

RelationSourceNode* RelationSourceNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	if (!copier.remap)
		BUGCHECK(221);	// msg 221 (CMP) copy: cannot remap

	RelationSourceNode* newSource = FB_NEW(*tdbb->getDefaultPool()) RelationSourceNode(
		*tdbb->getDefaultPool());

	// Last entry in the remap contains the the original stream number.
	// Get that stream number so that the flags can be copied
	// into the newly created child stream.

	const StreamType relativeStream = stream ? copier.remap[stream - 1] : stream;
	newSource->stream = copier.csb->nextStream();
	copier.remap[stream] = newSource->stream;

	newSource->context = context;
	newSource->relation = relation;
	newSource->view = view;

	CompilerScratch::csb_repeat* element = CMP_csb_element(copier.csb, newSource->stream);
	element->csb_relation = newSource->relation;
	element->csb_view = newSource->view;
	element->csb_view_stream = copier.remap[0];

	/** If there was a parent stream no., then copy the flags
		from that stream to its children streams. (Bug 10164/10166)
		For e.g.
		consider a view V1 with 2 streams
				stream #1 from table T1
			stream #2 from table T2
		consider a procedure P1 with 2 streams
				stream #1  from table X
			stream #2  from view V1

		During pass1 of procedure request, the engine tries to expand
		all the views into their base tables. It creates a compiler
		scratch block which initially looks like this
				stream 1  -------- X
				stream 2  -------- V1
			while expanding V1 the engine calls copy() with nod_relation.
			A new stream 3 is created. Now the CompilerScratch looks like
				stream 1  -------- X
				stream 2  -------- V1  map [2,3]
				stream 3  -------- T1
			After T1 stream has been created the flags are copied from
			stream #1 because V1's definition said the original stream
			number for T1 was 1. However since its being merged with
			the procedure request, stream #1 belongs to a different table.
			The flags should be copied from stream 2 i.e. V1. We can get
			this info from variable remap.

			Since we didn't do this properly before, V1's children got
			tagged with whatever flags X possesed leading to various
			errors.

			We now store the proper stream no in relativeStream and
			later use it to copy the flags. -Sudesh (03/05/99)
	**/

	copier.csb->csb_rpt[newSource->stream].csb_flags |=
		copier.csb->csb_rpt[relativeStream].csb_flags & csb_no_dbkey;

	return newSource;
}

void RelationSourceNode::ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const
{
	csb->csb_rpt[stream].csb_flags |= csb_no_dbkey;
	const CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];
	const jrd_rel* relation = tail->csb_relation;

	if (relation)
	{
		CMP_post_access(tdbb, csb, relation->rel_security_name,
			(tail->csb_view) ? tail->csb_view->rel_id : (view ? view->rel_id : 0),
			SCL_select, SCL_object_table, relation->rel_name);
	}
}

void RelationSourceNode::pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
	BoolExprNode** boolean, RecordSourceNodeStack& stack)
{
	stack.push(this);	// Assume that the source will be used. Push it on the final stream stack.

	// We have a view or a base table;
	// prepare to check protection of relation when a field in the stream of the
	// relation is accessed.

	jrd_rel* const parentView = csb->csb_view;
	const StreamType viewStream = csb->csb_view_stream;

	jrd_rel* relationView = relation;
	CMP_post_resource(&csb->csb_resources, relationView, Resource::rsc_relation, relationView->rel_id);
	view = parentView;

	CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, stream);
	element->csb_view = parentView;
	fb_assert(viewStream <= MAX_STREAMS);
	element->csb_view_stream = viewStream;

	// in the case where there is a parent view, find the context name

	if (parentView)
	{
		const ViewContexts& ctx = parentView->rel_view_contexts;
		const USHORT key = context;
		size_t pos;

		if (ctx.find(key, pos))
		{
			element->csb_alias = FB_NEW(csb->csb_pool)
				string(csb->csb_pool, ctx[pos]->vcx_context_name);
		}
	}

	// check for a view - if not, nothing more to do

	RseNode* viewRse = relationView->rel_view_rse;
	if (!viewRse)
		return;

	// we've got a view, expand it

	DEBUG;
	stack.pop();
	StreamType* map = CMP_alloc_map(tdbb, csb, stream);

	AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
		(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);
	AutoSetRestore<jrd_rel*> autoView(&csb->csb_view, relationView);
	AutoSetRestore<StreamType> autoViewStream(&csb->csb_view_stream, stream);

	// We don't expand the view in two cases:
	// 1) If the view has a projection, sort, first/skip or explicit plan.
	// 2) If it's part of an outer join.

	if (rse->rse_jointype || // viewRse->rse_jointype || ???
		viewRse->rse_sorted || viewRse->rse_projection || viewRse->rse_first ||
		viewRse->rse_skip || viewRse->rse_plan)
	{
		NodeCopier copier(csb, map);
		RseNode* copy = viewRse->copy(tdbb, copier);
		DEBUG;
		doPass1(tdbb, csb, &copy);
		stack.push(copy);
		DEBUG;
		return;
	}

	// ASF: Below we start to do things when viewRse->rse_projection is not NULL.
	// But we should never come here, as the code above returns in this case.

	// if we have a projection which we can bubble up to the parent rse, set the
	// parent rse to our projection temporarily to flag the fact that we have already
	// seen one so that lower-level views will not try to map their projection; the
	// projection will be copied and correctly mapped later, but we don't have all
	// the base streams yet

	if (viewRse->rse_projection)
		rse->rse_projection = viewRse->rse_projection;

	// disect view into component relations

	NestConst<RecordSourceNode>* arg = viewRse->rse_relations.begin();
	for (const NestConst<RecordSourceNode>* const end = viewRse->rse_relations.end(); arg != end; ++arg)
	{
		// this call not only copies the node, it adds any streams it finds to the map
		NodeCopier copier(csb, map);
		RecordSourceNode* node = (*arg)->copy(tdbb, copier);

		// Now go out and process the base table itself. This table might also be a view,
		// in which case we will continue the process by recursion.
		processSource(tdbb, csb, rse, node, boolean, stack);
	}

	// When there is a projection in the view, copy the projection up to the query RseNode.
	// In order to make this work properly, we must remap the stream numbers of the fields
	// in the view to the stream number of the base table. Note that the map at this point
	// contains the stream numbers of the referenced relations, since it was added during the call
	// to copy() above. After the copy() below, the fields in the projection will reference the
	// base table(s) instead of the view's context (see bug #8822), so we are ready to context-
	// recognize them in doPass1() - that is, replace the field nodes with actual field blocks.

	if (viewRse->rse_projection)
	{
		NodeCopier copier(csb, map);
		rse->rse_projection = viewRse->rse_projection->copy(tdbb, copier);
		doPass1(tdbb, csb, rse->rse_projection.getAddress());
	}

	// if we encounter a boolean, copy it and retain it by ANDing it in with the
	// boolean on the parent view, if any

	if (viewRse->rse_boolean)
	{
		NodeCopier copier(csb, map);
		BoolExprNode* node = copier.copy(tdbb, viewRse->rse_boolean);

		doPass1(tdbb, csb, &node);

		if (*boolean)
		{
			// The order of the nodes here is important! The
			// boolean from the view must appear first so that
			// it gets expanded first in pass1.

			BinaryBoolNode* andNode = FB_NEW(csb->csb_pool) BinaryBoolNode(csb->csb_pool, blr_and);
			andNode->arg1 = node;
			andNode->arg2 = *boolean;

			*boolean = andNode;
		}
		else
			*boolean = node;
	}
}

void RelationSourceNode::pass2Rse(thread_db* tdbb, CompilerScratch* csb)
{
	fb_assert(stream <= MAX_STREAMS);
	csb->csb_rpt[stream].activate();

	pass2(tdbb, csb);
}

RecordSource* RelationSourceNode::compile(thread_db* tdbb, OptimizerBlk* opt, bool /*innerSubStream*/)
{
	// we have found a base relation; record its stream
	// number in the streams array as a candidate for
	// merging into a river

	opt->beds.add(stream);
	opt->compileStreams.add(stream);

	if (opt->rse->rse_jointype == blr_left)
		opt->outerStreams.add(stream);

	// if we have seen any booleans or sort fields, we may be able to
	// use an index to optimize them; retrieve the current format of
	// all indices at this time so we can determine if it's possible

	if (opt->opt_conjuncts.getCount() || opt->rse->rse_sorted || opt->rse->rse_aggregate)
	{
		if (relation && !relation->rel_file && !relation->isVirtual())
		{
			opt->opt_csb->csb_rpt[stream].csb_indices =
				BTR_all(tdbb, relation, &opt->opt_csb->csb_rpt[stream].csb_idx, relation->getPages(tdbb));
			sortIndicesBySelectivity(&opt->opt_csb->csb_rpt[stream]);
			markIndices(&opt->opt_csb->csb_rpt[stream], relation->rel_id);
		}
		else
			opt->opt_csb->csb_rpt[stream].csb_indices = 0;

		const Format* format = CMP_format(tdbb, opt->opt_csb, stream);
		opt->opt_csb->csb_rpt[stream].csb_cardinality =
			OPT_getRelationCardinality(tdbb, relation, format);
	}

	return NULL;
}


//--------------------


// Parse an procedural view reference.
ProcedureSourceNode* ProcedureSourceNode::parse(thread_db* tdbb, CompilerScratch* csb,
	const SSHORT blrOp)
{
	SET_TDBB(tdbb);

	jrd_prc* procedure = NULL;
	string* aliasString = NULL;
	QualifiedName name;

	switch (blrOp)
	{
		case blr_pid:
		case blr_pid2:
		{
			const SSHORT pid = csb->csb_blr_reader.getWord();

			if (blrOp == blr_pid2)
			{
				aliasString = FB_NEW(csb->csb_pool) string(csb->csb_pool);
				PAR_name(csb, *aliasString);
			}

			if (!(procedure = MET_lookup_procedure_id(tdbb, pid, false, false, 0)))
				name.identifier.printf("id %d", pid);

			break;
		}

		case blr_procedure:
		case blr_procedure2:
		case blr_procedure3:
		case blr_procedure4:
		case blr_subproc:
			if (blrOp == blr_procedure3 || blrOp == blr_procedure4)
				PAR_name(csb, name.package);

			PAR_name(csb, name.identifier);

			if (blrOp == blr_procedure2 || blrOp == blr_procedure4 || blrOp == blr_subproc)
			{
				aliasString = FB_NEW(csb->csb_pool) string(csb->csb_pool);
				PAR_name(csb, *aliasString);

				if (blrOp == blr_subproc && aliasString->isEmpty())
				{
					delete aliasString;
					aliasString = NULL;
				}
			}

			if (blrOp == blr_subproc)
			{
				DeclareSubProcNode* node;
				if (csb->subProcedures.get(name.identifier, node))
					procedure = node->routine;
			}
			else
				procedure = MET_lookup_procedure(tdbb, name, false);

			break;

		default:
			fb_assert(false);
	}

	if (!procedure)
		PAR_error(csb, Arg::Gds(isc_prcnotdef) << Arg::Str(name.toString()));

	if (procedure->prc_type == prc_executable)
	{
		const string name = procedure->getName().toString();

		if (tdbb->getAttachment()->att_flags & ATT_gbak_attachment)
			PAR_warning(Arg::Warning(isc_illegal_prc_type) << Arg::Str(name));
		else
			PAR_error(csb, Arg::Gds(isc_illegal_prc_type) << Arg::Str(name));
	}

	ProcedureSourceNode* node = FB_NEW(*tdbb->getDefaultPool()) ProcedureSourceNode(
		*tdbb->getDefaultPool());

	node->procedure = procedure;
	node->stream = PAR_context(csb, &node->context);

	csb->csb_rpt[node->stream].csb_procedure = procedure;
	csb->csb_rpt[node->stream].csb_alias = aliasString;

	PAR_procedure_parms(tdbb, csb, procedure, node->in_msg.getAddress(),
		node->sourceList.getAddress(), node->targetList.getAddress(), true);

	if (csb->csb_g_flags & csb_get_dependencies)
		PAR_dependency(tdbb, csb, node->stream, (SSHORT) -1, "");

	return node;
}

RecordSourceNode* ProcedureSourceNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	return dsqlPassRelProc(dsqlScratch, this);
}

bool ProcedureSourceNode::dsqlAggregateFinder(AggregateFinder& visitor)
{
	// Check if relation is a procedure.
	if (dsqlContext->ctx_procedure)
	{
		// Check if an aggregate is buried inside the input parameters.
		return visitor.visit(dsqlContext->ctx_proc_inputs);
	}

	return false;
}

bool ProcedureSourceNode::dsqlAggregate2Finder(Aggregate2Finder& visitor)
{
	if (dsqlContext->ctx_procedure)
		return visitor.visit(dsqlContext->ctx_proc_inputs);

	return false;
}

bool ProcedureSourceNode::dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor)
{
	// If relation is a procedure, check if the parameters are valid.
	if (dsqlContext->ctx_procedure)
		return visitor.visit(dsqlContext->ctx_proc_inputs);

	return false;
}

bool ProcedureSourceNode::dsqlSubSelectFinder(SubSelectFinder& /*visitor*/)
{
	return false;
}

bool ProcedureSourceNode::dsqlFieldFinder(FieldFinder& /*visitor*/)
{
	return false;
}

RecordSourceNode* ProcedureSourceNode::dsqlFieldRemapper(FieldRemapper& visitor)
{
	// Check if relation is a procedure.
	if (dsqlContext->ctx_procedure)
		doDsqlFieldRemapper(visitor, dsqlContext->ctx_proc_inputs);	// Remap the input parameters.

	return this;
}

bool ProcedureSourceNode::dsqlMatch(const ExprNode* other, bool /*ignoreMapCast*/) const
{
	const ProcedureSourceNode* o = other->as<ProcedureSourceNode>();
	return o && dsqlContext == o->dsqlContext;
}

// Generate blr for a procedure reference.
void ProcedureSourceNode::genBlr(DsqlCompilerScratch* dsqlScratch)
{
	const dsql_prc* procedure = dsqlContext->ctx_procedure;

	if (procedure->prc_flags & PRC_subproc)
	{
		dsqlScratch->appendUChar(blr_subproc);
		dsqlScratch->appendMetaString(procedure->prc_name.identifier.c_str());
		dsqlScratch->appendMetaString(dsqlContext->ctx_alias.c_str());
	}
	else
	{
		// If this is a trigger or procedure, don't want procedure id used.

		if (DDL_ids(dsqlScratch))
		{
			dsqlScratch->appendUChar(dsqlContext->ctx_alias.hasData() ? blr_pid2 : blr_pid);
			dsqlScratch->appendUShort(procedure->prc_id);
		}
		else
		{
			if (procedure->prc_name.package.hasData())
			{
				dsqlScratch->appendUChar(dsqlContext->ctx_alias.hasData() ? blr_procedure4 : blr_procedure3);
				dsqlScratch->appendMetaString(procedure->prc_name.package.c_str());
				dsqlScratch->appendMetaString(procedure->prc_name.identifier.c_str());
			}
			else
			{
				dsqlScratch->appendUChar(dsqlContext->ctx_alias.hasData() ? blr_procedure2 : blr_procedure);
				dsqlScratch->appendMetaString(procedure->prc_name.identifier.c_str());
			}
		}

		if (dsqlContext->ctx_alias.hasData())
			dsqlScratch->appendMetaString(dsqlContext->ctx_alias.c_str());
	}

	GEN_stuff_context(dsqlScratch, dsqlContext);

	ValueListNode* inputs = dsqlContext->ctx_proc_inputs;

	if (inputs)
	{
		dsqlScratch->appendUShort(inputs->items.getCount());

		for (NestConst<ValueExprNode>* ptr = inputs->items.begin();
			 ptr != inputs->items.end();
			 ++ptr)
		{
			GEN_expr(dsqlScratch, *ptr);
		}
	}
	else
		dsqlScratch->appendUShort(0);
}

ProcedureSourceNode* ProcedureSourceNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	if (!copier.remap)
		BUGCHECK(221);	// msg 221 (CMP) copy: cannot remap

	ProcedureSourceNode* newSource = FB_NEW(*tdbb->getDefaultPool()) ProcedureSourceNode(
		*tdbb->getDefaultPool());

	// dimitr: See the appropriate code and comment in NodeCopier (in nod_argument).
	// We must copy the message first and only then use the new pointer to
	// copy the inputs properly.
	newSource->in_msg = copier.copy(tdbb, in_msg);

	{	// scope
		AutoSetRestore<MessageNode*> autoMessage(&copier.message, newSource->in_msg);
		newSource->sourceList = copier.copy(tdbb, sourceList);
		newSource->targetList = copier.copy(tdbb, targetList);
	}

	newSource->stream = copier.csb->nextStream();
	copier.remap[stream] = newSource->stream;
	newSource->context = context;
	newSource->procedure = procedure;
	newSource->view = view;
	CompilerScratch::csb_repeat* element = CMP_csb_element(copier.csb, newSource->stream);
	element->csb_procedure = newSource->procedure;
	element->csb_view = newSource->view;
	element->csb_view_stream = copier.remap[0];

	copier.csb->csb_rpt[newSource->stream].csb_flags |= copier.csb->csb_rpt[stream].csb_flags & csb_no_dbkey;

	return newSource;
}

RecordSourceNode* ProcedureSourceNode::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	doPass1(tdbb, csb, sourceList.getAddress());
	doPass1(tdbb, csb, targetList.getAddress());
	doPass1(tdbb, csb, in_msg.getAddress());
	return this;
}

void ProcedureSourceNode::pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* /*rse*/,
	BoolExprNode** /*boolean*/, RecordSourceNodeStack& stack)
{
	stack.push(this);	// Assume that the source will be used. Push it on the final stream stack.

	pass1(tdbb, csb);

	if (!procedure->isSubRoutine())
	{
		CMP_post_procedure_access(tdbb, csb, procedure);
		CMP_post_resource(&csb->csb_resources, procedure, Resource::rsc_procedure, procedure->getId());
	}

	jrd_rel* const parentView = csb->csb_view;
	const StreamType viewStream = csb->csb_view_stream;
	view = parentView;

	CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, stream);
	element->csb_view = parentView;
	fb_assert(viewStream <= MAX_STREAMS);
	element->csb_view_stream = viewStream;

	// in the case where there is a parent view, find the context name

	if (parentView)
	{
		const ViewContexts& ctx = parentView->rel_view_contexts;
		const USHORT key = context;
		size_t pos;

		if (ctx.find(key, pos))
		{
			element->csb_alias = FB_NEW(csb->csb_pool) string(
				csb->csb_pool, ctx[pos]->vcx_context_name);
		}
	}
}

RecordSourceNode* ProcedureSourceNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	ExprNode::doPass2(tdbb, csb, sourceList.getAddress());
	ExprNode::doPass2(tdbb, csb, targetList.getAddress());
	ExprNode::doPass2(tdbb, csb, in_msg.getAddress());
	return this;
}

void ProcedureSourceNode::pass2Rse(thread_db* tdbb, CompilerScratch* csb)
{
	fb_assert(stream <= MAX_STREAMS);
	csb->csb_rpt[stream].activate();

	pass2(tdbb, csb);
}

RecordSource* ProcedureSourceNode::compile(thread_db* tdbb, OptimizerBlk* opt, bool /*innerSubStream*/)
{
	opt->beds.add(stream);
	opt->localStreams.add(stream);

	return generate(tdbb, opt);
}

// Compile and optimize a record selection expression into a set of record source blocks (rsb's).
ProcedureScan* ProcedureSourceNode::generate(thread_db* tdbb, OptimizerBlk* opt)
{
	SET_TDBB(tdbb);

	CompilerScratch* const csb = opt->opt_csb;
	CompilerScratch::csb_repeat* const csbTail = &csb->csb_rpt[stream];
	const string alias = OPT_make_alias(tdbb, csb, csbTail);

	return FB_NEW(*tdbb->getDefaultPool()) ProcedureScan(csb, alias, stream, procedure,
		sourceList, targetList, in_msg);
}

bool ProcedureSourceNode::computable(CompilerScratch* csb, StreamType stream,
	bool allowOnlyCurrentStream, ValueExprNode* /*value*/)
{
	if (sourceList && !sourceList->computable(csb, stream, allowOnlyCurrentStream))
		return false;

	if (targetList && !targetList->computable(csb, stream, allowOnlyCurrentStream))
		return false;

	return true;
}

void ProcedureSourceNode::findDependentFromStreams(const OptimizerRetrieval* optRet,
	SortedStreamList* streamList)
{
	if (sourceList)
		sourceList->findDependentFromStreams(optRet, streamList);

	if (targetList)
		targetList->findDependentFromStreams(optRet, streamList);
}

void ProcedureSourceNode::collectStreams(SortedStreamList& streamList) const
{
	RecordSourceNode::collectStreams(streamList);

	if (sourceList)
		sourceList->collectStreams(streamList);

	if (targetList)
		targetList->collectStreams(streamList);
}


//--------------------


// Parse an aggregate reference.
AggregateSourceNode* AggregateSourceNode::parse(thread_db* tdbb, CompilerScratch* csb)
{
	SET_TDBB(tdbb);

	AggregateSourceNode* node = FB_NEW(*tdbb->getDefaultPool()) AggregateSourceNode(
		*tdbb->getDefaultPool());

	node->stream = PAR_context(csb, NULL);
	fb_assert(node->stream <= MAX_STREAMS);
	node->rse = PAR_rse(tdbb, csb);
	node->group = PAR_sort(tdbb, csb, blr_group_by, true);
	node->map = parseMap(tdbb, csb, node->stream);

	return node;
}

bool AggregateSourceNode::dsqlAggregateFinder(AggregateFinder& visitor)
{
	return !visitor.ignoreSubSelects && visitor.visit(dsqlRse);
}

bool AggregateSourceNode::dsqlAggregate2Finder(Aggregate2Finder& visitor)
{
	// Pass only dsqlGroup.
	return visitor.visit(dsqlGroup);
}

bool AggregateSourceNode::dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor)
{
	return visitor.visit(dsqlRse);
}

bool AggregateSourceNode::dsqlSubSelectFinder(SubSelectFinder& /*visitor*/)
{
	return false;
}

bool AggregateSourceNode::dsqlFieldFinder(FieldFinder& visitor)
{
	// Pass only dsqlGroup.
	return visitor.visit(dsqlGroup);
}

RecordSourceNode* AggregateSourceNode::dsqlFieldRemapper(FieldRemapper& visitor)
{
	doDsqlFieldRemapper(visitor, dsqlRse);
	return this;
}

bool AggregateSourceNode::dsqlMatch(const ExprNode* other, bool ignoreMapCast) const
{
	const AggregateSourceNode* o = other->as<AggregateSourceNode>();

	return o && dsqlContext == o->dsqlContext &&
		PASS1_node_match(dsqlGroup, o->dsqlGroup, ignoreMapCast) &&
		PASS1_node_match(dsqlRse, o->dsqlRse, ignoreMapCast);
}

void AggregateSourceNode::genBlr(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->appendUChar((dsqlWindow ? blr_window : blr_aggregate));

	if (!dsqlWindow)
		GEN_stuff_context(dsqlScratch, dsqlContext);

	GEN_rse(dsqlScratch, dsqlRse);

	// Handle PARTITION BY and GROUP BY clause

	if (dsqlWindow)
	{
		fb_assert(dsqlContext->ctx_win_maps.hasData());
		dsqlScratch->appendUChar(dsqlContext->ctx_win_maps.getCount());	// number of windows

		for (Array<PartitionMap*>::iterator i = dsqlContext->ctx_win_maps.begin();
			 i != dsqlContext->ctx_win_maps.end();
			 ++i)
		{
			dsqlScratch->appendUChar(blr_partition_by);
			ValueListNode* partition = (*i)->partition;
			ValueListNode* partitionRemapped = (*i)->partitionRemapped;
			ValueListNode* order = (*i)->order;

			dsqlScratch->appendUChar((*i)->context);

			if (partition)
			{
				dsqlScratch->appendUChar(partition->items.getCount());	// partition by expression count

				NestConst<ValueExprNode>* ptr = partition->items.begin();
				for (const NestConst<ValueExprNode>* end = partition->items.end(); ptr != end; ++ptr)
					GEN_expr(dsqlScratch, *ptr);

				ptr = partitionRemapped->items.begin();
				for (const NestConst<ValueExprNode>* end = partitionRemapped->items.end(); ptr != end; ++ptr)
					GEN_expr(dsqlScratch, *ptr);
			}
			else
				dsqlScratch->appendUChar(0);	// partition by expression count

			if (order)
				GEN_sort(dsqlScratch, order);
			else
			{
				dsqlScratch->appendUChar(blr_sort);
				dsqlScratch->appendUChar(0);
			}

			genMap(dsqlScratch, (*i)->map);
		}
	}
	else
	{
		dsqlScratch->appendUChar(blr_group_by);

		ValueListNode* list = dsqlGroup;

		if (list)
		{
			dsqlScratch->appendUChar(list->items.getCount());
			NestConst<ValueExprNode>* ptr = list->items.begin();

			for (const NestConst<ValueExprNode>* end = list->items.end(); ptr != end; ++ptr)
				(*ptr)->genBlr(dsqlScratch);
		}
		else
			dsqlScratch->appendUChar(0);

		genMap(dsqlScratch, dsqlContext->ctx_map);
	}
}

// Generate a value map for a record selection expression.
void AggregateSourceNode::genMap(DsqlCompilerScratch* dsqlScratch, dsql_map* map)
{
	USHORT count = 0;

	for (dsql_map* temp = map; temp; temp = temp->map_next)
		++count;

	//if (count >= STREAM_MAP_LENGTH) // not sure if the same limit applies
	//	ERR_post(Arg::Gds(isc_too_many_contexts)); // maybe there's better msg.

	dsqlScratch->appendUChar(blr_map);
	dsqlScratch->appendUShort(count);

	for (dsql_map* temp = map; temp; temp = temp->map_next)
	{
		dsqlScratch->appendUShort(temp->map_position);
		GEN_expr(dsqlScratch, temp->map_node);
	}
}

AggregateSourceNode* AggregateSourceNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	if (!copier.remap)
		BUGCHECK(221);	// msg 221 (CMP) copy: cannot remap

	AggregateSourceNode* newSource = FB_NEW(*tdbb->getDefaultPool()) AggregateSourceNode(
		*tdbb->getDefaultPool());

	fb_assert(stream <= MAX_STREAMS);
	newSource->stream = copier.csb->nextStream();
	// fb_assert(newSource->stream <= MAX_UCHAR);
	copier.remap[stream] = newSource->stream;
	CMP_csb_element(copier.csb, newSource->stream);

	copier.csb->csb_rpt[newSource->stream].csb_flags |=
		copier.csb->csb_rpt[stream].csb_flags & csb_no_dbkey;

	newSource->rse = rse->copy(tdbb, copier);
	if (group)
		newSource->group = group->copy(tdbb, copier);
	newSource->map = map->copy(tdbb, copier);

	return newSource;
}

void AggregateSourceNode::ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const
{
	rse->ignoreDbKey(tdbb, csb);
}

RecordSourceNode* AggregateSourceNode::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	fb_assert(stream <= MAX_STREAMS);
	csb->csb_rpt[stream].csb_flags |= csb_no_dbkey;
	rse->ignoreDbKey(tdbb, csb);

	doPass1(tdbb, csb, rse.getAddress());
	doPass1(tdbb, csb, map.getAddress());
	doPass1(tdbb, csb, group.getAddress());

	return this;
}

void AggregateSourceNode::pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* /*rse*/,
	BoolExprNode** /*boolean*/, RecordSourceNodeStack& stack)
{
	stack.push(this);	// Assume that the source will be used. Push it on the final stream stack.

	fb_assert(stream <= MAX_STREAMS);
	pass1(tdbb, csb);

	jrd_rel* const parentView = csb->csb_view;
	const StreamType viewStream = csb->csb_view_stream;

	CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, stream);
	element->csb_view = parentView;
	fb_assert(viewStream <= MAX_STREAMS);
	element->csb_view_stream = viewStream;

}

RecordSourceNode* AggregateSourceNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	rse->pass2Rse(tdbb, csb);
	ExprNode::doPass2(tdbb, csb, map.getAddress());
	ExprNode::doPass2(tdbb, csb, group.getAddress());

	fb_assert(stream <= MAX_STREAMS);

	processMap(tdbb, csb, map, &csb->csb_rpt[stream].csb_internal_format);
	csb->csb_rpt[stream].csb_format = csb->csb_rpt[stream].csb_internal_format;

	return this;
}

void AggregateSourceNode::pass2Rse(thread_db* tdbb, CompilerScratch* csb)
{
	fb_assert(stream <= MAX_STREAMS);
	csb->csb_rpt[stream].activate();

	pass2(tdbb, csb);
}

bool AggregateSourceNode::containsStream(StreamType checkStream) const
{
	// for aggregates, check current RseNode, if not found then check
	// the sub-rse

	if (checkStream == stream)
		return true;		// do not mark as variant

	if (rse->containsStream(checkStream))
		return true;		// do not mark as variant

	return false;
}

RecordSource* AggregateSourceNode::compile(thread_db* tdbb, OptimizerBlk* opt, bool /*innerSubStream*/)
{
	opt->beds.add(stream);

	BoolExprNodeStack conjunctStack;
	for (USHORT i = 0; i < opt->opt_conjuncts.getCount(); i++)
		conjunctStack.push(opt->opt_conjuncts[i].opt_conjunct_node);

	RecordSource* const rsb = generate(tdbb, opt, &conjunctStack, stream);

	opt->localStreams.add(stream);

	return rsb;
}

// Generate a RecordSource (Record Source Block) for each aggregate operation.
// Generate an AggregateSort (Aggregate SortedStream Block) for each DISTINCT aggregate.
RecordSource* AggregateSourceNode::generate(thread_db* tdbb, OptimizerBlk* opt,
	BoolExprNodeStack* parentStack, StreamType shellStream)
{
	SET_TDBB(tdbb);

	CompilerScratch* const csb = opt->opt_csb;
	rse->rse_sorted = group;

	// AB: Try to distribute items from the HAVING CLAUSE to the WHERE CLAUSE.
	// Zip thru stack of booleans looking for fields that belong to shellStream.
	// Those fields are mappings. Mappings that hold a plain field may be used
	// to distribute. Handle the simple cases only.
	BoolExprNodeStack deliverStack;
	genDeliverUnmapped(tdbb, &deliverStack, map, parentStack, shellStream);

	// try to optimize MAX and MIN to use an index; for now, optimize
	// only the simplest case, although it is probably possible
	// to use an index in more complex situations
	NestConst<ValueExprNode>* ptr;
	AggNode* aggNode = NULL;

	if (map->sourceList.getCount() == 1 && (ptr = map->sourceList.begin()) &&
		(aggNode = (*ptr)->as<AggNode>()) &&
		(aggNode->aggInfo.blr == blr_agg_min || aggNode->aggInfo.blr == blr_agg_max))
	{
		// generate a sort block which the optimizer will try to map to an index

		SortNode* aggregate = rse->rse_aggregate =
			FB_NEW(*tdbb->getDefaultPool()) SortNode(*tdbb->getDefaultPool());

		aggregate->expressions.add(aggNode->arg);
		// in the max case, flag the sort as descending
		aggregate->descending.add(aggNode->aggInfo.blr == blr_agg_max);
		// 10-Aug-2004. Nickolay Samofatov - Unneeded nulls seem to be skipped somehow.
		aggregate->nullOrder.add(rse_nulls_default);
	}

	RecordSource* const nextRsb = OPT_compile(tdbb, csb, rse, &deliverStack);

	// allocate and optimize the record source block

	AggregatedStream* const rsb = FB_NEW(*tdbb->getDefaultPool()) AggregatedStream(tdbb, csb,
		stream, (group ? &group->expressions : NULL), map, nextRsb);

	if (rse->rse_aggregate)
	{
		// The rse_aggregate is still set. That means the optimizer
		// was able to match the field to an index, so flag that fact
		// so that it can be handled in EVL_group
		aggNode->indexed = true;
	}

	OPT_gen_aggregate_distincts(tdbb, csb, map);

	return rsb;
}

bool AggregateSourceNode::computable(CompilerScratch* csb, StreamType stream,
	bool allowOnlyCurrentStream, ValueExprNode* /*value*/)
{
	rse->rse_sorted = group;
	return rse->computable(csb, stream, allowOnlyCurrentStream, NULL);
}

void AggregateSourceNode::findDependentFromStreams(const OptimizerRetrieval* optRet,
	SortedStreamList* streamList)
{
	rse->rse_sorted = group;
	rse->findDependentFromStreams(optRet, streamList);
}


//--------------------


// Parse a union reference.
UnionSourceNode* UnionSourceNode::parse(thread_db* tdbb, CompilerScratch* csb, const SSHORT blrOp)
{
	SET_TDBB(tdbb);

	// Make the node, parse the context number, get a stream assigned,
	// and get the number of sub-RseNode's.

	UnionSourceNode* node = FB_NEW(*tdbb->getDefaultPool()) UnionSourceNode(
		*tdbb->getDefaultPool());

	node->recursive = blrOp == blr_recurse;

	node->stream = PAR_context(csb, NULL);
	fb_assert(node->stream <= MAX_STREAMS);

	// assign separate context for mapped record if union is recursive
	StreamType stream2 = node->stream;

	if (node->recursive)
	{
		stream2 = PAR_context(csb, 0);
		node->mapStream = stream2;
	}

	// bottleneck
	int count = (unsigned int) csb->csb_blr_reader.getByte();

	// Pick up the sub-RseNode's and maps.

	while (--count >= 0)
	{
		node->clauses.push(PAR_rse(tdbb, csb));
		node->maps.push(parseMap(tdbb, csb, stream2));
	}

	return node;
}

void UnionSourceNode::genBlr(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->appendUChar((recursive ? blr_recurse : blr_union));

	// Obtain the context for UNION from the first dsql_map* node.
	ValueExprNode* mapItem = dsqlParentRse->dsqlSelectList->items[0];

	// AB: First item could be a virtual field generated by derived table.
	DerivedFieldNode* derivedField = mapItem->as<DerivedFieldNode>();

	if (derivedField)
		mapItem = derivedField->value;

	if (mapItem->is<CastNode>())
		mapItem = mapItem->as<CastNode>()->source;

	DsqlMapNode* mapNode = mapItem->as<DsqlMapNode>();
	fb_assert(mapNode);

	if (!mapNode)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_dsql_internal_err) <<
				  Arg::Gds(isc_random) << Arg::Str("UnionSourceNode::genBlr: expected DsqlMapNode") );
	}

	dsql_ctx* dsqlContext = mapNode->context;

	GEN_stuff_context(dsqlScratch, dsqlContext);
	// secondary context number must be present once in generated blr
	dsqlContext->ctx_flags &= ~CTX_recursive;

	RecSourceListNode* streams = dsqlClauses;
	dsqlScratch->appendUChar(streams->items.getCount());	// number of substreams

	NestConst<RecordSourceNode>* ptr = streams->items.begin();
	for (const NestConst<RecordSourceNode>* const end = streams->items.end(); ptr != end; ++ptr)
	{
		RseNode* sub_rse = (*ptr)->as<RseNode>();
		GEN_rse(dsqlScratch, sub_rse);

		ValueListNode* items = sub_rse->dsqlSelectList;

		dsqlScratch->appendUChar(blr_map);
		dsqlScratch->appendUShort(items->items.getCount());

		USHORT count = 0;
		NestConst<ValueExprNode>* iptr = items->items.begin();

		for (const NestConst<ValueExprNode>* const iend = items->items.end(); iptr != iend; ++iptr)
		{
			dsqlScratch->appendUShort(count);
			GEN_expr(dsqlScratch, *iptr);
			++count;
		}
	}
}

UnionSourceNode* UnionSourceNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	if (!copier.remap)
		BUGCHECK(221);		// msg 221 (CMP) copy: cannot remap

	UnionSourceNode* newSource = FB_NEW(*tdbb->getDefaultPool()) UnionSourceNode(
		*tdbb->getDefaultPool());
	newSource->recursive = recursive;

	fb_assert(stream <= MAX_STREAMS);
	newSource->stream = copier.csb->nextStream();
	copier.remap[stream] = newSource->stream;
	CMP_csb_element(copier.csb, newSource->stream);

	StreamType oldStream = stream;
	StreamType newStream = newSource->stream;

	if (newSource->recursive)
	{
		oldStream = mapStream;
		fb_assert(oldStream <= MAX_STREAMS);
		newStream = copier.csb->nextStream();
		newSource->mapStream = newStream;
		copier.remap[oldStream] = newStream;
		CMP_csb_element(copier.csb, newStream);
	}

	copier.csb->csb_rpt[newStream].csb_flags |=
		copier.csb->csb_rpt[oldStream].csb_flags & csb_no_dbkey;

	const NestConst<RseNode>* ptr = clauses.begin();
	const NestConst<MapNode>* ptr2 = maps.begin();

	for (const NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr, ++ptr2)
	{
		newSource->clauses.add((*ptr)->copy(tdbb, copier));
		newSource->maps.add((*ptr2)->copy(tdbb, copier));
	}

	return newSource;
}

void UnionSourceNode::ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const
{
	const NestConst<RseNode>* ptr = clauses.begin();

	for (const NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr)
		(*ptr)->ignoreDbKey(tdbb, csb);
}

void UnionSourceNode::pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* /*rse*/,
	BoolExprNode** /*boolean*/, RecordSourceNodeStack& stack)
{
	stack.push(this);	// Assume that the source will be used. Push it on the final stream stack.

	NestConst<RseNode>* ptr = clauses.begin();
	NestConst<MapNode>* ptr2 = maps.begin();

	for (NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr, ++ptr2)
	{
		doPass1(tdbb, csb, ptr->getAddress());
		doPass1(tdbb, csb, ptr2->getAddress());
	}

	jrd_rel* const parentView = csb->csb_view;
	const StreamType viewStream = csb->csb_view_stream;

	CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, stream);
	element->csb_view = parentView;
	fb_assert(viewStream <= MAX_STREAMS);
	element->csb_view_stream = viewStream;
}

// Process a union clause of a RseNode.
RecordSourceNode* UnionSourceNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	SET_TDBB(tdbb);

	// make up a format block sufficiently large to hold instantiated record

	const StreamType id = getStream();
	Format** format = &csb->csb_rpt[id].csb_internal_format;

	// Process RseNodes and map blocks.

	NestConst<RseNode>* ptr = clauses.begin();
	NestConst<MapNode>* ptr2 = maps.begin();

	for (NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr, ++ptr2)
	{
		(*ptr)->pass2Rse(tdbb, csb);
		ExprNode::doPass2(tdbb, csb, ptr2->getAddress());
		processMap(tdbb, csb, *ptr2, format);
		csb->csb_rpt[id].csb_format = *format;
	}

	if (recursive)
		csb->csb_rpt[mapStream].csb_format = *format;

	return this;
}

void UnionSourceNode::pass2Rse(thread_db* tdbb, CompilerScratch* csb)
{
	fb_assert(stream <= MAX_STREAMS);
	csb->csb_rpt[stream].activate();

	pass2(tdbb, csb);
}

bool UnionSourceNode::containsStream(StreamType checkStream) const
{
	// for unions, check current RseNode, if not found then check
	// all sub-rse's

	if (checkStream == stream)
		return true;		// do not mark as variant

	const NestConst<RseNode>* ptr = clauses.begin();

	for (const NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr)
	{
		if ((*ptr)->containsStream(checkStream))
			return true;
	}

	return false;
}

RecordSource* UnionSourceNode::compile(thread_db* tdbb, OptimizerBlk* opt, bool /*innerSubStream*/)
{
	opt->beds.add(stream);

	const size_t oldCount = opt->keyStreams.getCount();
	computeDbKeyStreams(opt->keyStreams);

	BoolExprNodeStack conjunctStack;
	for (USHORT i = 0; i < opt->opt_conjuncts.getCount(); i++)
		conjunctStack.push(opt->opt_conjuncts[i].opt_conjunct_node);

	RecordSource* const rsb = generate(tdbb, opt, opt->keyStreams.begin() + oldCount,
		opt->keyStreams.getCount() - oldCount, &conjunctStack, stream);

	opt->localStreams.add(stream);

	return rsb;
}

// Generate an union complex.
RecordSource* UnionSourceNode::generate(thread_db* tdbb, OptimizerBlk* opt, const StreamType* streams,
	size_t nstreams, BoolExprNodeStack* parentStack, StreamType shellStream)
{
	SET_TDBB(tdbb);

	CompilerScratch* csb = opt->opt_csb;
	HalfStaticArray<RecordSource*, OPT_STATIC_ITEMS> rsbs;

	const ULONG baseImpure = CMP_impure(csb, 0);

	NestConst<RseNode>* ptr = clauses.begin();
	NestConst<MapNode>* ptr2 = maps.begin();

	for (NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr, ++ptr2)
	{
		RseNode* rse = *ptr;
		MapNode* map = *ptr2;

		// AB: Try to distribute booleans from the top rse for an UNION to
		// the WHERE clause of every single rse.
		// hvlad: don't do it for recursive unions else they will work wrong !
		BoolExprNodeStack deliverStack;
		if (!recursive)
			genDeliverUnmapped(tdbb, &deliverStack, map, parentStack, shellStream);

		rsbs.add(OPT_compile(tdbb, csb, rse, &deliverStack));

		// hvlad: activate recursive union itself after processing first (non-recursive)
		// member to allow recursive members be optimized
		if (recursive)
			csb->csb_rpt[stream].activate();
	}

	if (recursive)
	{
		fb_assert(rsbs.getCount() == 2 && maps.getCount() == 2);
		// hvlad: save size of inner impure area and context of mapped record
		// for recursive processing later
		return FB_NEW(*tdbb->getDefaultPool()) RecursiveStream(csb, stream, mapStream,
			rsbs[0], rsbs[1], maps[0], maps[1], nstreams, streams, baseImpure);
	}

	return FB_NEW(*tdbb->getDefaultPool()) Union(csb, stream, clauses.getCount(), rsbs.begin(),
		maps.begin(), nstreams, streams);
}

// Identify all of the streams for which a dbkey may need to be carried through a sort.
void UnionSourceNode::computeDbKeyStreams(StreamList& streamList) const
{
	const NestConst<RseNode>* ptr = clauses.begin();

	for (const NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr)
		(*ptr)->computeDbKeyStreams(streamList);
}

bool UnionSourceNode::computable(CompilerScratch* csb, StreamType stream,
	bool allowOnlyCurrentStream, ValueExprNode* /*value*/)
{
	NestConst<RseNode>* ptr = clauses.begin();

	for (NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr)
	{
		if (!(*ptr)->computable(csb, stream, allowOnlyCurrentStream, NULL))
			return false;
	}

	return true;
}

void UnionSourceNode::findDependentFromStreams(const OptimizerRetrieval* optRet,
	SortedStreamList* streamList)
{
	NestConst<RseNode>* ptr = clauses.begin();

	for (NestConst<RseNode>* const end = clauses.end(); ptr != end; ++ptr)
		(*ptr)->findDependentFromStreams(optRet, streamList);
}


//--------------------


// Parse a window reference.
WindowSourceNode* WindowSourceNode::parse(thread_db* tdbb, CompilerScratch* csb)
{
	SET_TDBB(tdbb);

	WindowSourceNode* node = FB_NEW(*tdbb->getDefaultPool()) WindowSourceNode(
		*tdbb->getDefaultPool());

	node->rse = PAR_rse(tdbb, csb);

	unsigned partitionCount = csb->csb_blr_reader.getByte();

	for (unsigned i = 0; i < partitionCount; ++i)
		node->parsePartitionBy(tdbb, csb);

	return node;
}

// Parse PARTITION BY subclauses of window functions.
void WindowSourceNode::parsePartitionBy(thread_db* tdbb, CompilerScratch* csb)
{
	SET_TDBB(tdbb);

	if (csb->csb_blr_reader.getByte() != blr_partition_by)
		PAR_syntax_error(csb, "blr_partition_by");

	SSHORT context;
	Partition& partition = partitions.add();
	partition.stream = PAR_context(csb, &context);

	const UCHAR count = csb->csb_blr_reader.getByte();

	if (count != 0)
	{
		partition.group = PAR_sort_internal(tdbb, csb, blr_partition_by, count);
		partition.regroup = PAR_sort_internal(tdbb, csb, blr_partition_by, count);
	}

	partition.order = PAR_sort(tdbb, csb, blr_sort, true);
	partition.map = parseMap(tdbb, csb, partition.stream);
}

WindowSourceNode* WindowSourceNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	if (!copier.remap)
		BUGCHECK(221);		// msg 221 (CMP) copy: cannot remap

	WindowSourceNode* newSource = FB_NEW(*tdbb->getDefaultPool()) WindowSourceNode(
		*tdbb->getDefaultPool());

	newSource->rse = rse->copy(tdbb, copier);

	for (ObjectsArray<Partition>::const_iterator inputPartition = partitions.begin();
		 inputPartition != partitions.end();
		 ++inputPartition)
	{
		fb_assert(inputPartition->stream <= MAX_STREAMS);

		Partition& copyPartition = newSource->partitions.add();

		copyPartition.stream = copier.csb->nextStream();
		// fb_assert(copyPartition.stream <= MAX_UCHAR);

		copier.remap[inputPartition->stream] = copyPartition.stream;
		CMP_csb_element(copier.csb, copyPartition.stream);

		if (inputPartition->group)
			copyPartition.group = inputPartition->group->copy(tdbb, copier);
		if (inputPartition->regroup)
			copyPartition.regroup = inputPartition->regroup->copy(tdbb, copier);
		if (inputPartition->order)
			copyPartition.order = inputPartition->order->copy(tdbb, copier);
		copyPartition.map = inputPartition->map->copy(tdbb, copier);
	}

	return newSource;
}

void WindowSourceNode::ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const
{
	rse->ignoreDbKey(tdbb, csb);
}

RecordSourceNode* WindowSourceNode::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		fb_assert(partition->stream <= MAX_STREAMS);
		csb->csb_rpt[partition->stream].csb_flags |= csb_no_dbkey;
	}

	rse->ignoreDbKey(tdbb, csb);
	doPass1(tdbb, csb, rse.getAddress());

	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		doPass1(tdbb, csb, partition->group.getAddress());
		doPass1(tdbb, csb, partition->regroup.getAddress());
		doPass1(tdbb, csb, partition->order.getAddress());
		doPass1(tdbb, csb, partition->map.getAddress());
	}

	return this;
}

void WindowSourceNode::pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* /*rse*/,
	BoolExprNode** /*boolean*/, RecordSourceNodeStack& stack)
{
	stack.push(this);	// Assume that the source will be used. Push it on the final stream stack.

	pass1(tdbb, csb);

	jrd_rel* const parentView = csb->csb_view;
	const StreamType viewStream = csb->csb_view_stream;
	fb_assert(viewStream <= MAX_STREAMS);

	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, partition->stream);
		element->csb_view = parentView;
		element->csb_view_stream = viewStream;
	}
}

RecordSourceNode* WindowSourceNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	rse->pass2Rse(tdbb, csb);

	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		ExprNode::doPass2(tdbb, csb, partition->map.getAddress());
		ExprNode::doPass2(tdbb, csb, partition->group.getAddress());
		ExprNode::doPass2(tdbb, csb, partition->order.getAddress());

		fb_assert(partition->stream <= MAX_STREAMS);

		processMap(tdbb, csb, partition->map, &csb->csb_rpt[partition->stream].csb_internal_format);
		csb->csb_rpt[partition->stream].csb_format =
			csb->csb_rpt[partition->stream].csb_internal_format;
	}

	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		ExprNode::doPass2(tdbb, csb, partition->regroup.getAddress());
	}

	return this;
}

void WindowSourceNode::pass2Rse(thread_db* tdbb, CompilerScratch* csb)
{
	pass2(tdbb, csb);

	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		csb->csb_rpt[partition->stream].activate();
	}
}

bool WindowSourceNode::containsStream(StreamType checkStream) const
{
	for (ObjectsArray<Partition>::const_iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		if (checkStream == partition->stream)
			return true;		// do not mark as variant
	}

	if (rse->containsStream(checkStream))
		return true;		// do not mark as variant

	return false;
}

void WindowSourceNode::collectStreams(SortedStreamList& streamList) const
{
	for (ObjectsArray<Partition>::const_iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		if (!streamList.exist(partition->stream))
			streamList.add(partition->stream);
	}
}

RecordSource* WindowSourceNode::compile(thread_db* tdbb, OptimizerBlk* opt, bool /*innerSubStream*/)
{
	for (ObjectsArray<Partition>::iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		opt->beds.add(partition->stream);
	}

	RecordSource* const rsb = FB_NEW(*tdbb->getDefaultPool()) WindowedStream(tdbb, opt->opt_csb,
		partitions, OPT_compile(tdbb, opt->opt_csb, rse, NULL));

	StreamList rsbStreams;
	rsb->findUsedStreams(rsbStreams);

	opt->localStreams.join(rsbStreams);

	return rsb;
}

bool WindowSourceNode::computable(CompilerScratch* csb, StreamType stream,
	bool allowOnlyCurrentStream, ValueExprNode* /*value*/)
{
	return rse->computable(csb, stream, allowOnlyCurrentStream, NULL);
}

void WindowSourceNode::computeRseStreams(StreamList& streamList) const
{
	for (ObjectsArray<Partition>::const_iterator partition = partitions.begin();
		 partition != partitions.end();
		 ++partition)
	{
		streamList.add(partition->stream);
	}
}

void WindowSourceNode::findDependentFromStreams(const OptimizerRetrieval* optRet,
	SortedStreamList* streamList)
{
	rse->findDependentFromStreams(optRet, streamList);
}


//--------------------


bool RseNode::dsqlAggregateFinder(AggregateFinder& visitor)
{
	AutoSetRestore<USHORT> autoValidateExpr(&visitor.currentLevel, visitor.currentLevel + 1);
	return visitor.visit(dsqlStreams) | visitor.visit(dsqlWhere) | visitor.visit(dsqlSelectList);
}

bool RseNode::dsqlAggregate2Finder(Aggregate2Finder& visitor)
{
	AutoSetRestore<bool> autoCurrentScopeLevelEqual(&visitor.currentScopeLevelEqual, false);
	// Pass dsqlWhere, dsqlSelectList and dsqlStreams.
	return visitor.visit(dsqlWhere) | visitor.visit(dsqlSelectList) | visitor.visit(dsqlStreams);
}

bool RseNode::dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor)
{
	return (flags & FLAG_DSQL_COMPARATIVE) && RecordSourceNode::dsqlInvalidReferenceFinder(visitor);
}

bool RseNode::dsqlSubSelectFinder(SubSelectFinder& visitor)
{
	return !(flags & FLAG_DSQL_COMPARATIVE) || RecordSourceNode::dsqlSubSelectFinder(visitor);
}

bool RseNode::dsqlFieldFinder(FieldFinder& visitor)
{
	// Pass dsqlWhere and dsqlSelectList
	return visitor.visit(dsqlWhere) | visitor.visit(dsqlSelectList);
}

RseNode* RseNode::dsqlFieldRemapper(FieldRemapper& visitor)
{
	AutoSetRestore<USHORT> autoCurrentLevel(&visitor.currentLevel, visitor.currentLevel + 1);

	doDsqlFieldRemapper(visitor, dsqlStreams);
	doDsqlFieldRemapper(visitor, dsqlWhere);
	doDsqlFieldRemapper(visitor, dsqlSelectList);
	doDsqlFieldRemapper(visitor, dsqlOrder);

	return this;
}

bool RseNode::dsqlMatch(const ExprNode* other, bool /*ignoreMapCast*/) const
{
	const RseNode* o = other->as<RseNode>();

	if (!o)
		return false;

	// ASF: Commented-out code "Fixed assertion when subquery is used in group by" to make
	// CORE-4084 work again.
	return /***dsqlContext &&***/ dsqlContext == o->dsqlContext;
}

// Make up join node and mark relations as "possibly NULL" if they are in outer joins (inOuterJoin).
RseNode* RseNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	// Set up a new (empty) context to process the joins, but ensure
	// that it includes system (e.g. trigger) contexts (if present),
	// as well as any outer (from other levels) contexts.

	DsqlContextStack* const base_context = dsqlScratch->context;
	DsqlContextStack temp;
	dsqlScratch->context = &temp;

	for (DsqlContextStack::iterator iter(*base_context); iter.hasData(); ++iter)
	{
		if ((iter.object()->ctx_flags & CTX_system) ||
			iter.object()->ctx_scope_level != dsqlScratch->scopeLevel ||
			iter.object() == dsqlScratch->recursiveCtx)	// CORE-4322, CORE-4354
		{
			temp.push(iter.object());
		}
	}

	const size_t visibleContexts = temp.getCount();

	RecSourceListNode* fromList = dsqlFrom;
	RecSourceListNode* streamList = FB_NEW(getPool()) RecSourceListNode(
		getPool(), fromList->items.getCount());

	RseNode* node = FB_NEW(getPool()) RseNode(getPool());
	node->dsqlExplicitJoin = dsqlExplicitJoin;
	node->rse_jointype = rse_jointype;
	node->dsqlStreams = streamList;

	switch (rse_jointype)
	{
		case blr_inner:
			streamList->items[0] = doDsqlPass(dsqlScratch, fromList->items[0]);
			streamList->items[1] = doDsqlPass(dsqlScratch, fromList->items[1]);
			break;

		case blr_left:
			streamList->items[0] = doDsqlPass(dsqlScratch, fromList->items[0]);
			++dsqlScratch->inOuterJoin;
			streamList->items[1] = doDsqlPass(dsqlScratch, fromList->items[1]);
			--dsqlScratch->inOuterJoin;
			break;

		case blr_right:
			++dsqlScratch->inOuterJoin;
			streamList->items[0] = doDsqlPass(dsqlScratch, fromList->items[0]);
			--dsqlScratch->inOuterJoin;
			streamList->items[1] = doDsqlPass(dsqlScratch, fromList->items[1]);
			break;

		case blr_full:
			++dsqlScratch->inOuterJoin;
			streamList->items[0] = doDsqlPass(dsqlScratch, fromList->items[0]);
			streamList->items[1] = doDsqlPass(dsqlScratch, fromList->items[1]);
			--dsqlScratch->inOuterJoin;
			break;

		default:
			fb_assert(false);
			break;
	}

	NestConst<BoolExprNode> boolean = dsqlWhere;
	ValueListNode* usingList = dsqlJoinUsing;

	if (usingList)
	{
		if (dsqlScratch->clientDialect < SQL_DIALECT_V6)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_dsql_unsupp_feature_dialect) << Arg::Num(dsqlScratch->clientDialect));
		}

		ValueListNode leftStack(dsqlScratch->getPool(), 0u);
		ValueListNode rightStack(dsqlScratch->getPool(), 0u);

		if (usingList->items.isEmpty())	// NATURAL JOIN
		{
			StrArray leftNames(dsqlScratch->getPool());
			ValueListNode* matched = FB_NEW(getPool()) ValueListNode(getPool(), 0u);

			PASS1_expand_select_node(dsqlScratch, streamList->items[0], &leftStack, true);
			PASS1_expand_select_node(dsqlScratch, streamList->items[1], &rightStack, true);

			// verify columns that exist in both sides
			for (int i = 0; i < 2; ++i)
			{
				ValueListNode& currentStack = i == 0 ? leftStack : rightStack;

				for (NestConst<ValueExprNode>* j = currentStack.items.begin();
					 j != currentStack.items.end();
					 ++j)
				{
					const TEXT* name = NULL;
					ValueExprNode* item = *j;
					DsqlAliasNode* aliasNode;
					FieldNode* fieldNode;
					DerivedFieldNode* derivedField;

					if ((aliasNode = item->as<DsqlAliasNode>()))
						name = aliasNode->name.c_str();
					else if ((fieldNode = item->as<FieldNode>()))
						name = fieldNode->dsqlField->fld_name.c_str();
					else if ((derivedField = item->as<DerivedFieldNode>()))
						name = derivedField->name.c_str();

					if (name)
					{
						if (i == 0)	// left
							leftNames.add(name);
						else	// right
						{
							if (leftNames.exist(name))
								matched->add(MAKE_field_name(name));
						}
					}
				}
			}

			if (matched->items.isEmpty())
			{
				// There is no match. Transform to CROSS JOIN.
				node->rse_jointype = blr_inner;
				usingList = NULL;

				delete matched;
			}
			else
				usingList = matched;	// Transform to USING
		}

		if (usingList)	// JOIN ... USING
		{
			BoolExprNode* newBoolean = NULL;
			StrArray usedColumns(dsqlScratch->getPool());

			for (size_t i = 0; i < usingList->items.getCount(); ++i)
			{
				const FieldNode* field = usingList->items[i]->as<FieldNode>();

				// verify if the column was already used
				size_t pos;
				if (usedColumns.find(field->dsqlName.c_str(), pos))
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_col_more_than_once_using) << field->dsqlName);
				}
				else
					usedColumns.insert(pos, field->dsqlName.c_str());

				dsql_ctx* leftCtx = NULL;
				dsql_ctx* rightCtx = NULL;

				// clear the stacks for the next pass
				leftStack.clear();
				rightStack.clear();

				// get the column names from both sides
				PASS1_expand_select_node(dsqlScratch, streamList->items[0], &leftStack, true);
				PASS1_expand_select_node(dsqlScratch, streamList->items[1], &rightStack, true);

				// create the boolean

				ValueExprNode* arg1 = resolveUsingField(dsqlScratch, field->dsqlName, &leftStack,
					field, "left", leftCtx);
				ValueExprNode* arg2 = resolveUsingField(dsqlScratch, field->dsqlName, &rightStack,
					field, "right", rightCtx);

				ComparativeBoolNode* eqlNode = FB_NEW(getPool()) ComparativeBoolNode(getPool(),
					blr_eql, arg1, arg2);

				fb_assert(leftCtx);
				fb_assert(rightCtx);

				// We should hide the (unqualified) column in one side
				ImplicitJoin* impJoinLeft;
				if (!leftCtx->ctx_imp_join.get(field->dsqlName, impJoinLeft))
				{
					impJoinLeft = FB_NEW(dsqlScratch->getPool()) ImplicitJoin();
					impJoinLeft->value = eqlNode->arg1;
					impJoinLeft->visibleInContext = leftCtx;
				}
				else
					fb_assert(impJoinLeft->visibleInContext == leftCtx);

				ImplicitJoin* impJoinRight;
				if (!rightCtx->ctx_imp_join.get(field->dsqlName, impJoinRight))
				{
					impJoinRight = FB_NEW(dsqlScratch->getPool()) ImplicitJoin();
					impJoinRight->value = arg2;
				}
				else
					fb_assert(impJoinRight->visibleInContext == rightCtx);

				// create the COALESCE
				ValueListNode* stack = FB_NEW(getPool()) ValueListNode(getPool(), 0u);

				NestConst<ValueExprNode> tempNode = impJoinLeft->value;
				NestConst<DsqlAliasNode> aliasNode = tempNode->as<DsqlAliasNode>();
				NestConst<CoalesceNode> coalesceNode;

				if (aliasNode)
					tempNode = aliasNode->value;

				{	// scope
					PsqlChanger changer(dsqlScratch, false);

					if ((coalesceNode = tempNode->as<CoalesceNode>()))
					{
						ValueListNode* list = coalesceNode->args;

						for (NestConst<ValueExprNode>* ptr = list->items.begin();
							 ptr != list->items.end();
							 ++ptr)
						{
							stack->add(doDsqlPass(dsqlScratch, *ptr));
						}
					}
					else
						stack->add(doDsqlPass(dsqlScratch, tempNode));

					tempNode = impJoinRight->value;

					if ((aliasNode = tempNode->as<DsqlAliasNode>()))
						tempNode = aliasNode->value;

					if ((coalesceNode = tempNode->as<CoalesceNode>()))
					{
						ValueListNode* list = coalesceNode->args;

						for (NestConst<ValueExprNode>* ptr = list->items.begin();
							 ptr != list->items.end();
							 ++ptr)
						{
							stack->add(doDsqlPass(dsqlScratch, *ptr));
						}
					}
					else
						stack->add(doDsqlPass(dsqlScratch, tempNode));
				}

				coalesceNode = FB_NEW(getPool()) CoalesceNode(getPool(), stack);

				aliasNode = FB_NEW(getPool()) DsqlAliasNode(getPool(), field->dsqlName, coalesceNode);
				aliasNode->implicitJoin = impJoinLeft;

				impJoinLeft->value = aliasNode;

				impJoinRight->visibleInContext = NULL;

				// both sides should refer to the same ImplicitJoin
				leftCtx->ctx_imp_join.put(field->dsqlName, impJoinLeft);
				rightCtx->ctx_imp_join.put(field->dsqlName, impJoinLeft);

				newBoolean = PASS1_compose(newBoolean, eqlNode, blr_and);
			}

			boolean = newBoolean;
		}
	}

	node->dsqlWhere = doDsqlPass(dsqlScratch, boolean);

	// Merge the newly created contexts with the original ones

	while (temp.getCount() > visibleContexts)
		base_context->push(temp.pop());

	dsqlScratch->context = base_context;

	return node;
}

RseNode* RseNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	RseNode* newSource = FB_NEW(*tdbb->getDefaultPool()) RseNode(*tdbb->getDefaultPool());

	const NestConst<RecordSourceNode>* ptr = rse_relations.begin();

	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
		newSource->rse_relations.add((*ptr)->copy(tdbb, copier));

	newSource->flags = flags;
	newSource->rse_jointype = rse_jointype;
	newSource->rse_first = copier.copy(tdbb, rse_first);
	newSource->rse_skip = copier.copy(tdbb, rse_skip);

	if (rse_boolean)
		newSource->rse_boolean = copier.copy(tdbb, rse_boolean);

	if (rse_sorted)
		newSource->rse_sorted = rse_sorted->copy(tdbb, copier);

	if (rse_projection)
		newSource->rse_projection = rse_projection->copy(tdbb, copier);

	return newSource;
}

// For each relation or aggregate in the RseNode, mark it as not having a dbkey.
void RseNode::ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const
{
	const NestConst<RecordSourceNode>* ptr = rse_relations.begin();

	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
		(*ptr)->ignoreDbKey(tdbb, csb);
}

// Process a record select expression during pass 1 of compilation.
// Mostly this involves expanding views.
RseNode* RseNode::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	SET_TDBB(tdbb);

	// for scoping purposes, maintain a stack of RseNode's which are
	// currently being parsed; if there are none on the stack as
	// yet, mark the RseNode as variant to make sure that statement-
	// level aggregates are not treated as invariants -- bug #6535

	bool topLevelRse = true;

	for (RseOrExprNode* node = csb->csb_current_nodes.begin();
		 node != csb->csb_current_nodes.end(); ++node)
	{
		if (node->rseNode)
		{
			topLevelRse = false;
			break;
		}
	}

	if (topLevelRse)
		flags |= FLAG_VARIANT;

	csb->csb_current_nodes.push(this);

	RecordSourceNodeStack stack;
	BoolExprNode* boolean = NULL;
	SortNode* sort = rse_sorted;
	SortNode* project = rse_projection;
	ValueExprNode* first = rse_first;
	ValueExprNode* skip = rse_skip;
	PlanNode* plan = rse_plan;

	// zip thru RseNode expanding views and inner joins
	NestConst<RecordSourceNode>* arg = rse_relations.begin();
	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); arg != end; ++arg)
		processSource(tdbb, csb, this, *arg, &boolean, stack);

	// Now, rebuild the RseNode block.

	rse_relations.resize(stack.getCount());
	arg = rse_relations.end();

	while (stack.hasData())
		*--arg = stack.pop();

	AutoSetRestore<bool> autoValidateExpr(&csb->csb_validate_expr, false);

	// finish of by processing other clauses

	if (first)
	{
		doPass1(tdbb, csb, &first);
		rse_first = first;
	}

	if (skip)
	{
		doPass1(tdbb, csb, &skip);
		rse_skip = skip;
	}

	if (boolean)
	{
		if (rse_boolean)
		{
			BinaryBoolNode* andNode = FB_NEW(csb->csb_pool) BinaryBoolNode(csb->csb_pool, blr_and);
			andNode->arg1 = boolean;
			andNode->arg2 = rse_boolean;

			doPass1(tdbb, csb, andNode->arg2.getAddress());

			rse_boolean = andNode;
		}
		else
			rse_boolean = boolean;
	}
	else if (rse_boolean)
		doPass1(tdbb, csb, rse_boolean.getAddress());

	if (sort)
	{
		doPass1(tdbb, csb, &sort);
		rse_sorted = sort;
	}

	if (project)
	{
		doPass1(tdbb, csb, &project);
		rse_projection = project;
	}

	if (plan)
		rse_plan = plan;

	// we are no longer in the scope of this RseNode
	csb->csb_current_nodes.pop();

	return this;
}

void RseNode::pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
	BoolExprNode** boolean, RecordSourceNodeStack& stack)
{
	// in the case of an RseNode, it is possible that a new RseNode will be generated,
	// so wait to process the source before we push it on the stack (bug 8039)

	// The addition of the JOIN syntax for specifying inner joins causes an
	// RseNode tree to be generated, which is undesirable in the simplest case
	// where we are just trying to inner join more than 2 streams. If possible,
	// try to flatten the tree out before we go any further.

	if (!rse->rse_jointype && !rse_jointype && !rse_sorted && !rse_projection &&
		!rse_first && !rse_skip && !rse_plan)
	{
		NestConst<RecordSourceNode>* arg = rse_relations.begin();
		for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); arg != end; ++arg)
			processSource(tdbb, csb, rse, *arg, boolean, stack);

		// fold in the boolean for this inner join with the one for the parent

		if (rse_boolean)
		{
			BoolExprNode* node = rse_boolean;
			doPass1(tdbb, csb, &node);

			if (*boolean)
			{
				BinaryBoolNode* andNode = FB_NEW(csb->csb_pool) BinaryBoolNode(csb->csb_pool, blr_and);
				andNode->arg1 = node;
				andNode->arg2 = *boolean;

				*boolean = andNode;
			}
			else
				*boolean = node;
		}

		return;
	}

	pass1(tdbb, csb);
	stack.push(this);
}

// Perform the first half of record selection expression compilation.
// The actual optimization is done in "post_rse".
void RseNode::pass2Rse(thread_db* tdbb, CompilerScratch* csb)
{
	SET_TDBB(tdbb);

	// Maintain stack of RSEe for scoping purposes
	csb->csb_current_nodes.push(this);

	if (rse_first)
		ExprNode::doPass2(tdbb, csb, rse_first.getAddress());

	if (rse_skip)
	    ExprNode::doPass2(tdbb, csb, rse_skip.getAddress());

	NestConst<RecordSourceNode>* ptr = rse_relations.begin();

	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
		(*ptr)->pass2Rse(tdbb, csb);

	ExprNode::doPass2(tdbb, csb, rse_boolean.getAddress());
	ExprNode::doPass2(tdbb, csb, rse_sorted.getAddress());
	ExprNode::doPass2(tdbb, csb, rse_projection.getAddress());

	// If the user has submitted a plan for this RseNode, check it for correctness.

	if (rse_plan)
	{
		planSet(csb, rse_plan);
		planCheck(csb);
	}

	csb->csb_current_nodes.pop();
}

// Return true if stream is contained in the specified RseNode.
bool RseNode::containsStream(StreamType checkStream) const
{
	// Look through all relation nodes in this RseNode to see
	// if the field references this instance of the relation.

	const NestConst<RecordSourceNode>* ptr = rse_relations.begin();

	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
	{
		const RecordSourceNode* sub = *ptr;

		if (sub->containsStream(checkStream))
			return true;		// do not mark as variant
	}

	return false;
}

RecordSource* RseNode::compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream)
{
	// for nodes which are not relations, generate an rsb to
	// represent that work has to be done to retrieve them;
	// find all the substreams involved and compile them as well

	computeRseStreams(opt->beds);
	computeRseStreams(opt->localStreams);
	computeDbKeyStreams(opt->keyStreams);

	BoolExprNodeStack conjunctStack;

	// pass RseNode boolean only to inner substreams because join condition
	// should never exclude records from outer substreams
	if (opt->rse->rse_jointype == blr_inner ||
		(opt->rse->rse_jointype == blr_left && innerSubStream))
	{
		// AB: For an (X LEFT JOIN Y) mark the outer-streams (X) as
		// active because the inner-streams (Y) are always "dependent"
		// on the outer-streams. So that index retrieval nodes could be made.

		if (opt->rse->rse_jointype == blr_left)
		{
			for (StreamList::iterator i = opt->outerStreams.begin();
				 i != opt->outerStreams.end();
				 ++i)
			{
				opt->opt_csb->csb_rpt[*i].activate();
			}

			// Push all conjuncts except "missing" ones (e.g. IS NULL)

			for (USHORT i = 0; i < opt->opt_base_missing_conjuncts; i++)
				conjunctStack.push(opt->opt_conjuncts[i].opt_conjunct_node);
		}
		else
		{
			for (USHORT i = 0; i < opt->opt_conjuncts.getCount(); i++)
				conjunctStack.push(opt->opt_conjuncts[i].opt_conjunct_node);
		}

		RecordSource* const rsb = OPT_compile(tdbb, opt->opt_csb, this, &conjunctStack);

		if (opt->rse->rse_jointype == blr_left)
		{
			for (StreamList::iterator i = opt->outerStreams.begin();
				i != opt->outerStreams.end(); ++i)
			{
				opt->opt_csb->csb_rpt[*i].deactivate();
			}
		}

		return rsb;
	}

	// Push only parent conjuncts to the outer stream

	for (USHORT i = opt->opt_base_parent_conjuncts; i < opt->opt_conjuncts.getCount(); i++)
		conjunctStack.push(opt->opt_conjuncts[i].opt_conjunct_node);

	return OPT_compile(tdbb, opt->opt_csb, this, &conjunctStack);
}

// Check that all streams in the RseNode have a plan specified for them.
// If they are not, there are streams in the RseNode which were not mentioned in the plan.
void RseNode::planCheck(const CompilerScratch* csb) const
{
	// if any streams are not marked with a plan, give an error

	const NestConst<RecordSourceNode>* ptr = rse_relations.begin();
	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
	{
		const RecordSourceNode* node = *ptr;

		if (node->type == RelationSourceNode::TYPE)
		{
			const StreamType stream = node->getStream();

			if (!(csb->csb_rpt[stream].csb_plan))
			{
				ERR_post(Arg::Gds(isc_no_stream_plan) <<
					Arg::Str(csb->csb_rpt[stream].csb_relation->rel_name));
			}
		}
		else if (node->type == RseNode::TYPE)
			static_cast<const RseNode*>(node)->planCheck(csb);
	}
}

// Go through the streams in the plan, find the corresponding streams in the RseNode and store the
// plan for that stream. Do it once and only once to make sure there is a one-to-one correspondence
// between streams in the query and streams in the plan.
void RseNode::planSet(CompilerScratch* csb, PlanNode* plan)
{
	if (plan->type == PlanNode::TYPE_JOIN)
	{
		for (NestConst<PlanNode>* ptr = plan->subNodes.begin(), *end = plan->subNodes.end();
			 ptr != end;
			 ++ptr)
		{
			planSet(csb, *ptr);
		}
	}

	if (plan->type != PlanNode::TYPE_RETRIEVE)
		return;

	const jrd_rel* viewRelation = NULL;
	const jrd_rel* planRelation = plan->relationNode->relation;
	const char* planAlias = plan->relationNode->alias.c_str();

	// find the tail for the relation specified in the RseNode

	const StreamType stream = plan->relationNode->getStream();
	CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];

	// if the plan references a view, find the real base relation
	// we are interested in by searching the view map
	StreamType* map = NULL;

	if (tail->csb_map)
	{
		const TEXT* p = planAlias;

		// if the user has specified an alias, skip past it to find the alias
		// for the base table (if multiple aliases are specified)
		if (p && *p &&
			((tail->csb_relation && !strcmpSpace(tail->csb_relation->rel_name.c_str(), p)) ||
			 (tail->csb_alias && !strcmpSpace(tail->csb_alias->c_str(), p))))
		{
			while (*p && *p != ' ')
				p++;

			if (*p == ' ')
				p++;
		}

		// loop through potentially a stack of views to find the appropriate base table
		StreamType* mapBase;

		while ( (mapBase = tail->csb_map) )
		{
			map = mapBase;
			tail = &csb->csb_rpt[*map];
			viewRelation = tail->csb_relation;

			// if the plan references the view itself, make sure that
			// the view is on a single table; if it is, fix up the plan
			// to point to the base relation

			if (viewRelation->rel_id == planRelation->rel_id)
			{
				if (!mapBase[2])
				{
					map++;
					tail = &csb->csb_rpt[*map];
				}
				else
				{
					// view %s has more than one base relation; use aliases to distinguish
					ERR_post(Arg::Gds(isc_view_alias) << Arg::Str(planRelation->rel_name));
				}

				break;
			}

			viewRelation = NULL;

			// if the user didn't specify an alias (or didn't specify one
			// for this level), check to make sure there is one and only one
			// base relation in the table which matches the plan relation

			if (!*p)
			{
				const jrd_rel* duplicateRelation = NULL;
				StreamType* duplicateMap = mapBase;

				map = NULL;

				for (duplicateMap++; *duplicateMap; ++duplicateMap)
				{
					CompilerScratch::csb_repeat* duplicateTail = &csb->csb_rpt[*duplicateMap];
					const jrd_rel* relation = duplicateTail->csb_relation;

					if (relation && relation->rel_id == planRelation->rel_id)
					{
						if (duplicateRelation)
						{
							// table %s is referenced twice in view; use an alias to distinguish
							ERR_post(Arg::Gds(isc_duplicate_base_table) <<
								Arg::Str(duplicateRelation->rel_name));
						}
						else
						{
							duplicateRelation = relation;
							map = duplicateMap;
							tail = duplicateTail;
						}
					}
				}

				break;
			}

			// look through all the base relations for a match

			map = mapBase;
			for (map++; *map; map++)
			{
				tail = &csb->csb_rpt[*map];
				const jrd_rel* relation = tail->csb_relation;

				// match the user-supplied alias with the alias supplied
				// with the view definition; failing that, try the base
				// table name itself

				// CVC: I found that "relation" can be NULL, too. This may be an
				// indication of a logic flaw while parsing the user supplied SQL plan
				// and not an oversight here. It's hard to imagine a csb->csb_rpt with
				// a NULL relation. See exe.h for CompilerScratch struct and its inner csb_repeat struct.

				if ((tail->csb_alias && !strcmpSpace(tail->csb_alias->c_str(), p)) ||
					(relation && !strcmpSpace(relation->rel_name.c_str(), p)))
				{
					break;
				}
			}

			// skip past the alias

			while (*p && *p != ' ')
				p++;

			if (*p == ' ')
				p++;

			if (!*map)
			{
				// table %s is referenced in the plan but not the from list
				ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(planRelation->rel_name));
			}
		}

		// fix up the relation node to point to the base relation's stream

		if (!map || !*map)
		{
			// table %s is referenced in the plan but not the from list
			ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(planRelation->rel_name));
		}

		plan->relationNode->setStream(*map);
	}

	// make some validity checks

	if (!tail->csb_relation)
	{
		// table %s is referenced in the plan but not the from list
		ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(planRelation->rel_name));
	}

	if ((tail->csb_relation->rel_id != planRelation->rel_id) && !viewRelation)
	{
		// table %s is referenced in the plan but not the from list
		ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(planRelation->rel_name));
	}

	// check if we already have a plan for this stream

	if (tail->csb_plan)
	{
		// table %s is referenced more than once in plan; use aliases to distinguish
		ERR_post(Arg::Gds(isc_stream_twice) << Arg::Str(tail->csb_relation->rel_name));
	}

	tail->csb_plan = plan;
}

void RseNode::computeDbKeyStreams(StreamList& streamList) const
{
	const NestConst<RecordSourceNode>* ptr = rse_relations.begin();

	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
		(*ptr)->computeDbKeyStreams(streamList);
}

void RseNode::computeRseStreams(StreamList& streamList) const
{
	const NestConst<RecordSourceNode>* ptr = rse_relations.begin();

	for (const NestConst<RecordSourceNode>* const end = rse_relations.end(); ptr != end; ++ptr)
		(*ptr)->computeRseStreams(streamList);
}

bool RseNode::computable(CompilerScratch* csb, StreamType stream,
	bool allowOnlyCurrentStream, ValueExprNode* value)
{
	if (rse_first && !rse_first->computable(csb, stream, allowOnlyCurrentStream))
		return false;

    if (rse_skip && !rse_skip->computable(csb, stream, allowOnlyCurrentStream))
        return false;

	const NestConst<RecordSourceNode>* const end = rse_relations.end();
	NestConst<RecordSourceNode>* ptr;

	// Set sub-streams of rse active
	AutoActivateResetStreams activator(csb, this);

	// Check sub-stream
	if ((rse_boolean && !rse_boolean->computable(csb, stream, allowOnlyCurrentStream)) ||
	    (rse_sorted && !rse_sorted->computable(csb, stream, allowOnlyCurrentStream)) ||
	    (rse_projection && !rse_projection->computable(csb, stream, allowOnlyCurrentStream)))
	{
		return false;
	}

	for (ptr = rse_relations.begin(); ptr != end; ++ptr)
	{
		if (!(*ptr)->computable(csb, stream, allowOnlyCurrentStream, NULL))
			return false;
	}

	// Check value expression, if any
	if (value && !value->computable(csb, stream, allowOnlyCurrentStream))
		return false;

	return true;
}

void RseNode::findDependentFromStreams(const OptimizerRetrieval* optRet,
	SortedStreamList* streamList)
{
	if (rse_first)
		rse_first->findDependentFromStreams(optRet, streamList);

    if (rse_skip)
        rse_skip->findDependentFromStreams(optRet, streamList);

	if (rse_boolean)
		rse_boolean->findDependentFromStreams(optRet, streamList);

	if (rse_sorted)
		rse_sorted->findDependentFromStreams(optRet, streamList);

	if (rse_projection)
		rse_projection->findDependentFromStreams(optRet, streamList);

	NestConst<RecordSourceNode>* ptr;
	const NestConst<RecordSourceNode>* end;

	for (ptr = rse_relations.begin(), end = rse_relations.end(); ptr != end; ++ptr)
		(*ptr)->findDependentFromStreams(optRet, streamList);
}

void RseNode::collectStreams(SortedStreamList& streamList) const
{
	if (rse_first)
		rse_first->collectStreams(streamList);

	if (rse_skip)
		rse_skip->collectStreams(streamList);

	if (rse_boolean)
		rse_boolean->collectStreams(streamList);

	// ASF: The legacy code used to visit rse_sorted and rse_projection, but the nod_sort was never
	// handled.
	// rse_sorted->collectStreams(streamList);
	// rse_projection->collectStreams(streamList);

	const NestConst<RecordSourceNode>* ptr;
	const NestConst<RecordSourceNode>* end;

	for (ptr = rse_relations.begin(), end = rse_relations.end(); ptr != end; ++ptr)
		(*ptr)->collectStreams(streamList);
}


//--------------------


RseNode* SelectExprNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	fb_assert(dsqlFlags & DFLAG_DERIVED);
	return PASS1_derived_table(dsqlScratch, this, NULL);
}


//--------------------


static RecordSourceNode* dsqlPassRelProc(DsqlCompilerScratch* dsqlScratch, RecordSourceNode* source)
{
	ProcedureSourceNode* procNode = source->as<ProcedureSourceNode>();
	RelationSourceNode* relNode = source->as<RelationSourceNode>();

	fb_assert(procNode || relNode);

	bool couldBeCte = true;
	MetaName relName;
	string relAlias;

	if (procNode)
	{
		relName = procNode->dsqlName.identifier;
		relAlias = procNode->alias;
		couldBeCte = !procNode->sourceList && procNode->dsqlName.package.isEmpty();
	}
	else if (relNode)
	{
		relName = relNode->dsqlName;
		relAlias = relNode->alias;
	}

	if (relAlias.isEmpty())
		relAlias = relName.c_str();

	SelectExprNode* cte = couldBeCte ? dsqlScratch->findCTE(relName) : NULL;

	if (!cte)
		return PASS1_relation(dsqlScratch, source);

	cte->dsqlFlags |= RecordSourceNode::DFLAG_DT_CTE_USED;

	if ((dsqlScratch->flags & DsqlCompilerScratch::FLAG_RECURSIVE_CTE) &&
		 dsqlScratch->currCtes.hasData() &&
		 (dsqlScratch->currCtes.object() == cte))
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive CTE member (%s) can refer itself only in FROM clause
				  Arg::Gds(isc_dsql_cte_wrong_reference) << relName);
	}

	for (Stack<SelectExprNode*>::const_iterator stack(dsqlScratch->currCtes); stack.hasData(); ++stack)
	{
		SelectExprNode* cte1 = stack.object();
		if (cte1 == cte)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // CTE %s has cyclic dependencies
					  Arg::Gds(isc_dsql_cte_cycle) << relName);
		}
	}

	RecordSourceNode* const query = cte->querySpec;
	UnionSourceNode* unionQuery = ExprNode::as<UnionSourceNode>(query);
	const bool isRecursive = unionQuery && unionQuery->recursive;

	const string saveCteName = cte->alias;
	if (!isRecursive)
		cte->alias = relAlias;

	dsqlScratch->currCtes.push(cte);

	RseNode* derivedNode = PASS1_derived_table(dsqlScratch,
		cte, (isRecursive ? relAlias.c_str() : NULL));

	if (!isRecursive)
		cte->alias = saveCteName;

	dsqlScratch->currCtes.pop();

	return derivedNode;
}

// Parse a MAP clause for a union or global aggregate expression.
static MapNode* parseMap(thread_db* tdbb, CompilerScratch* csb, StreamType stream)
{
	SET_TDBB(tdbb);

	if (csb->csb_blr_reader.getByte() != blr_map)
		PAR_syntax_error(csb, "blr_map");

	unsigned int count = csb->csb_blr_reader.getWord();
	MapNode* node = FB_NEW(csb->csb_pool) MapNode(csb->csb_pool);

	while (count-- > 0)
	{
		node->targetList.add(PAR_gen_field(tdbb, stream, csb->csb_blr_reader.getWord()));
		node->sourceList.add(PAR_parse_value(tdbb, csb));
	}

	return node;
}

// Compare two strings, which could be either space-terminated or null-terminated.
static int strcmpSpace(const char* p, const char* q)
{
	for (; *p && *p != ' ' && *q && *q != ' '; p++, q++)
	{
		if (*p != *q)
			break;
	}

	if ((!*p || *p == ' ') && (!*q || *q == ' '))
		return 0;

	return (*p > *q) ? 1 : -1;
}

// Process a single record source stream from an RseNode.
// Obviously, if the source is a view, there is more work to do.
static void processSource(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
	RecordSourceNode* source, BoolExprNode** boolean, RecordSourceNodeStack& stack)
{
	SET_TDBB(tdbb);

	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	AutoSetRestore<bool> autoValidateExpr(&csb->csb_validate_expr, false);

	source->pass1Source(tdbb, csb, rse, boolean, stack);
}

// Translate a map block into a format. If the format is missing or incomplete, extend it.
static void processMap(thread_db* tdbb, CompilerScratch* csb, MapNode* map, Format** inputFormat)
{
	SET_TDBB(tdbb);

	Format* format = *inputFormat;
	if (!format)
		format = *inputFormat = Format::newFormat(*tdbb->getDefaultPool(), map->sourceList.getCount());

	// process alternating rse and map blocks
	dsc desc2;
	NestConst<ValueExprNode>* source = map->sourceList.begin();
	NestConst<ValueExprNode>* target = map->targetList.begin();

	for (const NestConst<ValueExprNode>* const sourceEnd = map->sourceList.end();
		 source != sourceEnd;
		 ++source, ++target)
	{
		FieldNode* field = (*target)->as<FieldNode>();
		const USHORT id = field->fieldId;

		if (id >= format->fmt_count)
			format->fmt_desc.resize(id + 1);

		dsc* desc = &format->fmt_desc[id];
		(*source)->getDesc(tdbb, csb, &desc2);
		const USHORT min = MIN(desc->dsc_dtype, desc2.dsc_dtype);
		const USHORT max = MAX(desc->dsc_dtype, desc2.dsc_dtype);

		if (!min)	// eg: dtype_unknown
			*desc = desc2;
		else if (max == dtype_blob)
		{
			USHORT subtype = DataTypeUtil::getResultBlobSubType(desc, &desc2);
			USHORT ttype = DataTypeUtil::getResultTextType(desc, &desc2);
			desc->makeBlob(subtype, ttype);
		}
		else if (min <= dtype_any_text)
		{
			// either field a text field?
			const USHORT len1 = DSC_string_length(desc);
			const USHORT len2 = DSC_string_length(&desc2);
			desc->dsc_dtype = dtype_varying;
			desc->dsc_length = MAX(len1, len2) + sizeof(USHORT);

			// pick the max text type, so any transparent casts from ints are
			// not left in ASCII format, but converted to the richer text format

			desc->setTextType(MAX(INTL_TEXT_TYPE(*desc), INTL_TEXT_TYPE(desc2)));
			desc->dsc_scale = 0;
			desc->dsc_flags = 0;
		}
		else if (DTYPE_IS_DATE(max) && !DTYPE_IS_DATE(min))
		{
			desc->dsc_dtype = dtype_varying;
			desc->dsc_length = DSC_convert_to_text_length(max) + sizeof(USHORT);
			desc->dsc_ttype() = ttype_ascii;
			desc->dsc_scale = 0;
			desc->dsc_flags = 0;
		}
		else if (max != min)
		{
			// different numeric types: if one is inexact use double,
			// if both are exact use int64
			if ((!DTYPE_IS_EXACT(max)) || (!DTYPE_IS_EXACT(min)))
			{
				desc->dsc_dtype = DEFAULT_DOUBLE;
				desc->dsc_length = sizeof(double);
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
			}
			else
			{
				desc->dsc_dtype = dtype_int64;
				desc->dsc_length = sizeof(SINT64);
				desc->dsc_scale = MIN(desc->dsc_scale, desc2.dsc_scale);
				desc->dsc_sub_type = MAX(desc->dsc_sub_type, desc2.dsc_sub_type);
				desc->dsc_flags = 0;
			}
		}
	}

	// flesh out the format of the record

	ULONG offset = FLAG_BYTES(format->fmt_count);

	Format::fmt_desc_iterator desc3 = format->fmt_desc.begin();
	for (const Format::fmt_desc_const_iterator end_desc = format->fmt_desc.end();
		 desc3 < end_desc; ++desc3)
	{
		const USHORT align = type_alignments[desc3->dsc_dtype];

		if (align)
			offset = FB_ALIGN(offset, align);

		desc3->dsc_address = (UCHAR*)(IPTR) offset;
		offset += desc3->dsc_length;
	}

	format->fmt_length = offset;
	format->fmt_count = format->fmt_desc.getCount();
}

// Make new boolean nodes from nodes that contain a field from the given shellStream.
// Those fields are references (mappings) to other nodes and are used by aggregates and union rse's.
static void genDeliverUnmapped(thread_db* tdbb, BoolExprNodeStack* deliverStack, MapNode* map,
	BoolExprNodeStack* parentStack, StreamType shellStream)
{
	SET_TDBB(tdbb);

	MemoryPool& pool = *tdbb->getDefaultPool();

	for (BoolExprNodeStack::iterator stack1(*parentStack); stack1.hasData(); ++stack1)
	{
		BoolExprNode* const boolean = stack1.object();

		// Handle the "OR" case first

		BinaryBoolNode* const binaryNode = boolean->as<BinaryBoolNode>();
		if (binaryNode && binaryNode->blrOp == blr_or)
		{
			BoolExprNodeStack orgStack, newStack;

			orgStack.push(binaryNode->arg1);
			orgStack.push(binaryNode->arg2);

			genDeliverUnmapped(tdbb, &newStack, map, &orgStack, shellStream);

			if (newStack.getCount() == 2)
			{
				BoolExprNode* const newArg2 = newStack.pop();
				BoolExprNode* const newArg1 = newStack.pop();

				BinaryBoolNode* const newBinaryNode =
					FB_NEW(pool) BinaryBoolNode(pool, blr_or, newArg1, newArg2);

				deliverStack->push(newBinaryNode);
			}
			else
			{
				while (newStack.hasData())
					delete newStack.pop();
			}

			continue;
		}

		// Reduce to simple comparisons

		ComparativeBoolNode* const cmpNode = boolean->as<ComparativeBoolNode>();
		MissingBoolNode* const missingNode = boolean->as<MissingBoolNode>();
		HalfStaticArray<ValueExprNode*, 2> children;

		if (cmpNode &&
			(cmpNode->blrOp == blr_eql || cmpNode->blrOp == blr_gtr || cmpNode->blrOp == blr_geq ||
			 cmpNode->blrOp == blr_leq || cmpNode->blrOp == blr_lss || cmpNode->blrOp == blr_starting))
		{
			children.add(cmpNode->arg1);
			children.add(cmpNode->arg2);
		}
		else if (missingNode)
			children.add(missingNode->arg);
		else
			continue;

		// At least 1 mapping should be used in the arguments
		size_t indexArg;
		bool mappingFound = false;

		for (indexArg = 0; (indexArg < children.getCount()) && !mappingFound; ++indexArg)
		{
			FieldNode* fieldNode = children[indexArg]->as<FieldNode>();

			if (fieldNode && fieldNode->fieldStream == shellStream)
				mappingFound = true;
		}

		if (!mappingFound)
			continue;

		// Create new node and assign the correct existing arguments

		BoolExprNode* deliverNode = NULL;
		HalfStaticArray<ValueExprNode**, 2> newChildren;

		if (cmpNode)
		{
			ComparativeBoolNode* const newCmpNode =
				FB_NEW(pool) ComparativeBoolNode(pool, cmpNode->blrOp);

			newChildren.add(newCmpNode->arg1.getAddress());
			newChildren.add(newCmpNode->arg2.getAddress());

			deliverNode = newCmpNode;
		}
		else if (missingNode)
		{
			MissingBoolNode* const newMissingNode = FB_NEW(pool) MissingBoolNode(pool);

			newChildren.add(newMissingNode->arg.getAddress());

			deliverNode = newMissingNode;
		}

		deliverNode->nodFlags = boolean->nodFlags;
		deliverNode->impureOffset = boolean->impureOffset;

		bool okNode = true;

		for (indexArg = 0; (indexArg < children.getCount()) && okNode; ++indexArg)
		{
			// Check if node is a mapping and if so unmap it, but only for root nodes (not contained
			// in another node). This can be expanded by checking complete expression (Then don't
			// forget to leave aggregate-functions alone in case of aggregate rse).
			// Because this is only to help using an index we keep it simple.

			FieldNode* fieldNode = children[indexArg]->as<FieldNode>();

			if (fieldNode && fieldNode->fieldStream == shellStream)
			{
				const USHORT fieldId = fieldNode->fieldId;

				if (fieldId >= map->sourceList.getCount())
					okNode = false;
				else
				{
					// Check also the expression inside the map, because aggregate
					// functions aren't allowed to be delivered to the WHERE clause.
					ValueExprNode* value = map->sourceList[fieldId];
					okNode = value->unmappable(map, shellStream);

					if (okNode)
						*newChildren[indexArg] = map->sourceList[fieldId];
				}
			}
			else
			{
				if ((okNode = children[indexArg]->unmappable(map, shellStream)))
					*newChildren[indexArg] = children[indexArg];
			}
		}

		if (!okNode)
			delete deliverNode;
		else
			deliverStack->push(deliverNode);
	}
}

// Mark indices that were not included in the user-specified access plan.
static void markIndices(CompilerScratch::csb_repeat* csbTail, SSHORT relationId)
{
	const PlanNode* plan = csbTail->csb_plan;

	if (!plan || plan->type != PlanNode::TYPE_RETRIEVE)
		return;

	// Go through each of the indices and mark it unusable
	// for indexed retrieval unless it was specifically mentioned
	// in the plan; also mark indices for navigational access.

	// If there were none indices, this is a sequential retrieval.

	index_desc* idx = csbTail->csb_idx->items;

	for (USHORT i = 0; i < csbTail->csb_indices; i++)
	{
		if (plan->accessType)
		{
			ObjectsArray<PlanNode::AccessItem>::iterator arg = plan->accessType->items.begin();
			const ObjectsArray<PlanNode::AccessItem>::iterator end = plan->accessType->items.end();

			for (; arg != end; ++arg)
			{
				if (relationId != arg->relationId)
				{
					// index %s cannot be used in the specified plan
					ERR_post(Arg::Gds(isc_index_unused) << arg->indexName);
				}

				if (idx->idx_id == arg->indexId)
				{
					if (plan->accessType->type == PlanNode::AccessType::TYPE_NAVIGATIONAL &&
						arg == plan->accessType->items.begin())
					{
						// dimitr:	navigational access can use only one index,
						//			hence the extra check added (see the line above)
						idx->idx_runtime_flags |= idx_plan_navigate;
					}
					else
					{
						// nod_indices
						break;
					}
				}
			}

			if (arg == end)
				idx->idx_runtime_flags |= idx_plan_dont_use;
		}
		else
			idx->idx_runtime_flags |= idx_plan_dont_use;

		++idx;
	}
}

// Resolve a field for JOIN USING purposes.
static ValueExprNode* resolveUsingField(DsqlCompilerScratch* dsqlScratch, const MetaName& name,
	ValueListNode* list, const FieldNode* flawedNode, const TEXT* side, dsql_ctx*& ctx)
{
	ValueExprNode* node = PASS1_lookup_alias(dsqlScratch, name, list, false);

	if (!node)
	{
		string qualifier;
		qualifier.printf("<%s side of USING>", side);
		PASS1_field_unknown(qualifier.c_str(), name.c_str(), flawedNode);
	}

	DsqlAliasNode* aliasNode;
	FieldNode* fieldNode;
	DerivedFieldNode* derivedField;

	if ((aliasNode = ExprNode::as<DsqlAliasNode>(node)))
		ctx = aliasNode->implicitJoin->visibleInContext;
	else if ((fieldNode = ExprNode::as<FieldNode>(node)))
		ctx = fieldNode->dsqlContext;
	else if ((derivedField = ExprNode::as<DerivedFieldNode>(node)))
		ctx = derivedField->context;
	else
	{
		fb_assert(false);
	}

	return node;
}

// Sort SortedStream indices based on there selectivity. Lowest selectivy as first, highest as last.
static void sortIndicesBySelectivity(CompilerScratch::csb_repeat* csbTail)
{
	if (csbTail->csb_plan)
		return;

	index_desc* selectedIdx = NULL;
	Array<index_desc> idxSort(csbTail->csb_indices);
	bool sameSelectivity = false;

	// Walk through the indices and sort them into into idxSort
	// where idxSort[0] contains the lowest selectivity (best) and
	// idxSort[csbTail->csb_indices - 1] the highest (worst)

	if (csbTail->csb_idx && (csbTail->csb_indices > 1))
	{
		for (USHORT j = 0; j < csbTail->csb_indices; j++)
		{
			float selectivity = 1; // Maximum selectivity is 1 (when all keys are the same)
			index_desc* idx = csbTail->csb_idx->items;

			for (USHORT i = 0; i < csbTail->csb_indices; i++)
			{
				// Prefer ASC indices in the case of almost the same selectivities
				if (selectivity > idx->idx_selectivity)
					sameSelectivity = ((selectivity - idx->idx_selectivity) <= 0.00001);
				else
					sameSelectivity = ((idx->idx_selectivity - selectivity) <= 0.00001);

				if (!(idx->idx_runtime_flags & idx_marker) &&
					 (idx->idx_selectivity <= selectivity) &&
					 !((idx->idx_flags & idx_descending) && sameSelectivity))
				{
					selectivity = idx->idx_selectivity;
					selectedIdx = idx;
				}

				++idx;
			}

			// If no index was found than pick the first one available out of the list
			if ((!selectedIdx) || (selectedIdx->idx_runtime_flags & idx_marker))
			{
				idx = csbTail->csb_idx->items;

				for (USHORT i = 0; i < csbTail->csb_indices; i++)
				{
					if (!(idx->idx_runtime_flags & idx_marker))
					{
						selectedIdx = idx;
						break;
					}

					++idx;
				}
			}

			selectedIdx->idx_runtime_flags |= idx_marker;
			idxSort.add(*selectedIdx);
		}

		// Finally store the right order in cbs_tail->csb_idx
		index_desc* idx = csbTail->csb_idx->items;

		for (USHORT j = 0; j < csbTail->csb_indices; j++)
		{
			idx->idx_runtime_flags &= ~idx_marker;
			memcpy(idx, &idxSort[j], sizeof(index_desc));
			++idx;
		}
	}
}
