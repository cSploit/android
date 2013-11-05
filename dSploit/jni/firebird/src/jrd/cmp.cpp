/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cmp.cpp
 *	DESCRIPTION:	Request compiler
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
 * 2001.07.28: John Bellardo: Added code to handle rse_skip.
 * 2001.07.17 Claudio Valderrama: Stop crash when parsing user-supplied SQL plan.
 * 2001.10.04 Claudio Valderrama: Fix annoying & invalid server complaint about
 *   triggers not having REFERENCES privilege over their owner table.
 * 2002.02.24 Claudio Valderrama: substring() should signal output as string even
 *   if source is blob and should check implementation limits on field lengths.
 * 2002.02.25 Claudio Valderrama: concatenate() should be a civilized function.
 *   This closes the heart of SF Bug #518282.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>				// abort
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/align.h"
#include "../jrd/lls.h"
#include "../jrd/exe.h"
#include "../jrd/rse.h"
#include "../jrd/scl.h"
#include "../jrd/tra.h"
#include "../jrd/lck.h"
#include "../jrd/irq.h"
#include "../jrd/drq.h"
#include "../jrd/intl.h"
#include "../jrd/btr.h"
#include "../jrd/sort.h"
#include "../jrd/gdsassert.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/fun_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/dbg_proto.h"	// DBG_supervisor
#include "../jrd/sort_proto.h"
#include "../jrd/execute_statement.h"
#include "../jrd/Optimizer.h"

#include "../jrd/DataTypeUtil.h"
#include "../jrd/SysFunction.h"
#include "../jrd/misc_func_ids.h"

/* Pick up relation ids */
#include "../jrd/ini.h"

#include "../common/classes/auto.h"
#include "../common/utils_proto.h"
#include "../dsql/Nodes.h"

using Firebird::AutoSetRestore;


/* Firebird provides transparent conversion from string to date in
 * contexts where it makes sense.  This macro checks a descriptor to
 * see if it is something that *could* represent a date value
 */
inline bool COULD_BE_DATE(const dsc desc)
{
	return ((DTYPE_IS_DATE(desc.dsc_dtype)) || (desc.dsc_dtype <= dtype_any_text));
}
//#define COULD_BE_DATE(d)	((DTYPE_IS_DATE((d).dsc_dtype)) || ((d).dsc_dtype <= dtype_any_text))

/* One of d1,d2 is time, the other is date */
inline bool IS_DATE_AND_TIME(const dsc d1, const dsc d2)
{
	return (((d1.dsc_dtype == dtype_sql_time) && (d2.dsc_dtype == dtype_sql_date)) ||
	((d2.dsc_dtype == dtype_sql_time) && (d1.dsc_dtype == dtype_sql_date)));
}
//#define IS_DATE_AND_TIME(d1, d2)
//  ((((d1).dsc_dtype==dtype_sql_time)&&((d2).dsc_dtype==dtype_sql_date)) ||
//   (((d2).dsc_dtype==dtype_sql_time)&&((d1).dsc_dtype==dtype_sql_date)))

// size of req_rpb[0]
const size_t REQ_TAIL = sizeof(Jrd::jrd_req::blk_repeat_type);
const int MAP_LENGTH = 256;

/* RITTER - changed HP10 to HPUX */
#if defined (HPUX) && defined (SUPERSERVER)
const int MAX_RECURSION		= 96;
#else
const int MAX_RECURSION		= 128;
#endif

const int MAX_REQUEST_SIZE	= 10485760;	// 10 MB - just to be safe

using namespace Jrd;
using namespace Firebird;

static UCHAR* alloc_map(thread_db*, CompilerScratch*, USHORT);
static jrd_nod* catenate_nodes(thread_db*, NodeStack&);
static jrd_nod* convertNeqAllToNotAny(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node);
static jrd_nod* copy(thread_db*, CompilerScratch*, jrd_nod*, UCHAR *, USHORT, jrd_nod*, bool);
static void expand_view_nodes(thread_db*, CompilerScratch*, USHORT, NodeStack&, nod_t, bool);
static void ignore_dbkey(thread_db*, CompilerScratch*, RecordSelExpr*, const jrd_rel*);
static jrd_nod* make_defaults(thread_db*, CompilerScratch*, USHORT, jrd_nod*);
static jrd_nod* make_validation(thread_db*, CompilerScratch*, USHORT);
static void mark_variant(thread_db* tdbb, CompilerScratch* csb, USHORT stream);
static void pass1_erase(thread_db*, CompilerScratch*, jrd_nod*);
static jrd_nod* pass1_expand_view(thread_db*, CompilerScratch*, USHORT, USHORT, bool);
static void pass1_modify(thread_db*, CompilerScratch*, jrd_nod*);
static RecordSelExpr* pass1_rse(thread_db*, CompilerScratch*, RecordSelExpr*);
static void pass1_source(thread_db*, CompilerScratch*, RecordSelExpr*, jrd_nod*, jrd_nod**, NodeStack&);
static bool pass1_store(thread_db*, CompilerScratch*, jrd_nod*);
static jrd_nod* pass1_update(thread_db*, CompilerScratch*, jrd_rel*, const trig_vec*, USHORT, USHORT,
	SecurityClass::flags_t, jrd_rel*, USHORT);
static void pass2_rse(thread_db*, CompilerScratch*, RecordSelExpr*);
static jrd_nod* pass2_union(thread_db*, CompilerScratch*, jrd_nod*);
static jrd_nod* pass2_validation(thread_db*, CompilerScratch*, const Item&);
static void plan_check(const CompilerScratch*, const RecordSelExpr*);
static void plan_set(CompilerScratch*, RecordSelExpr*, jrd_nod*);
static void post_procedure_access(thread_db*, CompilerScratch*, jrd_prc*);
static RecordSource* post_rse(thread_db*, CompilerScratch*, RecordSelExpr*);
static void	post_trigger_access(CompilerScratch*, jrd_rel*, ExternalAccess::exa_act, jrd_rel*);
static void process_map(thread_db*, CompilerScratch*, jrd_nod*, Format**);
static SSHORT strcmp_space(const char*, const char*);
static bool stream_in_rse(USHORT, const RecordSelExpr*);
static void build_external_access(thread_db* tdbb, ExternalAccessList& list, jrd_req* request);
static void verify_trigger_access(thread_db* tdbb, jrd_rel* owner_relation, trig_vec* triggers, jrd_rel* view);

#ifdef CMP_DEBUG
#include <stdarg.h>
IMPLEMENT_TRACE_ROUTINE(cmp_trace, "CMP")
#endif

bool CMP_clone_is_active(const jrd_req* request)
{
/**************************************
 *
 *	C M P _ c l o n e _ i s _ a c t i v e
 *
 **************************************
 *
 * Functional description
 *	Determine if a request or any of its clones are active.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	if (request->req_flags & req_in_use)
		return true;

	vec<jrd_req*>* vector = request->req_sub_requests;
	if (vector) {
		for (vec<jrd_req*>::const_iterator sub_req = vector->begin(), end = vector->end();
			sub_req < end; ++sub_req)
		{
			if (*sub_req && (*sub_req)->req_flags & req_in_use)
				return true;
		}
	}

	return false;
}


jrd_nod* CMP_clone_node(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	C M P _ c l o n e _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Clone a node.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	return copy(tdbb, csb, node, NULL, 0, NULL, false);
}


jrd_nod* CMP_clone_node_opt(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	C M P _ c l o n e _ n o d e _ o p t
 *
 **************************************
 *
 * Functional description
 *	Clone a value node for the optimizer.  Make a copy of the node
 *	(if necessary) and assign impure space.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	if (node->nod_type == nod_argument) {
		return node;
	}

	jrd_nod* clone = copy(tdbb, csb, node, NULL, 0, NULL, false);
	CMP_pass2(tdbb, csb, clone, 0);

	return clone;
}


inline void triggers_external_access(thread_db* tdbb, ExternalAccessList& list, trig_vec* tvec)
/**************************************
 *
 *	t r i g g e r s _ e x t e r n a l _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *  Invoke build_external_access for triggers in vector
 *
 **************************************/
{
	if (tvec) {
		for (size_t i = 0; i < tvec->getCount(); i++)
		{
			Trigger& t = (*tvec)[i];
			t.compile(tdbb);
			if (t.request)
			{
				build_external_access(tdbb, list, t.request);
			}
		}
	}
}


static void build_external_access(thread_db* tdbb, ExternalAccessList& list, jrd_req* request)
{
/**************************************
 *
 *	b u i l d _ e x t e r n a l _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *  Recursively walk external dependencies (procedures, triggers) for request to assemble full
 *  list of requests it depends on
 *
 **************************************/
	for (ExternalAccess *item = request->req_external.begin(); item < request->req_external.end(); item++)
	{
		size_t i;
		if (list.find(*item, i))
			continue;

		list.insert(i, *item);

		// Add externals recursively
		if (item->exa_action == ExternalAccess::exa_procedure) {
			jrd_prc* prc = MET_lookup_procedure_id(tdbb, item->exa_prc_id, false, false, 0);
			if (prc && prc->prc_request)
				build_external_access(tdbb, list, prc->prc_request);
		}
		else {
			jrd_rel* relation = MET_lookup_relation_id(tdbb, item->exa_rel_id, false);

			if (!relation)
				continue;

			trig_vec *vec1, *vec2;
			switch (item->exa_action)
			{
			case ExternalAccess::exa_insert:
				vec1 = relation->rel_pre_store;
				vec2 = relation->rel_post_store;
				break;
			case ExternalAccess::exa_update:
				vec1 = relation->rel_pre_modify;
				vec2 = relation->rel_post_modify;
				break;
			case ExternalAccess::exa_delete:
				vec1 = relation->rel_pre_erase;
				vec2 = relation->rel_post_erase;
				break;
			default:
				continue; // should never happen, silence the compiler
			}
			triggers_external_access(tdbb, list, vec1);
			triggers_external_access(tdbb, list, vec2);
		}
	}
}


static void verify_trigger_access(thread_db* tdbb, jrd_rel* owner_relation, trig_vec* triggers, jrd_rel* view)
{
/**************************************
 *
 *	v e r i f y _ t r i g g e r _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *  Check that we have enough rights to access all resources this list of triggers touches
 *
 **************************************/
	if (!triggers) {
		return;
	}

	SET_TDBB(tdbb);

	for (size_t i = 0; i < triggers->getCount(); i++)
	{
		Trigger& t = (*triggers)[i];
		t.compile(tdbb);
		if (!t.request)
		{
			continue;
		}

		for (const AccessItem* access = t.request->req_access.begin();
			access < t.request->req_access.end(); access++)
		{
			// If this is not a system relation, we don't post access check if:
			//
			// - The table being checked is the owner of the trigger that's accessing it.
			// - The field being checked is owned by the same table than the trigger
			//   that's accessing the field.
			// - Since the trigger name comes in the triggers vector of the table and each
			//   trigger can be owned by only one table for now, we know for sure that
			//   it's a trigger defined on our target table.

			if (!(owner_relation->rel_flags & REL_system))
			{
				if (!strcmp(access->acc_type, object_table) &&
					(owner_relation->rel_name == access->acc_name))
				{
					continue;
				}
				if (!strcmp(access->acc_type, object_column) &&
					(owner_relation->rel_name == access->acc_r_name))
				{
					continue;
				}
			}

			// a direct access to an object from this trigger
			const SecurityClass* sec_class = SCL_get_class(tdbb, access->acc_security_name.c_str());
			SCL_check_access(tdbb, sec_class,
							(access->acc_view_id) ? access->acc_view_id : (view ? view->rel_id : 0),
							t.request->req_trg_name, NULL, access->acc_mask,
							access->acc_type, access->acc_name, access->acc_r_name);
		}
	}
}


void CMP_verify_access(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	C M P _ v e r i f y _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *  Check that we have enough rights to access all resources this request touches including
 *  resources it used indirectecty via procedures or triggers
 *
 **************************************/

	SET_TDBB(tdbb);

	ExternalAccessList external;
	build_external_access(tdbb, external, request);

	for (ExternalAccess* item = external.begin(); item < external.end(); item++)
	{
		if (item->exa_action == ExternalAccess::exa_procedure) {
			jrd_prc* prc = MET_lookup_procedure_id(tdbb, item->exa_prc_id, false, false, 0);
			if (!prc->prc_request)
				continue;

			for (const AccessItem* access = prc->prc_request->req_access.begin();
				 access < prc->prc_request->req_access.end();
				 access++)
			{
				const SecurityClass* sec_class = SCL_get_class(tdbb, access->acc_security_name.c_str());
				SCL_check_access(tdbb, sec_class, access->acc_view_id, NULL, prc->prc_name,
								access->acc_mask, access->acc_type, access->acc_name, access->acc_r_name);
			}
		}
		else {
			jrd_rel* relation = MET_lookup_relation_id(tdbb, item->exa_rel_id, false);
			jrd_rel* view = NULL;
			if (item->exa_view_id)
				view = MET_lookup_relation_id(tdbb, item->exa_view_id, false);

			if (!relation)
				continue;

			switch (item->exa_action)
			{
			case ExternalAccess::exa_insert:
				verify_trigger_access(tdbb, relation, relation->rel_pre_store, view);
				verify_trigger_access(tdbb, relation, relation->rel_post_store, view);
				break;
			case ExternalAccess::exa_update:
				verify_trigger_access(tdbb, relation, relation->rel_pre_modify, view);
				verify_trigger_access(tdbb, relation, relation->rel_post_modify, view);
				break;
			case ExternalAccess::exa_delete:
				verify_trigger_access(tdbb, relation, relation->rel_pre_erase, view);
				verify_trigger_access(tdbb, relation, relation->rel_post_erase, view);
				break;
			default:
				continue; // should never happen, silence the compiler
			}
		}
	}

	// Inherit privileges of caller stored procedure or trigger if and only if
	// this request is called immediately by caller (check for empty req_caller).
	// Currently (in v2.5) this rule will work for EXECUTE STATEMENT only, as
	// tra_callback_count incremented only by it.
	// When external SP's will be introduced we need to decide if they also can
	// inherit caller's privileges
	jrd_tra* transaction = tdbb->getTransaction();
	const jrd_req* exec_stmt_caller =
		(transaction && transaction->tra_callback_count && !request->req_caller) ?
			transaction->tra_callback_caller : NULL;

	for (const AccessItem* access = request->req_access.begin();
		access < request->req_access.end();
		access++)
	{
		const SecurityClass* sec_class = SCL_get_class(tdbb, access->acc_security_name.c_str());

		Firebird::MetaName trgName(exec_stmt_caller ? exec_stmt_caller->req_trg_name : NULL);
		Firebird::MetaName prcName(exec_stmt_caller && exec_stmt_caller->req_procedure ?
			exec_stmt_caller->req_procedure->prc_name : NULL);

		SCL_check_access(tdbb, sec_class, access->acc_view_id, trgName, prcName,
			access->acc_mask, access->acc_type, access->acc_name, access->acc_r_name);
	}
}


jrd_req* CMP_clone_request(thread_db* tdbb, jrd_req* request, USHORT level, bool validate)
{
/**************************************
 *
 *	C M P _ c l o n e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Get the incarnation of the request appropriate for a given level.
 *	If the incarnation doesn't exist, clone the request.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);

	Database* const dbb = tdbb->getDatabase();
	Attachment* const attachment = tdbb->getAttachment();
	fb_assert(dbb);

	// find the request if we've got it

	if (!level) {
		return request;
	}

	jrd_req* clone;
	vec<jrd_req*>* vector = request->req_sub_requests;
	if (vector && level < vector->count() && (clone = (*vector)[level]))
	{
		return clone;
	}

	if (validate) {
		jrd_prc* procedure = request->req_procedure;
		if (procedure) {
			const TEXT* prc_sec_name = (procedure->prc_security_name.length() > 0 ?
				procedure->prc_security_name.c_str() : NULL);
			const SecurityClass* sec_class = SCL_get_class(tdbb, prc_sec_name);
			SCL_check_access(tdbb, sec_class, 0, NULL, NULL, SCL_execute,
							 object_procedure, procedure->prc_name);
		}

		CMP_verify_access(tdbb, request);

	}

	MemoryPool* const pool = request->req_pool;

	// we need to clone the request - find someplace to put it

	vector = request->req_sub_requests =
		vec<jrd_req*>::newVector(*pool, request->req_sub_requests, level + 1);

	// clone the request

	const size_t n = (request->req_impure_size - REQ_SIZE + REQ_TAIL - 1) / REQ_TAIL;
	clone = FB_NEW_RPT(*pool, n) jrd_req(pool, &dbb->dbb_memory_stats);
	(*vector)[level] = clone;
	clone->req_attachment = attachment;
	clone->req_count = request->req_count;
	clone->req_impure_size = request->req_impure_size;
	clone->req_top_node = request->req_top_node;
	clone->req_trg_name = request->req_trg_name;
	clone->req_procedure = request->req_procedure;
	clone->req_flags = request->req_flags & REQ_FLAGS_CLONE_MASK;
	clone->req_last_xcp = request->req_last_xcp;
	clone->req_id = dbb->generateStatementId(tdbb);

	// We are cloning full lists here, not assigning pointers
	clone->req_invariants = request->req_invariants;
	clone->req_fors = request->req_fors;
	clone->req_exec_sta = request->req_exec_sta;
	clone->req_map_field_info.assign(request->req_map_field_info);

	record_param* rpb1 = clone->req_rpb;
	const record_param* const end = rpb1 + clone->req_count;

	for (const record_param* rpb2 = request->req_rpb; rpb1 < end; rpb1++, rpb2++)
	{
		if (rpb2->rpb_stream_flags & RPB_s_update) {
			rpb1->rpb_stream_flags |= RPB_s_update;
		}
		rpb1->rpb_relation = rpb2->rpb_relation;
	}

	return clone;
}


jrd_req* CMP_compile2(thread_db* tdbb, const UCHAR* blr, ULONG blr_length, bool internal_flag,
					  USHORT dbginfo_length, const UCHAR* dbginfo)
{
/**************************************
 *
 *	C M P _ c o m p i l e 2
 *
 **************************************
 *
 * Functional description
 *	Compile a BLR request.
 *
 **************************************/
	jrd_req* request = NULL;

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	// 26.09.2002 Nickolay Samofatov: default memory pool will become statement pool
	// and will be freed by CMP_release
	MemoryPool* const new_pool = dbb->createPool();

	try
	{
		Jrd::ContextPoolHolder context(tdbb, new_pool);

		AutoPtr<CompilerScratch> csb;
		PAR_parse(tdbb, csb, blr, blr_length, internal_flag, dbginfo_length, dbginfo);

		request = CMP_make_request(tdbb, csb, internal_flag);
		new_pool->setStatsGroup(request->req_memory_stats);

		if (internal_flag) {
			request->req_flags |= req_internal;
		}

		CMP_verify_access(tdbb, request);
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (request)
			CMP_release(tdbb, request);
		else
			dbb->deletePool(new_pool);
		ERR_punt();
	}

	return request;
}


CompilerScratch::csb_repeat* CMP_csb_element(CompilerScratch* csb, USHORT element)
{
/**************************************
 *
 *	C M P _ c s b _ e l e m e n t
 *
 **************************************
 *
 * Functional description
 *	Find tail element of compiler scratch block.  If the csb isn't big
 *	enough, extend it.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	CompilerScratch::csb_repeat empty_item;
	while (element >= csb->csb_rpt.getCount()) {
		csb->csb_rpt.add(empty_item);
	}
	return &csb->csb_rpt[element];
}


jrd_req* CMP_find_request(thread_db* tdbb, USHORT id, USHORT which)
{
/**************************************
 *
 *	C M P _ f i n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Find an inactive incarnation of a system request.  If necessary,
 *	clone it.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// if the request hasn't been compiled or isn't active,
	// there're nothing to do

	Database::CheckoutLockGuard guard(dbb, dbb->dbb_cmp_clone_mutex);

	Attachment* attachment = tdbb->getAttachment();
	jrd_req* request;
	if ((which == IRQ_REQUESTS && !(request = REQUEST(id))) ||
		(which == DYN_REQUESTS && !(request = DYN_REQUEST(id))) ||
		!(request->req_flags & (req_active | req_reserved)))
	{
		if (request) {
			request->req_flags |= req_reserved;
			request->req_attachment = attachment;
		}
		return request;
	}

	// Request exists and is in use. Look for clones until we find
	// one that is available.

	for (int n = 1; true; n++) {
		if (n > MAX_RECURSION) {
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_req_depth_exceeded) << Arg::Num(MAX_RECURSION));
			// Msg363 "request depth exceeded. (Recursive definition?)"
		}
		jrd_req* clone = CMP_clone_request(tdbb, request, n, false);
		if (!(clone->req_flags & (req_active | req_reserved))) {
			clone->req_flags |= req_reserved;
			clone->req_attachment = attachment;
			return clone;
		}
	}
}


void CMP_fini(thread_db* tdbb)
{
/**************************************
 *
 *	C M P _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Get rid of resource locks during shutdown.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	// shutdown shared database locks

	CMP_shutdown_database(tdbb);

	// unwind any active system requests

	while (dbb->dbb_sys_trans->tra_requests)
	{
		EXE_unwind(tdbb, dbb->dbb_sys_trans->tra_requests);
	}

	// and release them

	for (jrd_req** itr = dbb->dbb_internal.begin(); itr < dbb->dbb_internal.end(); itr++)
	{
		if (*itr)
		{
			(*itr)->req_attachment = NULL;
			CMP_release(tdbb, *itr);
		}
	}

	for (jrd_req** itr = dbb->dbb_dyn_req.begin(); itr < dbb->dbb_dyn_req.end(); itr++)
	{
		if (*itr)
		{
			(*itr)->req_attachment = NULL;
			CMP_release(tdbb, *itr);
		}
	}
}


Format* CMP_format(thread_db* tdbb, CompilerScratch* csb, USHORT stream)
{
/**************************************
 *
 *	C M P _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Pick up a format for a stream.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];

	if (tail->csb_format) {
		return tail->csb_format;
	}

	if (tail->csb_relation) {
		return tail->csb_format = MET_current(tdbb, tail->csb_relation);
	}

	if (tail->csb_procedure) {
		return tail->csb_format = tail->csb_procedure->prc_format;
	}

	IBERROR(222);				// msg 222 bad blr - invalid stream
	return NULL;
}


void CMP_get_desc(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node, DSC* desc)
{
/**************************************
 *
 *	C M P _ g e t _ d e s c
 *
 **************************************
 *
 * Functional description
 *	Compute descriptor for value expression.
 *
 **************************************/
	USHORT dtype = dtype_unknown;

	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	switch (node->nod_type)
	{
	case nod_max:
	case nod_min:
	case nod_from:
		CMP_get_desc(tdbb, csb, node->nod_arg[e_stat_value], desc);
		return;

	case nod_agg_total:
	case nod_agg_total_distinct:
	case nod_total:
		if (node->nod_type == nod_total)
			CMP_get_desc(tdbb, csb, node->nod_arg[e_stat_value], desc);
		else
			CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		switch (dtype = desc->dsc_dtype)
		{
		case dtype_short:
			desc->dsc_dtype = dtype_long;
			desc->dsc_length = sizeof(SLONG);
			node->nod_scale = desc->dsc_scale;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			return;

		case dtype_unknown:
			desc->dsc_dtype = dtype_unknown;
			desc->dsc_length = 0;
			node->nod_scale = 0;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			return;

		case dtype_long:
		case dtype_int64:
		case dtype_real:
		case dtype_double:
		case dtype_text:
		case dtype_cstring:
		case dtype_varying:
			desc->dsc_dtype = DEFAULT_DOUBLE;
			desc->dsc_length = sizeof(double);
			desc->dsc_scale = 0;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			node->nod_flags |= nod_double;
			return;

		case dtype_quad:
			desc->dsc_dtype = dtype_quad;
			desc->dsc_length = sizeof(SQUAD);
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			node->nod_scale = desc->dsc_scale;
			node->nod_flags |= nod_quad;
#ifdef NATIVE_QUAD
			return;
#endif

		default:
			fb_assert(false);
			// FALLINTO
		case dtype_sql_time:
		case dtype_sql_date:
		case dtype_timestamp:
		case dtype_blob:
		case dtype_array:
		case dtype_dbkey:
			// break to error reporting code
			break;
		}
		break;

	case nod_agg_total2:
	case nod_agg_total_distinct2:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		switch (dtype = desc->dsc_dtype)
		{
		case dtype_short:
		case dtype_long:
		case dtype_int64:
			desc->dsc_dtype = dtype_int64;
			desc->dsc_length = sizeof(SINT64);
			node->nod_scale = desc->dsc_scale;
			desc->dsc_flags = 0;
			return;

		case dtype_unknown:
			desc->dsc_dtype = dtype_unknown;
			desc->dsc_length = 0;
			node->nod_scale = 0;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			return;

		case dtype_real:
		case dtype_double:
		case dtype_text:
		case dtype_cstring:
		case dtype_varying:
			desc->dsc_dtype = DEFAULT_DOUBLE;
			desc->dsc_length = sizeof(double);
			desc->dsc_scale = 0;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			node->nod_flags |= nod_double;
			return;


		case dtype_quad:
			desc->dsc_dtype = dtype_quad;
			desc->dsc_length = sizeof(SQUAD);
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			node->nod_scale = desc->dsc_scale;
			node->nod_flags |= nod_quad;
#ifdef NATIVE_QUAD
			return;
#endif

		default:
			fb_assert(false);
			// FALLINTO
		case dtype_sql_time:
		case dtype_sql_date:
		case dtype_timestamp:
		case dtype_blob:
		case dtype_array:
		case dtype_dbkey:
			// break to error reporting code
			break;
		}
		break;

	case nod_prot_mask:
	case nod_null:
	case nod_agg_count:
	case nod_agg_count2:
	case nod_agg_count_distinct:
	//case nod_count2:
	case nod_count:
	case nod_gen_id:
	case nod_lock_state:
#ifdef SCROLLABLE_CURSORS
	case nod_seek:
#endif
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_field:
		{
			const USHORT stream = (USHORT) (IPTR) node->nod_arg[e_fld_stream];
			const USHORT id = (USHORT) (IPTR) node->nod_arg[e_fld_id];
			const Format* format = CMP_format(tdbb, csb, stream);

			if (id >= format->fmt_count)
			{
				desc->dsc_dtype = dtype_unknown;
				desc->dsc_length = 0;
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
			}
			else
			{
				*desc = format->fmt_desc[id];

				// fix UNICODE_FSS wrong length used in system tables
				jrd_rel* relation = csb->csb_rpt[stream].csb_relation;

				if (relation && (relation->rel_flags & REL_system) &&
					desc->isText() && desc->getCharSet() == CS_UNICODE_FSS)
				{
					USHORT adjust = 0;

					if (desc->dsc_dtype == dtype_varying)
						adjust = sizeof(USHORT);
					else if (desc->dsc_dtype == dtype_cstring)
						adjust = 1;

					desc->dsc_length -= adjust;
					desc->dsc_length *= 3;
					desc->dsc_length += adjust;
				}
			}
			return;
		}

	case nod_scalar:
		{
			jrd_nod* sub = node->nod_arg[e_scl_field];
			jrd_rel* relation = csb->csb_rpt[(USHORT)(IPTR) sub->nod_arg[e_fld_stream]].csb_relation;
			const USHORT id = (USHORT)(IPTR) sub->nod_arg[e_fld_id];
			const jrd_fld* field = MET_get_field(relation, id);
			const ArrayField* array;
			if (!field || !(array = field->fld_array)) {
				IBERROR(223);	// msg 223 argument of scalar operation must be an array
			}
			*desc = array->arr_desc.iad_rpt[0].iad_desc;
			if (array->arr_desc.iad_dimensions > MAX_ARRAY_DIMENSIONS)
				IBERROR(306); // Found array data type with more than 16 dimensions
			return;
		}

	case nod_divide:
		{
			DSC desc1, desc2;
			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc1);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &desc2);
			// for compatibility with older versions of the product, we accept
			// text types for division in blr_version4 (dialect <= 1) only
			if (!(DTYPE_IS_NUMERIC(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc1.dsc_dtype)))
			{
				if (desc1.dsc_dtype != dtype_unknown) {
					break;		// error, dtype not supported by arithmetic
				}
			}
			if (!(DTYPE_IS_NUMERIC(desc2.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype)))
			{
				if (desc2.dsc_dtype != dtype_unknown) {
					break;		// error, dtype not supported by arithmetic
				}
			}
		}
		desc->dsc_dtype = DEFAULT_DOUBLE;
		desc->dsc_length = sizeof(double);
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_agg_average:
	case nod_agg_average_distinct:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		// FALL INTO
	case nod_average:
		if (node->nod_type == nod_average) {
			CMP_get_desc(tdbb, csb, node->nod_arg[e_stat_value], desc);
		}
		if (!(DTYPE_IS_NUMERIC(desc->dsc_dtype) || DTYPE_IS_TEXT(desc->dsc_dtype)))
		{
			if (desc->dsc_dtype != dtype_unknown) {
				break;
			}
		}
		desc->dsc_dtype = DEFAULT_DOUBLE;
		desc->dsc_length = sizeof(double);
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

		// In 6.0, the AVERAGE of an exact numeric type is int64 with the
		// same scale. Only AVERAGE on an approximate numeric type can
		// return a double.

	case nod_agg_average2:
	case nod_agg_average_distinct2:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		// In V6, the average of an exact type is computed in SINT64,
		// rather than double as in prior releases
		switch (dtype = desc->dsc_dtype)
		{
		case dtype_short:
		case dtype_long:
		case dtype_int64:
			desc->dsc_dtype = dtype_int64;
			desc->dsc_length = sizeof(SINT64);
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			node->nod_scale = desc->dsc_scale;
			return;

		case dtype_unknown:
			desc->dsc_dtype = dtype_unknown;
			desc->dsc_length = 0;
			desc->dsc_scale = 0;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			return;

		default:
			if (!DTYPE_IS_NUMERIC(desc->dsc_dtype)) {
				break;
			}
			desc->dsc_dtype = DEFAULT_DOUBLE;
			desc->dsc_length = sizeof(double);
			desc->dsc_scale = 0;
			desc->dsc_sub_type = 0;
			desc->dsc_flags = 0;
			node->nod_flags |= nod_double;
			return;
		}
		break;

	case nod_agg_list:
	case nod_agg_list_distinct:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		desc->makeBlob(desc->getBlobSubType(), desc->getTextType());
		return;

	case nod_add:
	case nod_subtract:
		{
			DSC desc1, desc2;

			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc1);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &desc2);

			/* 92/05/29 DAVES - don't understand why this is done for ONLY
			   dtype_text (eg: not dtype_cstring or dtype_varying) Doesn't
			   appear to hurt.

			   94/04/04 DAVES - NOW I understand it!  QLI will pass floating
			   point values to the engine as text.  All other numeric constants
			   it turns into either integers or longs (with scale). */

			USHORT dtype1 = desc1.dsc_dtype;
			if (dtype_int64 == dtype1) {
				dtype1 = dtype_double;
			}
			USHORT dtype2 = desc2.dsc_dtype;
			if (dtype_int64 == dtype2) {
				dtype2 = dtype_double;
			}

			if (dtype1 == dtype_text || dtype2 == dtype_text) {
				dtype = MAX(MAX(dtype1, dtype2), (UCHAR) DEFAULT_DOUBLE);
			}
			else {
				dtype = MAX(dtype1, dtype2);
			}

			switch (dtype)
			{
			case dtype_short:
				desc->dsc_dtype = dtype_long;
				desc->dsc_length = sizeof(SLONG);
				if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
				{
					desc->dsc_scale = 0;
				}
				else {
					desc->dsc_scale = MIN(desc1.dsc_scale, desc2.dsc_scale);
				}
				node->nod_scale = desc->dsc_scale;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_sql_date:
			case dtype_sql_time:
				if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
				{
					ERR_post(Arg::Gds(isc_expression_eval_err));
				}
				// FALL INTO

			case dtype_timestamp:
				node->nod_flags |= nod_date;

				fb_assert(DTYPE_IS_DATE(desc1.dsc_dtype) || DTYPE_IS_DATE(desc2.dsc_dtype));

				if (COULD_BE_DATE(desc1) && COULD_BE_DATE(desc2)) {
					if (node->nod_type == nod_subtract) {
						// <any date> - <any date>

						/* Legal permutations are:
						   <timestamp> - <timestamp>
						   <timestamp> - <date>
						   <date> - <date>
						   <date> - <timestamp>
						   <time> - <time>
						   <timestamp> - <string>
						   <string> - <timestamp>
						   <string> - <string>   */

						if (DTYPE_IS_TEXT(dtype1)) {
							dtype = dtype_timestamp;
						}
						else if (DTYPE_IS_TEXT(dtype2)) {
							dtype = dtype_timestamp;
						}
						else if (dtype1 == dtype2) {
							dtype = dtype1;
						}
						else if (dtype1 == dtype_timestamp && dtype2 == dtype_sql_date)
						{
							dtype = dtype_timestamp;
						}
						else if (dtype2 == dtype_timestamp && dtype1 == dtype_sql_date)
						{
							dtype = dtype_timestamp;
						}
						else {
							ERR_post(Arg::Gds(isc_expression_eval_err));
						}

						if (dtype == dtype_sql_date) {
							desc->dsc_dtype = dtype_long;
							desc->dsc_length = type_lengths[desc->dsc_dtype];
							desc->dsc_scale = 0;
							desc->dsc_sub_type = 0;
							desc->dsc_flags = 0;
						}
						else if (dtype == dtype_sql_time) {
							desc->dsc_dtype = dtype_long;
							desc->dsc_length = type_lengths[desc->dsc_dtype];
							desc->dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
							desc->dsc_sub_type = 0;
							desc->dsc_flags = 0;
						}
						else {
							fb_assert(dtype == dtype_timestamp);
							desc->dsc_dtype = DEFAULT_DOUBLE;
							desc->dsc_length = type_lengths[desc->dsc_dtype];
							desc->dsc_scale = 0;
							desc->dsc_sub_type = 0;
							desc->dsc_flags = 0;
						}
					}
					else if (IS_DATE_AND_TIME(desc1, desc2)) {
						// <date> + <time>
						// <time> + <date>
						desc->dsc_dtype = dtype_timestamp;
						desc->dsc_length = type_lengths[desc->dsc_dtype];
						desc->dsc_scale = 0;
						desc->dsc_sub_type = 0;
						desc->dsc_flags = 0;
					}
					else {
						// <date> + <date>
						ERR_post(Arg::Gds(isc_expression_eval_err));
					}
				}
				else if (DTYPE_IS_DATE(desc1.dsc_dtype) || (node->nod_type == nod_add))
				{
					// <date> +/- <non-date> || <non-date> + <date>
					desc->dsc_dtype = desc1.dsc_dtype;
					if (!DTYPE_IS_DATE(desc->dsc_dtype)) {
						desc->dsc_dtype = desc2.dsc_dtype;
					}
					fb_assert(DTYPE_IS_DATE(desc->dsc_dtype));
					desc->dsc_length = type_lengths[desc->dsc_dtype];
					desc->dsc_scale = 0;
					desc->dsc_sub_type = 0;
					desc->dsc_flags = 0;
				}
				else {
					// <non-date> - <date>
					ERR_post(Arg::Gds(isc_expression_eval_err));
				}
				return;

			case dtype_text:
			case dtype_cstring:
			case dtype_varying:
			case dtype_long:
			case dtype_real:
			case dtype_double:
				node->nod_flags |= nod_double;
				desc->dsc_dtype = DEFAULT_DOUBLE;
				desc->dsc_length = sizeof(double);
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_unknown:
				desc->dsc_dtype = dtype_unknown;
				desc->dsc_length = 0;
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_quad:
				node->nod_flags |= nod_quad;
				desc->dsc_dtype = dtype_quad;
				desc->dsc_length = sizeof(SQUAD);
				if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
				{
					desc->dsc_scale = 0;
				}
				else {
					desc->dsc_scale = MIN(desc1.dsc_scale, desc2.dsc_scale);
				}
				node->nod_scale = desc->dsc_scale;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
#ifdef NATIVE_QUAD
				return;
#endif
			default:
				fb_assert(false);
				// FALLINTO
			case dtype_blob:
			case dtype_array:
				break;
			}
		}
		break;

	case nod_gen_id2:
		desc->dsc_dtype = dtype_int64;
		desc->dsc_length = sizeof(SINT64);
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_add2:
	case nod_subtract2:
		{
			DSC desc1, desc2;

			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc1);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &desc2);
			USHORT dtype1 = desc1.dsc_dtype;
			USHORT dtype2 = desc2.dsc_dtype;

			// In Dialect 2 or 3, strings can never partipate in addition / sub
			// (use a specific cast instead)
			if (DTYPE_IS_TEXT(dtype1) || DTYPE_IS_TEXT(dtype2))
				ERR_post(Arg::Gds(isc_expression_eval_err));

			// Because dtype_int64 > dtype_double, we cannot just use the MAX macro to set
			// the result dtype. The rule is that two exact numeric operands yield an int64
			// result, while an approximate numeric and anything yield a double result.

			if (DTYPE_IS_EXACT(desc1.dsc_dtype) && DTYPE_IS_EXACT(desc2.dsc_dtype))
			{
				dtype = dtype_int64;
			}
			else if (DTYPE_IS_NUMERIC(desc1.dsc_dtype) && DTYPE_IS_NUMERIC(desc2.dsc_dtype))
			{
				dtype = dtype_double;
			}
			else
			{
				// mixed numeric and non-numeric:

				fb_assert(COULD_BE_DATE(desc1) || COULD_BE_DATE(desc2));

				// the MAX(dtype) rule doesn't apply with dtype_int64

				if (dtype_int64 == dtype1) {
					dtype1 = dtype_double;
				}
				if (dtype_int64 == dtype2) {
					dtype2 = dtype_double;
				}

				dtype = MAX(dtype1, dtype2);
			}

			switch (dtype)
			{
			case dtype_timestamp:
			case dtype_sql_date:
			case dtype_sql_time:
				node->nod_flags |= nod_date;

				fb_assert(DTYPE_IS_DATE(desc1.dsc_dtype) || DTYPE_IS_DATE(desc2.dsc_dtype));

				if ((DTYPE_IS_DATE(dtype1) || (dtype1 == dtype_unknown)) &&
					(DTYPE_IS_DATE(dtype2) || (dtype2 == dtype_unknown)))
				{
					if (node->nod_type == nod_subtract2) {
						// <any date> - <any date>

						/* Legal permutations are:
						   <timestamp> - <timestamp>
						   <timestamp> - <date>
						   <date> - <date>
						   <date> - <timestamp>
						   <time> - <time> */

						if (dtype1 == dtype_unknown) {
							dtype1 = dtype2;
						}
						else if (dtype2 == dtype_unknown) {
							dtype2 = dtype1;
						}
						if (dtype1 == dtype2) {
							dtype = dtype1;
						}
						else if ((dtype1 == dtype_timestamp) && (dtype2 == dtype_sql_date))
						{
							dtype = dtype_timestamp;
						}
						else if ((dtype2 == dtype_timestamp) && (dtype1 == dtype_sql_date))
						{
							dtype = dtype_timestamp;
						}
						else {
							ERR_post(Arg::Gds(isc_expression_eval_err));
						}

						if (dtype == dtype_sql_date) {
							desc->dsc_dtype = dtype_long;
							desc->dsc_length = type_lengths[desc->dsc_dtype];
							desc->dsc_scale = 0;
							desc->dsc_sub_type = 0;
							desc->dsc_flags = 0;
						}
						else if (dtype == dtype_sql_time) {
							desc->dsc_dtype = dtype_long;
							desc->dsc_length = type_lengths[desc->dsc_dtype];
							desc->dsc_scale = ISC_TIME_SECONDS_PRECISION_SCALE;
							desc->dsc_sub_type = 0;
							desc->dsc_flags = 0;
						}
						else {
							fb_assert(dtype == dtype_timestamp || dtype == dtype_unknown);
							desc->dsc_dtype = DEFAULT_DOUBLE;
							desc->dsc_length = type_lengths[desc->dsc_dtype];
							desc->dsc_scale = 0;
							desc->dsc_sub_type = 0;
							desc->dsc_flags = 0;
						}
					}
					else if (IS_DATE_AND_TIME(desc1, desc2)) {
						// <date> + <time>
						// <time> + <date>
						desc->dsc_dtype = dtype_timestamp;
						desc->dsc_length = type_lengths[desc->dsc_dtype];
						desc->dsc_scale = 0;
						desc->dsc_sub_type = 0;
						desc->dsc_flags = 0;
					}
					else {
						// <date> + <date>
						ERR_post(Arg::Gds(isc_expression_eval_err));
					}
				}
				else if (DTYPE_IS_DATE(desc1.dsc_dtype) || (node->nod_type == nod_add2))

				{
					// <date> +/- <non-date> || <non-date> + <date>
					desc->dsc_dtype = desc1.dsc_dtype;
					if (!DTYPE_IS_DATE(desc->dsc_dtype)) {
						desc->dsc_dtype = desc2.dsc_dtype;
					}
					fb_assert(DTYPE_IS_DATE(desc->dsc_dtype));
					desc->dsc_length = type_lengths[desc->dsc_dtype];
					desc->dsc_scale = 0;
					desc->dsc_sub_type = 0;
					desc->dsc_flags = 0;
				}
				else {
					// <non-date> - <date>
					ERR_post(Arg::Gds(isc_expression_eval_err));
				}
				return;

			case dtype_text:
			case dtype_cstring:
			case dtype_varying:
			case dtype_real:
			case dtype_double:
				node->nod_flags |= nod_double;
				desc->dsc_dtype = DEFAULT_DOUBLE;
				desc->dsc_length = sizeof(double);
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_short:
			case dtype_long:
			case dtype_int64:
				desc->dsc_dtype = dtype_int64;
				desc->dsc_length = sizeof(SINT64);
				if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
				{
					desc->dsc_scale = 0;
				}
				else {
					desc->dsc_scale = MIN(desc1.dsc_scale, desc2.dsc_scale);
				}
				node->nod_scale = desc->dsc_scale;
				desc->dsc_sub_type = MAX(desc1.dsc_sub_type, desc2.dsc_sub_type);
				desc->dsc_flags = 0;
				return;

			case dtype_unknown:
				desc->dsc_dtype = dtype_unknown;
				desc->dsc_length = 0;
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_quad:
				node->nod_flags |= nod_quad;
				desc->dsc_dtype = dtype_quad;
				desc->dsc_length = sizeof(SQUAD);
				if (DTYPE_IS_TEXT(desc1.dsc_dtype) || DTYPE_IS_TEXT(desc2.dsc_dtype))
				{
					desc->dsc_scale = 0;
				}
				else {
					desc->dsc_scale = MIN(desc1.dsc_scale, desc2.dsc_scale);
				}
				node->nod_scale = desc->dsc_scale;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
#ifdef NATIVE_QUAD
				return;
#endif
			default:
				fb_assert(false);
				// FALLINTO
			case dtype_blob:
			case dtype_array:
				break;
			}
		}
		break;

	case nod_multiply:
		{
			DSC desc1, desc2;

			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc1);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &desc2);
			dtype = DSC_multiply_blr4_result[desc1.dsc_dtype][desc2.dsc_dtype];

			switch (dtype)
			{
			case dtype_long:
				desc->dsc_dtype = dtype_long;
				desc->dsc_length = sizeof(SLONG);
				desc->dsc_scale = node->nod_scale = NUMERIC_SCALE(desc1) + NUMERIC_SCALE(desc2);
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_double:
				node->nod_flags |= nod_double;
				desc->dsc_dtype = DEFAULT_DOUBLE;
				desc->dsc_length = sizeof(double);
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_unknown:
				desc->dsc_dtype = dtype_unknown;
				desc->dsc_length = 0;
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			default:
				fb_assert(false);
				// FALLINTO
			case DTYPE_CANNOT:
				// break to error reporting code
				break;
			}
		}
		break;

	case nod_multiply2:
	case nod_divide2:
		{
			DSC desc1, desc2;

			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc1);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &desc2);
			dtype = DSC_multiply_result[desc1.dsc_dtype][desc2.dsc_dtype];

			switch (dtype)
			{
			case dtype_double:
				node->nod_flags |= nod_double;
				desc->dsc_dtype = DEFAULT_DOUBLE;
				desc->dsc_length = sizeof(double);
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			case dtype_int64:
				desc->dsc_dtype = dtype_int64;
				desc->dsc_length = sizeof(SINT64);
				desc->dsc_scale = node->nod_scale = NUMERIC_SCALE(desc1) + NUMERIC_SCALE(desc2);
				desc->dsc_sub_type = MAX(desc1.dsc_sub_type, desc2.dsc_sub_type);
				desc->dsc_flags = 0;
				return;

			case dtype_unknown:
				desc->dsc_dtype = dtype_unknown;
				desc->dsc_length = 0;
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
				return;

			default:
				fb_assert(false);
				// FALLINTO
			case DTYPE_CANNOT:
				// break to error reporting code
				break;
			}
		}
		break;

	case nod_concatenate:
		{
			DSC desc1, desc2;
			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc1);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &desc2);
			DataTypeUtil(tdbb).makeConcatenate(desc, &desc1, &desc2);
			return;
		}

	case nod_upcase:
	case nod_lowcase:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		if (desc->dsc_dtype > dtype_varying && desc->dsc_dtype != dtype_blob) {
			desc->dsc_length = DSC_convert_to_text_length(desc->dsc_dtype);
			desc->dsc_dtype = dtype_text;
			desc->dsc_ttype() = ttype_ascii;
			desc->dsc_scale = 0;
			desc->dsc_flags = 0;
		}
		return;

	case nod_dbkey:
		desc->dsc_dtype = dtype_dbkey;
		desc->dsc_length = type_lengths[dtype_dbkey];
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		return;

	case nod_rec_version:
		desc->dsc_dtype = dtype_text;
		desc->dsc_ttype() = ttype_binary;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		return;

	case nod_current_time:
		desc->dsc_dtype = dtype_sql_time;
		desc->dsc_length = type_lengths[desc->dsc_dtype];
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_current_timestamp:
		desc->dsc_dtype = dtype_timestamp;
		desc->dsc_length = type_lengths[desc->dsc_dtype];
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_current_date:
		desc->dsc_dtype = dtype_sql_date;
		desc->dsc_length = type_lengths[desc->dsc_dtype];
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_user_name:
    case nod_current_role:
		desc->dsc_dtype = dtype_text;
		desc->dsc_ttype() = ttype_metadata;
		// ASF: should be multiplied by maxBytesPerChar of ttype_metadata in the future
		desc->dsc_length = USERNAME_LENGTH;
		desc->dsc_scale = 0;
		desc->dsc_flags = 0;
		return;

	case nod_internal_info:
		{
			jrd_nod* const arg_node = node->nod_arg[0];
			fb_assert(arg_node->nod_type == nod_literal);

			dsc arg_desc;
			CMP_get_desc(tdbb, csb, arg_node, &arg_desc);
			fb_assert(arg_desc.dsc_dtype == dtype_long);

			const internal_info_id id = *reinterpret_cast<internal_info_id*>(arg_desc.dsc_address);

			if (id == internal_sqlstate)
			{
				desc->makeText(FB_SQLSTATE_LENGTH, ttype_ascii);
			}
			else
			{
				desc->makeLong(0);
			}
		}
		return;

	case nod_extract:
		switch ((IPTR) node->nod_arg[e_extract_part])
		{
			case blr_extract_second:
				// QUADDATE - SECOND returns a float, or scaled!
				desc->makeLong(ISC_TIME_SECONDS_PRECISION_SCALE);
				break;

			case blr_extract_millisecond:
				desc->makeLong(0);
				break;

			default:
				desc->makeShort(0);
				break;
		}
		return;

	case nod_strlen:
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(ULONG);
		desc->dsc_scale = 0;
		desc->dsc_sub_type = 0;
		desc->dsc_flags = 0;
		return;

	case nod_agg_min:
	case nod_agg_max:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		return;

	case nod_negate:
		CMP_get_desc(tdbb, csb, node->nod_arg[0], desc);
		node->nod_flags = node->nod_arg[0]->nod_flags & (nod_double | nod_quad);
		return;

	case nod_literal:
		*desc = ((Literal*) node)->lit_desc;

		// ASF: I expect only dtype_text could occur here.
		// But I'll treat all string types for sure.
		if (DTYPE_IS_TEXT(desc->dsc_dtype))
		{
			const UCHAR* p;
			USHORT adjust = 0;

			if (desc->dsc_dtype == dtype_varying)
			{
				p = desc->dsc_address + sizeof(USHORT);
				adjust = sizeof(USHORT);
			}
			else
			{
				p = desc->dsc_address;

				if (desc->dsc_dtype == dtype_cstring)
					adjust = 1;
			}

			// Do the same thing which DSQL does.
			// Increase descriptor size to evaluate dependent expressions correctly.
			CharSet* cs = INTL_charset_lookup(tdbb, desc->getCharSet());
			desc->dsc_length = (cs->length(desc->dsc_length - adjust, p, true) *
				cs->maxBytesPerChar()) + adjust;
		}
		return;

	case nod_cast:
		{
			const Format* format = (Format*) node->nod_arg[e_cast_fmt];
			*desc = format->fmt_desc[0];
			if ((desc->dsc_dtype <= dtype_any_text && !desc->dsc_length) ||
				(desc->dsc_dtype == dtype_varying && desc->dsc_length <= sizeof(USHORT)))
			{
			    dsc desc1;
				CMP_get_desc(tdbb, csb, node->nod_arg[e_cast_source], &desc1);
				desc->dsc_length = DSC_string_length(&desc1);
				if (desc->dsc_dtype == dtype_cstring) {
					desc->dsc_length++;
				}
				else if (desc->dsc_dtype == dtype_varying) {
					desc->dsc_length += sizeof(USHORT);
				}
			}
			return;
		}

	case nod_argument:
		{
			const jrd_nod* message = node->nod_arg[e_arg_message];
			const Format* format = (Format*) message->nod_arg[e_msg_format];
			*desc = format->fmt_desc[(IPTR) node->nod_arg[e_arg_number]];
			return;
		}

	case nod_substr:
		{
			DSC desc0, desc1, desc2, desc3;

			CMP_get_desc(tdbb, csb, node->nod_arg[0], &desc0);

			jrd_nod* offset_node = node->nod_arg[1];
			jrd_nod* decrement_node = NULL;
			if (offset_node->nod_type == nod_subtract2)
			{
				// This node is created by the DSQL layer, but the
				// system BLR code bypasses it and uses zero-based
				// string offsets instead
				decrement_node = offset_node->nod_arg[1];
				CMP_get_desc(tdbb, csb, decrement_node, &desc3);
				offset_node = offset_node->nod_arg[0];
			}
			CMP_get_desc(tdbb, csb, offset_node, &desc1);

			jrd_nod* length_node = node->nod_arg[2];
			CMP_get_desc(tdbb, csb, length_node, &desc2);

			DataTypeUtil(tdbb).makeSubstr(desc, &desc0, &desc1, &desc2);

			if (desc1.dsc_flags & DSC_null || desc2.dsc_flags & DSC_null)
			{
				desc->dsc_flags |= DSC_null;
			}
			else
			{
				if (offset_node->nod_type == nod_literal && desc1.dsc_dtype == dtype_long)
				{
					SLONG offset = MOV_get_long(&desc1, 0);
					if (decrement_node && decrement_node->nod_type == nod_literal &&
						desc3.dsc_dtype == dtype_long)
					{
						offset -= MOV_get_long(&desc3, 0);
					}
					// error() is a local routine in par.cpp, so we use plain ERR_post
					if (offset < 0)
					{
						ERR_post(Arg::Gds(isc_bad_substring_offset) << Arg::Num(offset + 1));
					}
				}
				if (length_node->nod_type == nod_literal && desc2.dsc_dtype == dtype_long)
				{
					const SLONG length = MOV_get_long(&desc2, 0);
					// error() is a local routine in par.cpp, so we use plain ERR_post
					if (length < 0)
					{
						ERR_post(Arg::Gds(isc_bad_substring_length) << Arg::Num(length));
					}
				}
			}

			return;
		}

	case nod_trim:
		CMP_get_desc(tdbb, csb, node->nod_arg[e_trim_value], desc);

		if (node->nod_arg[e_trim_characters])
		{
			DSC desc1;
			CMP_get_desc(tdbb, csb, node->nod_arg[e_trim_characters], &desc1);
			desc->dsc_flags |= desc1.dsc_flags & DSC_null;
		}

		if (desc->dsc_dtype != dtype_blob)
		{
			USHORT length = DSC_string_length(desc);

			if (!DTYPE_IS_TEXT(desc->dsc_dtype))
			{
				desc->dsc_ttype() = ttype_ascii;
				desc->dsc_scale = 0;
			}

			desc->dsc_dtype = dtype_varying;
			desc->dsc_length = length + sizeof(USHORT);
		}

		return;

	case nod_function:
		{
			const UserFunction* function = (UserFunction*) node->nod_arg[e_fun_function];
			// Null value for the function indicates that the function was not
			// looked up during parsing the blr. This is true if the function
			// referenced in the procedure blr was dropped before dropping the
			// procedure itself. Ignore the case because we are currently trying
			// to drop the procedure.
			// For normal requests, function would never be null. We would have
			// created a valid block while parsing in par_function/par.c.
			if (function) {
				*desc = function->fun_rpt[function->fun_return_arg].fun_desc;
			}
			else {
				/* Note that CMP_get_desc is always called with a pre-allocated DSC, i.e:
					   DSC desc;
					   CMP_get_desc (.... &desc);
				   Hence the code:
					   *desc = NULL;
				   will not work. What I've done is memset the structure to zero. */
				MOVE_CLEAR(desc, sizeof(DSC));
			}
			return;
		}

	case nod_sys_function:
		{
			jrd_nod* nodeArgs = node->nod_arg[e_sysfun_args];
			fb_assert(nodeArgs->nod_type == nod_list);

			Firebird::Array<dsc*> args;

			for (jrd_nod** p = nodeArgs->nod_arg; p < nodeArgs->nod_arg + nodeArgs->nod_count; ++p)
			{
				dsc* targetDesc = FB_NEW(*tdbb->getDefaultPool()) dsc();
				args.push(targetDesc);
				CMP_get_desc(tdbb, csb, *p, targetDesc);

				// dsc_address is verified in makeFunc to get literals. If the node is not a
				// literal, set it to NULL, to prevent wrong interpretation of offsets as
				// pointers - CORE-2612.
				if ((*p)->nod_type != nod_literal)
					targetDesc->dsc_address = NULL;
			}

			DataTypeUtil dataTypeUtil(tdbb);
			SysFunction* function = ((SysFunction*) node->nod_arg[e_sysfun_function]);

			function->makeFunc(&dataTypeUtil, function,
				desc, args.getCount(), const_cast<const dsc**>(args.begin()));

			for (dsc** pArgs = args.begin(); pArgs != args.end(); ++pArgs)
				delete *pArgs;
		}
		return;

	case nod_variable:
		{
			const jrd_nod* value = node->nod_arg[e_var_variable];
			*desc = *(DSC*) (value->nod_arg + e_dcl_desc);
			return;
		}

	case nod_value_if:
		CMP_get_desc(tdbb, csb, node->nod_arg[1]->nod_type != nod_null ? 
								node->nod_arg[1] : node->nod_arg[2], desc);
		return;

	case nod_domain_validation:
		*desc = *(DSC*) (node->nod_arg + e_domval_desc);
		return;

	case nod_stmt_expr:
		CMP_get_desc(tdbb, csb, node->nod_arg[e_stmt_expr_expr], desc);
		return;

	case nod_derived_expr:
		CMP_get_desc(tdbb, csb, node->nod_arg[e_derived_expr_expr], desc);
		return;

	default:
		fb_assert(false);
		break;
	}

	if (dtype == dtype_quad)
		IBERROR(224);				// msg 224 quad word arithmetic not supported

	ERR_post(Arg::Gds(isc_datype_notsup));	// data type not supported for arithmetic
}


IndexLock* CMP_get_index_lock(thread_db* tdbb, jrd_rel* relation, USHORT id)
{
/**************************************
 *
 *	C M P _ g e t _ i n d e x _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Get index lock block for index.  If one doesn't exist,
 *	make one.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	DEV_BLKCHK(relation, type_rel);

	if (relation->rel_id <= dbb->dbb_max_sys_rel) {
		return NULL;
	}

	// for to find an existing block

	for (IndexLock* index = relation->rel_index_locks; index; index = index->idl_next) {
		if (index->idl_id == id) {
			return index;
		}
	}

	IndexLock* index = FB_NEW(*dbb->dbb_permanent) IndexLock();
	index->idl_next = relation->rel_index_locks;
	relation->rel_index_locks = index;
	index->idl_relation = relation;
	index->idl_id = id;
	index->idl_count = 0;

	Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0) Lock;
	index->idl_lock = lock;
	lock->lck_parent = dbb->dbb_lock;
	lock->lck_dbb = dbb;
	lock->lck_key.lck_long = (relation->rel_id << 16) | id;
	lock->lck_length = sizeof(lock->lck_key.lck_long);
	lock->lck_type = LCK_idx_exist;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);

	return index;
}


SLONG CMP_impure(CompilerScratch* csb, USHORT size)
{
/**************************************
 *
 *	C M P _ i m p u r e
 *
 **************************************
 *
 * Functional description
 *	Allocate space (offset) in request.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);

	if (!csb) {
		return 0;
	}

	const SLONG offset = FB_ALIGN(csb->csb_impure, FB_ALIGNMENT);
	csb->csb_impure = offset + size;

	return offset;
}


jrd_req* CMP_make_request(thread_db* tdbb, CompilerScratch* csb, bool internal_flag)
{
/**************************************
 *
 *	C M P _ m a k e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Turn a parsed request into an executable request.
 *
 **************************************/
	jrd_req* request = NULL;

	DEV_BLKCHK(csb, type_csb);

	SET_TDBB(tdbb);

	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);
	Attachment* const attachment = tdbb->getAttachment();

	jrd_req* const old_request = tdbb->getRequest();
	tdbb->setRequest(NULL);

	try {

	// Once any expansion required has been done, make a pass to assign offsets
	// into the impure area and throw away any unnecessary crude. Execution
	// optimizations can be performed here.

	DEBUG;
	csb->csb_node = CMP_pass1(tdbb, csb, csb->csb_node);

	// Copy and compile (pass1) domains DEFAULT and constraints.
	MapFieldInfo::Accessor accessor(&csb->csb_map_field_info);

	for (bool found = accessor.getFirst(); found; found = accessor.getNext())
	{
		FieldInfo& fieldInfo = accessor.current()->second;
		UCHAR local_map[MAP_LENGTH];

		AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
			(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);

		fieldInfo.defaultValue = copy(tdbb, csb, fieldInfo.defaultValue, local_map, 0, NULL, false);

		csb->csb_remap_variable = (csb->csb_variables ? csb->csb_variables->count() : 0) + 1;

		fieldInfo.validation = copy(tdbb, csb, fieldInfo.validation, local_map, 0, NULL, false);

		fieldInfo.defaultValue = CMP_pass1(tdbb, csb, fieldInfo.defaultValue);
		fieldInfo.validation = CMP_pass1(tdbb, csb, fieldInfo.validation);
	}

	csb->csb_impure = REQ_SIZE + REQ_TAIL * MAX(csb->csb_n_stream, 1);
	csb->csb_exec_sta.clear();

	csb->csb_node = CMP_pass2(tdbb, csb, csb->csb_node, 0);

	// Compile (pass2) domains DEFAULT and constraints
	for (bool found = accessor.getFirst(); found; found = accessor.getNext())
	{
		FieldInfo& fieldInfo = accessor.current()->second;

		fieldInfo.defaultValue = CMP_pass2(tdbb, csb, fieldInfo.defaultValue, 0);
		fieldInfo.validation = CMP_pass2(tdbb, csb, fieldInfo.validation, 0);
	}

	if (csb->csb_impure > MAX_REQUEST_SIZE) {
		IBERROR(226);			// msg 226 request size limit exceeded
	}

	// Build the final request block. First, compute the "effective" repeat
	// count of hold the impure areas.

	MemoryPool* const pool = tdbb->getDefaultPool();
	const size_t n = (csb->csb_impure - REQ_SIZE + REQ_TAIL - 1) / REQ_TAIL;
	Firebird::MemoryStats* const parent_stats =
		internal_flag ? &dbb->dbb_memory_stats : &attachment->att_memory_stats;
	request = FB_NEW_RPT(*pool, n) jrd_req(pool, parent_stats);
	request->req_count = csb->csb_n_stream;
	request->req_impure_size = csb->csb_impure;
	request->req_top_node = csb->csb_node;
	request->req_access = csb->csb_access;
	request->req_external = csb->csb_external;
	request->req_map_field_info.takeOwnership(csb->csb_map_field_info);
	request->req_id = dbb->generateStatementId(tdbb);

	// CVC: Unused.
	//request->req_variables = csb->csb_variables;
	request->req_resources = csb->csb_resources; // Assign array contents
	if (csb->csb_g_flags & csb_blr_version4) {
		request->req_flags |= req_blr_version4;
	}

#ifdef SCROLLABLE_CURSORS
	request->req_async_message = csb->csb_async_message;
#endif

	// Take out existence locks on resources used in request. This is
	// a little complicated since relation locks MUST be taken before
	// index locks.

	for (Resource* resource = request->req_resources.begin();
		resource < request->req_resources.end(); resource++)
	{
		switch (resource->rsc_type)
		{
		case Resource::rsc_relation:
			{
				jrd_rel* relation = resource->rsc_rel;
				MET_post_existence(tdbb, relation);
				break;
			}
		case Resource::rsc_index:
			{
				jrd_rel* relation = resource->rsc_rel;
				IndexLock* index = CMP_get_index_lock(tdbb, relation, resource->rsc_id);
				if (index)
				{
					++index->idl_count;
					if (index->idl_count == 1) {
						LCK_lock(tdbb, index->idl_lock, LCK_SR, LCK_WAIT);
					}
				}
				break;
			}
		case Resource::rsc_procedure:
			{
				jrd_prc* procedure = resource->rsc_prc;
				procedure->prc_use_count++;
#ifdef DEBUG_PROCS
				{
					char buffer[256];
					sprintf(buffer,
							"Called from CMP_make_request():\n\t Incrementing use count of %s\n",
							procedure->prc_name->c_str());
					JRD_print_procedure_info(tdbb, buffer);
				}
#endif
				break;
			}
		case Resource::rsc_collation:
			{
				Collation* coll = resource->rsc_coll;
				coll->incUseCount(tdbb);
				break;
			}
		default:
			BUGCHECK(219);		// msg 219 request of unknown resource
		}
	}

    CompilerScratch::csb_repeat* tail = csb->csb_rpt.begin();
	const CompilerScratch::csb_repeat* const streams_end = tail + csb->csb_n_stream;
	DEBUG;

	for (record_param* rpb = request->req_rpb; tail < streams_end; rpb++, tail++)
	{
		// fetch input stream for update if all booleans matched against indices

		if (tail->csb_flags & csb_update && !(tail->csb_flags & csb_unmatched))
		{
			 rpb->rpb_stream_flags |= RPB_s_update;
		}

		// if no fields are referenced and this stream is not intended for update,
		// mark the stream as not requiring record's data

		if (!tail->csb_fields && !(tail->csb_flags & csb_update))
		{
			 rpb->rpb_stream_flags |= RPB_s_no_data;
		}

		rpb->rpb_relation = tail->csb_relation;

		delete tail->csb_fields;
		tail->csb_fields = NULL;
	}

	// make a vector of all used RSEs
	request->req_fors = csb->csb_fors;

	// make a vector of all used ExecuteStatements into
	request->req_exec_sta = csb->csb_exec_sta;

	// make a vector of all invariant-type nodes, so that we will
	// be able to easily reinitialize them when we restart the request

	request->req_invariants = csb->csb_invariants;

	DEBUG;
	tdbb->setRequest(old_request);

	} // try
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		tdbb->setRequest(old_request);
		ERR_punt();
	}

	if (!internal_flag)
	{
		tdbb->bumpStats(RuntimeStatistics::STMT_PREPARES);
	}

	return request;
}


void CMP_post_access(thread_db* tdbb,
					 CompilerScratch* csb,
					 const Firebird::MetaName& security_name,
					 SLONG view_id,
					 SecurityClass::flags_t mask,
					 const TEXT* type_name,
					 const Firebird::MetaName& name,
					 const Firebird::MetaName& r_name)
{
/**************************************
 *
 *	C M P _ p o s t _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *	Post access to security class to request.
 *      We append the new security class to the existing list of
 *      security classes for that request.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(view, type_rel);

	// allow all access to internal requests

	if (csb->csb_g_flags & (csb_internal | csb_ignore_perm))
		return;

	SET_TDBB(tdbb);

	AccessItem access(security_name, view_id, name, type_name, mask, r_name);

	size_t i;

	if (!csb->csb_access.find(access, i))
	{
		csb->csb_access.insert(i, access);
	}
}


void CMP_post_resource(	ResourceList* rsc_ptr,
						void* obj,
						enum Resource::rsc_s type,
						USHORT id)
{
/**************************************
 *
 *	C M P _ p o s t _ r e s o u r c e
 *
 **************************************
 *
 * Functional description
 *	Post a resource usage to the compiler scratch block.
 *
 **************************************/
	// Initialize resource block
	Resource resource(type, id, NULL, NULL, NULL);
	switch (type)
	{
	case Resource::rsc_relation:
	case Resource::rsc_index:
		resource.rsc_rel = (jrd_rel*) obj;
		break;
	case Resource::rsc_procedure:
		resource.rsc_prc = (jrd_prc*) obj;
		break;
	case Resource::rsc_collation:
		resource.rsc_coll = (Collation*) obj;
		break;
	default:
		BUGCHECK(220);			/* msg 220 unknown resource */
		break;
	}

	// Add it into list if not present already
	size_t pos;
	if (!rsc_ptr->find(resource, pos))
		rsc_ptr->insert(pos, resource);
}


void CMP_decrement_prc_use_count(thread_db* tdbb, jrd_prc* procedure)
{
/*********************************************
 *
 *	C M P _ d e c r e m e n t _ p r c _ u s e _ c o u n t
 *
 *********************************************
 *
 * Functional description
 *	decrement the procedure's use count
 *
 *********************************************/
	DEV_BLKCHK(procedure, type_prc);

	// Actually, it's possible for procedures to have intermixed dependencies, so
	// this routine can be called for the procedure which is being freed itself.
	// Hence we should just silently ignore such a situation.

	if (!procedure->prc_use_count)
		return;

	if (procedure->prc_int_use_count > 0)
		procedure->prc_int_use_count--;

	--procedure->prc_use_count;

#ifdef DEBUG_PROCS
	{
		char buffer[256];
		sprintf(buffer,
				"Called from CMP_decrement():\n\t Decrementing use count of %s\n",
				procedure->prc_name->c_str());
		JRD_print_procedure_info(tdbb, buffer);
	}
#endif

	// Call recursively if and only if the use count is zero AND the procedure
	// in dbb_procedures is different than this procedure.
	// The procedure will be different than in dbb_procedures only if it is a
	// floating copy, i.e. an old copy or a deleted procedure.
	if ((procedure->prc_use_count == 0) &&
		( (*tdbb->getDatabase()->dbb_procedures)[procedure->prc_id] != procedure)) // &procedure->prc_header))
	{
		if (procedure->prc_request) {
			MET_release_procedure_request(tdbb, procedure);
		}
		procedure->prc_flags &= ~PRC_being_altered;
		MET_remove_procedure(tdbb, procedure->prc_id, procedure);
	}
}


void CMP_release(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	C M P _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release an unneeded and unloved request.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	DEV_BLKCHK(request, type_req);

	// release existence locks on references

	Attachment* attachment = request->req_attachment;
	if (!attachment || !(attachment->att_flags & ATT_shutdown))
	{
		for (Resource* resource = request->req_resources.begin();
			 resource < request->req_resources.end(); resource++)
		{
			switch (resource->rsc_type)
			{
			case Resource::rsc_relation:
				{
					jrd_rel* relation = resource->rsc_rel;
					MET_release_existence(tdbb, relation);
					break;
				}
			case Resource::rsc_index:
				{
					jrd_rel* relation = resource->rsc_rel;
					IndexLock* index = CMP_get_index_lock(tdbb, relation, resource->rsc_id);
					if (index && index->idl_count)
					{
						--index->idl_count;
						if (!index->idl_count)
							LCK_release(tdbb, index->idl_lock);
					}
					break;
				}
			case Resource::rsc_procedure:
				{
					CMP_decrement_prc_use_count(tdbb, resource->rsc_prc);
					break;
				}
			case Resource::rsc_collation:
				{
					Collation* coll = resource->rsc_coll;
					coll->decUseCount(tdbb);
					break;
				}
			default:
				BUGCHECK(220);	// msg 220 release of unknown resource
				break;
			}
		}
	}

	vec<jrd_req*>* vector = request->req_sub_requests;
	if (vector)
	{
		vec<jrd_req*>::const_iterator sub_req = vector->begin();
		vec<jrd_req*>::const_iterator end = vector->end();
		for (; sub_req < end; ++sub_req)
		{
			if (*sub_req)
				EXE_unwind(tdbb, *sub_req);
		}
	}

	EXE_unwind(tdbb, request);

	if (request->req_attachment) {
		for (jrd_req** next = &request->req_attachment->att_requests;
			*next; next = &(*next)->req_request)
		{
			if (*next == request) {
				*next = request->req_request;
#ifdef DEV_BUILD
				// Once I've seen att_requests == 0x00000014,
				// so some debugging code added to catch it earlier in dev_builds.
				// This place is one of two, where att_requests modified.
				// In another one (jrd.cpp/GDS_COMPILE()), it's value is used
				// right before pointer assignment. So make some use of pointer here
				// to try to detect false in it earlier ...
				if (*next) {
					jrd_req* req = (*next)->req_request;
					req++;
				}
#endif
				break;
			}
		}
	}

	request->req_sql_text = NULL;

	// We have to call the destructor explicitly because of
	// the delete-by-pool cleanup practice for requests
	request->req_sorts.~SortOwner();

	dbb->deletePool(request->req_pool);
}


void CMP_shutdown_database(thread_db* tdbb)
{
/**************************************
 *
 *	C M P _ s h u t d o w n _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Release compile-time locks for database.
 *	Since this can be called at AST level, don't
 *	release any data structures.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	DEV_BLKCHK(dbb, type_dbb);

	// go through relations and indices and release
	// all existence locks that might have been taken

	vec<jrd_rel*>* rvector = dbb->dbb_relations;
	if (rvector)
	{
		vec<jrd_rel*>::iterator ptr, end;
		for (ptr = rvector->begin(), end = rvector->end(); ptr < end; ++ptr)
		{
			jrd_rel* relation = *ptr;
			if (relation)
			{
				if (relation->rel_existence_lock)
				{
					LCK_release(tdbb, relation->rel_existence_lock);
					relation->rel_flags |= REL_check_existence;
					relation->rel_use_count = 0;
				}
				if (relation->rel_partners_lock)
				{
					LCK_release(tdbb, relation->rel_partners_lock);
					relation->rel_flags |= REL_check_partners;
				}
				for (IndexLock* index = relation->rel_index_locks; index; index = index->idl_next)
				{
					if (index->idl_lock) {
						index->idl_count = 0;
						LCK_release(tdbb, index->idl_lock);
					}
				}
			}
		}
	}

	// release all procedure existence locks that might have been taken

	vec<jrd_prc*>* pvector = dbb->dbb_procedures;
	if (pvector)
	{
		vec<jrd_prc*>::iterator pptr, pend;
		for (pptr = pvector->begin(), pend = pvector->end(); pptr < pend; ++pptr)
		{
			jrd_prc* procedure = *pptr;
			if (procedure)
			{
				if (procedure->prc_existence_lock)
				{
					LCK_release(tdbb, procedure->prc_existence_lock);
					procedure->prc_flags |= PRC_check_existence;
					procedure->prc_use_count = 0;
				}
			}
		}
	}

	// release collation existence locks

	dbb->releaseIntlObjects();
}


static UCHAR* alloc_map(thread_db* tdbb, CompilerScratch* csb, USHORT stream)
{
/**************************************
 *
 *	a l l o c _ m a p
 *
 **************************************
 *
 * Functional description
 *	Allocate and initialize stream map for view processing.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);

	SET_TDBB(tdbb);

	fb_assert(stream <= MAX_STREAMS); // CVC: MAX_UCHAR maybe?
	UCHAR* const p = FB_NEW(*tdbb->getDefaultPool()) UCHAR[MAP_LENGTH];
	memset(p, 0, MAP_LENGTH);
	p[0] = (UCHAR) stream;
	csb->csb_rpt[stream].csb_map = p;

	return p;
}


static jrd_nod* catenate_nodes(thread_db* tdbb, NodeStack& stack)
{
/**************************************
 *
 *	c a t e n a t e _ n o d e s
 *
 **************************************
 *
 * Functional description
 *	Take a stack of nodes
 *	and turn them into a tree of concatenations.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_nod* node1 = stack.pop();

	if (stack.isEmpty())
		return node1;

	jrd_nod* cat_node = PAR_make_node(tdbb, 2);
	cat_node->nod_type = nod_concatenate;
	cat_node->nod_arg[0] = node1;
	cat_node->nod_arg[1] = catenate_nodes(tdbb, stack);

	return cat_node;
}


// Try to convert nodes of expression:
//   select ... from <t1>
//     where <x> not in (select <y> from <t2>)
//   (and its variants that uses the same BLR: {NOT (a = ANY b)} and {a <> ALL b})
// to:
//   select ... from <t1>
//     where not ((x is null and exists (select 1 from <t2>)) or
//                exists (select <y> from <t2> where <y> = <x> or <y> is null))
//
// Because the second form can use indexes.
// Returns NULL when not converted, and a new node to be processed when converted.
static jrd_nod* convertNeqAllToNotAny(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
	SET_TDBB(tdbb);

	DEV_BLKCHK(node, type_nod);

	fb_assert(node->nod_type == nod_ansi_all);

	RecordSelExpr* outerRse = (RecordSelExpr*) node->nod_arg[e_any_rse];	// nod_ansi_all rse
	if (!outerRse || outerRse->nod_type != nod_rse || outerRse->rse_count != 1 ||
		!outerRse->rse_boolean || outerRse->rse_boolean->nod_type != nod_neq)
	{
		return NULL;
	}

	RecordSelExpr* innerRse = (RecordSelExpr*) outerRse->rse_relation[0];	// user rse
	// If the rse is different than we expected, do nothing. Do nothing also if it uses FIRST or
	// SKIP, as we can't inject booleans there without changing the behavior.
	if (!innerRse || innerRse->nod_type != nod_rse || innerRse->rse_first || innerRse->rse_skip)
		return NULL;

	jrd_nod* newNode = PAR_make_node(tdbb, 1);
	newNode->nod_type = nod_not;
	newNode->nod_count = 1;

	newNode->nod_arg[0] = PAR_make_node(tdbb, 2);
	newNode->nod_arg[0]->nod_type = nod_or;
	newNode->nod_arg[0]->nod_count = 2;

	newNode->nod_arg[0]->nod_arg[0] = PAR_make_node(tdbb, 2);
	newNode->nod_arg[0]->nod_arg[0]->nod_type = nod_and;
	newNode->nod_arg[0]->nod_arg[0]->nod_count = 2;

	newNode->nod_arg[0]->nod_arg[0]->nod_arg[0] = PAR_make_node(tdbb, 2);
	newNode->nod_arg[0]->nod_arg[0]->nod_arg[0]->nod_type = nod_missing;
	newNode->nod_arg[0]->nod_arg[0]->nod_arg[0]->nod_count = 1;
	newNode->nod_arg[0]->nod_arg[0]->nod_arg[0]->nod_arg[0] = outerRse->rse_boolean->nod_arg[0];

	newNode->nod_arg[0]->nod_arg[0]->nod_arg[1] = PAR_make_node(tdbb, e_any_length);
	newNode->nod_arg[0]->nod_arg[0]->nod_arg[1]->nod_type = nod_any;
	newNode->nod_arg[0]->nod_arg[0]->nod_arg[1]->nod_count = 1;
	newNode->nod_arg[0]->nod_arg[0]->nod_arg[1]->nod_arg[e_any_rse] = (jrd_nod*) innerRse;

	RecordSelExpr* newInnerRse =
		(RecordSelExpr*) PAR_make_node(tdbb, innerRse->rse_count + rse_delta + 2);

	*newInnerRse = *innerRse;

	for (USHORT i = 0; i < innerRse->rse_count; ++i)
		newInnerRse->rse_relation[i] = innerRse->rse_relation[i];

	ignore_dbkey(tdbb, csb, newInnerRse, csb->csb_view);

	newNode->nod_arg[0]->nod_arg[1] = PAR_make_node(tdbb, e_any_length);
	newNode->nod_arg[0]->nod_arg[1]->nod_type = nod_any;
	newNode->nod_arg[0]->nod_arg[1]->nod_count = 1;
	newNode->nod_arg[0]->nod_arg[1]->nod_arg[e_any_rse] = (jrd_nod*) newInnerRse;

	jrd_nod* boolean = PAR_make_node(tdbb, 2);
	boolean->nod_type = nod_or;
	boolean->nod_count = 2;

	boolean->nod_arg[0] = PAR_make_node(tdbb, 1);
	boolean->nod_arg[0]->nod_type = nod_missing;
	boolean->nod_arg[0]->nod_count = 1;
	boolean->nod_arg[0]->nod_arg[0] = outerRse->rse_boolean->nod_arg[1];

	boolean->nod_arg[1] = outerRse->rse_boolean;
	boolean->nod_arg[1]->nod_type = nod_eql;

	// If there was a boolean on the stream, append (AND) the new one
	if (newInnerRse->rse_boolean)
	{
		jrd_nod* temp = PAR_make_node(tdbb, 2);
		temp->nod_type = nod_and;
		temp->nod_count = 2;
		temp->nod_arg[0] = newInnerRse->rse_boolean;
		temp->nod_arg[1] = boolean;
		boolean = temp;
	}

	newInnerRse->rse_boolean = boolean;

	UCHAR localMap[MAP_LENGTH];
	// Initialize the map so all streams initially resolve to the original number. As soon copy
	// creates new streams, the map are being overwritten.
	for (unsigned i = 0; i < MAP_LENGTH; ++i)
		localMap[i] = i;

	return copy(tdbb, csb, newNode, localMap, 0, NULL, false);
}


static jrd_nod* copy(thread_db* tdbb,
					CompilerScratch* csb,
					jrd_nod* input,
					UCHAR* remap,
					USHORT field_id,
					jrd_nod* message,
					bool remap_fld)
{
/**************************************
 *
 *	c o p y
 *
 **************************************
 *
 * Functional description
 *	Copy an expression tree remapping field streams.  If the
 *	map isn't present, don't remap.
 *
 **************************************/
	jrd_nod* node;
	USHORT stream, new_stream;

	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(input, type_nod);

	if (!input)
		return NULL;

	// special case interesting nodes

	USHORT args = input->nod_count;

	switch (input->nod_type)
	{
	case nod_ansi_all:
	case nod_ansi_any:
	case nod_any:
	case nod_exists:
	case nod_unique:
		args = e_any_length;
		break;

	case nod_for:
		args = e_for_length;
		break;

	case nod_argument:
		if (remap_fld)
			return input;
		node = PAR_make_node(tdbb, e_arg_length);
		node->nod_count = input->nod_count;
		node->nod_flags = input->nod_flags;
		node->nod_type = input->nod_type;
		node->nod_arg[e_arg_number] = input->nod_arg[e_arg_number];
		// dimitr:	IMPORTANT!!!
		//			nod_message copying must be done in the only place
		//			(the nod_procedure code below). Hence we don't call
		//			copy() here to keep argument->nod_arg[e_arg_message]
		//			and procedure->nod_arg[e_prc_in_msg] in sync. The
		//			message is passed to copy() as a parameter. If the
		//			passed message is NULL, it means nod_argument is
		//			cloned outside nod_procedure (e.g. in the optimizer)
		//			and we must keep the input message.
		// ASF: We should only use "message" if its number matches the number
		// in nod_argument. If it don't, it may be an input parameter cloned
		// in convertNeqAllToNotAny - see CORE-3094.

		if (message &&
			(IPTR) message->nod_arg[e_msg_number] ==
				(IPTR) input->nod_arg[e_arg_message]->nod_arg[e_msg_number])
		{
			node->nod_arg[e_arg_message] = message;
		}
		else
			node->nod_arg[e_arg_message] = input->nod_arg[e_arg_message];

		node->nod_arg[e_arg_flag] =
			copy(tdbb, csb, input->nod_arg[e_arg_flag], remap, field_id, message, remap_fld);
		node->nod_arg[e_arg_indicator] =
			copy(tdbb, csb, input->nod_arg[e_arg_indicator], remap, field_id, message, remap_fld);
		return node;

	case nod_assignment:
		args = e_asgn_length;
		break;

	case nod_erase:
		args = e_erase_length;
		break;

	case nod_user_savepoint:
		args = e_sav_length;
		break;

	case nod_modify:
		args = e_mod_length;
		break;

	case nod_variable:
		if (csb->csb_remap_variable != 0)
		{
			node = PAR_make_node(tdbb, e_var_length);
			node->nod_type = input->nod_type;
			node->nod_count = input->nod_count;

			USHORT n = csb->csb_remap_variable + (USHORT)(IPTR) input->nod_arg[e_var_id];
			node->nod_arg[e_var_id] = (jrd_nod*)(IPTR) n;
			node->nod_arg[e_var_variable] = input->nod_arg[e_var_variable];
			node->nod_arg[e_var_info] = input->nod_arg[e_var_info];
			return node;
		}
		return input;

	case nod_init_variable:
		if (csb->csb_remap_variable != 0)
		{
			node = PAR_make_node(tdbb, e_init_var_length);
			node->nod_type = input->nod_type;
			node->nod_count = input->nod_count;

			USHORT n = csb->csb_remap_variable + (USHORT)(IPTR) input->nod_arg[e_init_var_id];
			node->nod_arg[e_init_var_id] = (jrd_nod*)(IPTR) n;
			node->nod_arg[e_init_var_variable] = input->nod_arg[e_init_var_variable];
			node->nod_arg[e_init_var_info] = input->nod_arg[e_init_var_info];
			return node;
		}
		return input;

	case nod_literal:
		{
			dsc* inputDesc = &((Literal*) input)->lit_desc;

			const int count = lit_delta +
				(inputDesc->dsc_length + sizeof(jrd_nod*) - 1) / sizeof(jrd_nod*);

			jrd_nod* node = PAR_make_node(tdbb, count);
			node->nod_type = nod_literal;
			node->nod_count = 0;

			Literal* literal = (Literal*) node;
			UCHAR* p = reinterpret_cast<UCHAR*>(literal->lit_data);

			literal->lit_desc = *inputDesc;
			literal->lit_desc.dsc_address = p;
			literal->lit_desc.dsc_flags = 0;

			memcpy(p, inputDesc->dsc_address, inputDesc->dsc_length);

			return node;
		}

	case nod_field:
		{
			if (field_id && (input->nod_flags & nod_id) && !input->nod_arg[e_fld_id] &&
				!input->nod_arg[e_fld_stream])
			{
				--field_id;
			}
			else {
				field_id = (USHORT)(IPTR) input->nod_arg[e_fld_id];
			}
			stream = (USHORT)(IPTR) input->nod_arg[e_fld_stream];
			if (remap_fld) {
				jrd_rel* relation = csb->csb_rpt[stream].csb_relation;
				jrd_fld* field = MET_get_field(relation, field_id);
				if (field->fld_source)
					field_id = (USHORT)(IPTR) field->fld_source->nod_arg[e_fld_id];
			}
			if (remap)
				stream = remap[stream];

			jrd_nod* temp_node = PAR_gen_field(tdbb, stream, field_id);
			if (input->nod_type == nod_field && input->nod_arg[e_fld_default_value])
			{
				temp_node->nod_arg[e_fld_default_value] = input->nod_arg[e_fld_default_value];
			}
			return temp_node;
		}

	case nod_derived_expr:
	{
		node = PAR_make_node(tdbb, e_derived_expr_length);
		node->nod_count = e_derived_expr_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_derived_expr_expr] =
			copy(tdbb, csb, input->nod_arg[e_derived_expr_expr], remap, field_id, message, remap_fld);

		if (remap)
		{
			const UCHAR streamCount = (UCHAR)(IPTR) input->nod_arg[e_derived_expr_stream_count];
			const USHORT* oldStreamList = (USHORT*) input->nod_arg[e_derived_expr_stream_list];
			USHORT* newStreamList = FB_NEW(*tdbb->getDefaultPool()) USHORT[streamCount];

			for (UCHAR i = 0; i < streamCount; ++i)
				newStreamList[i] = remap[oldStreamList[i]];

			node->nod_arg[e_derived_expr_stream_list] = (jrd_nod*) newStreamList;
		}
		else
			node->nod_arg[e_derived_expr_stream_list] = input->nod_arg[e_derived_expr_stream_list];

		node->nod_arg[e_derived_expr_stream_count] = input->nod_arg[e_derived_expr_stream_count];
		return node;
	}

	case nod_function:
		node = PAR_make_node(tdbb, e_fun_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_fun_args] =
			copy(tdbb, csb, input->nod_arg[e_fun_args], remap, field_id, message, remap_fld);
		node->nod_arg[e_fun_function] = input->nod_arg[e_fun_function];
		return (node);

	case nod_current_time:
	case nod_current_timestamp:
		fb_assert(e_current_time_length == e_current_timestamp_length);
		node = PAR_make_node(tdbb, e_current_time_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[0] = input->nod_arg[0];
		return (node);

	case nod_gen_id:
	case nod_gen_id2:			// 20001013 PJPG
		node = PAR_make_node(tdbb, e_gen_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_gen_value] =
			copy(tdbb, csb, input->nod_arg[e_gen_value], remap, field_id, message, remap_fld);
		node->nod_arg[e_gen_id] = input->nod_arg[e_gen_id];
		return (node);

	case nod_cast:
		node = PAR_make_node(tdbb, e_cast_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_cast_source] =
			copy(tdbb, csb, input->nod_arg[e_cast_source], remap, field_id, message, remap_fld);
		node->nod_arg[e_cast_fmt] = input->nod_arg[e_cast_fmt];
		return (node);

	case nod_extract:
		node = PAR_make_node(tdbb, e_extract_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_extract_value] =
			copy(tdbb, csb, input->nod_arg[e_extract_value], remap, field_id, message, remap_fld);
		node->nod_arg[e_extract_part] = input->nod_arg[e_extract_part];
		return (node);

	case nod_strlen:
		node = PAR_make_node(tdbb, e_strlen_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_strlen_value] =
			copy(tdbb, csb, input->nod_arg[e_strlen_value], remap, field_id, message, remap_fld);
		node->nod_arg[e_strlen_type] = input->nod_arg[e_strlen_type];
		return (node);

	case nod_trim:
		node = PAR_make_node(tdbb, e_trim_length);
		node->nod_count = input->nod_count;
		node->nod_type = input->nod_type;
		node->nod_arg[e_trim_characters] =
			copy(tdbb, csb, input->nod_arg[e_trim_characters], remap, field_id, message, remap_fld);
		node->nod_arg[e_trim_value] =
			copy(tdbb, csb, input->nod_arg[e_trim_value], remap, field_id, message, remap_fld);
		node->nod_arg[e_trim_specification] = input->nod_arg[e_trim_specification];
		return (node);

	case nod_count:
	//case nod_count2:
	case nod_max:
	case nod_min:
	case nod_total:
	case nod_average:
	case nod_from:
		args = e_stat_length;
		break;

	case nod_rse:
		{
			RecordSelExpr* old_rse = (RecordSelExpr*) input;
			RecordSelExpr* new_rse =
				(RecordSelExpr*) PAR_make_node(tdbb, old_rse->rse_count + rse_delta + 2);
			new_rse->nod_type = input->nod_type;
			new_rse->nod_flags = input->nod_flags;
			new_rse->nod_count = 0;
			new_rse->rse_count = old_rse->rse_count;
			jrd_nod** arg1 = old_rse->rse_relation;
			jrd_nod** arg2 = new_rse->rse_relation;
			for (const jrd_nod* const* const end = arg1 + old_rse->rse_count;
				arg1 < end; arg1++, arg2++)
			{
				*arg2 = copy(tdbb, csb, *arg1, remap, field_id, message, remap_fld);
			}
			new_rse->rse_jointype = old_rse->rse_jointype;
			new_rse->rse_writelock = old_rse->rse_writelock;
			new_rse->rse_first =
				copy(tdbb, csb, old_rse->rse_first, remap, field_id, message, remap_fld);
			new_rse->rse_skip =
				copy(tdbb, csb, old_rse->rse_skip, remap, field_id, message, remap_fld);
			new_rse->rse_boolean =
				copy(tdbb, csb, old_rse->rse_boolean, remap, field_id, message, remap_fld);
			new_rse->rse_sorted =
				copy(tdbb, csb, old_rse->rse_sorted, remap, field_id, message, remap_fld);
			new_rse->rse_projection =
				copy(tdbb, csb, old_rse->rse_projection, remap, field_id, message, remap_fld);
			return (jrd_nod*) new_rse;
		}

	case nod_relation:
		{
			if (!remap)
				BUGCHECK(221);	// msg 221 (CMP) copy: cannot remap
			node = PAR_make_node(tdbb, e_rel_length);
			node->nod_type = input->nod_type;
			node->nod_count = 0;

			stream = (USHORT)(IPTR) input->nod_arg[e_rel_stream];

			// Last entry in the remap contains the the original stream number.
			// Get that stream number so that the flags can be copied
			// into the newly created child stream.

			const int relative_stream = stream ? remap[stream - 1] : stream;
			new_stream = csb->nextStream();
			node->nod_arg[e_rel_stream] = (jrd_nod*) (IPTR) new_stream;
			remap[stream] = (UCHAR) new_stream;

			node->nod_arg[e_rel_context] = input->nod_arg[e_rel_context];
			node->nod_arg[e_rel_relation] = input->nod_arg[e_rel_relation];
			node->nod_arg[e_rel_view] = input->nod_arg[e_rel_view];

			CompilerScratch::csb_repeat* element = CMP_csb_element(csb, new_stream);
			element->csb_relation = (jrd_rel*) node->nod_arg[e_rel_relation];
			element->csb_view = (jrd_rel*) node->nod_arg[e_rel_view];
			element->csb_view_stream = remap[0];

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
			all the views into their base tables. It creates a compilier
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

			 We now store the proper stream no in relative_stream and
			 later use it to copy the flags. -Sudesh (03/05/99)
		**/
			csb->csb_rpt[new_stream].csb_flags |=
				csb->csb_rpt[relative_stream].csb_flags & csb_no_dbkey;

			return node;
		}

	case nod_procedure:
		{
			if (!remap)
				BUGCHECK(221);	// msg 221 (CMP) copy: cannot remap
			node = PAR_make_node(tdbb, e_prc_length);
			node->nod_type = input->nod_type;
			node->nod_count = input->nod_count;

			// dimitr:	see the appropriate code and comment above (in nod_argument).
			//			We must copy the message first and only then use the new
			//			pointer to copy the inputs properly.
			node->nod_arg[e_prc_in_msg] =
				copy(tdbb, csb, input->nod_arg[e_prc_in_msg], remap, field_id, message, remap_fld);
			node->nod_arg[e_prc_inputs] =
				copy(tdbb, csb, input->nod_arg[e_prc_inputs], remap, field_id,
					 node->nod_arg[e_prc_in_msg], remap_fld);

			stream = (USHORT)(IPTR) input->nod_arg[e_prc_stream];
			new_stream = csb->nextStream();
			node->nod_arg[e_prc_stream] = (jrd_nod*) (IPTR) new_stream;
			remap[stream] = (UCHAR) new_stream;
			node->nod_arg[e_prc_context] = input->nod_arg[e_prc_context];
			node->nod_arg[e_prc_procedure] = input->nod_arg[e_prc_procedure];
			node->nod_arg[e_prc_view] = input->nod_arg[e_prc_view];
			CompilerScratch::csb_repeat* element = CMP_csb_element(csb, new_stream);
			// SKIDDER: Maybe we need to check if we really found a procedure?
			element->csb_procedure = MET_lookup_procedure_id(tdbb,
			  (SSHORT)(IPTR) node->nod_arg[e_prc_procedure], false, false, 0);
			element->csb_view = (jrd_rel*) node->nod_arg[e_prc_view];
			element->csb_view_stream = remap[0];

			csb->csb_rpt[new_stream].csb_flags |= csb->csb_rpt[stream].csb_flags & csb_no_dbkey;

			return node;
		}

	case nod_aggregate:
		if (!remap)
			BUGCHECK(221);		// msg 221 (CMP) copy: cannot remap
		node = PAR_make_node(tdbb, e_agg_length);
		node->nod_type = input->nod_type;
		node->nod_count = 0;
		stream = (USHORT)(IPTR) input->nod_arg[e_agg_stream];
		fb_assert(stream <= MAX_STREAMS);
		new_stream = csb->nextStream();
		node->nod_arg[e_agg_stream] = (jrd_nod*) (IPTR) new_stream;
		// fb_assert(new_stream <= MAX_UCHAR);
		remap[stream] = (UCHAR) new_stream;
		CMP_csb_element(csb, new_stream);

		csb->csb_rpt[new_stream].csb_flags |= csb->csb_rpt[stream].csb_flags & csb_no_dbkey;
		node->nod_arg[e_agg_rse] =
			copy(tdbb, csb, input->nod_arg[e_agg_rse], remap, field_id, message, remap_fld);
		node->nod_arg[e_agg_group] =
			copy(tdbb, csb, input->nod_arg[e_agg_group], remap, field_id, message, remap_fld);
		node->nod_arg[e_agg_map] =
			copy(tdbb, csb, input->nod_arg[e_agg_map], remap, field_id, message, remap_fld);
		return node;

	case nod_union:
		if (!remap)
			BUGCHECK(221);		// msg 221 (CMP) copy: cannot remap
		node = PAR_make_node(tdbb, e_uni_length);
		node->nod_type = input->nod_type;
		node->nod_count = 3;
		node->nod_flags = input->nod_flags;
		stream = (USHORT)(IPTR) input->nod_arg[e_uni_stream];
		fb_assert(stream <= MAX_STREAMS);
		new_stream = csb->nextStream();
		node->nod_arg[e_uni_stream] = (jrd_nod*) (IPTR) new_stream;
		remap[stream] = (UCHAR) new_stream;
		CMP_csb_element(csb, new_stream);

		if (node->nod_flags & nod_recurse)
		{
			stream = (USHORT)(IPTR) input->nod_arg[e_uni_map_stream];
			fb_assert(stream <= MAX_STREAMS);
			new_stream = csb->nextStream();
			node->nod_arg[e_uni_map_stream] = (jrd_nod*) (IPTR) new_stream;
			remap[stream] = (UCHAR) new_stream;
			CMP_csb_element(csb, new_stream);
		}

		csb->csb_rpt[new_stream].csb_flags |= csb->csb_rpt[stream].csb_flags & csb_no_dbkey;
		node->nod_arg[e_uni_clauses] =
			copy(tdbb, csb, input->nod_arg[e_uni_clauses], remap, field_id, message, remap_fld);
		return node;

	case nod_message:
		node = PAR_make_node(tdbb, e_msg_length);
		node->nod_type = input->nod_type;
		node->nod_count = input->nod_count;
		node->nod_arg[e_msg_number] = input->nod_arg[e_msg_number];
		node->nod_arg[e_msg_format] = input->nod_arg[e_msg_format];
		node->nod_arg[e_msg_impure_flags] = input->nod_arg[e_msg_impure_flags];
		// dimitr: hmmm, cannot find where the following one is used...
		node->nod_arg[e_msg_next] =
			copy(tdbb, csb, input->nod_arg[e_msg_next], remap, field_id, message, remap_fld);
		return node;

	case nod_sort:
		args *= 3;
		break;

	case nod_domain_validation:
		node = PAR_make_node(tdbb, e_domval_length);
		node->nod_type = nod_domain_validation;
		node->nod_count = 0;
		*(dsc*) (node->nod_arg + e_domval_desc) = *(dsc*) (input->nod_arg + e_domval_desc);
		return node;

	case nod_dbkey:
		stream = (USHORT)(IPTR) input->nod_arg[0];
		if (remap)
			stream = remap[stream];

		node = PAR_make_node(tdbb, 1);
		node->nod_type = input->nod_type;
		node->nod_count = 0;
		node->nod_arg[0] = (jrd_nod*)(IPTR) stream;
		return node;

	case nod_sys_function:
		node = PAR_make_node(tdbb, e_sysfun_length);
		node->nod_type = input->nod_type;
		node->nod_count = e_sysfun_count;
		node->nod_arg[e_sysfun_args] =
			copy(tdbb, csb, input->nod_arg[e_sysfun_args], remap, field_id, message, remap_fld);
		node->nod_arg[e_sysfun_function] = input->nod_arg[e_sysfun_function];
		return node;

	case nod_dcl_variable:
		if (csb->csb_remap_variable != 0)
		{
			node = PAR_make_node(tdbb, e_dcl_length);
			node->nod_type = input->nod_type;
			node->nod_count = input->nod_count;

			const USHORT n = csb->csb_remap_variable + (USHORT)(IPTR) input->nod_arg[e_dcl_id];
			node->nod_arg[e_dcl_id] = (jrd_nod*)(IPTR) n;
			*(dsc*) (node->nod_arg + e_dcl_desc) = *(dsc*) (input->nod_arg + e_dcl_desc);

			csb->csb_variables =
				vec<jrd_nod*>::newVector(*tdbb->getDefaultPool(), csb->csb_variables, n + 1);

			return node;
		}
		return input;

	default:
		break;
	}

	// fall thru on generic nodes

	node = PAR_make_node(tdbb, args);
	node->nod_count = input->nod_count;
	node->nod_type = input->nod_type;
	node->nod_flags = input->nod_flags;

	jrd_nod** arg1 = input->nod_arg;
	jrd_nod** arg2 = node->nod_arg;

	for (const jrd_nod* const* const end = arg1 + input->nod_count; arg1 < end; arg1++, arg2++)
	{
		if (*arg1) {
			*arg2 = copy(tdbb, csb, *arg1, remap, field_id, message, remap_fld);
		}
	}

	// finish off sort

	if (input->nod_type == nod_sort) {
		for (jrd_nod** end = arg1 + input->nod_count * 2; arg1 < end; arg1++, arg2++)
		{
			*arg2 = *arg1;
		}
	}

	return node;
}


static void expand_view_nodes(thread_db* tdbb,
							  CompilerScratch* csb,
							  USHORT stream,
							  NodeStack& stack,
							  nod_t type,
							  bool allStreams)
{
/**************************************
 *
 *	e x p a n d _ v i e w _ n o d e s
 *
 **************************************
 *
 * Functional description
 *	Expand dbkey for view.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	// if the stream's dbkey should be ignored, do so

	if (!allStreams && (csb->csb_rpt[stream].csb_flags & csb_no_dbkey))
		return;

	// if the stream references a view, follow map
	const UCHAR* map = csb->csb_rpt[stream].csb_map;
	if (map)
	{
		++map;
		while (*map)
			expand_view_nodes(tdbb, csb, *map++, stack, type, allStreams);
		return;
	}

	// relation is primitive - make dbkey node

	if (allStreams || csb->csb_rpt[stream].csb_relation)
	{
		jrd_nod* node = PAR_make_node(tdbb, 1);
		node->nod_count = 0;
		node->nod_type = type;
		node->nod_arg[0] = (jrd_nod*)(IPTR) stream;
		stack.push(node);
	}
}


static void ignore_dbkey(thread_db* tdbb, CompilerScratch* csb, RecordSelExpr* rse,
	const jrd_rel* view)
{
/**************************************
 *
 *	i g n o r e _ d b k e y
 *
 **************************************
 *
 * Functional description
 *	For each relation or aggregate in the
 *	RecordSelExpr, mark it as not having a dbkey.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);
	DEV_BLKCHK(view, type_rel);

	const jrd_nod* const* ptr = rse->rse_relation;
	for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end;)
	{
		const jrd_nod* node = *ptr++;
		switch (node->nod_type)
		{
		case nod_relation:
		{
			const USHORT stream = (USHORT)(IPTR) node->nod_arg[e_rel_stream];
			csb->csb_rpt[stream].csb_flags |= csb_no_dbkey;
			const CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];
			const jrd_rel* relation = tail->csb_relation;
			if (relation) {
				CMP_post_access(tdbb, csb, relation->rel_security_name,
								(tail->csb_view) ? tail->csb_view->rel_id : (view ? view->rel_id : 0),
								SCL_read, object_table,
								relation->rel_name);
			}
			break;
		}
		case nod_rse:
			ignore_dbkey(tdbb, csb, (RecordSelExpr*) node, view);
			break;
		case nod_aggregate:
			ignore_dbkey(tdbb, csb, (RecordSelExpr*) node->nod_arg[e_agg_rse], view);
			break;
		case nod_union:
			const jrd_nod* clauses = node->nod_arg[e_uni_clauses];
			const jrd_nod* const* ptr_uni = clauses->nod_arg;
			for (const jrd_nod* const* const end_uni = ptr_uni + clauses->nod_count;
				ptr_uni < end_uni; ptr_uni++)
			{
				ignore_dbkey(tdbb, csb, (RecordSelExpr*) *ptr_uni++, view);
			}
			break;
		}
	}
}


static jrd_nod* make_defaults(thread_db* tdbb, CompilerScratch* csb, USHORT stream, jrd_nod* statement)
{
/**************************************
 *
 *	m a k e _ d e f a u l t s
 *
 **************************************
 *
 * Functional description
 *	Build an default value assignments.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(statement, type_nod);

	jrd_rel* relation = csb->csb_rpt[stream].csb_relation;

	vec<jrd_fld*>* vector = relation->rel_fields;
	if (!vector)
		return statement;

	UCHAR local_map[MAP_LENGTH];
	UCHAR* map = csb->csb_rpt[stream].csb_map;
	if (!map) {
		map = local_map;
		fb_assert(stream <= MAX_STREAMS); // CVC: MAX_UCHAR relevant, too?
		map[0] = (UCHAR) stream;
		map[1] = 1;
		map[2] = 2;
	}

	NodeStack stack;

	USHORT field_id = 0;
	vec<jrd_fld*>::iterator ptr1 = vector->begin();
	for (const vec<jrd_fld*>::const_iterator end = vector->end(); ptr1 < end; ++ptr1, ++field_id)
	{
		jrd_nod* value;
		if (*ptr1 && (value = (*ptr1)->fld_default_value))
		{
			fb_assert(statement->nod_type == nod_list);
			if (statement->nod_type == nod_list)
			{
				bool inList = false;

				for (unsigned i = 0; i < statement->nod_count; ++i)
				{
					const jrd_nod* assign = statement->nod_arg[i];

					fb_assert(assign->nod_type == nod_assignment);
					if (assign->nod_type == nod_assignment)
					{
						const jrd_nod* to = assign->nod_arg[e_asgn_to];

						fb_assert(to->nod_type == nod_field);
						if (to->nod_type == nod_field &&
							(USHORT)(IPTR) to->nod_arg[e_fld_stream] == stream &&
							(USHORT)(IPTR) to->nod_arg[e_fld_id] == field_id)
						{
							inList = true;
							break;
						}
					}
				}

				if (inList)
					continue;
			}

			jrd_nod* node = PAR_make_node(tdbb, e_asgn_length);
			node->nod_type = nod_assignment;
			node->nod_arg[e_asgn_from] =
				copy(tdbb, csb, value, map, (USHORT) (field_id + 1), NULL, false);
			node->nod_arg[e_asgn_to] = PAR_gen_field(tdbb, stream, field_id);
			stack.push(node);
		}
	}

	if (stack.isEmpty())
		return statement;

	// we have some default - add the original statement and make a list out of
	// the whole mess

	stack.push(statement);

	return PAR_make_list(tdbb, stack);
}


static jrd_nod* make_validation(thread_db* tdbb, CompilerScratch* csb, USHORT stream)
{
/**************************************
 *
 *	m a k e _ v a l i d a t i o n
 *
 **************************************
 *
 * Functional description
 *	Build a validation list for a relation, if appropriate.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	jrd_rel* relation = csb->csb_rpt[stream].csb_relation;

	vec<jrd_fld*>* vector = relation->rel_fields;
	if (!vector)
		return NULL;

	UCHAR local_map[MAP_LENGTH];
	UCHAR* map = csb->csb_rpt[stream].csb_map;
	if (!map) {
		map = local_map;
		fb_assert(stream <= MAX_STREAMS); // CVC: MAX_UCHAR still relevant for the bitmap?
		map[0] = (UCHAR) stream;
	}

	NodeStack stack;

	USHORT field_id = 0;
	vec<jrd_fld*>::iterator ptr1 = vector->begin();
	for (const vec<jrd_fld*>::const_iterator end = vector->end(); ptr1 < end; ++ptr1, ++field_id)
	{
		jrd_nod* validation;
		if (*ptr1 && (validation = (*ptr1)->fld_validation))
		{
			AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
				(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);

			jrd_nod* node = PAR_make_node(tdbb, e_val_length);
			node->nod_type = nod_validate;
			node->nod_arg[e_val_boolean] =
				copy(tdbb, csb, validation, map, (USHORT) (field_id + 1), NULL, false);
			node->nod_arg[e_val_value] = PAR_gen_field(tdbb, stream, field_id);
			stack.push(node);
		}

		if (*ptr1 && (validation = (*ptr1)->fld_not_null))
		{
			AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
				(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);

			jrd_nod* node = PAR_make_node(tdbb, e_val_length);
			node->nod_type = nod_validate;
			node->nod_arg[e_val_boolean] =
				copy(tdbb, csb, validation, map, (USHORT) (field_id + 1), NULL, false);
			node->nod_arg[e_val_value] = PAR_gen_field(tdbb, stream, field_id);
			stack.push(node);
		}
	}

	if (stack.isEmpty())
		return NULL;

	return PAR_make_list(tdbb, stack);
}


// Look at all RecordSelExpr's which are lower in scope than the RecordSelExpr which this field
// is referencing, and mark them as varying - the rule is that if a field
// from one RecordSelExpr is referenced within the scope of another RecordSelExpr, the first RecordSelExpr
// can't be invariant. This won't optimize all cases, but it is the simplest
// operating assumption for now.
static void mark_variant(thread_db* tdbb, CompilerScratch* csb, USHORT stream)
{
	if (csb->csb_current_nodes.getCount())
	{
		for (jrd_node_base **i_node = csb->csb_current_nodes.end() - 1;
			 i_node >= csb->csb_current_nodes.begin(); i_node--)
		{
			if ((*i_node)->nod_type == nod_rse)
			{
				RecordSelExpr* rse = reinterpret_cast<RecordSelExpr*>(*i_node);
				if (stream_in_rse(stream, rse))
					break;
				rse->nod_flags |= rse_variant;
			}
			else
				(*i_node)->nod_flags &= ~nod_invariant;
		}
	}
}


jrd_nod* CMP_pass1(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	C M P _ p a s s 1
 *
 **************************************
 *
 * Functional description
 *	Merge missing values, computed values, validation expressions,
 *	and views into a parsed request.
 *
 * The csb->csb_validate_expr becomes true if an ancestor of the
 * current node (the one being passed in) in the parse tree has nod_type
 * == nod_validate. "ancestor" does not include the current node
 * being passed in as an argument.
 * If we are in a "validate subtree" (as determined by the
 * csb->csb_validate_expr), we must not post update access to the fields involved
 * in the validation clause. (see the call for CMP_post_access in this
 * function.)
 *
 **************************************/
	jrd_nod* sub;
	jrd_nod** ptr;
	USHORT stream;
	CompilerScratch::csb_repeat* tail;
	jrd_prc* procedure;

	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);
	DEV_BLKCHK(view, type_rel);

	if (!node)
		return node;

	AutoSetRestore<bool> autoValidateExpr(&csb->csb_validate_expr,
		csb->csb_validate_expr || node->nod_type == nod_validate);

	jrd_rel* const view = csb->csb_view;

	// if there is processing to be done before sub expressions, do it here

	switch (node->nod_type)
	{
	case nod_like:
	case nod_similar:
		ptr = node->nod_arg;
		ptr[0] = CMP_pass1(tdbb, csb, ptr[0]);
		// We need to take care of invariantness of like/similar pattern expression to be
		// able to pre-compile its pattern
		node->nod_flags |= nod_invariant;
		csb->csb_current_nodes.push(node);
		ptr[1] = CMP_pass1(tdbb, csb, ptr[1]);
		if (node->nod_count == 3) {
			// escape symbol also needs to be taken care of
			ptr[2] = CMP_pass1(tdbb, csb, ptr[2]);
		}
		csb->csb_current_nodes.pop();

		// If there is no top-level RSE present and patterns are not constant,
		// unmark node as invariant because it may be dependent on data or variables.
		// See the same for nod_contains and nod_starts below.
		if ((node->nod_flags & nod_invariant) && (ptr[1]->nod_type != nod_literal ||
			(node->nod_count == 3 && ptr[2]->nod_type != nod_literal)))
		{
			jrd_node_base **ctx_node, **end;
			for (ctx_node = csb->csb_current_nodes.begin(), end = csb->csb_current_nodes.end();
				ctx_node < end;	ctx_node++)
			{
				if ((*ctx_node)->nod_type == nod_rse)
					break;
			}
			if (ctx_node >= end)
				node->nod_flags &= ~nod_invariant;
		}
		return node;

	case nod_contains:
	case nod_starts:
		ptr = node->nod_arg;
		ptr[0] = CMP_pass1(tdbb, csb, ptr[0]);
		// We need to take care of invariantness of contains and starts
		// expression to be able to pre-compile it for searching
		node->nod_flags |= nod_invariant;
		csb->csb_current_nodes.push(node);
		ptr[1] = CMP_pass1(tdbb, csb, ptr[1]);
		csb->csb_current_nodes.pop();

		// If there is no top-level RSE present and patterns are not constant,
		// unmark node as invariant because it may be dependent on data or variables.
		// See the same for nod_like above.
		if ((node->nod_flags & nod_invariant) && (ptr[1]->nod_type != nod_literal))
		{
			jrd_node_base **ctx_node, **end;
			for (ctx_node = csb->csb_current_nodes.begin(), end = csb->csb_current_nodes.end();
				 ctx_node < end; ctx_node++)
			{
				if ((*ctx_node)->nod_type == nod_rse)
					break;
			}
			if (ctx_node >= end)
				node->nod_flags &= ~nod_invariant;
		}
		return node;

	case nod_variable:
	{
		const USHORT n = (USHORT)(IPTR) node->nod_arg[e_var_id];
		vec<jrd_nod*>* vector = csb->csb_variables;
		if (!vector || n >= vector->count() || !(node->nod_arg[e_var_variable] = (*vector)[n]))
		{
			PAR_syntax_error(csb, "variable identifier");
		}
		break;
	}

	case nod_init_variable:
	{
		const USHORT n = (USHORT)(IPTR) node->nod_arg[e_init_var_id];
		vec<jrd_nod*>* vector = csb->csb_variables;
		if (!vector || n >= vector->count() || !(node->nod_arg[e_init_var_variable] = (*vector)[n]))
		{
			PAR_syntax_error(csb, "variable identifier");
		}
		break;
	}

	case nod_argument:
		break;

	case nod_cast:
		{
			node->nod_arg[e_cast_source] = CMP_pass1(tdbb, csb, node->nod_arg[e_cast_source]);

			const Format* const format = (Format*) node->nod_arg[e_cast_fmt];
			const USHORT ttype = INTL_TEXT_TYPE(format->fmt_desc[0]);

			// Are we using a collation?
			if (TTYPE_TO_COLLATION(ttype) != 0)
			{
				CMP_post_resource(&csb->csb_resources, INTL_texttype_lookup(tdbb, ttype),
								  Resource::rsc_collation, ttype);
			}

			return node;
		}

	case nod_field:
		{
			stream = (USHORT)(IPTR) node->nod_arg[e_fld_stream];

			mark_variant(tdbb, csb, stream);

			jrd_fld* field;
			tail = &csb->csb_rpt[stream];
			jrd_rel* relation = tail->csb_relation;
			if (!relation ||
				!(field = MET_get_field(relation, (USHORT)(IPTR) node->nod_arg[e_fld_id])))
			{
				break;
			}

			dsc desc;
			CMP_get_desc(tdbb, csb, node, &desc);

			const USHORT ttype = INTL_TEXT_TYPE(desc);

			// Are we using a collation?
			if (TTYPE_TO_COLLATION(ttype) != 0)
			{
				Jrd::Collation* collation = NULL;

				try
				{
					ThreadStatusGuard local_status(tdbb);

					collation = INTL_texttype_lookup(tdbb, ttype);
				}
				catch (Firebird::Exception&)
				{
					// ASF: Swallow the exception if we fail to load the collation here.
					// This allows us to backup databases when the collation isn't available.
					if (!(tdbb->getAttachment()->att_flags & ATT_gbak_attachment))
						throw;
				}

				if (collation)
				{
					CMP_post_resource(&csb->csb_resources, collation, Resource::rsc_collation, ttype);
				}
			}

			// if this is a modify or store, check REFERENCES access to any foreign keys

/* CVC: This is against the SQL standard. REFERENCES should be enforced only at the
				time the FK is defined in DDL, not when a DML is going to be executed.
			if (((tail->csb_flags & csb_modify) || (tail->csb_flags & csb_store)) &&
				!(relation->rel_view_rse || relation->rel_file))
			{
				IDX_check_access(tdbb, csb, tail->csb_view, relation);
			}
*/

			// posting the required privilege access to the current relation and field

			// If this is in a "validate_subtree" then we must not
			// post access checks to the table and the fields in the table.
			// If any node of the parse tree is a nod_validate type node,
			// the nodes in the subtree are involved in a validation
			// clause only, the subtree is a validate_subtree in our notation.

			if (tail->csb_flags & csb_modify)
			{
				if (!csb->csb_validate_expr) {
					CMP_post_access(tdbb, csb, relation->rel_security_name,
									(tail->csb_view) ? tail->csb_view->rel_id :
										(view ? view->rel_id : 0),
									SCL_sql_update, object_table,
									relation->rel_name);
					CMP_post_access(tdbb, csb, field->fld_security_name,
									(tail->csb_view) ? tail->csb_view->rel_id :
										(view ? view->rel_id : 0),
									SCL_sql_update, object_column,
									field->fld_name, relation->rel_name);
				}
			}
			else if (tail->csb_flags & csb_erase)
			{
				CMP_post_access(tdbb, csb, relation->rel_security_name,
								(tail->csb_view) ? tail->csb_view->rel_id :
									(view ? view->rel_id : 0),
								SCL_sql_delete, object_table,
								relation->rel_name);
			}
			else if (tail->csb_flags & csb_store)
			{
				CMP_post_access(tdbb, csb, relation->rel_security_name,
								(tail->csb_view) ? tail->csb_view->rel_id :
									(view ? view->rel_id : 0),
								SCL_sql_insert, object_table,
								relation->rel_name);
				CMP_post_access(tdbb, csb, field->fld_security_name,
								(tail->csb_view) ? tail->csb_view->rel_id :
									(view ? view->rel_id : 0),
								SCL_sql_insert, object_column,
								field->fld_name, relation->rel_name);
			}
			else
			{
				CMP_post_access(tdbb, csb, relation->rel_security_name,
								(tail->csb_view) ? tail->csb_view->rel_id :
									(view ? view->rel_id : 0),
								SCL_read, object_table, relation->rel_name);
				CMP_post_access(tdbb, csb, field->fld_security_name,
								(tail->csb_view) ? tail->csb_view->rel_id :
									(view ? view->rel_id : 0),
								SCL_read, object_column, field->fld_name, relation->rel_name);
			}

			if (!(sub = field->fld_computation) && !(sub = field->fld_source)) {

				if (!relation->rel_view_rse)
					break;

				ERR_post(Arg::Gds(isc_no_field_access) << Arg::Str(field->fld_name) <<
														  Arg::Str(relation->rel_name));
				// Msg 364 "cannot access column %s in view %s"
			}

			// The previous test below is an apparent temporary fix
			// put in by Root & Harrison in Summer/Fall 1991.
			// Old Code:
			// if (tail->csb_flags & (csb_view_update | csb_trigger))
			//   break;
			// If the field is a computed field - we'll go on and make
			// the substitution.
			// Comment 1994-August-08 David Schnepper

			if (tail->csb_flags & (csb_view_update | csb_trigger)) {
				// dimitr:	added an extra check for views, because we don't
				//			want their old/new contexts to be substituted
				if (relation->rel_view_rse || !field->fld_computation)
					break;
			}

			UCHAR local_map[MAP_LENGTH];
			UCHAR* map = tail->csb_map;
			if (!map) {
				map = local_map;
				fb_assert(stream + 2 <= MAX_STREAMS);
				local_map[0] = (UCHAR) stream;
				map[1] = stream + 1;
				map[2] = stream + 2;
			}

			AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
				(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);

			sub = copy(tdbb, csb, sub, map, 0, NULL, false);

			if (relation->rel_view_rse)
			{
				// dimitr:	if we reference view columns, we need to pass them
				//			as belonging to a view (in order to compute the access
				//			permissions properly).
				AutoSetRestore<jrd_rel*> autoView(&csb->csb_view, relation);
				AutoSetRestore<USHORT> autoViewStream(&csb->csb_view_stream, stream);

				// ASF: If the view field doesn't reference an item of a stream, evaluate it
				// based on the view dbkey - CORE-1245.
				if (sub->nod_type != nod_field &&
					sub->nod_type != nod_map &&
					sub->nod_type != nod_dbkey)
				{
					NodeStack stack;
					expand_view_nodes(tdbb, csb, stream, stack, nod_dbkey, true);
					const UCHAR streamCount = (UCHAR) stack.getCount();

					if (streamCount)
					{
						jrd_nod* new_node = PAR_make_node(tdbb, e_derived_expr_length);
						new_node->nod_type = nod_derived_expr;
						new_node->nod_count = e_derived_expr_count;
						new_node->nod_arg[e_derived_expr_expr] = sub;

						USHORT* streamList = FB_NEW(*tdbb->getDefaultPool()) USHORT[streamCount];

						new_node->nod_arg[e_derived_expr_stream_list] = (jrd_nod*) streamList;
						new_node->nod_arg[e_derived_expr_stream_count] = (jrd_nod*)(IPTR) streamCount;

						for (NodeStack::iterator i(stack); i.hasData(); ++i)
							*streamList++ = (USHORT)(IPTR) i.object()->nod_arg[0];

						sub = new_node;
					}
				}

				return CMP_pass1(tdbb, csb, sub);	// note: scope of AutoSetRestore
			}

			return CMP_pass1(tdbb, csb, sub);
		}

	case nod_derived_expr:
		{
			UCHAR streamCount = (UCHAR)(IPTR) node->nod_arg[e_derived_expr_stream_count];
			USHORT* streamList = (USHORT*) node->nod_arg[e_derived_expr_stream_list];
			NodeStack stack;

			for (UCHAR i = 0; i < streamCount; ++i)
			{
				mark_variant(tdbb, csb, streamList[i]);
				expand_view_nodes(tdbb, csb, streamList[i], stack, nod_dbkey, true);
			}

			streamCount = (UCHAR) stack.getCount();
			streamList = FB_NEW(*tdbb->getDefaultPool()) USHORT[streamCount];

			node->nod_arg[e_derived_expr_stream_list] = (jrd_nod*) streamList;
			node->nod_arg[e_derived_expr_stream_count] = (jrd_nod*)(IPTR) streamCount;

			for (NodeStack::iterator i(stack); i.hasData(); ++i)
				*streamList++ = (USHORT)(IPTR) i.object()->nod_arg[0];
		}
		break;

	case nod_assignment:
		{
			sub = node->nod_arg[e_asgn_from];
			if (sub->nod_type == nod_field) {
				stream = (USHORT)(IPTR) sub->nod_arg[e_fld_stream];
				jrd_fld* field = MET_get_field(csb->csb_rpt[stream].csb_relation,
									 		   (USHORT)(IPTR) sub->nod_arg[e_fld_id]);
				if (field)
					node->nod_arg[e_asgn_missing2] = field->fld_missing_value;
			}

			sub = node->nod_arg[e_asgn_to];
			if (sub->nod_type != nod_field)
				break;
			stream = (USHORT)(IPTR) sub->nod_arg[e_fld_stream];
			tail = &csb->csb_rpt[stream];
			jrd_fld* field = MET_get_field(tail->csb_relation, (USHORT)(IPTR) sub->nod_arg[e_fld_id]);
			if (!field) {
				break;
			}
			if (field->fld_missing_value) {
				node->nod_arg[e_asgn_missing] = field->fld_missing_value;
				node->nod_count = 3;
			}
		}
		break;

	case nod_modify:
		pass1_modify(tdbb, csb, node);
		break;

	case nod_erase:
		pass1_erase(tdbb, csb, node);
		break;

	case nod_exec_proc:
		procedure = (jrd_prc*) node->nod_arg[e_esp_procedure];
		// Post access to procedure
		post_procedure_access(tdbb, csb, procedure);
		CMP_post_resource(&csb->csb_resources, procedure,
						  Resource::rsc_procedure, procedure->prc_id);
		break;

	case nod_store:
		if (pass1_store(tdbb, csb, node))
		{
			stream = (USHORT)(IPTR) node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];
			node->nod_arg[e_sto_statement] =
				make_defaults(tdbb, csb, stream, node->nod_arg[e_sto_statement]);
		}
		break;

	case nod_rse:
		return (jrd_nod*) pass1_rse(tdbb, csb, (RecordSelExpr*) node);

	case nod_cursor_stmt:
		if ((UCHAR) (IPTR) node->nod_arg[e_cursor_stmt_op] == blr_cursor_fetch) {
			node->nod_arg[e_cursor_stmt_seek] = CMP_pass1(tdbb, csb, node->nod_arg[e_cursor_stmt_seek]);
			node->nod_arg[e_cursor_stmt_into] = CMP_pass1(tdbb, csb, node->nod_arg[e_cursor_stmt_into]);
		}
		break;

	case nod_max:
	case nod_min:
	case nod_average:
	case nod_from:
	case nod_count:
	//case nod_count2:
	case nod_total:
		ignore_dbkey(tdbb, csb, (RecordSelExpr*) node->nod_arg[e_stat_rse], view);
		node->nod_arg[e_stat_rse] = CMP_pass1(tdbb, csb, node->nod_arg[e_stat_rse]);
		csb->csb_current_nodes.push(node->nod_arg[e_stat_rse]);
		node->nod_arg[e_stat_value] = CMP_pass1(tdbb, csb, node->nod_arg[e_stat_value]);
		node->nod_arg[e_stat_default] = CMP_pass1(tdbb, csb, node->nod_arg[e_stat_default]);
		csb->csb_current_nodes.pop();
		return node;

	case nod_aggregate:
		fb_assert((int) (IPTR) node->nod_arg[e_agg_stream] <= MAX_STREAMS);
		csb->csb_rpt[(USHORT)(IPTR) node->nod_arg[e_agg_stream]].csb_flags |= csb_no_dbkey;
		ignore_dbkey(tdbb, csb, (RecordSelExpr*) node->nod_arg[e_agg_rse], view);
		node->nod_arg[e_agg_rse] = CMP_pass1(tdbb, csb, node->nod_arg[e_agg_rse]);
		node->nod_arg[e_agg_map] = CMP_pass1(tdbb, csb, node->nod_arg[e_agg_map]);
		node->nod_arg[e_agg_group] = CMP_pass1(tdbb, csb, node->nod_arg[e_agg_group]);
		break;

	case nod_gen_id:
	case nod_gen_id2:
		node->nod_arg[e_gen_value] = CMP_pass1(tdbb, csb, node->nod_arg[e_gen_value]);
		return node;

	case nod_rec_version:
	case nod_dbkey:
		{
			const nod_t type = node->nod_type;
			stream = (USHORT)(IPTR) node->nod_arg[0];

			mark_variant(tdbb, csb, stream);

			if (!csb->csb_rpt[stream].csb_map)
				return node;
			NodeStack stack;
			expand_view_nodes(tdbb, csb, stream, stack, type, false);

			if (stack.hasData())
			{
				const size_t stackCount = stack.getCount();

				// If that is a DB_KEY of a view, it's possible (in case of
				// outer joins) that some sub-stream have a NULL DB_KEY.
				// In this case, we build a COALESCE(DB_KEY, _OCTETS x"0000000000000000"),
				// for the concatenation of sub DB_KEYs not result in NULL.
				if (type == nod_dbkey && stackCount > 1)
				{
					NodeStack stack2;

					for (NodeStack::iterator i(stack); i.hasData(); ++i)
					{
						jrd_nod* new_node = PAR_make_node(tdbb, 3);
						new_node->nod_type = nod_value_if;
						new_node->nod_count = 3;

						// build an IF (RDB$DB_KEY IS NOT NULL)
						new_node->nod_arg[0] = PAR_make_node(tdbb, 1);
						new_node->nod_arg[0]->nod_type = nod_not;
						new_node->nod_arg[0]->nod_count = 1;
						new_node->nod_arg[0]->nod_arg[0] = PAR_make_node(tdbb, 1);
						new_node->nod_arg[0]->nod_arg[0]->nod_type = nod_missing;
						new_node->nod_arg[0]->nod_arg[0]->nod_count = 1;
						new_node->nod_arg[0]->nod_arg[0]->nod_arg[0] = i.object();

						new_node->nod_arg[1] = i.object();	// THEN

						const SSHORT count = lit_delta +
							(0 + sizeof(jrd_nod*) - 1) / sizeof(jrd_nod*);
						new_node->nod_arg[2] = PAR_make_node(tdbb, count);	// ELSE
						new_node->nod_arg[2]->nod_type = nod_literal;
						new_node->nod_arg[2]->nod_count = 0;
						Literal* literal = (Literal*) new_node->nod_arg[2];
						literal->lit_desc.dsc_dtype = dtype_text;
						literal->lit_desc.dsc_ttype() = CS_BINARY;
						literal->lit_desc.dsc_scale = 0;
						literal->lit_desc.dsc_length = 8;
						literal->lit_desc.dsc_address = reinterpret_cast<UCHAR*>(
							const_cast<char*>("\0\0\0\0\0\0\0\0"));	// safe const_cast

						stack2.push(new_node);
					}

					stack.clear();

					// stack2 is in reverse order, pushing everything in stack
					// will correct the order.
					for (NodeStack::iterator i2(stack2); i2.hasData(); ++i2)
						stack.push(i2.object());
				}

				node = catenate_nodes(tdbb, stack);

				if (type == nod_dbkey && stackCount > 1)
				{
					// ASF: If the view is in null state (with outer joins) we need to transform
					// the view RDB$KEY to NULL. (CORE-1245)

					jrd_nod* new_node = PAR_make_node(tdbb, 3);
					new_node->nod_type = nod_value_if;
					new_node->nod_count = 3;

					// build an IF (RDB$DB_KEY = '')
					new_node->nod_arg[0] = PAR_make_node(tdbb, 2);
					new_node->nod_arg[0]->nod_type = nod_eql;
					new_node->nod_arg[0]->nod_flags = nod_comparison;
					new_node->nod_arg[0]->nod_arg[0] = copy(tdbb, csb, node, NULL, 0, NULL, false);
					const SSHORT count = lit_delta + (0 + sizeof(jrd_nod*) - 1) / sizeof(jrd_nod*);
					new_node->nod_arg[0]->nod_arg[1] = PAR_make_node(tdbb, count);
					new_node->nod_arg[0]->nod_arg[1]->nod_type = nod_literal;
					new_node->nod_arg[0]->nod_arg[1]->nod_count = 0;
					Literal* literal = (Literal*) new_node->nod_arg[0]->nod_arg[1];
					literal->lit_desc.dsc_dtype = dtype_text;
					literal->lit_desc.dsc_ttype() = CS_BINARY;
					literal->lit_desc.dsc_scale = 0;
					literal->lit_desc.dsc_length = 0;
					literal->lit_desc.dsc_address = reinterpret_cast<UCHAR*>(literal->lit_data);

					new_node->nod_arg[1] = PAR_make_node(tdbb, 0);	// THEN: NULL
					new_node->nod_arg[1]->nod_type = nod_null;
					new_node->nod_arg[2] = node;					// ELSE: RDB$DB_KEY

					node = new_node;
				}

				return node;
			}

			// The user is asking for the dbkey/record version of an aggregate.
			// Humor him with a key filled with zeros.

			node = PAR_make_node(tdbb, 1);
			node->nod_count = 0;
			node->nod_type = type;
			node->nod_flags |= nod_agg_dbkey;
			node->nod_arg[0] = (jrd_nod*) (IPTR) stream;
			return node;
		}

	case nod_abort:
		CMP_pass1(tdbb, csb, node->nod_arg[e_xcp_msg]);
		break;

	case nod_not:
		sub = node->nod_arg[0];
		if (sub->nod_type == nod_ansi_any)
			sub->nod_flags |= nod_deoptimize;
		break;

	case nod_ansi_all:
		{
			jrd_nod* newNode = convertNeqAllToNotAny(tdbb, csb, node);
			if (newNode)
				return CMP_pass1(tdbb, csb, newNode);

			node->nod_flags |= nod_deoptimize;
		}
		// fall into

	case nod_ansi_any:
		if (node->nod_flags & nod_deoptimize)
		{
			node->nod_flags &= ~nod_deoptimize;
			// Deoptimize the conjunct, not the ALL node itself
			jrd_nod* boolean = ((RecordSelExpr*) (node->nod_arg[e_any_rse]))->rse_boolean;
			if (boolean)
			{
				if (boolean->nod_type == nod_and)
				{
					boolean = boolean->nod_arg[1];
				}
				// Deoptimize the injected boolean of a quantified predicate
				// when it's necessary. ALL predicate does not require an index scan.
				// This fixes bug SF #543106.
				boolean->nod_flags |= nod_deoptimize;
			}
		}
		// fall into

	case nod_any:
	case nod_exists:
	case nod_unique:
		ignore_dbkey(tdbb, csb, (RecordSelExpr*) node->nod_arg[e_any_rse], view);
		break;

	case nod_src_info:
		node->nod_arg[e_src_info_node] = CMP_pass1(tdbb, csb, node->nod_arg[e_src_info_node]);
		return node;

	case nod_class_node_jrd:
		node->nod_arg[0] = reinterpret_cast<jrd_nod*>(
			reinterpret_cast<DmlNode*>(node->nod_arg[0])->pass1(tdbb, csb));
		return node;

	case nod_dcl_variable:
	{
		const USHORT n = (USHORT)(IPTR) node->nod_arg[e_dcl_id];
		vec<jrd_nod*>* vector = csb->csb_variables =
			vec<jrd_nod*>::newVector(*tdbb->getDefaultPool(), csb->csb_variables, n + 1);
		fb_assert(!(*vector)[n]);
		(*vector)[n] = node;
		break;
	}

	default:
		break;
	}

	// handle sub-expressions here

	ptr = node->nod_arg;

	for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++) {
		*ptr = CMP_pass1(tdbb, csb, *ptr);
	}

	// perform any post-processing here

	if (node->nod_type == nod_assignment)
	{
		sub = node->nod_arg[e_asgn_to];
		switch (sub->nod_type)
		{
		case nod_field:
			stream = (USHORT)(IPTR) sub->nod_arg[e_fld_stream];
			tail = &csb->csb_rpt[stream];
			// assignments to the OLD context are prohibited
			// for all trigger types
			if ((tail->csb_flags & csb_trigger) && stream == 0)
			{
				ERR_post(Arg::Gds(isc_read_only_field));
			}
			// assignments to the NEW context are prohibited
			// for post-action triggers
			if ((tail->csb_flags & csb_trigger) && stream == 1 &&
				(csb->csb_g_flags & csb_post_trigger))
			{
				ERR_post(Arg::Gds(isc_read_only_field));
			}
			break;
		case nod_argument:
		case nod_variable:
		case nod_null:
			// Nothing to do here
			break;
		default:
			ERR_post(Arg::Gds(isc_read_only_field));
		}
	}

	return node;
}


static void pass1_erase(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	p a s s 1 _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Checkout an erase statement.  If it references a view, and
 *	is kosher, fix it up.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	// if updateable views with triggers are involved, there
	// maybe a recursive call to be ignored

	if (node->nod_arg[e_erase_sub_erase])
		return;

	// to support nested views, loop until we hit a table or
	// a view with user-defined triggers (which means no update)

	jrd_rel* parent = NULL;
	jrd_rel* view = NULL;
	USHORT parent_stream = 0;

	for (;;) {
		USHORT new_stream = (USHORT)(IPTR) node->nod_arg[e_erase_stream];
		const USHORT stream = new_stream;

		CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];
		tail->csb_flags |= csb_erase;

		jrd_rel* relation = csb->csb_rpt[stream].csb_relation;
		view = (relation->rel_view_rse) ? relation : view;
		if (!parent) {
			parent = tail->csb_view;
		}

		post_trigger_access(csb, relation, ExternalAccess::exa_delete, view);

		// Check out delete. If this is a delete thru a view, verify the
		// view by checking for read access on the base table. If field-level select
		// privileges are implemented, this needs to be enhanced.

		SecurityClass::flags_t priv = SCL_sql_delete;
		if (parent) {
			priv |= SCL_read;
		}

		const trig_vec* trigger = (relation->rel_pre_erase) ?
			relation->rel_pre_erase : relation->rel_post_erase;

		// if we have a view with triggers, let's expand it

		if (relation->rel_view_rse && trigger) {

			new_stream = csb->nextStream();
			node->nod_arg[e_erase_stream] = (jrd_nod*) (IPTR) new_stream;
			CMP_csb_element(csb, new_stream)->csb_relation = relation;

			node->nod_arg[e_erase_statement] = pass1_expand_view(tdbb, csb, stream, new_stream, false);
			node->nod_count = MAX(node->nod_count, (USHORT) e_erase_statement + 1);
		}

		// get the source relation, either a table or yet another view

		jrd_nod* source =
			pass1_update(tdbb, csb, relation, trigger, stream, new_stream,
						 priv, parent, parent_stream);

		if (!source) {

			// no source means we're done

			return;
		}

		parent = relation;
		parent_stream = stream;

		// remap the source stream

		UCHAR* map = csb->csb_rpt[stream].csb_map;

		if (trigger) {

			// set up the new target stream

			jrd_nod* view_node = copy(tdbb, csb, node, map, 0, NULL, false);

			view_node->nod_arg[e_erase_statement] = NULL;
			view_node->nod_arg[e_erase_sub_erase] = NULL;

			node->nod_arg[e_erase_sub_erase] = view_node;
			node->nod_count = MAX(node->nod_count, (USHORT) e_erase_sub_erase + 1);

			// substitute the original delete node with the newly created one

			node = view_node;
		}
		else {

			// this relation is not actually being updated as this operation
			// goes deeper (we have a naturally updatable view)

			csb->csb_rpt[new_stream].csb_flags &= ~csb_view_update;
		}

		// let's reset the target stream

		new_stream = (USHORT)(IPTR) source->nod_arg[e_rel_stream];
		node->nod_arg[e_erase_stream] = (jrd_nod*)(IPTR) map[new_stream];
	}
}


static jrd_nod* pass1_expand_view(thread_db* tdbb,
								 CompilerScratch* csb,
								 USHORT org_stream,
								 USHORT new_stream,
								 bool remap)
{
/**************************************
 *
 *	p a s s 1 _ e x p a n d _ v i e w
 *
 **************************************
 *
 * Functional description
 *	Process a view update performed by a trigger.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	NodeStack stack;
	jrd_rel* relation = csb->csb_rpt[org_stream].csb_relation;
	vec<jrd_fld*>* fields = relation->rel_fields;

	dsc desc;
	USHORT id = 0, new_id = 0;
	vec<jrd_fld*>::iterator ptr = fields->begin();
	for (const vec<jrd_fld*>::const_iterator end = fields->end(); ptr < end; ++ptr, ++id)
	{
		if (*ptr) {
			if (remap) {
				const jrd_fld* field = MET_get_field(relation, id);
				if (field->fld_source) {
					new_id = (USHORT)(IPTR) (field->fld_source)->nod_arg[e_fld_id];
				}
				else {
					new_id = id;
				}
			}
			else {
				new_id = id;
			}
			jrd_nod* node = PAR_gen_field(tdbb, new_stream, new_id);
			CMP_get_desc(tdbb, csb, node, &desc);
			if (!desc.dsc_address) {
				delete node;
				continue;
			}
			jrd_nod* assign = PAR_make_node(tdbb, e_asgn_length);
			assign->nod_type = nod_assignment;
			assign->nod_arg[e_asgn_to] = node;
			assign->nod_arg[e_asgn_from] = PAR_gen_field(tdbb, org_stream, id);
			stack.push(assign);
		}
	}

	return PAR_make_list(tdbb, stack);
}


static void pass1_modify(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	p a s s 1 _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Process a source for a modify statement.  This can
 *	get a little tricky if the relation is a view.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	// if updateable views with triggers are involved, there
	// maybe a recursive call to be ignored

	if (node->nod_arg[e_mod_sub_mod])
		return;

	jrd_rel* parent = NULL;
	jrd_rel* view = NULL;
	USHORT parent_stream = 0;

	// to support nested views, loop until we hit a table or
	// a view with user-defined triggers (which means no update)

	for (;;) {
		USHORT stream = (USHORT)(IPTR) node->nod_arg[e_mod_org_stream];
		USHORT new_stream = (USHORT)(IPTR) node->nod_arg[e_mod_new_stream];

		CompilerScratch::csb_repeat* tail = &csb->csb_rpt[new_stream];
		tail->csb_flags |= csb_modify;

		jrd_rel* relation = csb->csb_rpt[stream].csb_relation;
		view = (relation->rel_view_rse) ? relation : view;
		if (!parent) {
			parent = tail->csb_view;
		}

		post_trigger_access(csb, relation, ExternalAccess::exa_update, view);

		// Check out update. If this is an update thru a view, verify the
		// view by checking for read access on the base table. If field-level select
		// privileges are implemented, this needs to be enhanced.

		SecurityClass::flags_t priv = SCL_sql_update;
		if (parent) {
			priv |= SCL_read;
		}

		const trig_vec* trigger = (relation->rel_pre_modify) ?
			relation->rel_pre_modify : relation->rel_post_modify;

		// if we have a view with triggers, let's expand it

		if (relation->rel_view_rse && trigger) {

			node->nod_arg[e_mod_map_view] = pass1_expand_view(tdbb, csb, stream, new_stream, false);
			node->nod_count = MAX(node->nod_count, (USHORT) e_mod_map_view + 1);
		}

		// get the source relation, either a table or yet another view

		jrd_nod* source = pass1_update(tdbb, csb, relation, trigger, stream,
									   new_stream, priv, parent, parent_stream);

		if (!source) {

			// no source means we're done

			if (!relation->rel_view_rse) {

				// apply validation constraints

				if ( (node->nod_arg[e_mod_validate] = make_validation(tdbb, csb, new_stream)) )
				{
					node->nod_count = MAX(node->nod_count, (USHORT) e_mod_validate + 1);
				}
			}

			return;
		}

		parent = relation;
		parent_stream = stream;

		// remap the source stream

		UCHAR* map = csb->csb_rpt[stream].csb_map;

		stream = (USHORT)(IPTR) source->nod_arg[e_rel_stream];
		stream = map[stream];

		// copy the view source

		map = alloc_map(tdbb, csb, (SSHORT)(IPTR) node->nod_arg[e_mod_new_stream]);
		source = copy(tdbb, csb, source, map, 0, NULL, false);

		if (trigger) {

			// set up the new target stream

			const USHORT view_stream = new_stream;
			new_stream = (USHORT)(IPTR) source->nod_arg[e_rel_stream];
			fb_assert(new_stream <= MAX_STREAMS);
			map[view_stream] = new_stream;

			jrd_nod* view_node = copy(tdbb, csb, node, map, 0, NULL, true);
			view_node->nod_arg[e_mod_map_view] = NULL;
			view_node->nod_arg[e_mod_statement] =
				pass1_expand_view(tdbb, csb, view_stream, new_stream, true);

			node->nod_arg[e_mod_sub_mod] = view_node;
			node->nod_count = MAX(node->nod_count, (USHORT) e_mod_sub_mod + 1);

			// substitute the original update node with the newly created one

			node = view_node;
		}
		else {

			// this relation is not actually being updated as this operation
			// goes deeper (we have a naturally updatable view)

			csb->csb_rpt[new_stream].csb_flags &= ~csb_view_update;

		}

		// let's reset streams to represent the mapped source and target

		node->nod_arg[e_mod_org_stream] = (jrd_nod*)(IPTR) stream;
		node->nod_arg[e_mod_new_stream] = source->nod_arg[e_rel_stream];
	}
}


static RecordSelExpr* pass1_rse(thread_db* tdbb,
					 CompilerScratch* csb,
					 RecordSelExpr* rse)
{
/**************************************
 *
 *	p a s s 1 _ r s e
 *
 **************************************
 *
 * Functional description
 *	Process a record select expression during pass 1 of compilation.
 *	Mostly this involves expanding views.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);
	DEV_BLKCHK(view, type_rel);

	// for scoping purposes, maintain a stack of RecordSelExpr's which are
	// currently being parsed; if there are none on the stack as
	// yet, mark the RecordSelExpr as variant to make sure that statement-
	// level aggregates are not treated as invariants -- bug #6535

	bool top_level_rse = true;
	for (jrd_node_base **i_node = csb->csb_current_nodes.begin();
		 i_node < csb->csb_current_nodes.end(); i_node++)
	{
		if ((*i_node)->nod_type == nod_rse)
		{
			top_level_rse = false;
			break;
		}
	}
	if (top_level_rse)
		rse->nod_flags |= rse_variant;

	csb->csb_current_nodes.push(rse);

	NodeStack stack;
	jrd_nod* boolean = NULL;
	jrd_nod* sort = rse->rse_sorted;
	jrd_nod* project = rse->rse_projection;
	jrd_nod* first = rse->rse_first;
	jrd_nod* skip = rse->rse_skip;
	jrd_nod* plan = rse->rse_plan;
	const bool writelock = rse->rse_writelock;
#ifdef SCROLLABLE_CURSORS
	jrd_nod* async_message = rse->rse_async_message;
#endif

	// zip thru RecordSelExpr expanding views and inner joins
	jrd_nod** arg = rse->rse_relation;
	for (const jrd_nod* const* const end = arg + rse->rse_count; arg < end; arg++)
	{
		pass1_source(tdbb, csb, rse, *arg, &boolean, stack);
	}

	// Now, rebuild the RecordSelExpr block. If possible, re-use the old block,
	// otherwise allocate a new one.

	USHORT count = stack.getCount();

	if (count != rse->rse_count) {
		RecordSelExpr* new_rse = (RecordSelExpr*) PAR_make_node(tdbb, count + rse_delta + 2);
		*new_rse = *rse;
		new_rse->rse_count = count;
		rse = new_rse;

		// AB: Because we've build an new RecordSelExpr, we must put this one in the stack
		// of current_rses else could RecordSelExpr's not be flagged an rse_variant.
		// See SF BUG # [ 523589 ] for an example.

		csb->csb_current_nodes.pop();
		csb->csb_current_nodes.push(new_rse);
	}


	arg = rse->rse_relation + count;

	while (stack.hasData())
	{
		*--arg = stack.pop();
	}

	AutoSetRestore<bool> autoValidateExpr(&csb->csb_validate_expr, false);

	// finish of by processing other clauses

	if (first) {
		rse->rse_first = CMP_pass1(tdbb, csb, first);
	}
	if (skip) {
		rse->rse_skip = CMP_pass1(tdbb, csb, skip);
	}

	if (boolean) {
		if (rse->rse_boolean) {
			jrd_nod* additional = PAR_make_node(tdbb, 2);
			additional->nod_type = nod_and;
			additional->nod_arg[0] = boolean;
			additional->nod_arg[1] = CMP_pass1(tdbb, csb, rse->rse_boolean);
			rse->rse_boolean = additional;
		}
		else {
			rse->rse_boolean = boolean;
		}
	}
	else
		rse->rse_boolean = CMP_pass1(tdbb, csb, rse->rse_boolean);

	if (sort)
		rse->rse_sorted = CMP_pass1(tdbb, csb, sort);

	if (project)
		rse->rse_projection = CMP_pass1(tdbb, csb, project);

	if (plan) {
		rse->rse_plan = plan;
	}

	rse->rse_writelock = writelock;

#ifdef SCROLLABLE_CURSORS
	if (async_message) {
		rse->rse_async_message = CMP_pass1(tdbb, csb, async_message);
		csb->csb_async_message = rse->rse_async_message;
	}
#endif

	// we are no longer in the scope of this RecordSelExpr

	csb->csb_current_nodes.pop();

	return rse;
}


static void pass1_source(thread_db*			tdbb,
						 CompilerScratch*	csb,
						 RecordSelExpr*		rse,
						 jrd_nod*	source,
						 jrd_nod**	boolean,
						 NodeStack&	stack)
{
/**************************************
 *
 *	p a s s 1 _ s o u r c e
 *
 **************************************
 *
 * Functional description
 *	Process a single record source stream from an RecordSelExpr.  Obviously,
 *	if the source is a view, there is more work to do.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);
	DEV_BLKCHK(source, type_nod);
	DEV_BLKCHK(*boolean, type_nod);
	DEV_BLKCHK(parent_view, type_rel);

	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	AutoSetRestore<bool> autoValidateExpr(&csb->csb_validate_expr, false);

	// in the case of an RecordSelExpr, it is possible that a new RecordSelExpr will be generated,
	// so wait to process the source before we push it on the stack (bug 8039)

	if (source->nod_type == nod_rse)
	{
		// The addition of the JOIN syntax for specifying inner joins causes an
		// RecordSelExpr tree to be generated, which is undesirable in the simplest case
		// where we are just trying to inner join more than 2 streams. If possible,
		// try to flatten the tree out before we go any further.

		RecordSelExpr* sub_rse = (RecordSelExpr*) source;
		if (!rse->rse_jointype && !sub_rse->rse_jointype &&
			!sub_rse->rse_sorted && !sub_rse->rse_projection &&
			!sub_rse->rse_first && !sub_rse->rse_skip && !sub_rse->rse_plan)
		{
			jrd_nod** arg = sub_rse->rse_relation;
			for (const jrd_nod* const* const end = arg + sub_rse->rse_count; arg < end; arg++)
			{
				pass1_source(tdbb, csb, rse, *arg, boolean, stack);
			}
			// fold in the boolean for this inner join with the one for the parent

			if (sub_rse->rse_boolean) {

				jrd_nod* node = CMP_pass1(tdbb, csb, sub_rse->rse_boolean);

				if (*boolean) {
					jrd_nod* additional = PAR_make_node(tdbb, 2);
					additional->nod_type = nod_and;
					additional->nod_arg[0] = node;
					additional->nod_arg[1] = *boolean;
					*boolean = additional;
				}
				else {
					*boolean = node;
				}
			}

			return;
		}

		source = CMP_pass1(tdbb, csb, source);
		stack.push(source);
		return;
	}

	// Assume that the source will be used. Push it on the final stream stack.

	stack.push(source);

	// special case: procedure

	if (source->nod_type == nod_procedure) {
		CMP_pass1(tdbb, csb, source);
		jrd_prc* const procedure =
			MET_lookup_procedure_id(tdbb, (SSHORT)(IPTR) source->nod_arg[e_prc_procedure], false, false, 0);
		post_procedure_access(tdbb, csb, procedure);
		CMP_post_resource(&csb->csb_resources, procedure, Resource::rsc_procedure, procedure->prc_id);

		jrd_rel* const parent_view = csb->csb_view;
		const USHORT view_stream = csb->csb_view_stream;
		source->nod_arg[e_prc_view] = (jrd_nod*) parent_view;

		const USHORT stream = (USHORT)(IPTR) source->nod_arg[e_prc_stream];
		CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, stream);
		element->csb_view = parent_view;
		fb_assert(view_stream <= MAX_STREAMS);
		element->csb_view_stream = (UCHAR) view_stream;

		if (parent_view)
		{
			ViewContexts& ctx = parent_view->rel_view_contexts;
			const USHORT key = (USHORT)(IPTR) source->nod_arg[e_prc_context];
			size_t pos;
			if (ctx.find(key, pos)) {
				element->csb_alias = FB_NEW(csb->csb_pool)
					Firebird::string(csb->csb_pool, ctx[pos]->vcx_context_name);
			}
		}
		return;
	}

	// special case: union

	if (source->nod_type == nod_union) {
		CMP_pass1(tdbb, csb, source->nod_arg[e_uni_clauses]);
		return;
	}

	// special case: group-by/global aggregates

	if (source->nod_type == nod_aggregate) {
		fb_assert((int) (IPTR) source->nod_arg[e_agg_stream] <= MAX_STREAMS);
		CMP_pass1(tdbb, csb, source);
		return;
	}

	// All the special cases are exhausted, so we must have a view or a base table;
	// prepare to check protection of relation when a field in the stream of the
	// relation is accessed.

	jrd_rel* const parent_view = csb->csb_view;
	const USHORT view_stream = csb->csb_view_stream;

	jrd_rel* view = (jrd_rel*) source->nod_arg[e_rel_relation];
	CMP_post_resource(&csb->csb_resources, view, Resource::rsc_relation, view->rel_id);
	source->nod_arg[e_rel_view] = (jrd_nod*) parent_view;

	const USHORT stream = (USHORT)(IPTR) source->nod_arg[e_rel_stream];
	CompilerScratch::csb_repeat* const element = CMP_csb_element(csb, stream);
	element->csb_view = parent_view;
	fb_assert(view_stream <= MAX_STREAMS);
	element->csb_view_stream = (UCHAR) view_stream;

	// in the case where there is a parent view, find the context name

	if (parent_view) {
		ViewContexts& ctx = parent_view->rel_view_contexts;
		const USHORT key = (USHORT)(IPTR) source->nod_arg[e_rel_context];
		size_t pos;
		if (ctx.find(key, pos)) {
			element->csb_alias = FB_NEW(csb->csb_pool)
				Firebird::string(csb->csb_pool, ctx[pos]->vcx_context_name);
		}
	}

	// check for a view - if not, nothing more to do

	RecordSelExpr* view_rse = view->rel_view_rse;
	if (!view_rse) {
		return;
	}

	// we've got a view, expand it

	DEBUG;
	stack.pop();
	UCHAR* map = alloc_map(tdbb, csb, stream);

	AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
		(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);
	AutoSetRestore<jrd_rel*> autoView(&csb->csb_view, view);
	AutoSetRestore<USHORT> autoViewStream(&csb->csb_view_stream, stream);

	// We don't expand the view in two cases:
	// 1) If the view has a projection, sort, first/skip or explicit plan.
	// 2) If it's part of an outer join.

	if (rse->rse_jointype || // view_rse->rse_jointype || ???
		view_rse->rse_sorted || view_rse->rse_projection || view_rse->rse_first ||
		view_rse->rse_skip || view_rse->rse_plan)
	{
		jrd_nod* node = copy(tdbb, csb, (jrd_nod*) view_rse, map, 0, NULL, false);
		DEBUG;
		stack.push(CMP_pass1(tdbb, csb, node));
		DEBUG;
		return;
	}

	// if we have a projection which we can bubble up to the parent rse, set the
	// parent rse to our projection temporarily to flag the fact that we have already
	// seen one so that lower-level views will not try to map their projection; the
	// projection will be copied and correctly mapped later, but we don't have all
	// the base streams yet

	if (view_rse->rse_projection) {
		rse->rse_projection = view_rse->rse_projection;
	}

	// disect view into component relations

	jrd_nod** arg = view_rse->rse_relation;
	for (const jrd_nod* const* const end = arg + view_rse->rse_count; arg < end; arg++)
	{
		// this call not only copies the node, it adds any streams it finds to the map

		jrd_nod* node = copy(tdbb, csb, *arg, map, 0, NULL, false);

		// Now go out and process the base table itself. This table might also be a view,
		// in which case we will continue the process by recursion.

		pass1_source(tdbb, csb, rse, node, boolean, stack);
	}

	// When there is a projection in the view, copy the projection up to the query RecordSelExpr.
	// In order to make this work properly, we must remap the stream numbers of the fields
	// in the view to the stream number of the base table. Note that the map at this point
	// contains the stream numbers of the referenced relations, since it was added during the call
	// to copy() above. After the copy() below, the fields in the projection will reference the
	// base table(s) instead of the view's context (see bug #8822), so we are ready to context-
	// recognize them in CMP_pass1() - that is, replace the field nodes with actual field blocks.

	if (view_rse->rse_projection) {
		rse->rse_projection =
			CMP_pass1(tdbb, csb, copy(tdbb, csb, view_rse->rse_projection, map, 0, NULL, false));
	}

	// if we encounter a boolean, copy it and retain it by ANDing it in with the
	// boolean on the parent view, if any

	if (view_rse->rse_boolean) {
		jrd_nod* node =
			CMP_pass1(tdbb, csb, copy(tdbb, csb, view_rse->rse_boolean, map, 0, NULL, false));
		if (*boolean) {
			// The order of the nodes here is important! The
			// boolean from the view must appear first so that
			// it gets expanded first in pass1.

			jrd_nod* additional = PAR_make_node(tdbb, 2);
			additional->nod_type = nod_and;
			additional->nod_arg[0] = node;
			additional->nod_arg[1] = *boolean;
			*boolean = additional;
		}
		else {
			*boolean = node;
		}
	}

	return;
}


static bool pass1_store(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	p a s s 1 _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Process a source for a store statement.  This can get a little tricky if
 *	the relation is a view.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	// if updateable views with triggers are involved, there
	// may be a recursive call to be ignored

	if (node->nod_arg[e_sto_sub_store])
		return false;

	jrd_rel* parent = NULL;
	jrd_rel* view = NULL;
	USHORT parent_stream = 0;

	// to support nested views, loop until we hit a table or
	// a view with user-defined triggers (which means no update)

	for (;;) {
		const USHORT stream = (USHORT)(IPTR) node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];

		CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];
		tail->csb_flags |= csb_store;

		jrd_rel* relation = csb->csb_rpt[stream].csb_relation;
		view = (relation->rel_view_rse) ? relation : view;
		if (!parent) {
			parent = tail->csb_view;
		}

		post_trigger_access(csb, relation, ExternalAccess::exa_insert, view);

		const trig_vec* trigger = (relation->rel_pre_store) ?
			relation->rel_pre_store : relation->rel_post_store;

		// Check out insert. If this is an insert thru a view, verify the
		// view by checking for read access on the base table. If field-level select
		// privileges are implemented, this needs to be enhanced.

		SecurityClass::flags_t priv = SCL_sql_insert;
		if (parent) {
			priv |= SCL_read;
		}

		// get the source relation, either a table or yet another view

		jrd_nod* source =
			 pass1_update(tdbb, csb, relation, trigger, stream, stream, priv, parent, parent_stream);

		if (!source) {

			CMP_post_resource(&csb->csb_resources, relation, Resource::rsc_relation, relation->rel_id);

			if (!relation->rel_view_rse) {

				// apply validation constraints

				if ( (node->nod_arg[e_sto_validate] = make_validation(tdbb, csb, stream)) )
				{
					node->nod_count = MAX(node->nod_count, (USHORT) e_sto_validate + 1);
				}
			}

			return true;
		}

		parent = relation;
		parent_stream = stream;

		UCHAR* map = alloc_map(tdbb, csb, stream);

		if (trigger) {

			CMP_post_resource(&csb->csb_resources, relation, Resource::rsc_relation, relation->rel_id);

			// set up the new target stream

			jrd_nod* view_node = copy(tdbb, csb, node, map, 0, NULL, false);
			view_node->nod_arg[e_sto_sub_store] = NULL;
			view_node->nod_arg[e_sto_relation] = copy(tdbb, csb, source, map, 0, NULL, false);
			const USHORT new_stream =
				(USHORT)(IPTR) view_node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];
			view_node->nod_arg[e_sto_statement] =
				pass1_expand_view(tdbb, csb, stream, new_stream, true);

// dimitr:	I don't think the below code is required, but time will show
//			view_node->nod_arg[e_sto_statement] =
//				copy(tdbb, csb, view_node->nod_arg[e_sto_statement], NULL, 0, NULL, false);

			// bug 8150: use of blr_store2 against a view with a trigger was causing
			// the second statement to be executed, which is not desirable

			view_node->nod_arg[e_sto_statement2] = NULL;

			node->nod_arg[e_sto_sub_store] = view_node;
			node->nod_count = MAX(node->nod_count, (USHORT) e_sto_sub_store + 1);

			// substitute the original update node with the newly created one

			node = view_node;
		}
		else {

			// this relation is not actually being updated as this operation
			// goes deeper (we have a naturally updatable view)

			csb->csb_rpt[stream].csb_flags &= ~csb_view_update;

			node->nod_arg[e_sto_relation] = copy(tdbb, csb, source, map, 0, NULL, false);
		}
	}
}


static jrd_nod* pass1_update(thread_db* tdbb,
							CompilerScratch* csb,
							jrd_rel* relation,
							const trig_vec* trigger,
							USHORT stream,
							USHORT update_stream,
							SecurityClass::flags_t priv,
							jrd_rel* view,
							USHORT view_stream)
{
/**************************************
 *
 *	p a s s 1 _ u p d a t e
 *
 **************************************
 *
 * Functional description
 *	Check out a prospective update to a relation.  If it fails
 *	security check, bounce it.  If it's a view update, make sure
 *	the view is updatable,  and return the view source for redirection.
 *	If it's a simple relation, return NULL.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(relation, type_rel);
	DEV_BLKCHK(view, type_rel);

	// unless this is an internal request, check access permission

	CMP_post_access(tdbb, csb, relation->rel_security_name, (view ? view->rel_id : 0),
					priv, object_table, relation->rel_name);

	// ensure that the view is set for the input streams,
	// so that access to views can be checked at the field level

	fb_assert(view_stream <= MAX_STREAMS);
	CMP_csb_element(csb, stream)->csb_view = view;
	CMP_csb_element(csb, stream)->csb_view_stream = (UCHAR) view_stream;
	CMP_csb_element(csb, update_stream)->csb_view = view;
	CMP_csb_element(csb, update_stream)->csb_view_stream = (UCHAR) view_stream;

	// if we're not a view, everything's cool

	RecordSelExpr* rse = relation->rel_view_rse;
	if (!rse) {
		return NULL;
	}

	// a view with triggers is always updatable

	if (trigger)
	{
		bool user_triggers = false;
		for (size_t i = 0; i < trigger->getCount(); i++)
		{
			if (!(*trigger)[i].sys_trigger)
			{
				user_triggers = true;
				break;
			}
		}
		if (user_triggers)
		{
			csb->csb_rpt[update_stream].csb_flags |= csb_view_update;
			return NULL;
		}
	}

	// we've got a view without triggers, let's check whether it's updateable

	const jrd_nod* node;
	if (rse->rse_count != 1 || rse->rse_projection || rse->rse_sorted ||
		!(node = rse->rse_relation[0]) || node->nod_type != nod_relation)
	{
		ERR_post(Arg::Gds(isc_read_only_view) << Arg::Str(relation->rel_name));
	}

	// for an updateable view, return the view source

	csb->csb_rpt[update_stream].csb_flags |= csb_view_update;
	return rse->rse_relation[0];
}


static jrd_nod* pass2_validation(thread_db* tdbb, CompilerScratch* csb, const Item& item)
{
/**************************************
 *
 *	p a s s 2 _ v a l i d a t i o n
 *
 **************************************
 *
 * Functional description
 *	Copy items' information into appropriate node
 *
 **************************************/
	ItemInfo itemInfo;
	return csb->csb_map_item_info.get(item, itemInfo) ?
		reinterpret_cast<jrd_nod*>(FB_NEW(*tdbb->getDefaultPool())
			ItemInfo(*tdbb->getDefaultPool(), itemInfo)) : 0;
}


jrd_nod* CMP_pass2(thread_db* tdbb, CompilerScratch* csb, jrd_nod* const node, jrd_nod* parent)
{
/**************************************
 *
 *	C M P _ p a s s 2
 *
 **************************************
 *
 * Functional description
 *	Allocate and assign impure space for various nodes.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);
	DEV_BLKCHK(parent, type_nod);

	if (!node) {
		return node;
	}

	if (parent) {
		node->nod_parent = parent;
	}

	// if there is processing to be done before sub expressions, do it here
	USHORT stream;

	DEBUG;
	RecordSource** rsb_ptr = 0;
	jrd_nod* rse_node = NULL;

	switch (node->nod_type)
	{
	case nod_rse:
		return NULL;

	case nod_union:
		return pass2_union(tdbb, csb, node);

	case nod_for:
		rse_node = node->nod_arg[e_for_re];
		rsb_ptr = (RecordSource**) & node->nod_arg[e_for_rsb];
#ifdef SCROLLABLE_CURSORS
		csb->csb_current_rse = rse_node;
#endif
		break;

	case nod_dcl_cursor:
		rse_node = node->nod_arg[e_dcl_cursor_rse];
		rsb_ptr = (RecordSource**) & node->nod_arg[e_dcl_cursor_rsb];
#ifdef SCROLLABLE_CURSORS
		csb->csb_current_rse = rse_node;
#endif
		break;

	case nod_cursor_stmt:
		if ((UCHAR) (IPTR) node->nod_arg[e_cursor_stmt_op] == blr_cursor_fetch) {
			CMP_pass2(tdbb, csb, node->nod_arg[e_cursor_stmt_seek], node);
			CMP_pass2(tdbb, csb, node->nod_arg[e_cursor_stmt_into], node);
		}
		break;

#ifdef SCROLLABLE_CURSORS
	case nod_seek:
		// store the RecordSelExpr in whose scope we are defined
		node->nod_arg[e_seek_rse] = (jrd_nod*) csb->csb_current_rse;
		break;
#endif

	case nod_max:
	case nod_min:
	case nod_count:
	//case nod_count2:
	case nod_average:
	case nod_total:
	case nod_from:
		rse_node = node->nod_arg[e_stat_rse];
		if (! rse_node) {
			ERR_post(Arg::Gds(isc_wish_list));
		}
		if (!(rse_node->nod_flags & rse_variant)) {
			node->nod_flags |= nod_invariant;
			csb->csb_invariants.push(node);
		}
		rsb_ptr = (RecordSource**) &node->nod_arg[e_stat_rsb];
		break;

	case nod_ansi_all:
	case nod_ansi_any:
	case nod_any:
	case nod_exists:
	case nod_unique:
		rse_node = node->nod_arg[e_any_rse];
		if (!(rse_node->nod_flags & rse_variant)) {
			node->nod_flags |= nod_invariant;
			csb->csb_invariants.push(node);
		}
		rsb_ptr = (RecordSource**) &node->nod_arg[e_any_rsb];
		break;

	case nod_like:
	case nod_contains:
	case nod_similar:
	case nod_starts:
		if (node->nod_flags & nod_invariant) {
			csb->csb_invariants.push(node);
		}
		break;

	case nod_sort:
		{
			jrd_nod** ptr = node->nod_arg;
			for (jrd_nod** end = ptr + node->nod_count; ptr < end; ptr++)
			{
				(*ptr)->nod_flags |= nod_value;
			}
		}
		break;

	case nod_src_info:
		node->nod_arg[e_src_info_node] = CMP_pass2(tdbb, csb, node->nod_arg[e_src_info_node], node);
		return node;

	case nod_variable:
		node->nod_arg[e_var_info] =
			pass2_validation(tdbb, csb, Item(nod_variable, (IPTR) node->nod_arg[e_var_id]));
		break;

	case nod_init_variable:
		node->nod_arg[e_init_var_info] =
			pass2_validation(tdbb, csb, Item(nod_variable, (IPTR) node->nod_arg[e_init_var_id]));
		break;

	case nod_argument:
		node->nod_arg[e_arg_info] =
			pass2_validation(tdbb, csb, Item(nod_argument,
				(IPTR) node->nod_arg[e_arg_message]->nod_arg[e_msg_number],
				(IPTR) node->nod_arg[e_arg_number]));
		break;

	default:
		break;
	}

	if (rse_node) {
		pass2_rse(tdbb, csb, (RecordSelExpr*) rse_node);
	}

	// handle sub-expressions here

	// AB: Mark the streams involved with INSERT/UPDATE statements active.
	// So that the optimizer can use indices for eventually used sub-selects.
	if (node->nod_type == nod_modify) {
		stream = (USHORT)(IPTR) node->nod_arg[e_mod_org_stream];
		csb->csb_rpt[stream].csb_flags |= csb_active;
		stream = (USHORT)(IPTR) node->nod_arg[e_mod_new_stream];
		csb->csb_rpt[stream].csb_flags |= csb_active;
	}
	else if (node->nod_type == nod_store) {
		stream = (USHORT)(IPTR) node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];
		csb->csb_rpt[stream].csb_flags |= csb_active;
	}

	jrd_nod** ptr = node->nod_arg;
	// This "end" is used later.
	const jrd_nod* const* const end = ptr + node->nod_count;

	for (; ptr < end; ptr++) {
		CMP_pass2(tdbb, csb, *ptr, node);
	}

	// AB: Remove the previous flags
	if (node->nod_type == nod_modify) {
		stream = (USHORT)(IPTR) node->nod_arg[e_mod_org_stream];
		csb->csb_rpt[stream].csb_flags &= ~csb_active;
		stream = (USHORT)(IPTR) node->nod_arg[e_mod_new_stream];
		csb->csb_rpt[stream].csb_flags &= ~csb_active;
	}
	else if (node->nod_type == nod_store) {
		stream = (USHORT)(IPTR) node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];
		csb->csb_rpt[stream].csb_flags &= ~csb_active;
	}

	// Handle any residual work

	node->nod_impure = CMP_impure(csb, 0);

	switch (node->nod_type)
	{
	case nod_abort:
		CMP_pass2(tdbb, csb, node->nod_arg[e_xcp_msg], node);
		break;

	case nod_assignment:
		CMP_pass2(tdbb, csb, node->nod_arg[e_asgn_missing2], node);
		break;

	case nod_average:
	case nod_agg_average:
	case nod_agg_average_distinct:
		node->nod_flags |= nod_double;
		// FALL INTO

	case nod_max:
	case nod_min:
	case nod_from:
	case nod_count:
	case nod_agg_count2:
	case nod_agg_count_distinct:
	//case nod_count2:
	case nod_agg_min:
	case nod_agg_max:
	case nod_agg_count:
		node->nod_count = 0;
		csb->csb_impure += sizeof(impure_value_ex);
		break;

	case nod_ansi_all:
	case nod_ansi_any:
	case nod_any:
	case nod_exists:
	case nod_unique:
		if (node->nod_flags & nod_invariant) {
			csb->csb_impure += sizeof(impure_value);
		}
		break;

	case nod_block:
		csb->csb_impure += sizeof(SLONG);
		break;

	case nod_dcl_variable:
		{
			const dsc* desc = (DSC*) (node->nod_arg + e_dcl_desc);
			csb->csb_impure += sizeof(impure_value) + desc->dsc_length;
		}
		break;

	case nod_total:
	case nod_agg_total:
	case nod_agg_total_distinct:
	case nod_agg_total2:
	case nod_agg_total_distinct2:
	case nod_agg_average2:
	case nod_agg_average_distinct2:
	case nod_agg_list:
	case nod_agg_list_distinct:
		{
			node->nod_count = 0;
			csb->csb_impure += sizeof(impure_value_ex);
			dsc descriptor_a;
			CMP_get_desc(tdbb, csb, node, &descriptor_a);
		}
		break;

	case nod_message:
		{
			const Format* format = (Format*) node->nod_arg[e_msg_format];
			if (!((tdbb->tdbb_flags & TDBB_prc_being_dropped) && !format))
			{
				csb->csb_impure += FB_ALIGN(format->fmt_length, 2);

				node->nod_arg[e_msg_impure_flags] = (jrd_nod*)(IPTR) CMP_impure(csb, 0);
				csb->csb_impure += sizeof(USHORT) * format->fmt_count;
			}
		}
		break;

	case nod_modify:
		{
			stream = (USHORT)(IPTR) node->nod_arg[e_mod_org_stream];
			csb->csb_rpt[stream].csb_flags |= csb_update;
			const Format* format = CMP_format(tdbb, csb, stream);
			Format::fmt_desc_const_iterator desc = format->fmt_desc.begin();
			for (ULONG id = 0; id < format->fmt_count; ++id, ++desc) {
				if (desc->dsc_dtype) {
					SBM_SET(tdbb->getDefaultPool(), &csb->csb_rpt[stream].csb_fields, id);
				}
			}
			csb->csb_impure += sizeof(impure_state);
		}
		break;

	case nod_list:
		// We are using the same "node" always. The "end" was set in the middle
		// of the two huge switch() statements (this is the second).
		if (node->nod_count > 0)
		{
			node->nod_type = nod_asn_list;
			for (ptr = node->nod_arg; ptr < end; ptr++) {
				if ((*ptr)->nod_type != nod_assignment) {
					node->nod_type = nod_list;
					break;
				}
			}
		}
		// FALL INTO

	case nod_store:
		csb->csb_impure += sizeof(impure_state);
		break;

	case nod_erase:
		stream = (USHORT)(IPTR) node->nod_arg[e_erase_stream];
		csb->csb_rpt[stream].csb_flags |= csb_update;
		break;

	case nod_field:
		{
			stream = (USHORT)(IPTR) node->nod_arg[e_fld_stream];
			// SMB_SET uses ULONG, not USHORT
			const ULONG id = (ULONG)(IPTR) node->nod_arg[e_fld_id];
			SBM_SET(tdbb->getDefaultPool(), &csb->csb_rpt[stream].csb_fields, id);
			if (csb->csb_rpt[stream].csb_relation || csb->csb_rpt[stream].csb_procedure)
				node->nod_arg[e_fld_format] = (jrd_nod*) CMP_format(tdbb, csb, stream);
			csb->csb_impure += sizeof(impure_value_ex);
			break;
		}

	case nod_argument:
	case nod_variable:
		csb->csb_impure += (node->nod_flags & nod_value) ?
			sizeof(impure_value_ex) : sizeof(dsc);
		break;

	case nod_concatenate:
    case nod_literal:
	case nod_dbkey:
	case nod_rec_version:
	case nod_negate:
	case nod_substr:
	case nod_trim:
	case nod_divide:
	case nod_null:
	case nod_user_name:
    case nod_current_role:
	case nod_internal_info:
	case nod_gen_id:
	case nod_gen_id2:
	case nod_upcase:
	case nod_lowcase:
	case nod_prot_mask:
	case nod_lock_state:
	case nod_scalar:
	case nod_cast:
	case nod_extract:
	case nod_strlen:
	case nod_current_time:
	case nod_current_timestamp:
	case nod_current_date:
	case nod_derived_expr:
#ifdef SCROLLABLE_CURSORS
	case nod_seek:
#endif
		{
			dsc descriptor_a;
			CMP_get_desc(tdbb, csb, node, &descriptor_a);
			csb->csb_impure += sizeof(impure_value);
		}
		break;

	case nod_stmt_expr:
		csb->csb_impure += sizeof(impure_value);
		break;

	// compute the target descriptor to compute computational class

	case nod_multiply:
	case nod_add:
	case nod_subtract:
	case nod_add2:
	case nod_subtract2:
	case nod_multiply2:
	case nod_divide2:
		{
			dsc descriptor_a;
			CMP_get_desc(tdbb, csb, node, &descriptor_a);
			csb->csb_impure += sizeof(impure_value);
		}
		break;

	case nod_function:
		{
			// For gbak attachments, there is no need to resolve the UDF function
			// Also if we are dropping a procedure don't bother resolving the
			// UDF that the procedure invokes.
			if (!(tdbb->getAttachment()->att_flags & ATT_gbak_attachment) &&
				!(tdbb->tdbb_flags & TDBB_prc_being_dropped))
			{
				jrd_nod* value = node->nod_arg[e_fun_args];
				UserFunction* function = (UserFunction*) node->nod_arg[e_fun_function];
				node->nod_arg[e_fun_function] = (jrd_nod*) FUN_resolve(tdbb, csb, function, value);
				if (!node->nod_arg[e_fun_function]) {
					ERR_post(Arg::Gds(isc_funmismat) << Arg::Str(function->fun_name));
				}
			}
			dsc descriptor_a;
			CMP_get_desc(tdbb, csb, node, &descriptor_a);
			csb->csb_impure += sizeof(impure_value);
		}
		break;

	case nod_sys_function:
		{
			SysFunction* function = ((SysFunction*) node->nod_arg[e_sysfun_function]);
			jrd_nod* nodeArgs = node->nod_arg[e_sysfun_args];
			fb_assert(nodeArgs->nod_type == nod_list);

			function->checkArgsMismatch(nodeArgs->nod_count);

			dsc descriptor_a;
			CMP_get_desc(tdbb, csb, node, &descriptor_a);
			csb->csb_impure += sizeof(impure_value);
		}
		break;

	case nod_aggregate:
		pass2_rse(tdbb, csb, (RecordSelExpr*) node->nod_arg[e_agg_rse]);
		CMP_pass2(tdbb, csb, node->nod_arg[e_agg_map], node);
		CMP_pass2(tdbb, csb, node->nod_arg[e_agg_group], node);
		stream = (USHORT)(IPTR) node->nod_arg[e_agg_stream];
		fb_assert(stream <= MAX_STREAMS);
		process_map(tdbb, csb, node->nod_arg[e_agg_map], &csb->csb_rpt[stream].csb_format);
		break;

		// boolean nodes taking three values as inputs
	case nod_like:
	case nod_between:
	case nod_similar:
	case nod_sleuth:
		if (node->nod_count > 2) {
			if (node->nod_arg[2]->nod_flags & nod_agg_dbkey) {
				ERR_post(Arg::Gds(isc_bad_dbkey));
			}
			dsc descriptor_c;
			CMP_get_desc(tdbb, csb, node->nod_arg[0], &descriptor_c);
			if (DTYPE_IS_DATE(descriptor_c.dsc_dtype)) {
				node->nod_arg[0]->nod_flags |= nod_date;
				node->nod_arg[1]->nod_flags |= nod_date;
			}
		}
		// FALLINTO

		// boolean nodes taking two values as inputs
	case nod_matches:
	case nod_contains:
	case nod_starts:
	case nod_equiv:
	case nod_eql:
	case nod_neq:
	case nod_geq:
	case nod_gtr:
	case nod_lss:
	case nod_leq:
		{
			if ((node->nod_arg[0]->nod_flags & nod_agg_dbkey) ||
				(node->nod_arg[1]->nod_flags & nod_agg_dbkey))
			{
				ERR_post(Arg::Gds(isc_bad_dbkey));
			}
			dsc descriptor_a, descriptor_b;
			CMP_get_desc(tdbb, csb, node->nod_arg[0], &descriptor_a);
			CMP_get_desc(tdbb, csb, node->nod_arg[1], &descriptor_b);
			if (DTYPE_IS_DATE(descriptor_a.dsc_dtype))
				node->nod_arg[1]->nod_flags |= nod_date;
			else if (DTYPE_IS_DATE(descriptor_b.dsc_dtype))
				node->nod_arg[0]->nod_flags |= nod_date;
		}
		if (node->nod_flags & nod_invariant) {
			// This may currently happen for nod_like, nod_contains and nod_similar
			csb->csb_impure += sizeof(impure_value);
		}
		break;

		// boolean nodes taking one value as input
	case nod_missing:
		{
			if (node->nod_arg[0]->nod_flags & nod_agg_dbkey) {
				ERR_post(Arg::Gds(isc_bad_dbkey));
			}

			// check for syntax errors in the calculation
			dsc descriptor_a;
			CMP_get_desc(tdbb, csb, node->nod_arg[0], &descriptor_a);
		}
		break;

	case nod_exec_into:
		csb->csb_impure += sizeof(ExecuteStatement);
		csb->csb_exec_sta.push(node);
		break;

	case nod_exec_stmt:
		csb->csb_impure += sizeof(void**);
		break;

	case nod_class_node_jrd:
		node->nod_arg[0] = reinterpret_cast<jrd_nod*>(
			reinterpret_cast<DmlNode*>(node->nod_arg[0])->pass2(tdbb, csb, node));
		return node;

	default:
		// note: no fb_assert(false); here as too many nodes are missing
		break;
	}

	// Bind values of invariant nodes to top-level RSE (if present)
	if (node->nod_flags & nod_invariant)
	{
		if (csb->csb_current_nodes.getCount()) {
			// CVC: Nickolay says this rse_node is local. Therefore, renamed to aux_rse_node.
			jrd_node_base* aux_rse_node = csb->csb_current_nodes[0];
			fb_assert(aux_rse_node->nod_type == nod_rse);
			RecordSelExpr* top_rse = static_cast<RecordSelExpr*>(aux_rse_node);
			if (!top_rse->rse_invariants)
				top_rse->rse_invariants =
					FB_NEW(*tdbb->getDefaultPool()) VarInvariantArray(*tdbb->getDefaultPool());
			top_rse->rse_invariants->add(node->nod_impure);
		}
	}

	// finish up processing of record selection expressions

	if (rse_node) {
		*rsb_ptr = post_rse(tdbb, csb, (RecordSelExpr*) rse_node);
	}

	return node;
}


static void pass2_rse(thread_db* tdbb, CompilerScratch* csb, RecordSelExpr* rse)
{
/**************************************
 *
 *	p a s s 2 _ r s e
 *
 **************************************
 *
 * Functional description
 *	Perform the first half of record selection expression compilation.
 *	The actual optimization is done in "post_rse".
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);

	// Maintain stack of RSEe for scoping purposes
	csb->csb_current_nodes.push(rse);

	if (rse->rse_first) {
		CMP_pass2(tdbb, csb, rse->rse_first, 0);
	}
	if (rse->rse_skip) {
	    CMP_pass2(tdbb, csb, rse->rse_skip, 0);
	}

	jrd_nod** ptr = rse->rse_relation;
	for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
	{
		jrd_nod* const node = *ptr;

		switch (node->nod_type)
		{
		case nod_rse:
			pass2_rse(tdbb, csb, (RecordSelExpr*) node);
			break;
		case nod_relation:
		case nod_procedure:
		case nod_aggregate:
		case nod_union:
		{
			const USHORT stream = (USHORT)(IPTR) node->nod_arg[STREAM_INDEX(node)];
			fb_assert(stream <= MAX_STREAMS);
			csb->csb_rpt[stream].csb_flags |= csb_active;
			// FALL INTO
		}
		default:
			CMP_pass2(tdbb, csb, node, (jrd_nod*) rse);
			break;
		}
	}

	if (rse->rse_boolean) {
		CMP_pass2(tdbb, csb, rse->rse_boolean, 0);
	}

	if (rse->rse_sorted) {
		CMP_pass2(tdbb, csb, rse->rse_sorted, 0);
	}

	if (rse->rse_projection) {
		CMP_pass2(tdbb, csb, rse->rse_projection, 0);
	}

	// if the user has submitted a plan for this RecordSelExpr, check it for correctness

	if (rse->rse_plan) {
		plan_set(csb, rse, rse->rse_plan);
		plan_check(csb, rse);
	}

#ifdef SCROLLABLE_CURSORS
	if (rse->rse_async_message) {
		CMP_pass2(tdbb, csb, rse->rse_async_message, 0);
	}
#endif
	csb->csb_current_nodes.pop();
}


static jrd_nod* pass2_union(thread_db* tdbb, CompilerScratch* csb, jrd_nod* node)
{
/**************************************
 *
 *	p a s s 2 _ u n i o n
 *
 **************************************
 *
 * Functional description
 *	Process a union clause of an RecordSelExpr.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	// make up a format block sufficiently large to hold instantiated record

	jrd_nod* clauses = node->nod_arg[e_uni_clauses];
	const USHORT id = (USHORT)(IPTR) node->nod_arg[e_uni_stream];
	Format** format = &csb->csb_rpt[id].csb_format;

	// process alternating RecordSelExpr and map blocks

	jrd_nod** ptr = clauses->nod_arg;
	for (const jrd_nod* const* const end = ptr + clauses->nod_count; ptr < end;)
	{
		pass2_rse(tdbb, csb, (RecordSelExpr*) * ptr++);
		jrd_nod* map = *ptr++;
		CMP_pass2(tdbb, csb, map, node);
		process_map(tdbb, csb, map, format);
	}

	if (node->nod_flags & nod_recurse)
	{
		const USHORT map_id = (USHORT)(IPTR) node->nod_arg[e_uni_map_stream];
		csb->csb_rpt[map_id].csb_format = *format;
	}

	return node;
}


static void plan_check(const CompilerScratch* csb, const RecordSelExpr* rse)
{
/**************************************
 *
 *	p l a n _ c h e c k
 *
 **************************************
 *
 * Functional description
 *	Check that all streams in the RecordSelExpr have
 *	a plan specified for them.
 *	If they are not, there are streams
 *	in the RecordSelExpr which were not mentioned
 *	in the plan.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);

	// if any streams are not marked with a plan, give an error

	const jrd_nod* const* ptr = rse->rse_relation;
	for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
	{
		if ((*ptr)->nod_type == nod_relation) {
			const USHORT stream = (USHORT)(IPTR) (*ptr)->nod_arg[e_rel_stream];
			if (!(csb->csb_rpt[stream].csb_plan)) {
				ERR_post(Arg::Gds(isc_no_stream_plan) << Arg::Str(csb->csb_rpt[stream].csb_relation->rel_name));
			}
		}
		else if ((*ptr)->nod_type == nod_rse) {
			plan_check(csb, (const RecordSelExpr*) *ptr);
		}
	}
}


static void plan_set(CompilerScratch* csb, RecordSelExpr* rse, jrd_nod* plan)
{
/**************************************
 *
 *	p l a n _ s e t
 *
 **************************************
 *
 * Functional description
 *	Go through the streams in the plan, find the
 *	corresponding streams in the RecordSelExpr and store the
 *	plan for that stream.   Do it once and only once
 *	to make sure there is a one-to-one correspondence
 *	between streams in the query and streams in
 *	the plan.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);
	DEV_BLKCHK(plan, type_nod);

	if (plan->nod_type == nod_join || plan->nod_type == nod_merge) {
		for (jrd_nod** ptr = plan->nod_arg, **end = ptr + plan->nod_count; ptr < end; ptr++)
		{
			plan_set(csb, rse, *ptr);
		}
	}

	if (plan->nod_type != nod_retrieve) {
		return;
	}

	const jrd_rel* view_relation = 0;
	jrd_nod* plan_relation_node = plan->nod_arg[e_retrieve_relation];
	const jrd_rel* plan_relation = (jrd_rel*) plan_relation_node->nod_arg[e_rel_relation];
	const char* plan_alias = (const char *) plan_relation_node->nod_arg[e_rel_alias];

	// find the tail for the relation specified in the RecordSelExpr

	const USHORT stream = (USHORT)(IPTR) plan_relation_node->nod_arg[e_rel_stream];
	CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];

	// if the plan references a view, find the real base relation
	// we are interested in by searching the view map */
	UCHAR* map = 0;

	if (tail->csb_map) {
		const TEXT* p = plan_alias;

		// if the user has specified an alias, skip past it to find the alias
		// for the base table (if multiple aliases are specified)
		if (p && *p &&
			((tail->csb_relation && !strcmp_space(tail->csb_relation->rel_name.c_str(), p)) ||
			 (tail->csb_alias && !strcmp_space(tail->csb_alias->c_str(), p))))
		{
			while (*p && *p != ' ') {
				p++;
			}
			if (*p == ' ') {
				p++;
			}
		}

		// loop through potentially a stack of views to find the appropriate base table
		UCHAR* map_base;
		while ( (map_base = tail->csb_map) ) {
			map = map_base;
			tail = &csb->csb_rpt[*map];
			view_relation = tail->csb_relation;

			// if the plan references the view itself, make sure that
			// the view is on a single table; if it is, fix up the plan
			// to point to the base relation

			if (view_relation->rel_id == plan_relation->rel_id) {
				if (!map_base[2]) {
					map++;
					tail = &csb->csb_rpt[*map];
				}
				else {
					// view %s has more than one base relation; use aliases to distinguish
					ERR_post(Arg::Gds(isc_view_alias) << Arg::Str(plan_relation->rel_name));
				}

				break;
			}

			view_relation = NULL;

			// if the user didn't specify an alias (or didn't specify one
			// for this level), check to make sure there is one and only one
			// base relation in the table which matches the plan relation

			if (!*p) {
				const jrd_rel* duplicate_relation = NULL;
				UCHAR* duplicate_map = map_base;
				map = NULL;
				for (duplicate_map++; *duplicate_map; duplicate_map++) {
					CompilerScratch::csb_repeat* duplicate_tail = &csb->csb_rpt[*duplicate_map];
					const jrd_rel* relation = duplicate_tail->csb_relation;
					if (relation && relation->rel_id == plan_relation->rel_id) {
						if (duplicate_relation) {
							// table %s is referenced twice in view; use an alias to distinguish
							ERR_post(Arg::Gds(isc_duplicate_base_table) << Arg::Str(duplicate_relation->rel_name));
						}
						else {
							duplicate_relation = relation;
							map = duplicate_map;
							tail = duplicate_tail;
						}
					}
				}

				break;
			}

			// look through all the base relations for a match

			map = map_base;
			for (map++; *map; map++) {
				tail = &csb->csb_rpt[*map];
				const jrd_rel* relation = tail->csb_relation;

				// match the user-supplied alias with the alias supplied
				// with the view definition; failing that, try the base
				// table name itself

				// CVC: I found that "relation" can be NULL, too. This may be an
				// indication of a logic flaw while parsing the user supplied SQL plan
				// and not an oversight here. It's hard to imagine a csb->csb_rpt with
				// a NULL relation. See exe.h for CompilerScratch struct and its inner csb_repeat struct.

				if ((tail->csb_alias && !strcmp_space(tail->csb_alias->c_str(), p)) ||
					(relation && !strcmp_space(relation->rel_name.c_str(), p)))
				{
					  break;
				}
			}

			// skip past the alias

			while (*p && *p != ' ') {
				p++;
			}
			if (*p == ' ') {
				p++;
			}

			if (!*map) {
				// table %s is referenced in the plan but not the from list
				ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(plan_relation->rel_name));
			}
		}

		// fix up the relation node to point to the base relation's stream

		if (!map || !*map) {
			// table %s is referenced in the plan but not the from list
			ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(plan_relation->rel_name));
		}

		plan_relation_node->nod_arg[e_rel_stream] = (jrd_nod*) (IPTR) *map;
	}

	// make some validity checks

	if (!tail->csb_relation) {
		// table %s is referenced in the plan but not the from list
		ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(plan_relation->rel_name));
	}

	if ((tail->csb_relation->rel_id != plan_relation->rel_id) && !view_relation)
	{
		// table %s is referenced in the plan but not the from list
		ERR_post(Arg::Gds(isc_stream_not_found) << Arg::Str(plan_relation->rel_name));
	}

	// check if we already have a plan for this stream

	if (tail->csb_plan) {
		// table %s is referenced more than once in plan; use aliases to distinguish
		ERR_post(Arg::Gds(isc_stream_twice) << Arg::Str(tail->csb_relation->rel_name));
	}

	tail->csb_plan = plan;
}


static void post_procedure_access(thread_db* tdbb, CompilerScratch* csb, jrd_prc* procedure)
{
/**************************************
 *
 *	p o s t _ p r o c e d u r e _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *
 *	The request will inherit access requirements to all the objects
 *	the called stored procedure has access requirements for.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(procedure, type_prc);

	// allow all access to internal requests

	if (csb->csb_g_flags & (csb_internal | csb_ignore_perm))
		return;

	// this request must have EXECUTE permission on the stored procedure
	CMP_post_access(tdbb, csb, procedure->prc_security_name, csb->csb_view ? csb->csb_view->rel_id : 0,
					SCL_execute, object_procedure, procedure->prc_name);

	// Add the procedure to list of external objects accessed
	ExternalAccess temp(procedure->prc_id);
	size_t idx;
	if (!csb->csb_external.find(temp, idx))
		csb->csb_external.insert(idx, temp);
}


static RecordSource* post_rse(thread_db* tdbb, CompilerScratch* csb, RecordSelExpr* rse)
{
/**************************************
 *
 *	p o s t _ r s e
 *
 **************************************
 *
 * Functional description
 *	Perform actual optimization of an RecordSelExpr and clear activity.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);

	RecordSource* rsb = OPT_compile(tdbb, csb, rse, NULL);

	if (rse->nod_flags & rse_singular) {
		rsb->rsb_flags |= rsb_singular;
	}

	// mark all the substreams as inactive

	jrd_nod** ptr = rse->rse_relation;
	for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
	{
		const jrd_nod* const node = *ptr;

		if (node->nod_type == nod_relation ||
			node->nod_type == nod_procedure ||
			node->nod_type == nod_aggregate ||
			node->nod_type == nod_union)
		{
			const USHORT stream = (USHORT)(IPTR) node->nod_arg[STREAM_INDEX(node)];
			fb_assert(stream <= MAX_STREAMS);
			csb->csb_rpt[stream].csb_flags &= ~csb_active;
		}
	}

	csb->csb_fors.push(rsb);
#ifdef SCROLLABLE_CURSORS
	rse->rse_rsb = rsb;
#endif

	return rsb;
}


static void post_trigger_access(CompilerScratch* csb,
								jrd_rel* owner_relation,
								ExternalAccess::exa_act operation, jrd_rel* view)
{
/**************************************
 *
 *	p o s t _ t r i g g e r _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *	Inherit access to triggers to be fired.
 *
 *	When we detect that a trigger could be fired by a request,
 *	then we add the access list for that trigger to the access
 *	list for this request.  That way, when we check access for
 *	the request we also check access for any other objects that
 *	could be fired off by the request.
 *
 *	Note that when we add the access item, we specify that
 *	   Trigger X needs access to resource Y.
 *	In the access list we parse here, if there is no "accessor"
 *	name then the trigger must access it directly.  If there is
 *	an "accessor" name, then something accessed by this trigger
 *	must require the access.
 *
 *  CVC: The third parameter is the owner of the triggers vector
 *   and was added to avoid triggers posting access checks to
 *   their base tables, since it's nonsense and causes weird
 *   messages about false REFERENCES right failures.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(view, type_rel);

	// allow all access to internal requests
	if (csb->csb_g_flags & (csb_internal | csb_ignore_perm))
		return;

	// Post trigger access
	ExternalAccess temp(operation, owner_relation->rel_id, view ? view->rel_id : 0);
	size_t i;
	if (!csb->csb_external.find(temp, i))
		csb->csb_external.insert(i, temp);
}


static void process_map(thread_db* tdbb, CompilerScratch* csb, jrd_nod* map,
						Format** input_format)
{
/**************************************
 *
 *	p r o c e s s _ m a p
 *
 **************************************
 *
 * Functional description
 *	Translate a map block into a format.  If the format
 *	is missing or incomplete, extend it.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(map, type_nod);
	DEV_BLKCHK(*input_format, type_fmt);

	SET_TDBB(tdbb);

	Format* format = *input_format;
	if (!format) {
		format = *input_format = Format::newFormat(*tdbb->getDefaultPool(), map->nod_count);
	}

	// process alternating rse and map blocks
	dsc desc2;
	jrd_nod** ptr = map->nod_arg;
	for (const jrd_nod* const* const end = ptr + map->nod_count; ptr < end; ptr++)
	{
		jrd_nod* assignment = *ptr;
		jrd_nod* field = assignment->nod_arg[e_asgn_to];
		const USHORT id = (USHORT)(IPTR) field->nod_arg[e_fld_id];
		if (id >= format->fmt_count) {
			format->fmt_desc.resize(id + 1);
		}
		dsc* desc = &format->fmt_desc[id];
		CMP_get_desc(tdbb, csb, assignment->nod_arg[e_asgn_from], &desc2);

		const USHORT min = MIN(desc->dsc_dtype, desc2.dsc_dtype);
		const USHORT max = MAX(desc->dsc_dtype, desc2.dsc_dtype);

		if (!min)			// eg: dtype_unknown
			*desc = desc2;
		else if (max == dtype_blob)
		{
			USHORT subtype = DataTypeUtil::getResultBlobSubType(desc, &desc2);
			USHORT ttype = DataTypeUtil::getResultTextType(desc, &desc2);
			desc->makeBlob(subtype, ttype);
		}
		else if (min <= dtype_any_text) {	// either field a text field?
			const USHORT len1 = DSC_string_length(desc);
			const USHORT len2 = DSC_string_length(&desc2);
			desc->dsc_dtype = dtype_varying;
			desc->dsc_length = MAX(len1, len2) + sizeof(USHORT);

			// pick the max text type, so any transparent casts from ints are
			// not left in ASCII format, but converted to the richer text format

			INTL_ASSIGN_TTYPE(desc, MAX(INTL_TEXT_TYPE(*desc), INTL_TEXT_TYPE(desc2)));
			desc->dsc_scale = 0;
			desc->dsc_flags = 0;
		}
		else if (DTYPE_IS_DATE(max) && !DTYPE_IS_DATE(min)) {
			desc->dsc_dtype = dtype_varying;
			desc->dsc_length = DSC_convert_to_text_length(max) + sizeof(USHORT);
			desc->dsc_ttype() = ttype_ascii;
			desc->dsc_scale = 0;
			desc->dsc_flags = 0;
		}
		else if (max != min) {
			// different numeric types: if one is inexact use double,
			// if both are exact use int64
			if ((!DTYPE_IS_EXACT(max)) || (!DTYPE_IS_EXACT(min))) {
				desc->dsc_dtype = DEFAULT_DOUBLE;
				desc->dsc_length = sizeof(double);
				desc->dsc_scale = 0;
				desc->dsc_sub_type = 0;
				desc->dsc_flags = 0;
			}
			else {
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
		if (align) {
			offset = FB_ALIGN(offset, align);
		}
		desc3->dsc_address = (UCHAR *) (IPTR) offset;
		offset += desc3->dsc_length;
	}

	if (offset > MAX_FORMAT_SIZE)
		ERR_post(Arg::Gds(isc_imp_exc) << Arg::Gds(isc_blktoobig));

	format->fmt_length = (USHORT) offset;
}


static SSHORT strcmp_space(const char* p, const char* q)
{
/**************************************
 *
 *	s t r c m p _ s p a c e
 *
 **************************************
 *
 * Functional description
 *	Compare two strings, which could be either
 *	space-terminated or null-terminated.
 *
 **************************************/

	for (; *p && *p != ' ' && *q && *q != ' '; p++, q++)
	{
		if (*p != *q)
			break;
	}

	if ((!*p || *p == ' ') && (!*q || *q == ' '))
		return 0;

	return (*p > *q) ? 1 : -1;
}


static bool stream_in_rse(USHORT stream, const RecordSelExpr* rse)
{
/**************************************
 *
 *	s t r e a m _ i n _ r s e
 *
 **************************************
 *
 * Functional description
 *	Return true if stream is contained in
 *	the specified RecordSelExpr.
 *
 **************************************/
	DEV_BLKCHK(rse, type_nod);

	// look through all relation nodes in this RecordSelExpr to see
	// if the field references this instance of the relation
	for (int i = 0; i < rse->rse_count; ++i)
	{
		const jrd_nod* sub = rse->rse_relation[i];

		// for RecordSelExpr, just recurse
		switch (sub->nod_type)
		{
		case nod_rse:
			if (stream_in_rse(stream, (const RecordSelExpr*) sub))
			{
				return true;		// do not mark as variant
			}
			break;
		// for unions, check current RecordSelExpr, if not found then check
		// all sub-rse's
		case nod_union:
			{
				if (stream == (USHORT)(IPTR) sub->nod_arg[e_uni_stream])
				{
					return true;		// do not mark as variant
				}
				const jrd_nod* clauses = sub->nod_arg[e_uni_clauses];
				for (int j = 0; j < clauses->nod_count; j += 2)
				{
					if (stream_in_rse(stream, (const RecordSelExpr*) clauses->nod_arg[j]))
					{
						return true;	// do not mark as variant
					}
				}
			}
			break;
		// for aggregates, check current RecordSelExpr, if not found then check
		// the sub-rse
		case nod_aggregate:
			if (stream == (USHORT)(IPTR) sub->nod_arg[e_agg_stream])
			{
				return true;		// do not mark as variant
			}
			if (stream_in_rse(stream, (const RecordSelExpr*) sub->nod_arg[e_agg_rse]))
			{
				return true;		// do not mark as variant
			}
			break;
		// the simplest case - relations
		case nod_relation:
			if (stream == (USHORT)(IPTR) sub->nod_arg[e_rel_stream])
			{
				return true;		// do not mark as variant
			}
			break;
		// procedures
		case nod_procedure:
			if (stream == (USHORT)(IPTR) sub->nod_arg[e_prc_stream])
				return true;		// do not mark as variant
			break;
		}
	}

	return false;					// mark this RecordSelExpr as variant
}
