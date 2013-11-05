/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		par.cpp
 *	DESCRIPTION:	BLR Parser
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
 * 27-May-2001 Claudio Valderrama: par_plan() no longer uppercases
 *			an index's name before doing a lookup of such index.
 * 2001.07.28: Added parse code for blr_skip to support LIMIT.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.29 Mike Nordell - Fixed breakage.
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 * 2004.01.16 Vlad Horsun: Added support for default parameters
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ibase.h"
#include "../jrd/ini.h"
#include "../jrd/val.h"
#include "../jrd/align.h"
#include "../jrd/exe.h"
#include "../jrd/extds/ExtDS.h"
#include "../jrd/lls.h"
#include "../jrd/rse.h"	// for MAX_STREAMS
#include "../jrd/scl.h"
#include "../jrd/req.h"
#include "../jrd/blb.h"
#include "../jrd/intl.h"
#include "../jrd/met.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/cvt_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/fun_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/par_proto.h"
#include "../common/utils_proto.h"
#include "../jrd/SysFunction.h"
#include "../jrd/BlrReader.h"

using namespace Jrd;
using namespace Firebird;

#include "gen/blrtable.h"

static const TEXT elements[][14] =
	{ "", "statement", "boolean", "value", "RecordSelExpr", "TABLE" };

#include "gen/codetext.h"


static NodeParseFunc blr_parsers[256] = {NULL};


static void error(CompilerScratch* csb, const Arg::StatusVector& v);
static SSHORT find_proc_field(const jrd_prc*, const Firebird::MetaName&);
static jrd_nod* par_args(thread_db*, CompilerScratch*, USHORT);
static jrd_nod* par_cast(thread_db*, CompilerScratch*);
static PsqlException* par_condition(thread_db*, CompilerScratch*);
static PsqlException* par_conditions(thread_db*, CompilerScratch*);
static SSHORT par_context(CompilerScratch*, SSHORT *);
static void par_dependency(thread_db*, CompilerScratch*, SSHORT, SSHORT, const Firebird::MetaName&);
static jrd_nod* par_exec_proc(thread_db*, CompilerScratch*, SSHORT);
static jrd_nod* par_fetch(thread_db*, CompilerScratch*, jrd_nod*);
static jrd_nod* par_field(thread_db*, CompilerScratch*, SSHORT);
static jrd_nod* par_function(thread_db*, CompilerScratch*);
static jrd_nod* par_literal(thread_db*, CompilerScratch*);
static jrd_nod* par_map(thread_db*, CompilerScratch*, USHORT);
static jrd_nod* par_message(thread_db*, CompilerScratch*);
static jrd_nod* par_modify(thread_db*, CompilerScratch*, SSHORT);
static USHORT par_name(CompilerScratch*, Firebird::MetaName&);
static size_t par_name(CompilerScratch* csb, Firebird::string& name);
static jrd_nod* par_plan(thread_db*, CompilerScratch*);
static jrd_nod* par_procedure(thread_db*, CompilerScratch*, SSHORT);
static void par_procedure_parms(thread_db*, CompilerScratch*, jrd_prc*, jrd_nod**, jrd_nod**, bool);
static jrd_nod* par_relation(thread_db*, CompilerScratch*, SSHORT, bool);
static jrd_nod* par_rse(thread_db*, CompilerScratch*, SSHORT);
static jrd_nod* par_sort(thread_db*, CompilerScratch*, bool);
#ifdef NOT_USED_OR_REPLACED
static jrd_nod* par_stream(thread_db*, CompilerScratch*);
#endif
static jrd_nod* par_sys_function(thread_db*, CompilerScratch*);
static jrd_nod* par_union(thread_db*, CompilerScratch*, bool);
static void warning(const Arg::StatusVector& v);


jrd_nod* PAR_blr(thread_db* tdbb, jrd_rel* relation, const UCHAR* blr, ULONG blr_length,
	CompilerScratch* view_csb, AutoPtr<CompilerScratch>& csb, jrd_req** request_ptr,
	const bool trigger, USHORT flags)
{
/**************************************
 *
 *	P A R _ b l r
 *
 **************************************
 *
 * Functional description
 *	Parse blr, returning a compiler scratch block with the results.
 *	Caller must do pool handling.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef CMP_DEBUG
	cmp_trace("BLR code given for JRD parsing:");
	// CVC: Couldn't find isc_trace_printer, so changed it to gds__trace_printer.
	fb_print_blr(blr, blr_length, gds__trace_printer, 0, 0);
#endif

	if (!csb) {
		size_t count = 5;
		if (view_csb)
			count += view_csb->csb_rpt.getCapacity();
		csb.reset(CompilerScratch::newCsb(*tdbb->getDatabase()->dbb_permanent, count));
		csb->csb_g_flags |= flags;
	}

/* If there is a request ptr, this is a trigger.  Set up contexts 0 and 1 for
   the target relation */

	if (trigger)
	{
		SSHORT stream = csb->nextStream();
		CompilerScratch::csb_repeat* t1 = CMP_csb_element(csb, 0);
		t1->csb_flags |= csb_used | csb_active | csb_trigger;
		t1->csb_relation = relation;
		t1->csb_stream = (UCHAR) stream;

		stream = csb->nextStream();
		t1 = CMP_csb_element(csb, 1);
		t1->csb_flags |= csb_used | csb_active | csb_trigger;
		t1->csb_relation = relation;
		t1->csb_stream = (UCHAR) stream;
	}
	else if (relation) {
		CompilerScratch::csb_repeat* t1 = CMP_csb_element(csb, 0);
		t1->csb_stream = csb->nextStream();
		t1->csb_relation = relation;
		t1->csb_flags = csb_used | csb_active;
	}

	csb->csb_blr_reader = BlrReader(blr, blr_length);

	if (view_csb)
	{
		CompilerScratch::rpt_itr ptr = view_csb->csb_rpt.begin();
		// AB: csb_n_stream replaced by view_csb->csb_rpt.getCount(), because there could
		// be more then just csb_n_stream-numbers that hold data.
		// Certainly csb_stream (see par_context where the context is retrieved)
		const CompilerScratch::rpt_const_itr end = view_csb->csb_rpt.end();
		for (SSHORT stream = 0; ptr != end; ++ptr, ++stream) {
			CompilerScratch::csb_repeat* t2 = CMP_csb_element(csb, stream);
			t2->csb_relation = ptr->csb_relation;
			t2->csb_procedure = ptr->csb_procedure;
			t2->csb_stream = ptr->csb_stream;
			t2->csb_flags = ptr->csb_flags & csb_used;
		}
		csb->csb_n_stream = view_csb->csb_n_stream;
	}

	const SSHORT version = csb->csb_blr_reader.getByte();
	switch (version)
	{
	case blr_version4:
		csb->csb_g_flags |= csb_blr_version4;
		break;
	case blr_version5:
		break; // nothing to do
	default:
		error(csb, Arg::Gds(isc_metadata_corrupt) <<
				   Arg::Gds(isc_wroblrver) << Arg::Num(blr_version4) << Arg::Num(version));
	}

	jrd_nod* node = PAR_parse_node(tdbb, csb, OTHER);
	csb->csb_node = node;

	if (csb->csb_blr_reader.getByte() != (UCHAR) blr_eoc)
		PAR_syntax_error(csb, "end_of_command");

	if (request_ptr)
		*request_ptr = CMP_make_request(tdbb, csb, true);

	return node;
}


jrd_nod* PAR_blr(thread_db* tdbb, jrd_rel* relation, const UCHAR* blr, ULONG blr_length,
	CompilerScratch* view_csb, jrd_req** request_ptr, const bool trigger, USHORT flags)
{
/**************************************
 *
 *	P A R _ b l r
 *
 **************************************
 *
 * Functional description
 *	Parse blr.
 *	Caller must do pool handling.
 *
 **************************************/
	Firebird::AutoPtr<Jrd::CompilerScratch> csb;
	return PAR_blr(tdbb, relation, blr, blr_length, view_csb, csb, request_ptr, trigger, flags);
}


USHORT PAR_desc(thread_db* tdbb, CompilerScratch* csb, DSC* desc, ItemInfo* itemInfo)
{
/**************************************
 *
 *	P A R _ d e s c
 *
 **************************************
 *
 * Functional description
 *	Parse a BLR descriptor.  Return the alignment requirements
 *	of the datatype.
 *
 **************************************/
	if (itemInfo)
	{
		itemInfo->nullable = true;
		itemInfo->explicitCollation = false;
		itemInfo->fullDomain = false;
	}

	desc->dsc_scale = 0;
	desc->dsc_sub_type = 0;
	desc->dsc_address = NULL;
	desc->dsc_flags = 0;

	const USHORT dtype = csb->csb_blr_reader.getByte();

	switch (dtype)
	{
	case blr_not_nullable:
		PAR_desc(tdbb, csb, desc, itemInfo);
		if (itemInfo)
			itemInfo->nullable = false;
		break;

	case blr_text:
		desc->dsc_dtype = dtype_text;
		desc->dsc_flags |= DSC_no_subtype;
		desc->dsc_length = csb->csb_blr_reader.getWord();
		INTL_ASSIGN_TTYPE(desc, ttype_dynamic);
		break;

	case blr_cstring:
		desc->dsc_dtype = dtype_cstring;
		desc->dsc_flags |= DSC_no_subtype;
		desc->dsc_length = csb->csb_blr_reader.getWord();
		INTL_ASSIGN_TTYPE(desc, ttype_dynamic);
		break;

	case blr_varying:
		desc->dsc_dtype = dtype_varying;
		desc->dsc_flags |= DSC_no_subtype;
		desc->dsc_length = csb->csb_blr_reader.getWord() + sizeof(USHORT);
		INTL_ASSIGN_TTYPE(desc, ttype_dynamic);
		break;

	case blr_text2:
		desc->dsc_dtype = dtype_text;
		INTL_ASSIGN_TTYPE(desc, csb->csb_blr_reader.getWord());
		desc->dsc_length = csb->csb_blr_reader.getWord();
		break;

	case blr_cstring2:
		desc->dsc_dtype = dtype_cstring;
		INTL_ASSIGN_TTYPE(desc, csb->csb_blr_reader.getWord());
		desc->dsc_length = csb->csb_blr_reader.getWord();
		break;

	case blr_varying2:
		desc->dsc_dtype = dtype_varying;
		INTL_ASSIGN_TTYPE(desc, csb->csb_blr_reader.getWord());
		desc->dsc_length = csb->csb_blr_reader.getWord() + sizeof(USHORT);
		break;

	case blr_short:
		desc->dsc_dtype = dtype_short;
		desc->dsc_length = sizeof(SSHORT);
		desc->dsc_scale = (int) csb->csb_blr_reader.getByte();
		break;

	case blr_long:
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_scale = (int) csb->csb_blr_reader.getByte();
		break;

	case blr_int64:
		desc->dsc_dtype = dtype_int64;
		desc->dsc_length = sizeof(SINT64);
		desc->dsc_scale = (int) csb->csb_blr_reader.getByte();
		break;

	case blr_quad:
		desc->dsc_dtype = dtype_quad;
		desc->dsc_length = sizeof(ISC_QUAD);
		desc->dsc_scale = (int) csb->csb_blr_reader.getByte();
		break;

	case blr_float:
		desc->dsc_dtype = dtype_real;
		desc->dsc_length = sizeof(float);
		break;

	case blr_timestamp:
		desc->dsc_dtype = dtype_timestamp;
		desc->dsc_length = sizeof(ISC_QUAD);
		break;

	case blr_sql_date:
		desc->dsc_dtype = dtype_sql_date;
		desc->dsc_length = type_lengths[dtype_sql_date];
		break;

	case blr_sql_time:
		desc->dsc_dtype = dtype_sql_time;
		desc->dsc_length = type_lengths[dtype_sql_time];
		break;

	case blr_double:
	case blr_d_float:
		desc->dsc_dtype = dtype_double;
		desc->dsc_length = sizeof(double);
		break;

	case blr_blob2:
		{
			desc->dsc_dtype = dtype_blob;
			desc->dsc_length = sizeof(ISC_QUAD);
			desc->dsc_sub_type = csb->csb_blr_reader.getWord();

			USHORT ttype = csb->csb_blr_reader.getWord();
			desc->dsc_scale = ttype & 0xFF;		// BLOB character set
			desc->dsc_flags = ttype & 0xFF00;	// BLOB collation
			break;
		}

	case blr_domain_name:
	case blr_domain_name2:
		{
			bool fullDomain = (csb->csb_blr_reader.getByte() == blr_domain_full);
			MetaName* name = FB_NEW(csb->csb_pool) MetaName(csb->csb_pool);
			par_name(csb, *name);

			MetaNamePair namePair(*name, "");

			FieldInfo fieldInfo;
			bool exist = csb->csb_map_field_info.get(namePair, fieldInfo);
			MET_get_domain(tdbb, *name, desc, (exist ? NULL : &fieldInfo));

			if (!exist)
				csb->csb_map_field_info.put(namePair, fieldInfo);

			if (itemInfo)
			{
				itemInfo->field = namePair;

				if (fullDomain)
				{
					itemInfo->nullable = fieldInfo.nullable;
					itemInfo->fullDomain = true;
				}
				else
					itemInfo->nullable = true;
			}

			if (dtype == blr_domain_name2)
			{
				const USHORT ttype = csb->csb_blr_reader.getWord();

				switch (desc->dsc_dtype)
				{
					case dtype_cstring:
					case dtype_text:
					case dtype_varying:
						INTL_ASSIGN_TTYPE(desc, ttype);
						break;

					case dtype_blob:
						desc->dsc_scale = ttype & 0xFF;		// BLOB character set
						desc->dsc_flags = ttype & 0xFF00;	// BLOB collation
						break;

					default:
						error(csb, Arg::Gds(isc_collation_requires_text));
						break;
				}
			}

			jrd_nod* dep_node = PAR_make_node(tdbb, e_dep_length);
			dep_node->nod_type = nod_dependency;
			dep_node->nod_arg[e_dep_object] = (jrd_nod*) name;
			dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_field;
			csb->csb_dependencies.push(dep_node);

			break;
		}

	case blr_column_name:
	case blr_column_name2:
		{
			const bool fullDomain = (csb->csb_blr_reader.getByte() == blr_domain_full);
			MetaName* relationName = FB_NEW(csb->csb_pool) MetaName(csb->csb_pool);
			par_name(csb, *relationName);
			MetaName* fieldName = FB_NEW(csb->csb_pool) MetaName(csb->csb_pool);
			par_name(csb, *fieldName);

			MetaNamePair namePair(*relationName, *fieldName);

			FieldInfo fieldInfo;
			bool exist = csb->csb_map_field_info.get(namePair, fieldInfo);
			MET_get_relation_field(tdbb, *relationName, *fieldName, desc, (exist ? NULL : &fieldInfo));

			if (!exist)
				csb->csb_map_field_info.put(namePair, fieldInfo);

			if (itemInfo)
			{
				itemInfo->field = namePair;

				if (fullDomain)
				{
					itemInfo->nullable = fieldInfo.nullable;
					itemInfo->fullDomain = true;
				}
				else
					itemInfo->nullable = true;
			}

			if (dtype == blr_column_name2)
			{
				const USHORT ttype = csb->csb_blr_reader.getWord();

				switch (desc->dsc_dtype)
				{
					case dtype_cstring:
					case dtype_text:
					case dtype_varying:
						INTL_ASSIGN_TTYPE(desc, ttype);
						break;

					case dtype_blob:
						desc->dsc_scale = ttype & 0xFF;		// BLOB character set
						desc->dsc_flags = ttype & 0xFF00;	// BLOB collation
						break;

					default:
						error(csb, Arg::Gds(isc_collation_requires_text));
						break;
				}
			}

			jrd_nod* dep_node = PAR_make_node(tdbb, e_dep_length);
			dep_node->nod_type = nod_dependency;
			dep_node->nod_arg[e_dep_object] = (jrd_nod*) MET_lookup_relation(tdbb, *relationName);
			dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_relation;

			dep_node->nod_arg[e_dep_field] = PAR_make_node(tdbb, 1);
			dep_node->nod_arg[e_dep_field]->nod_type = nod_literal;
			dep_node->nod_arg[e_dep_field]->nod_arg[0] = (jrd_nod*) fieldName->c_str();

			csb->csb_dependencies.push(dep_node);

			break;
		}

	default:
		error(csb, Arg::Gds(isc_datnotsup));
	}

	if (itemInfo)
	{
		if (dtype == blr_cstring2 || dtype == blr_text2 || dtype == blr_varying2 ||
			dtype == blr_blob2 || dtype == blr_domain_name2)
		{
			itemInfo->explicitCollation = true;
		}
	}

	return type_alignments[desc->dsc_dtype];
}


jrd_nod* PAR_gen_field(thread_db* tdbb, USHORT stream, USHORT id)
{
/**************************************
 *
 *	P A R _ g e n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Generate a field block.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_nod* node = FB_NEW_RPT(*tdbb->getDefaultPool(), e_fld_length) jrd_nod();
	node->nod_type = nod_field;
	node->nod_arg[e_fld_id] = (jrd_nod*) (IPTR) id;
	node->nod_arg[e_fld_stream] = (jrd_nod*) (IPTR) stream;

	return node;
}


jrd_nod* PAR_make_field(thread_db* tdbb, CompilerScratch* csb,
						USHORT context,
						const Firebird::MetaName& base_field)
{
/**************************************
 *
 *	P A R _ m a k e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Make up a field node in the permanent pool.  This is used
 *	by MET_scan_relation to handle view fields.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (context >= csb->csb_rpt.getCount() || !(csb->csb_rpt[context].csb_flags & csb_used))
		return NULL;

	const USHORT stream = csb->csb_rpt[context].csb_stream;

    /* CVC: This is just another case of a custom function that isn't prepared
       for quoted identifiers and that causes views with fields names like "z x"
       to fail miserably. Since this function was truncating field names like "z x",
       MET_lookup_field() call below failed and hence the function returned NULL
       so only caller MET_scan_relation() did field->fld_source = 0;
       This means a field without entry in rdb$fields. This is the origin of the
       mysterious message "cannot access column z x in view VF" when selecting from
       such view that has field "z x". This closes Firebird Bug #227758. */
	// solved by using MetaName& as parameter - AP
	jrd_rel* const relation = csb->csb_rpt[stream].csb_relation;
	jrd_prc* const procedure = csb->csb_rpt[stream].csb_procedure;

	const SSHORT id =
		relation ? MET_lookup_field(tdbb, relation, base_field) :
		procedure ? find_proc_field(procedure, base_field) : -1;

	if (id < 0)
		return NULL;

/* If rel_fields is NULL this means that the relation is
 * in a temporary state (partially loaded).  In this case
 * there is nothing we can do but post an error and exit.
 * Note: This will most likely happen if we have a large list
 * of deferred work which can not complete because of some
 * error, and while we are trying to commit, we find
 * that we have a dependency on something later in the list.
 * IF there were no error, then the dependency woyld have
 * been resolved, because we would have fully loaded the
 * relation, but if it can not be loaded, then we have this
 * problem. The only thing that can be done to remedy this
 * problem is to rollback.  This will clear the DeferredWork list and
 * allow the user to remedy the original error.  Note: it would
 * be incorrect for us (the server) to perform the rollback
 * implicitly, because this is a task for the user to do, and
 * should never be decided by the server. This fixes bug 10052 */

	// CVC: The code for procedures now compiles correctly, but Vlad has
	// pointed out that we don't have default for output fields, therefore
	// the code is commented till better times.
	jrd_fld* field = NULL;
	/*
	Parameter* param = NULL;

	if (procedure)
	{
		param = (*procedure->prc_output_fields)[id];
	}
	else
	*/
	if (relation)
	{
		if (!relation->rel_fields) {
			ERR_post(Arg::Gds(isc_depend_on_uncommitted_rel));
		}

		field = (*relation->rel_fields)[id];
	}

	if (csb->csb_g_flags & csb_get_dependencies) {
		par_dependency(tdbb, csb, stream, id, base_field);
	}

	jrd_nod* const temp_node = PAR_gen_field(tdbb, stream, id);
	/*
	if (param)
	{
		if (param->prm_default_value) //&& param->prm_not_null)
			temp_node->nod_arg[e_fld_default_value] =
				param->prm_default_value;
	}
	else
	*/
	if (field)
	{
		if (field->fld_default_value && field->fld_not_null)
			temp_node->nod_arg[e_fld_default_value] = field->fld_default_value;
	}

	return temp_node;
}


jrd_nod* PAR_make_list(thread_db* tdbb, NodeStack& stack)
{
/**************************************
 *
 *	P A R _ m a k e _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Make a list node out of a stack.
 *
 **************************************/
	SET_TDBB(tdbb);

/* Count the number of nodes */
	USHORT count = stack.getCount();

	jrd_nod* node = PAR_make_node(tdbb, count);
	node->nod_type = nod_list;
	jrd_nod** ptr = node->nod_arg + count;

	while (stack.hasData())
	{
		*--ptr = stack.pop();
	}

	return node;
}


jrd_nod* PAR_make_node(thread_db* tdbb, int size)
{
/**************************************
 *
 *	P A R _ m a k e _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Make a node element and pass it back.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_nod* node = FB_NEW_RPT(*tdbb->getDefaultPool(), size) jrd_nod();
	node->nod_count = size;

	return node;
}


void PAR_parse(thread_db* tdbb, AutoPtr<CompilerScratch>& csb, const UCHAR* blr, ULONG blr_length,
	bool internal_flag, USHORT dbginfo_length, const UCHAR* dbginfo)
{
/**************************************
 *
 *	P A R _ p a r s e
 *
 **************************************
 *
 * Functional description
 *	Parse blr, returning a compiler scratch block with the results.
 *
 **************************************/
	SET_TDBB(tdbb);

	csb.reset(CompilerScratch::newCsb(*tdbb->getDefaultPool(), 5));
	csb->csb_blr_reader = BlrReader(blr, blr_length);

	if (internal_flag)
		csb->csb_g_flags |= csb_internal;

	const SSHORT version = csb->csb_blr_reader.getByte();
	switch (version)
	{
	case blr_version4:
		csb->csb_g_flags |= csb_blr_version4;
		break;
	case blr_version5:
		break; // nothing to do
	default:
		error(csb, Arg::Gds(isc_wroblrver) << Arg::Num(blr_version4) << Arg::Num(version));
	}

	if (dbginfo_length > 0)
		DBG_parse_debug_info(dbginfo_length, dbginfo, csb->csb_dbg_info);

	jrd_nod* node = PAR_parse_node(tdbb, csb, OTHER);
	csb->csb_node = node;

	if (csb->csb_blr_reader.getByte() != (UCHAR) blr_eoc)
	{
		PAR_syntax_error(csb, "end_of_command");
	}
}


SLONG PAR_symbol_to_gdscode(const Firebird::string& name)
{
/**************************************
 *
 *	P A R _ s y m b o l _ t o _ g d s c o d e
 *
 **************************************
 *
 * Functional description
 *	Symbolic ASCII names are used in blr for posting and handling
 *	exceptions.  They are also used to identify error codes
 *	within system triggers in a database.
 *
 *	Returns the gds error status code for the given symbolic
 *	name, or 0 if not found.
 *
 *	Symbolic names may be null or space terminated.
 *
 **************************************/

	for (int i = 0; codes[i].code_number; ++i) {
		if (name == codes[i].code_string) {
			return codes[i].code_number;
		}
	}

	return 0;
}


// Registers a parse function (DmlNode creator) for a BLR code.
void PAR_register(UCHAR blr, NodeParseFunc parseFunc)
{
	fb_assert(!blr_parsers[blr]);
	blr_parsers[blr] = parseFunc;
}


static void error(CompilerScratch* csb, const Arg::StatusVector& v)
{
/**************************************
 *
 *	e r r o r
 *
 **************************************
 *
 * Functional description
 *	We've got a blr error other than a syntax error.  Handle it.
 *
 **************************************/
	fb_assert(v.value()[0] == isc_arg_gds);

/* Don't bother to pass tdbb for error handling */
	thread_db* tdbb = JRD_get_thread_data();

	csb->csb_blr_reader.seekBackward(1);
	Arg::Gds p(isc_invalid_blr);
	p << Arg::Num(csb->csb_blr_reader.getOffset());
	p.append(v);
	p.copyTo(tdbb->tdbb_status_vector);
	ERR_make_permanent(tdbb->tdbb_status_vector);

/* Give up whatever we were doing and return to the user. */
	ERR_punt();
}


static SSHORT find_proc_field(const jrd_prc* procedure, const Firebird::MetaName& name)
{
/**************************************
 *
 *	f i n d _ p r o c _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Look for named field in procedure output fields.
 *
 **************************************/
	vec<Parameter*>* list = procedure->prc_output_fields;
	if (!list)
		return -1;

	vec<Parameter*>::const_iterator ptr = list->begin();
	for (const vec<Parameter*>::const_iterator end = list->end(); ptr < end; ++ptr)
	{
		const Parameter* param = *ptr;
		if (name == param->prm_name)
			return param->prm_number;
	}

	return -1;
}


static jrd_nod* par_args(thread_db* tdbb, CompilerScratch* csb, USHORT expected)
{
/**************************************
 *
 *	p a r _ a r g s
 *
 **************************************
 *
 * Functional description
 *	Parse a counted argument list.
 *
 **************************************/
	SET_TDBB(tdbb);

	USHORT count = csb->csb_blr_reader.getByte();
	jrd_nod* node = PAR_make_node(tdbb, count);
	node->nod_type = nod_list;
	jrd_nod** ptr = node->nod_arg;

	if (count) {
		do {
			*ptr++ = PAR_parse_node(tdbb, csb, expected);
		} while (--count);
	}

	return node;
}


static jrd_nod* par_cast(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ c a s t
 *
 **************************************
 *
 * Functional description
 *	Parse a datatype cast
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_nod* node = PAR_make_node(tdbb, e_cast_length);
	node->nod_count = count_table[blr_cast];

	Format* format = Format::newFormat(*tdbb->getDefaultPool(), 1);
	node->nod_arg[e_cast_fmt] = (jrd_nod*) format;

	dsc* desc = &format->fmt_desc[0];
	ItemInfo itemInfo;
	PAR_desc(tdbb, csb, desc, &itemInfo);
	format->fmt_length = desc->dsc_length;

	node->nod_arg[e_cast_source] = PAR_parse_node(tdbb, csb, VALUE);

	if (itemInfo.isSpecial())
	{
		ItemInfo* p = FB_NEW(*tdbb->getDefaultPool()) ItemInfo(*tdbb->getDefaultPool(), itemInfo);
		node->nod_arg[e_cast_iteminfo] = (jrd_nod*) p;
	}

	if (itemInfo.explicitCollation)
	{
		jrd_nod* dep_node = PAR_make_node (tdbb, e_dep_length);
		dep_node->nod_type = nod_dependency;
		dep_node->nod_arg [e_dep_object] = (jrd_nod*)(IPTR) INTL_TEXT_TYPE(*desc);
		dep_node->nod_arg [e_dep_object_type] = (jrd_nod*)(IPTR) obj_collation;
		csb->csb_dependencies.push(dep_node);
	}

	return node;
}


static PsqlException* par_condition(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ c o n d i t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse an error conditions list.
 *
 **************************************/
	SET_TDBB(tdbb);

/* allocate a node to represent the conditions list */

	const USHORT code_type = csb->csb_blr_reader.getByte();

	/* don't create PsqlException if blr_raise is used,
	   just return NULL */
	if (code_type == blr_raise)
	{
		return NULL;
	}

	PsqlException* exception_list = FB_NEW_RPT(*tdbb->getDefaultPool(), 1) PsqlException();
	exception_list->xcp_count = 1;
	xcp_repeat& item = exception_list->xcp_rpt[0];

	switch (code_type)
	{
	case blr_sql_code:
		item.xcp_type = xcp_sql_code;
		item.xcp_code = (SSHORT) csb->csb_blr_reader.getWord();
		break;

	case blr_gds_code:
		{
			string name;
			item.xcp_type = xcp_gds_code;
			par_name(csb, name);
			name.lower();
			SLONG code_number = PAR_symbol_to_gdscode(name);
			if (code_number)
				item.xcp_code = code_number;
			else
				error(csb, Arg::Gds(isc_codnotdef) << Arg::Str(name));
		}
		break;

	case blr_exception:
	case blr_exception_msg:
		{
			MetaName name;
			item.xcp_type = xcp_xcp_code;
			par_name(csb, name);
			if (!(item.xcp_code = MET_lookup_exception_number(tdbb, name)))
				error(csb, Arg::Gds(isc_xcpnotdef) << Arg::Str(name));
			jrd_nod* dep_node = PAR_make_node(tdbb, e_dep_length);
			dep_node->nod_type = nod_dependency;
			dep_node->nod_arg[e_dep_object] = (jrd_nod*)(IPTR) item.xcp_code;
			dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_exception;
			csb->csb_dependencies.push(dep_node);
		}
		break;

	default:
		fb_assert(false);
		break;
	}

	return exception_list;
}


static PsqlException* par_conditions(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ c o n d i t i o n s
 *
 **************************************
 *
 * Functional description
 *	Parse an error conditions list.
 *
 **************************************/
	SET_TDBB(tdbb);

	// allocate a node to represent the conditions list

	const USHORT n = csb->csb_blr_reader.getWord();
	PsqlException* exception_list = FB_NEW_RPT(*tdbb->getDefaultPool(), n) PsqlException();
	exception_list->xcp_count = n;

	for (int i = 0; i < n; i++)
	{
		const USHORT code_type = csb->csb_blr_reader.getByte();
		xcp_repeat& item = exception_list->xcp_rpt[i];

		switch (code_type)
		{
		case blr_sql_code:
			item.xcp_type = xcp_sql_code;
			item.xcp_code = (SSHORT) csb->csb_blr_reader.getWord();
			break;

		case blr_gds_code:
			{
				string name;
				item.xcp_type = xcp_gds_code;
				par_name(csb, name);
				name.lower();
				SLONG code_number = PAR_symbol_to_gdscode(name);
				if (code_number)
					item.xcp_code = code_number;
				else
					error(csb, Arg::Gds(isc_codnotdef) << Arg::Str(name));
			}
			break;

		case blr_exception:
			{
				MetaName name;
				item.xcp_type = xcp_xcp_code;
				par_name(csb, name);
				if (!(item.xcp_code = MET_lookup_exception_number(tdbb, name)))
					error(csb, Arg::Gds(isc_xcpnotdef) << Arg::Str(name));
				jrd_nod* dep_node = PAR_make_node(tdbb, e_dep_length);
				dep_node->nod_type = nod_dependency;
				dep_node->nod_arg[e_dep_object] = (jrd_nod*)(IPTR) item.xcp_code;
				dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_exception;
				csb->csb_dependencies.push(dep_node);
			}
			break;

		case blr_default_code:
			item.xcp_type = xcp_default;
			item.xcp_code = 0;
			break;

		default:
			fb_assert(FALSE);
			break;
		}
	}

	return exception_list;
}


static SSHORT par_context(CompilerScratch* csb, SSHORT* context_ptr)
{
/**************************************
 *
 *	p a r _ c o n t e x t
 *
 **************************************
 *
 * Functional description
 *	Introduce a new context into the system.  This involves
 *	assigning a stream and possibly extending the compile
 *	scratch block.
 *
 **************************************/

	const SSHORT context = (unsigned int) csb->csb_blr_reader.getByte();

	if (context_ptr)
		*context_ptr = context;

	CompilerScratch::csb_repeat* tail = CMP_csb_element(csb, context);

	if (tail->csb_flags & csb_used) {
		if (csb->csb_g_flags & csb_reuse_context) {
			return tail->csb_stream;
		}

		error(csb, Arg::Gds(isc_ctxinuse));
	}

	const SSHORT stream = csb->nextStream(false);
	if (stream >= MAX_STREAMS)
	{
		error(csb, Arg::Gds(isc_too_many_contexts));
	}

	tail->csb_flags |= csb_used;
	tail->csb_stream = (UCHAR) stream;

	CMP_csb_element(csb, stream);

	return stream;
}


static void par_dependency(thread_db*   tdbb,
						   CompilerScratch*    csb,
						   SSHORT stream,
						   SSHORT id,
						   const Firebird::MetaName& field_name)
{
/**************************************
 *
 *	p a r _ d e p e n d e n c y
 *
 **************************************
 *
 * Functional description
 *	Register a field, relation, procedure or exception reference
 *	as a dependency.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_nod* node = PAR_make_node(tdbb, e_dep_length);
	node->nod_type = nod_dependency;
	if (csb->csb_rpt[stream].csb_relation) {
		node->nod_arg[e_dep_object] = (jrd_nod*) csb->csb_rpt[stream].csb_relation;
		// How do I determine reliably this is a view?
		// At this time, rel_view_rse is still null.
		//if (is_view)
		//	node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_view;
		//else
			node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_relation;
	}
	else if (csb->csb_rpt[stream].csb_procedure) {
		node->nod_arg[e_dep_object] = (jrd_nod*) csb->csb_rpt[stream].csb_procedure;
		node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_procedure;
	}

	if (field_name.length() > 0) {
		jrd_nod* field_node = PAR_make_node(tdbb, 1);
		node->nod_arg[e_dep_field] = field_node;
		field_node->nod_type = nod_literal;
		field_node->nod_arg[0] = (jrd_nod*) stringDup(*tdbb->getDefaultPool(), field_name.c_str());
	}
	else if (id >= 0) {
		jrd_nod* field_node = PAR_make_node(tdbb, 1);
		node->nod_arg[e_dep_field] = field_node;
		field_node->nod_type = nod_field;
		field_node->nod_arg[0] = (jrd_nod*) (IPTR) id;
	}

	csb->csb_dependencies.push(node);
}


static jrd_nod* par_exec_proc(thread_db* tdbb, CompilerScratch* csb, SSHORT blr_operator)
{
/**************************************
 *
 *	p a r _ e x e c _ p r o c
 *
 **************************************
 *
 * Functional description
 *	Parse an execute procedure  reference.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_prc* procedure = NULL;
	{
		Firebird::MetaName name;

		if (blr_operator == blr_exec_pid) {
			const USHORT pid = csb->csb_blr_reader.getWord();
			if (!(procedure = MET_lookup_procedure_id(tdbb, pid, false, false, 0)))
				name.printf("id %d", pid);
		}
		else {
			par_name(csb, name);
			procedure = MET_lookup_procedure(tdbb, name, false);
		}
		if (!procedure)
			error(csb, Arg::Gds(isc_prcnotdef) << Arg::Str(name));
	}

	jrd_nod* node = PAR_make_node(tdbb, e_esp_length);
	node->nod_type = nod_exec_proc;
	node->nod_count = count_table[blr_exec_proc];
	node->nod_arg[e_esp_procedure] = (jrd_nod*) procedure;

	par_procedure_parms(tdbb, csb, procedure, &node->nod_arg[e_esp_in_msg],
						&node->nod_arg[e_esp_inputs], true);
	par_procedure_parms(tdbb, csb, procedure, &node->nod_arg[e_esp_out_msg],
						&node->nod_arg[e_esp_outputs], false);

	jrd_nod* dep_node = PAR_make_node(tdbb, e_dep_length);
	dep_node->nod_type = nod_dependency;
	dep_node->nod_arg[e_dep_object] = (jrd_nod*) procedure;
	dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_procedure;

	csb->csb_dependencies.push(dep_node);

	return node;
}


static jrd_nod* par_fetch(thread_db* tdbb, CompilerScratch* csb, jrd_nod* for_node)
{
/**************************************
 *
 *	p a r _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	Parse a FETCH statement, and map it into
 *
 *	    FOR x IN relation WITH x.DBKEY EQ value ...
 *
 **************************************/
	SET_TDBB(tdbb);

/* Fake RecordSelExpr */

	for_node->nod_arg[e_for_re] = PAR_make_node(tdbb, 1 + rse_delta + 2);
	RecordSelExpr* rse = (RecordSelExpr*) for_node->nod_arg[e_for_re];
	rse->nod_type = nod_rse;
	rse->nod_count = 0;
	rse->rse_count = 1;
	jrd_nod* relation = PAR_parse_node(tdbb, csb, RELATION);
	rse->rse_relation[0] = relation;

/* Fake boolean */

	jrd_nod* node = rse->rse_boolean = PAR_make_node(tdbb, 2);
	node->nod_type = nod_eql;
	node->nod_flags = nod_comparison;
	node->nod_arg[1] = PAR_parse_node(tdbb, csb, VALUE);
	node->nod_arg[0] = PAR_make_node(tdbb, 1);
	node = node->nod_arg[0];
	node->nod_type = nod_dbkey;
	node->nod_count = 0;
	node->nod_arg[0] = relation->nod_arg[e_rel_stream];

/* Pick up statement */

	for_node->nod_arg[e_for_statement] = PAR_parse_node(tdbb, csb, STATEMENT);

	return for_node;
}


static jrd_nod* par_field(thread_db* tdbb, CompilerScratch* csb, SSHORT blr_operator)
{
/**************************************
 *
 *	p a r _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Parse a field.
 *
 **************************************/
	SET_TDBB(tdbb);

	const USHORT context = (unsigned int) csb->csb_blr_reader.getByte();

	// check if this is a VALUE of domain's check constraint
	if (!csb->csb_domain_validation.isEmpty() &&
		(blr_operator == blr_fid || blr_operator == blr_field) && context == 0)
	{
		if (blr_operator == blr_fid) {
#ifdef DEV_BUILD
			SSHORT id =
#endif
				csb->csb_blr_reader.getWord();
			fb_assert(id == 0);
		}
		else {
			Firebird::MetaName name;
			par_name(csb, name);
		}

		jrd_nod* node = PAR_make_node(tdbb, e_domval_length);
		node->nod_type = nod_domain_validation;
		node->nod_count = 0;

		dsc* desc = (dsc*) (node->nod_arg + e_domval_desc);
		MET_get_domain(tdbb, csb->csb_domain_validation, desc, NULL);

		return node;
	}

	if (context >= csb->csb_rpt.getCount())/* ||
		!(csb->csb_rpt[context].csb_flags & csb_used) )

		dimitr:	commented out to support system triggers implementing
				WITH CHECK OPTION. They reference the relation stream (#2)
				directly, without a DSQL context. It breaks the layering,
				but we must support legacy BLR.
		*/
	{
		error(csb, Arg::Gds(isc_ctxnotdef));
	}

	Firebird::MetaName name;
	SSHORT id;
	const SSHORT stream = csb->csb_rpt[context].csb_stream;
	SSHORT flags = 0;
	bool is_column = false;

	if (blr_operator == blr_fid)
	{
		id = csb->csb_blr_reader.getWord();
		flags = nod_id;
		is_column = true;
	}
	else if (blr_operator == blr_field)
	{
		CompilerScratch::csb_repeat* tail = &csb->csb_rpt[stream];
		const jrd_prc* procedure = tail->csb_procedure;

		/* make sure procedure has been scanned before using it */

		if (procedure && (!(procedure->prc_flags & PRC_scanned) ||
				(procedure->prc_flags & PRC_being_scanned) ||
				(procedure->prc_flags & PRC_being_altered)))
		{
			const jrd_prc* scan_proc = MET_procedure(tdbb, procedure->prc_id, false, 0);
			if (scan_proc != procedure)
				procedure = NULL;
		}

		if (procedure) {
			par_name(csb, name);
			if ((id = find_proc_field(procedure, name)) == -1)
				error(csb, Arg::Gds(isc_fldnotdef2) << Arg::Str(name) << Arg::Str(procedure->prc_name));
		}
		else
		{
			jrd_rel* relation = tail->csb_relation;
			if (!relation)
				error(csb, Arg::Gds(isc_ctxnotdef));

			/* make sure relation has been scanned before using it */

			if (!(relation->rel_flags & REL_scanned) || (relation->rel_flags & REL_being_scanned))
			{
				MET_scan_relation(tdbb, relation);
			}

			par_name(csb, name);
			if ((id = MET_lookup_field(tdbb, relation, name)) < 0)
			{
				if (csb->csb_g_flags & csb_validation) {
					id = 0;
					flags |= nod_id;
					is_column = true;
				}
				else {
					if (relation->rel_flags & REL_system)
					{
						jrd_nod* node = PAR_make_node(tdbb, 0);
						node->nod_type = nod_null;
						return node;
 					}

 					if (tdbb->getAttachment()->att_flags & ATT_gbak_attachment)
					{
						warning(Arg::Warning(isc_fldnotdef) << Arg::Str(name) <<
															   Arg::Str(relation->rel_name));
					}
					else if (!(relation->rel_flags & REL_deleted))
					{
						error(csb, Arg::Gds(isc_fldnotdef) << Arg::Str(name) <<
															  Arg::Str(relation->rel_name));
					}
					else
						error(csb, Arg::Gds(isc_ctxnotdef));
				}
			}
		}
	}

/* check for dependencies -- if a field name was given,
   use it because when restoring the database the field
   id's may not be valid yet */

	if (csb->csb_g_flags & csb_get_dependencies) {
		if (blr_operator == blr_fid)
			par_dependency(tdbb, csb, stream, id, "");
		else
			par_dependency(tdbb, csb, stream, id, name);
	}

	jrd_nod* node = PAR_gen_field(tdbb, stream, id);
	node->nod_flags |= flags;

	if (is_column)
	{
		const jrd_rel* const temp_rel = csb->csb_rpt[stream].csb_relation;

		if (temp_rel)
		{
			fb_assert(id >= 0);

			jrd_fld* field;

			if (temp_rel->rel_fields &&
				id < (int) temp_rel->rel_fields->count() &&
				(field = (*temp_rel->rel_fields)[id]))
			{
				if (field->fld_default_value && field->fld_not_null)
					node->nod_arg[e_fld_default_value] = field->fld_default_value;
			}
			else if (temp_rel->rel_flags & REL_system)
			{
				node = PAR_make_node(tdbb, 0);
				node->nod_type = nod_null;
			}
		}
	}

	return node;
}


static jrd_nod* par_function(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a function reference.
 *
 **************************************/
	SET_TDBB(tdbb);

	Firebird::MetaName name;
	const USHORT count = par_name(csb, name);

	// Isn't it strange that gbak presence means nothing to this function now?
	UserFunction* function =
		FUN_lookup_function(tdbb, name); //, !(tdbb->getAttachment()->att_flags & ATT_gbak_attachment));
	if (!function) {
		if (tdbb->tdbb_flags & TDBB_prc_being_dropped) {
			jrd_nod* anode = PAR_make_node(tdbb, e_fun_length);
			anode->nod_count = 1;
			anode->nod_arg[e_fun_function] = NULL;
			anode->nod_arg[e_fun_args] = par_args(tdbb, csb, VALUE);
			return anode;
		}

		csb->csb_blr_reader.seekBackward(count);
		error(csb, Arg::Gds(isc_funnotdef) << Arg::Str(name));
	}

	UserFunction* homonyms;
	for (homonyms = function; homonyms; homonyms = homonyms->fun_homonym) {
		if (homonyms->fun_entrypoint)
			break;
	}

	if (!homonyms)
		if (tdbb->getAttachment()->att_flags & ATT_gbak_attachment)
		{
			warning(Arg::Warning(isc_funnotdef) << Arg::Str(name) <<
					Arg::Warning(isc_modnotfound));
		}
		else {
			csb->csb_blr_reader.seekBackward(count);
			error(csb, Arg::Gds(isc_funnotdef) << Arg::Str(name) <<
					   Arg::Gds(isc_modnotfound));
		}

	jrd_nod* node = PAR_make_node(tdbb, e_fun_length);
	node->nod_count = 1;
	node->nod_arg[e_fun_function] = (jrd_nod*) function;
	node->nod_arg[e_fun_args] = par_args(tdbb, csb, VALUE);

    /* CVC: I will track ufds only if a proc is not being dropped. */
    if (csb->csb_g_flags & csb_get_dependencies) {
        jrd_nod* dep_node = PAR_make_node (tdbb, e_dep_length);
        dep_node->nod_type = nod_dependency;
        dep_node->nod_arg [e_dep_object] = (jrd_nod*) function;
        dep_node->nod_arg [e_dep_object_type] = (jrd_nod*)(IPTR) obj_udf;
        csb->csb_dependencies.push(dep_node);
    }

	return node;
}


static jrd_nod* par_literal(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ l i t e r a l
 *
 **************************************
 *
 * Functional description
 *	Parse a literal value.
 *
 **************************************/
	SSHORT scale;
	UCHAR dtype;

	SET_TDBB(tdbb);

	DSC desc;
	PAR_desc(tdbb, csb, &desc);
	const int count = lit_delta + (desc.dsc_length + sizeof(jrd_nod*) - 1) / sizeof(jrd_nod*);
	jrd_nod* node = PAR_make_node(tdbb, count);
	Literal* literal = (Literal*) node;
	node->nod_count = 0;
	literal->lit_desc = desc;
	UCHAR* p = reinterpret_cast<UCHAR*>(literal->lit_data);
	literal->lit_desc.dsc_address = p;
	literal->lit_desc.dsc_flags = 0;
	const UCHAR* q = csb->csb_blr_reader.getPos();
	USHORT l = desc.dsc_length;

	switch (desc.dsc_dtype)
	{
	case dtype_short:
		l = 2;
		*(SSHORT *) p = (SSHORT) gds__vax_integer(q, l);
		break;

	case dtype_long:
	case dtype_sql_date:
	case dtype_sql_time:
		l = 4;
		*(SLONG *) p = gds__vax_integer(q, l);
		break;

	case dtype_timestamp:
		l = 8;
		*(SLONG *) p = gds__vax_integer(q, 4);
		p += 4;
		q += 4;
		*(SLONG *) p = gds__vax_integer(q, 4);
		break;

	case dtype_int64:
		l = sizeof(SINT64);
		*(SINT64 *) p = isc_portable_integer(q, l);
		break;

	case dtype_double:
		/* the double literal could potentially be used for any
		   numeric literal - the value is passed as if it were a
		   text string. Convert the numeric string to its binary
		   value (int64, long or double as appropriate). */
		l = csb->csb_blr_reader.getWord();
		q = csb->csb_blr_reader.getPos();
		dtype = CVT_get_numeric(q, l, &scale, (double *) p);
		literal->lit_desc.dsc_dtype = dtype;
		switch (dtype)
		{
		case dtype_double:
			literal->lit_desc.dsc_length = sizeof(double);
			break;
		case dtype_long:
			literal->lit_desc.dsc_length = sizeof(SLONG);
			literal->lit_desc.dsc_scale = (SCHAR) scale;
			break;
		default:
			literal->lit_desc.dsc_length = sizeof(SINT64);
			literal->lit_desc.dsc_scale = (SCHAR) scale;
		}
		break;

	case dtype_text:
		memcpy(p, q, l);
		break;

	default:
		fb_assert(FALSE);
	}

	csb->csb_blr_reader.seekForward(l);

	return node;
}


static jrd_nod* par_map(thread_db* tdbb, CompilerScratch* csb, USHORT stream)
{
/**************************************
 *
 *	p a r _ m a p
 *
 **************************************
 *
 * Functional description
 *	Parse a MAP clause for a union or global aggregate expression.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (csb->csb_blr_reader.getByte() != blr_map)
		PAR_syntax_error(csb, "blr_map");

	SSHORT count = csb->csb_blr_reader.getWord();
	NodeStack map;

	while (--count >= 0) {
		jrd_nod* assignment = PAR_make_node(tdbb, e_asgn_length);
		assignment->nod_type = nod_assignment;
		assignment->nod_count = e_asgn_length;
		assignment->nod_arg[e_asgn_to] = PAR_gen_field(tdbb, stream, csb->csb_blr_reader.getWord());
		assignment->nod_arg[e_asgn_from] = PAR_parse_node(tdbb, csb, VALUE);
		map.push(assignment);
	}

	jrd_nod* node = PAR_make_list(tdbb, map);
	node->nod_type = nod_map;

	return node;
}


static jrd_nod* par_message(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	Parse a message declaration, including operator byte.
 *
 **************************************/
	SET_TDBB(tdbb);

/* Get message number, register it in the compiler scratch block, and
   allocate a node to represent the message */

	USHORT n = (unsigned int) csb->csb_blr_reader.getByte();
	CompilerScratch::csb_repeat* tail = CMP_csb_element(csb, n);
	jrd_nod* node = PAR_make_node(tdbb, e_msg_length);
	tail->csb_message = node;
	node->nod_count = 0;
	node->nod_arg[e_msg_number] = (jrd_nod*) (IPTR) n;
	if (n > csb->csb_msg_number)
		csb->csb_msg_number = n;

/* Get the number of parameters in the message and prepare to fill
   out the format block */

	n = csb->csb_blr_reader.getWord();
	Format* format = Format::newFormat(*tdbb->getDefaultPool(), n);
	node->nod_arg[e_msg_format] = (jrd_nod*) format;
	ULONG offset = 0;

	Format::fmt_desc_iterator desc, end;
	USHORT index = 0;

	for (desc = format->fmt_desc.begin(), end = desc + n; desc < end; ++desc, ++index)
	{
		ItemInfo itemInfo;
		const USHORT alignment = PAR_desc(tdbb, csb, &*desc, &itemInfo);
		if (alignment)
			offset = FB_ALIGN(offset, alignment);
		desc->dsc_address = (UCHAR *) (IPTR) offset;
		offset += desc->dsc_length;

		// ASF: Odd indexes are the nullable flag.
		// So we only check even indexes, which is the actual parameter.
		if (itemInfo.isSpecial() && index % 2 == 0)
		{
			csb->csb_dbg_info.argInfoToName.get(
				Firebird::ArgumentInfo(csb->csb_msg_number, index / 2), itemInfo.name);

			csb->csb_map_item_info.put(Item(nod_argument, csb->csb_msg_number, index), itemInfo);
		}
	}

	if (offset > MAX_FORMAT_SIZE)
		error(csb, Arg::Gds(isc_imp_exc) << Arg::Gds(isc_blktoobig));

	format->fmt_length = (USHORT) offset;

	return node;
}


static jrd_nod* par_modify(thread_db* tdbb, CompilerScratch* csb, SSHORT blr_operator)
{
/**************************************
 *
 *	p a r _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Parse a modify statement.
 *
 **************************************/
	SET_TDBB(tdbb);

/* Parse the original and new contexts */

	USHORT context = (unsigned int) csb->csb_blr_reader.getByte();
	if (context >= csb->csb_rpt.getCount() || !(csb->csb_rpt[context].csb_flags & csb_used))
	{
		error(csb, Arg::Gds(isc_ctxnotdef));
	}
	const SSHORT org_stream = csb->csb_rpt[context].csb_stream;
	const SSHORT new_stream = csb->nextStream(false);
	if (new_stream >= MAX_STREAMS)
	{
		error(csb, Arg::Gds(isc_too_many_contexts));
	}
	context = (unsigned int) csb->csb_blr_reader.getByte();

/* Make sure the compiler scratch block is big enough to hold
   everything */

	CompilerScratch::csb_repeat* tail = CMP_csb_element(csb, context);
	tail->csb_stream = (UCHAR) new_stream;
	tail->csb_flags |= csb_used;

	tail = CMP_csb_element(csb, new_stream);
	tail->csb_relation = csb->csb_rpt[org_stream].csb_relation;

/* Make the node and parse the sub-expression */

	jrd_nod* node = PAR_make_node(tdbb, e_mod_length);
	node->nod_count = 1;
	node->nod_arg[e_mod_org_stream] = (jrd_nod*) (IPTR) org_stream;
	node->nod_arg[e_mod_new_stream] = (jrd_nod*) (IPTR) new_stream;
	node->nod_arg[e_mod_statement] = PAR_parse_node(tdbb, csb, STATEMENT);

	if (blr_operator == blr_modify2)
	{
		node->nod_count = 2;
		node->nod_arg[e_mod_statement2] = PAR_parse_node(tdbb, csb, STATEMENT);
	}

	return node;
}


static USHORT par_name(CompilerScratch* csb, Firebird::MetaName& name)
{
/**************************************
 *
 *	p a r _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Parse a counted string, returning count.
 *
 **************************************/
	size_t l = csb->csb_blr_reader.getByte();

	// Check for overly long identifiers at BLR parse stage to prevent unwanted
	// surprises in deeper layers of the engine.
	if (l > MAX_SQL_IDENTIFIER_LEN) {
		SqlIdentifier st;
		char* s = st;
		l = MAX_SQL_IDENTIFIER_LEN;
		while (l--)
			*s++ = csb->csb_blr_reader.getByte();
		*s = 0;
		ERR_post(Arg::Gds(isc_identifier_too_long) << Arg::Str(st));
	}

	char* s = name.getBuffer(l);

	while (l--)
		*s++ = csb->csb_blr_reader.getByte();

	return name.length();
}


static size_t par_name(CompilerScratch* csb, Firebird::string& name)
{
/**************************************
 *
 *	p a r _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Parse a counted string of virtually unlimited size
 *  (up to 64K, actually <= 255), returning count.
 *
 **************************************/
	size_t l = csb->csb_blr_reader.getByte();
	char* s = name.getBuffer(l);

	while (l--)
		*s++ = csb->csb_blr_reader.getByte();

	return name.length();
}


static jrd_nod* par_plan(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ p l a n
 *
 **************************************
 *
 * Functional description
 *	Parse an access plan expression.
 *	At this stage we are just generating the
 *	parse tree and checking contexts
 *	and indices.
 *
 **************************************/
	SET_TDBB(tdbb);

	USHORT node_type = (USHORT) csb->csb_blr_reader.getByte();

/* a join type indicates a cross of two or more streams */

	if (node_type == blr_join || node_type == blr_merge) {
		USHORT count = (USHORT) csb->csb_blr_reader.getByte();
		jrd_nod* plan = PAR_make_node(tdbb, count);
		plan->nod_type = (nod_t)(USHORT) blr_table[node_type];

		for (jrd_nod** arg = plan->nod_arg; count--;)
			*arg++ = par_plan(tdbb, csb);
		return plan;
	}

/* we have hit a stream; parse the context number and access type */

	if (node_type == blr_retrieve)
	{
		jrd_nod* plan = PAR_make_node(tdbb, e_retrieve_length);
		plan->nod_type = (nod_t)(USHORT) blr_table[node_type];

		/* parse the relation name and context--the relation
		   itself is redundant except in the case of a view,
		   in which case the base relation (and alias) must be specified */

		USHORT n = (unsigned int) csb->csb_blr_reader.getByte();
		if (n != blr_relation && n != blr_relation2 && n != blr_rid && n != blr_rid2)
		{
			PAR_syntax_error(csb, elements[RELATION]);
		}

		/* don't have par_relation() parse the context, because
		   this would add a new context; while this is a reference to
		   an existing context */

		jrd_nod* relation_node = par_relation(tdbb, csb, n, false);
		plan->nod_arg[e_retrieve_relation] = relation_node;
		jrd_rel* relation = (jrd_rel*) relation_node->nod_arg[e_rel_relation];

		n = csb->csb_blr_reader.getByte();
		if (n >= csb->csb_rpt.getCount() || !(csb->csb_rpt[n].csb_flags & csb_used))
			error(csb, Arg::Gds(isc_ctxnotdef));
		const SSHORT stream = csb->csb_rpt[n].csb_stream;

		relation_node->nod_arg[e_rel_stream] = (jrd_nod*) (IPTR) stream;
		relation_node->nod_arg[e_rel_context] = (jrd_nod*) (IPTR) n;

		/* Access plan types (sequential is default) */

		node_type = (USHORT) csb->csb_blr_reader.getByte();
		USHORT extra_count = 0;
		jrd_nod* access_type = 0;
		Firebird::MetaName name;
		TEXT* idx_name = 0;

		switch (node_type)
		{
		case blr_navigational:
			{
				access_type = plan->nod_arg[e_retrieve_access_type] =
					PAR_make_node(tdbb, e_access_type_length);
				access_type->nod_type = nod_navigational;

				/* pick up the index name and look up the appropriate ids */

				par_name(csb, name);
	            /* CVC: We can't do this. Index names are identifiers.
	               for (p = name; *p; *p++)
	               *p = UPPER (*p);
	               */
				SLONG relation_id;
				SSHORT idx_status;
				const SLONG index_id = MET_lookup_index_name(tdbb, name, &relation_id, &idx_status);

				if (idx_status == MET_object_unknown || idx_status == MET_object_inactive)
				{
					if (tdbb->getAttachment()->att_flags & ATT_gbak_attachment)
					{
						warning(Arg::Warning(isc_indexname) << Arg::Str(name) <<
															   Arg::Str(relation->rel_name));
					}
					else
					{
						error(csb, Arg::Gds(isc_indexname) << Arg::Str(name) <<
															  Arg::Str(relation->rel_name));
					}
				}

				/* save both the relation id and the index id, since
				   the relation could be a base relation of a view;
				   save the index name also, for convenience */

				access_type->nod_arg[e_access_type_relation] = (jrd_nod*) (IPTR) relation_id;
				access_type->nod_arg[e_access_type_index] = (jrd_nod*) (IPTR) index_id;
				idx_name = stringDup(*tdbb->getDefaultPool(), name.c_str());
				access_type->nod_arg[e_access_type_index_name] = (jrd_nod*) idx_name;

				if (csb->csb_g_flags & csb_get_dependencies)
				{
	                jrd_nod* dep_node = PAR_make_node (tdbb, e_dep_length);
	                dep_node->nod_type = nod_dependency;
	                dep_node->nod_arg[e_dep_object] = (jrd_nod*) idx_name;
	                dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_index;
	                csb->csb_dependencies.push(dep_node);
	            }

				if (csb->csb_blr_reader.peekByte() == blr_indices)
				{
					// dimitr:	FALL INTO, if the plan item is ORDER ... INDEX (...)
					extra_count = 3;
				}
				else
					break;
			}
		case blr_indices:
			{
				if (extra_count)
					csb->csb_blr_reader.getByte(); // skip blr_indices
				USHORT count = (USHORT) csb->csb_blr_reader.getByte();
				jrd_nod* temp = plan->nod_arg[e_retrieve_access_type] =
					PAR_make_node(tdbb, count * e_access_type_length + extra_count);
				for (USHORT i = 0; i < extra_count; i++) {
					temp->nod_arg[i] = access_type->nod_arg[i];
				}
				temp->nod_type = extra_count ? nod_navigational : nod_indices;
				if (extra_count)
					delete access_type;
				access_type = temp;

				/* pick up the index names and look up the appropriate ids */

				for (jrd_nod** arg = access_type->nod_arg + extra_count; count--;)
				{
					par_name(csb, name);
	          		/* Nickolay Samofatov: We can't do this. Index names are identifiers.
					 for (p = name; *p; *p++)
					 *p = UPPER(*p);
	  	             */
					SLONG relation_id;
					SSHORT idx_status;
					const SLONG index_id = MET_lookup_index_name(tdbb, name, &relation_id, &idx_status);

					if (idx_status == MET_object_unknown || idx_status == MET_object_inactive)
					{
						if (tdbb->getAttachment()->att_flags & ATT_gbak_attachment)
						{
							warning(Arg::Warning(isc_indexname) << Arg::Str(name) <<
																   Arg::Str(relation->rel_name));
						}
						else
						{
							error(csb, Arg::Gds(isc_indexname) << Arg::Str(name) <<
																  Arg::Str(relation->rel_name));
						}
					}

					/* save both the relation id and the index id, since
					   the relation could be a base relation of a view;
					   save the index name also, for convenience */

					*arg++ = (jrd_nod*) (IPTR) relation_id;
					*arg++ = (jrd_nod*) (IPTR) index_id;
					idx_name = stringDup(*tdbb->getDefaultPool(), name.c_str());
					*arg++ = (jrd_nod*) idx_name;

					if (csb->csb_g_flags & csb_get_dependencies)
					{
		                jrd_nod* dep_node = PAR_make_node (tdbb, e_dep_length);
		                dep_node->nod_type = nod_dependency;
		                dep_node->nod_arg[e_dep_object] = (jrd_nod*) idx_name;
		                dep_node->nod_arg[e_dep_object_type] = (jrd_nod*)(IPTR) obj_index;
		                csb->csb_dependencies.push(dep_node);
		            }
				}
			}
			break;
		case blr_sequential:
			break;
		default:
			PAR_syntax_error(csb, "access type");
		}

		return plan;
	}

	PAR_syntax_error(csb, "plan item");
	return NULL;			/* Added to remove compiler warning */
}


static jrd_nod* par_procedure(thread_db* tdbb, CompilerScratch* csb, SSHORT blr_operator)
{
/**************************************
 *
 *	p a r _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Parse an procedural view reference.
 *
 **************************************/
	jrd_prc* procedure;

	SET_TDBB(tdbb);

	{
		Firebird::MetaName name;

		if (blr_operator == blr_procedure) {
			par_name(csb, name);
			procedure = MET_lookup_procedure(tdbb, name, false);
		}
		else {
			const SSHORT pid = csb->csb_blr_reader.getWord();
			if (!(procedure = MET_lookup_procedure_id(tdbb, pid, false, false, 0)))
				name.printf("id %d", pid);
		}
		if (!procedure)
			error(csb, Arg::Gds(isc_prcnotdef) << Arg::Str(name));
	}

	if (procedure->prc_type == prc_executable)
	{
		if (tdbb->getAttachment()->att_flags & ATT_gbak_attachment)
		{
			warning(Arg::Warning(isc_illegal_prc_type) << Arg::Str(procedure->prc_name));
		}
		else
		{
			error(csb, Arg::Gds(isc_illegal_prc_type) << Arg::Str(procedure->prc_name));
		}
	}

	jrd_nod* node = PAR_make_node(tdbb, e_prc_length);
	node->nod_type = nod_procedure;
	node->nod_count = count_table[blr_procedure];
	node->nod_arg[e_prc_procedure] = (jrd_nod*) (IPTR) procedure->prc_id;

	SSHORT context;
	const SSHORT stream = par_context(csb, &context);
	node->nod_arg[e_prc_stream] = (jrd_nod*) (IPTR) stream;
	csb->csb_rpt[stream].csb_procedure = procedure;
	node->nod_arg[e_prc_context] = (jrd_nod*) (IPTR) context;

	par_procedure_parms(tdbb, csb, procedure, &node->nod_arg[e_prc_in_msg],
						&node->nod_arg[e_prc_inputs], true);

	if (csb->csb_g_flags & csb_get_dependencies)
		par_dependency(tdbb, csb, stream, (SSHORT) -1, "");

	return node;
}


static void par_procedure_parms(thread_db* tdbb,
								CompilerScratch* csb,
								jrd_prc* procedure,
								jrd_nod** message_ptr,
								jrd_nod** parameter_ptr, bool input_flag)
{
/**************************************
 *
 *	p a r _ p r o c e d u r e _ p a r m s
 *
 **************************************
 *
 * Functional description
 *	Parse some procedure parameters.
 *
 **************************************/
	SET_TDBB(tdbb);
	bool mismatch = false;
	SLONG count = csb->csb_blr_reader.getWord();

/** Check to see if the parameter count matches **/
	if (input_flag ?
			(count < (procedure->prc_inputs - procedure->prc_defaults) ||
				(count > procedure->prc_inputs) ) :
			(count != procedure->prc_outputs))
	{
	/** They don't match...Hmmm...Its OK if we were dropping the procedure **/
		if (!(tdbb->tdbb_flags & TDBB_prc_being_dropped))
		{
			error(csb, Arg::Gds(input_flag ? isc_prcmismat : isc_prc_out_param_mismatch) <<
							Arg::Str(procedure->prc_name));
		}
		else
			mismatch = true;
	}

	if (count || input_flag && procedure->prc_defaults)
	{
	/** We have a few parameters. Get on with creating the message block **/
		USHORT n = ++csb->csb_msg_number;
		if (n < 2)
			csb->csb_msg_number = n = 2;
		CompilerScratch::csb_repeat* tail = CMP_csb_element(csb, n);
		jrd_nod* message = PAR_make_node(tdbb, e_msg_length);
		tail->csb_message = message;
		message->nod_type = nod_message;
		message->nod_count = count_table[blr_message];
		*message_ptr = message;
		message->nod_count = 0;
		message->nod_arg[e_msg_number] = (jrd_nod*)(IPTR) n;
		const Format* format = input_flag ? procedure->prc_input_fmt : procedure->prc_output_fmt;
		/* dimitr: procedure (with its parameter formats) is allocated out of
				   its own pool (prc_request->req_pool) and can be freed during
				   the cache cleanup (MET_clear_cache). Since the current
				   tdbb default pool is different from the procedure's one,
				   it's dangerous to copy a pointer from one request to another.
				   As an experiment, I've decided to copy format by value
				   instead of copying the reference. Since Format structure
				   doesn't contain any pointers, it should be safe to use a
				   default assignment operator which does a simple byte copy.
				   This change fixes one serious bug in the current codebase.
				   I think that this situation can (and probably should) be
				   handled by the metadata cache (via incrementing prc_use_count)
				   to avoid unexpected cache cleanups, but that area is out of my
				   knowledge. So this fix should be considered a temporary solution.

		message->nod_arg[e_msg_format] = (jrd_nod*) format;
		*/
		Format* fmt_copy = Format::newFormat(*tdbb->getDefaultPool(), format->fmt_count);
		*fmt_copy = *format;
		message->nod_arg[e_msg_format] = (jrd_nod*) fmt_copy;
		/* --- end of fix --- */
		if (!mismatch)
			n = format->fmt_count / 2;
		else {
			/*  There was a parameter mismatch hence can't depend upon the format's
			   fmt_count. Use count instead.
			 */
			n = count;
		}
		jrd_nod* list = *parameter_ptr = PAR_make_node(tdbb, n);
		list->nod_type = nod_list;
		list->nod_count = n;
		jrd_nod** ptr = list->nod_arg;
		USHORT asgn_arg1, asgn_arg2;
		if (input_flag) {
			asgn_arg1 = e_asgn_from;
			asgn_arg2 = e_asgn_to;
		}
		else {
			asgn_arg1 = e_asgn_to;
			asgn_arg2 = e_asgn_from;
		}
		for (USHORT i = 0; n; count--, n--)
		{
			jrd_nod* asgn = PAR_make_node(tdbb, e_asgn_length);
			*ptr++ = asgn;
			asgn->nod_type = nod_assignment;
			asgn->nod_count = count_table[blr_assignment];

			// default value for parameter
			if ((count <= 0) && input_flag) {
				Parameter* parameter = (*procedure->prc_input_fields)[procedure->prc_inputs - n];
				asgn->nod_arg[asgn_arg1] = CMP_clone_node(tdbb, csb, parameter->prm_default_value);
			}
			else {
				asgn->nod_arg[asgn_arg1] = PAR_parse_node(tdbb, csb, VALUE);
			}
			jrd_nod* prm = asgn->nod_arg[asgn_arg2] = PAR_make_node(tdbb, e_arg_length);
			prm->nod_type = nod_argument;
			prm->nod_count = 1;
			prm->nod_arg[e_arg_message] = message;
			prm->nod_arg[e_arg_number] = (jrd_nod*)(IPTR) i++;
			jrd_nod* prm_f = prm->nod_arg[e_arg_flag] = PAR_make_node(tdbb, e_arg_length);
			prm_f->nod_type = nod_argument;
			prm_f->nod_count = 0;
			prm_f->nod_arg[e_arg_message] = message;
			prm_f->nod_arg[e_arg_number] = (jrd_nod*)(IPTR) i++;
		}
	}
	else if ((input_flag ? procedure->prc_inputs : procedure->prc_outputs) && !mismatch)
	{
		error(csb, Arg::Gds(input_flag ? isc_prcmismat : isc_prc_out_param_mismatch) <<
						Arg::Str(procedure->prc_name));
	}
}


static jrd_nod* par_relation(thread_db* tdbb,
						CompilerScratch* csb, SSHORT blr_operator, bool parse_context)
{
/**************************************
 *
 *	p a r _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a relation reference.
 *
 **************************************/
	Firebird::MetaName name;

	SET_TDBB(tdbb);

/* Make a relation reference node */

	jrd_nod* node = PAR_make_node(tdbb, e_rel_length);
	node->nod_count = 0;

/* Find relation either by id or by name */
	jrd_rel* relation = 0;
	Firebird::string* alias_string = 0;
	switch (blr_operator)
	{
	case blr_rid:
	case blr_rid2:
		{
			const SSHORT id = csb->csb_blr_reader.getWord();
			if (blr_operator == blr_rid2) {
				alias_string = FB_NEW(csb->csb_pool) Firebird::string(csb->csb_pool);
				par_name(csb, *alias_string);
			}
			if (!(relation = MET_lookup_relation_id(tdbb, id, false))) {
				name.printf("id %d", id);
				error(csb, Arg::Gds(isc_relnotdef) << Arg::Str(name));
			}
		}
		break;
	case blr_relation:
	case blr_relation2:
		par_name(csb, name);
		if (blr_operator == blr_relation2) {
			alias_string = FB_NEW(csb->csb_pool) Firebird::string(csb->csb_pool);
			par_name(csb, *alias_string);
		}
		if (!(relation = MET_lookup_relation(tdbb, name)))
			error(csb, Arg::Gds(isc_relnotdef) << Arg::Str(name));
		break;
	}

/* if an alias was passed, store with the relation */

	if (alias_string)
	{
		node->nod_arg[e_rel_alias] =
			(jrd_nod*) stringDup(*tdbb->getDefaultPool(), alias_string->c_str());
	}

/* Scan the relation if it hasn't already been scanned for meta data */

	if ((!(relation->rel_flags & REL_scanned) || (relation->rel_flags & REL_being_scanned)) &&
		((relation->rel_flags & REL_force_scan) || !(csb->csb_g_flags & csb_internal)))
	{
		relation->rel_flags &= ~REL_force_scan;
		MET_scan_relation(tdbb, relation);
	}
	else if (relation->rel_flags & REL_sys_triggers)
	{
		MET_parse_sys_trigger(tdbb, relation);
	}

/* generate a stream for the relation reference,
   assuming it is a real reference */

	if (parse_context) {
		SSHORT context;
		const SSHORT stream = par_context(csb, &context);
		fb_assert(stream <= MAX_STREAMS);
		node->nod_arg[e_rel_stream] = (jrd_nod*) (IPTR) stream;
		node->nod_arg[e_rel_context] = (jrd_nod*) (IPTR) context;

		csb->csb_rpt[stream].csb_relation = relation;
		csb->csb_rpt[stream].csb_alias = alias_string;

		if (csb->csb_g_flags & csb_get_dependencies)
			par_dependency(tdbb, csb, stream, (SSHORT) -1, "");
	}
	else
	{
		delete alias_string;
	}

	node->nod_arg[e_rel_relation] = (jrd_nod*) relation;

	return node;
}


static jrd_nod* par_rse(thread_db* tdbb, CompilerScratch* csb, SSHORT rse_op)
{
/**************************************
 *
 *	p a r _ r s e
 *
 **************************************
 *
 * Functional description
 *	Parse a record selection expression.
 *
 **************************************/
	SET_TDBB(tdbb);

	SSHORT count = (unsigned int) csb->csb_blr_reader.getByte();
	RecordSelExpr* rse = (RecordSelExpr*) PAR_make_node(tdbb, count + rse_delta + 2);
	rse->nod_count = 0;
	rse->rse_count = count;
	jrd_nod** ptr = rse->rse_relation;

	while (--count >= 0) {
		// AB: Added TYPE_RSE for derived table support
		*ptr++ = PAR_parse_node(tdbb, csb, RELATION); // TYPE_RSE);
		//*ptr++ = PAR_parse_node(tdbb, csb, RELATION);
	}

	while (true)
	{
		const UCHAR op = csb->csb_blr_reader.getByte();
		switch (op)
		{
		case blr_boolean:
			rse->rse_boolean = PAR_parse_node(tdbb, csb, TYPE_BOOL);
			break;

		case blr_first:
			if (rse_op == blr_rs_stream)
				PAR_syntax_error(csb, "RecordSelExpr stream clause");
			rse->rse_first = PAR_parse_node(tdbb, csb, VALUE);
			break;

        case blr_skip:
            if (rse_op == blr_rs_stream)
                PAR_syntax_error(csb, "RecordSelExpr stream clause");
            rse->rse_skip = PAR_parse_node(tdbb, csb, VALUE);
            break;

		case blr_sort:
			if (rse_op == blr_rs_stream)
				PAR_syntax_error(csb, "RecordSelExpr stream clause");
			rse->rse_sorted = par_sort(tdbb, csb, true);
			break;

		case blr_project:
			if (rse_op == blr_rs_stream)
				PAR_syntax_error(csb, "RecordSelExpr stream clause");
			rse->rse_projection = par_sort(tdbb, csb, false);
			break;

		case blr_join_type:
			{
				const USHORT jointype = (USHORT) csb->csb_blr_reader.getByte();
				rse->rse_jointype = jointype;
				if (jointype != blr_inner && jointype != blr_left && jointype != blr_right &&
					jointype != blr_full)
				{
					PAR_syntax_error(csb, "join type clause");
				}
				break;
			}

		case blr_plan:
			rse->rse_plan = par_plan(tdbb, csb);
			break;

		case blr_writelock:
			rse->rse_writelock = true;
			break;

#ifdef SCROLLABLE_CURSORS
			/* if a receive is seen here, then it is intended to be an asynchronous
			   receive which can happen at any time during the scope of the RecordSelExpr--
			   this is intended to be a more efficient mechanism for scrolling through
			   a record stream, to prevent having to send a message to the engine
			   for each record */

		case blr_receive:
			csb->csb_blr_reader.seekBackward(1);
			rse->rse_async_message = PAR_parse_node(tdbb, csb, STATEMENT);
			break;
#endif

		default:
			if (op == (UCHAR) blr_end)
			{
				/* An outer join is only allowed when the stream count is 2
				   and a boolean expression has been supplied */

				if (!rse->rse_jointype || (rse->rse_count == 2 && rse->rse_boolean))
				{
					// Convert right outer joins to left joins to avoid
					// RIGHT JOIN handling at lower engine levels
					if (rse->rse_jointype == blr_right) {
						// Swap sub-streams
						jrd_nod* temp = rse->rse_relation[0];
						rse->rse_relation[0] = rse->rse_relation[1];
						rse->rse_relation[1] = temp;

						rse->rse_jointype = blr_left;
					}
					return (jrd_nod*) rse;
				}
			}
			PAR_syntax_error(csb, (TEXT*)((rse_op == blr_rs_stream) ?
						 "RecordSelExpr stream clause" :
						 "record selection expression clause"));
		}
	}
}


static jrd_nod* par_sort(thread_db* tdbb, CompilerScratch* csb, bool flag)
{
/**************************************
 *
 *	p a r _ s o r t
 *
 **************************************
 *
 * Functional description
 *	Parse a sort clause (sans header byte).  This is used for
 *	BLR_SORT, BLR_PROJECT, and BLR_GROUP.
 *
 **************************************/
	SET_TDBB(tdbb);

	SSHORT count = (unsigned int) csb->csb_blr_reader.getByte();
	jrd_nod* clause = PAR_make_node(tdbb, count * 3);
	if (!flag)
		clause->nod_flags = nod_unique_sort;
	clause->nod_type = nod_sort;
	clause->nod_count = count;
	jrd_nod** ptr = clause->nod_arg;
	jrd_nod** ptr2 = ptr + count;
	jrd_nod** ptr3 = ptr2 + count;

	while (--count >= 0)
	{
		if (flag) {
			UCHAR code = csb->csb_blr_reader.getByte();
			switch (code)
			{
			case blr_nullsfirst:
				*ptr3++ = (jrd_nod*) (IPTR) rse_nulls_first;
				code = csb->csb_blr_reader.getByte();
				break;
			case blr_nullslast:
				*ptr3++ = (jrd_nod*) (IPTR) rse_nulls_last;
				code = csb->csb_blr_reader.getByte();
				break;
			default:
				*ptr3++ = (jrd_nod*) (IPTR) rse_nulls_default;
			}

			*ptr2++ = (jrd_nod*) (IPTR) ((code == blr_descending) ? TRUE : FALSE);
		}
		*ptr++ = PAR_parse_node(tdbb, csb, VALUE);
	}

	return clause;
}


#ifdef NOT_USED_OR_REPLACED
static jrd_nod* par_stream(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ s t r e a m
 *
 **************************************
 *
 * Functional description
 *	Parse a stream expression.
 *
 **************************************/
	SET_TDBB(tdbb);

	RecordSelExpr* rse = (RecordSelExpr*) PAR_make_node(tdbb, 1 + rse_delta + 2);
	rse->nod_count = 0;
	rse->rse_count = 1;
	rse->rse_relation[0] = PAR_parse_node(tdbb, csb, RELATION);

	while (true) {
		const UCHAR op = csb->csb_blr_reader.getByte();
		switch (op)
		{
		case blr_boolean:
			rse->rse_boolean = PAR_parse_node(tdbb, csb, TYPE_BOOL);
			break;

		default:
			if (op == (UCHAR) blr_end)
				return (jrd_nod*) rse;
			PAR_syntax_error(csb, "stream_clause");
		}
	}
}
#endif


static jrd_nod* par_sys_function(thread_db* tdbb, CompilerScratch* csb)
{
/**************************************
 *
 *	p a r _ s y s _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a system function reference.
 *
 **************************************/
	SET_TDBB(tdbb);

	Firebird::MetaName name;
	const USHORT count = par_name(csb, name);

	const SysFunction* function = SysFunction::lookup(name);

	if (!function)
	{
		csb->csb_blr_reader.seekBackward(count);
		error(csb, Arg::Gds(isc_funnotdef) << Arg::Str(name));
	}

	jrd_nod* node = PAR_make_node(tdbb, e_sysfun_length);
	node->nod_count = count_table[blr_sys_function];
	node->nod_arg[e_sysfun_args] = par_args(tdbb, csb, VALUE);
	node->nod_arg[e_sysfun_function] = (jrd_nod*) function;

	return node;
}


static jrd_nod* par_union(thread_db* tdbb, CompilerScratch* csb, bool recursive)
{
/**************************************
 *
 *	p a r _ u n i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a union reference.
 *
 **************************************/
	SET_TDBB(tdbb);

/* Make the node, parse the context number, get a stream assigned,
   and get the number of sub-RecordSelExpr's. */

	jrd_nod* node = PAR_make_node(tdbb, e_uni_length);
	node->nod_count = 3;
	const USHORT stream = par_context(csb, 0);
	node->nod_arg[e_uni_stream] = (jrd_nod*) (IPTR) stream;

	// assign separate context for mapped record if union is recursive
	USHORT map_stream = stream;
	if (recursive)
	{
		node->nod_flags |= nod_recurse;
		map_stream = par_context(csb, 0);
		node->nod_arg[e_uni_map_stream] = (jrd_nod*) (IPTR) map_stream;
	}

	SSHORT count = (unsigned int) csb->csb_blr_reader.getByte();

/* Pick up the sub-RecordSelExpr's and maps */

	NodeStack clauses;

	while (--count >= 0) {
		clauses.push(PAR_parse_node(tdbb, csb, TYPE_RSE));
		clauses.push(par_map(tdbb, csb, map_stream));
	}

	node->nod_arg[e_uni_clauses] = PAR_make_list(tdbb, clauses);

	return node;
}


jrd_nod* PAR_parse_node(thread_db* tdbb, CompilerScratch* csb, USHORT expected)
{
/**************************************
 *
 *	P A R _ p a r s e _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Parse a BLR expression.
 *
 **************************************/
	SET_TDBB(tdbb);

	const USHORT blr_offset = csb->csb_blr_reader.getOffset();
	const SSHORT blr_operator = csb->csb_blr_reader.getByte();

	if (blr_operator < 0 || blr_operator >= FB_NELEM(type_table)) {
        // NS: This error string is correct, please do not mangle it again and again.
		// The whole error message is "BLR syntax error: expected %s at offset %d, encountered %d"
        PAR_syntax_error(csb, "valid BLR code");
    }

	const SSHORT sub_type = sub_type_table[blr_operator];

/* If there is a length given in the length table, pre-allocate
   the node and set its count.  This saves an enormous amount of
   repetitive code. */

	jrd_nod* node;
	jrd_nod** arg;
	USHORT n = length_table[blr_operator];
	if (n) {
		node = PAR_make_node(tdbb, n);
		node->nod_count = count_table[blr_operator];
		arg = node->nod_arg;
	}
	else {
		node = NULL;
		arg = NULL;
	}

	bool set_type = true;

/* Dispatch on operator type. */

	switch (blr_operator)
	{
	case blr_any:
	case blr_unique:
	case blr_ansi_any:
	case blr_ansi_all:
	case blr_exists:
		node->nod_arg[e_any_rse] = PAR_parse_node(tdbb, csb, sub_type);
		break;

		/* Boring operators -- no special handling req'd */

	case blr_value_if:
	case blr_substring:
	case blr_matching2:
	case blr_ansi_like:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_trim:
	{
		node->nod_count = e_trim_count;
		node->nod_arg[e_trim_specification] = (jrd_nod*)(U_IPTR) csb->csb_blr_reader.getByte();

		BYTE trimWhat = csb->csb_blr_reader.getByte();

		if (trimWhat == blr_trim_characters)
			node->nod_arg[e_trim_characters] = PAR_parse_node(tdbb, csb, sub_type);
		else
		{
			node->nod_arg[e_trim_characters] = NULL;
			--node->nod_count;
		}

		node->nod_arg[e_trim_value] = PAR_parse_node(tdbb, csb, sub_type);
		break;
	}

	case blr_and:
	case blr_or:

	case blr_prot_mask:
	case blr_containing:
	case blr_matching:
	case blr_like:
	case blr_starting:
	case blr_add:
	case blr_subtract:
	case blr_multiply:
	case blr_divide:
	case blr_concatenate:

	case blr_assignment:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		/* Fall into ... */

	case blr_handler:
	case blr_loop:

	case blr_lock_state:
	case blr_upcase:
	case blr_lowcase:
	case blr_negate:
	case blr_not:
	case blr_missing:
	case blr_agg_count2:
	case blr_agg_max:
	case blr_agg_min:
	case blr_agg_total:
	case blr_agg_average:
	case blr_agg_count_distinct:
	case blr_agg_total_distinct:
	case blr_agg_average_distinct:
	case blr_post:
	case blr_internal_info:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_similar:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		if (csb->csb_blr_reader.getByte() != 0)
			*arg++ = PAR_parse_node(tdbb, csb, sub_type);	// escape
		else	// without escape
		{
			*arg++ = NULL;
			--node->nod_count;
		}
		break;

	case blr_agg_list:
	case blr_agg_list_distinct:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		*arg++ = PAR_parse_node(tdbb, csb, VALUE);
		break;

	case blr_exec_sql:
		{
			n = e_exec_stmt_fixed_count;

			node = PAR_make_node(tdbb, n + e_exec_stmt_extra_count);
			node->nod_count = n;
			node->nod_type = nod_exec_stmt;
			set_type = false;

			arg = node->nod_arg;
			*arg++ = PAR_parse_node(tdbb, csb, VALUE);		// e_exec_stmt_stmt_sql
			*arg++ = NULL;		// e_exec_stmt_data_src
			*arg++ = NULL;		// e_exec_stmt_user
			*arg++ = NULL;		// e_exec_stmt_password
			*arg++ = NULL;		// e_exec_stmt_role;
			*arg++ = NULL;		// e_exec_stmt_proc_block;

			*arg++ = NULL;		// e_exec_stmt_extra_inputs
			*arg++ = NULL;		// e_exec_stmt_extra_input_names
			*arg++ = NULL;		// e_exec_stmt_extra_outputs
			*arg++ = NULL;		// e_exec_stmt_extra_tran
			*arg++ = NULL;		// e_exec_stmt_extra_privs
		}
		break;

	case blr_exec_into:
		{
			const USHORT outputs = csb->csb_blr_reader.getWord();
			n = outputs + e_exec_stmt_fixed_count;

			node = PAR_make_node(tdbb, n + e_exec_stmt_extra_count);
			node->nod_count = n;
			node->nod_type = nod_exec_stmt;
			set_type = false;

			arg = node->nod_arg;
			*arg++ = PAR_parse_node(tdbb, csb, VALUE);		// e_exec_stmt_stmt_sql
			*arg++ = NULL;		// e_exec_stmt_data_src
			*arg++ = NULL;		// e_exec_stmt_user
			*arg++ = NULL;		// e_exec_stmt_password
			*arg++ = NULL;		// e_exec_stmt_role;

			if (csb->csb_blr_reader.getByte())	// singleton flag
				*arg++ = NULL;							// e_exec_stmt_proc_block
			else
				*arg++ = PAR_parse_node(tdbb, csb, STATEMENT);	// e_exec_stmt_proc_block

			// output parameters
			for (n = e_exec_stmt_fixed_count; n < node->nod_count; n++) {
				*arg++ = PAR_parse_node(tdbb, csb, VALUE);
			}

			*arg++ = NULL;		// e_exec_stmt_extra_inputs
			*arg++ = NULL;		// e_exec_stmt_extra_input_names
			*arg++ = (jrd_nod*)(IPTR) outputs;		// e_exec_stmt_extra_outputs
			*arg++ = NULL;		// e_exec_stmt_extra_tran
			*arg++ = NULL;		// e_exec_stmt_extra_privs
		}
		break;

	case blr_exec_stmt:
		{
			USHORT inputs = 0;
			USHORT outputs = 0;
			UCHAR tra_mode = 0;
			int use_caller_privs = 0;
			EDS::ParamNames* paramNames = NULL;

			while (true)
			{
				const UCHAR code = csb->csb_blr_reader.getByte();
				switch (code)
				{
				case blr_exec_stmt_inputs:
					inputs = csb->csb_blr_reader.getWord();
					break;

				case blr_exec_stmt_outputs:
					outputs = csb->csb_blr_reader.getWord();
					break;

				case blr_exec_stmt_sql:
					n = inputs + outputs + e_exec_stmt_fixed_count;

					node = PAR_make_node(tdbb, n + e_exec_stmt_extra_count);
					node->nod_count = n;
					node->nod_arg[e_exec_stmt_stmt_sql] = PAR_parse_node(tdbb, csb, VALUE);
					break;

				case blr_exec_stmt_proc_block:
					node->nod_arg[e_exec_stmt_proc_block] = PAR_parse_node(tdbb, csb, STATEMENT);
					break;

				case blr_exec_stmt_data_src:
					node->nod_arg[e_exec_stmt_data_src] = PAR_parse_node(tdbb, csb, VALUE);
					break;

				case blr_exec_stmt_user:
					node->nod_arg[e_exec_stmt_user] = PAR_parse_node(tdbb, csb, VALUE);
					break;

				case blr_exec_stmt_pwd:
					node->nod_arg[e_exec_stmt_password] = PAR_parse_node(tdbb, csb, VALUE);
					break;

				case blr_exec_stmt_role:
					node->nod_arg[e_exec_stmt_role] = PAR_parse_node(tdbb, csb, VALUE);
					break;

				case blr_exec_stmt_tran:
					PAR_syntax_error(csb, "external transaction parameters");
					break;

				case blr_exec_stmt_tran_clone:
					tra_mode = csb->csb_blr_reader.getByte();
					break;

				case blr_exec_stmt_privs:
					use_caller_privs = 1;
					break;

				case blr_exec_stmt_in_params:
				case blr_exec_stmt_in_params2:
					// input parameters and their names
					n = e_exec_stmt_fixed_count;
					arg = node->nod_arg + n;

					for (; n < e_exec_stmt_fixed_count + inputs; n++)
					{
						if (code == blr_exec_stmt_in_params2)
						{
							Firebird::string name;
							if (par_name(csb, name))
							{
								Firebird::MemoryPool& pool = csb->csb_pool;
								if (!paramNames) {
									paramNames = FB_NEW (pool) EDS::ParamNames(pool);
								}
								Firebird::string* newName = FB_NEW (pool) Firebird::string(pool, name);
								paramNames->add(newName);
							}
						}
						*arg++ = PAR_parse_node(tdbb, csb, VALUE);
					}
					break;

				case blr_exec_stmt_out_params:
					// output parameters
					n = e_exec_stmt_fixed_count + inputs;
					arg = node->nod_arg + n;
					for (; n < node->nod_count; n++) {
						*arg++ = PAR_parse_node(tdbb, csb, VALUE);
					}
					break;

				case blr_end:
					break;

				default:
					PAR_syntax_error(csb, "unknown EXECUTE STATEMENT option");
				}

				if (code == blr_end)
					break;
			}

			arg = node->nod_arg + node->nod_count;
			*arg++ = (jrd_nod*)(IPTR) inputs;		// e_exec_stmt_extra_inputs
			*arg++ = (jrd_nod*) paramNames;			// e_exec_stmt_extra_input_names
			*arg++ = (jrd_nod*)(IPTR) outputs;		// e_exec_stmt_extra_outputs
			*arg++ = (jrd_nod*)(IPTR) tra_mode;		// e_exec_stmt_extra_tran
			*arg++ = (jrd_nod*)(IPTR) use_caller_privs;		// e_exec_stmt_extra_privs
		}
		break;

	case blr_post_arg:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_null:
	case blr_agg_count:
	case blr_user_name:
    case blr_current_role:
	case blr_current_date:
	case blr_start_savepoint:
	case blr_end_savepoint:
		break;

	case blr_current_time:
		node->nod_arg[0] = (jrd_nod*) (IPTR) DEFAULT_TIME_PRECISION;
		break;

	case blr_current_timestamp:
		node->nod_arg[0] = (jrd_nod*) (IPTR) DEFAULT_TIMESTAMP_PRECISION;
		break;

	case blr_current_time2:
	case blr_current_timestamp2:
		n = csb->csb_blr_reader.getByte();
		if (n > MAX_TIME_PRECISION) {
			ERR_post(Arg::Gds(isc_invalid_time_precision) << Arg::Num(MAX_TIME_PRECISION));
		}
		node->nod_arg[0] = (jrd_nod*) (IPTR) n;
		break;

	case blr_user_savepoint:
		{
			*arg++ = (jrd_nod*) (IPTR) csb->csb_blr_reader.getByte();
			Firebird::MetaName name;
			par_name(csb, name);
			*arg++ = (jrd_nod*) stringDup(*tdbb->getDefaultPool(), name.c_str());
			break;
		}

	case blr_store:
	case blr_store2:
		node->nod_arg[e_sto_relation] = PAR_parse_node(tdbb, csb, RELATION);
		node->nod_arg[e_sto_statement] = PAR_parse_node(tdbb, csb, sub_type);
		if (blr_operator == blr_store2)
			node->nod_arg[e_sto_statement2] = PAR_parse_node(tdbb, csb, sub_type);
		break;

		/* Comparison operators */

	case blr_between:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);

	case blr_equiv:
	case blr_eql:
	case blr_neq:
	case blr_geq:
	case blr_gtr:
	case blr_leq:
	case blr_lss:
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		*arg++ = PAR_parse_node(tdbb, csb, sub_type);
		node->nod_flags = nod_comparison;
		break;

	case blr_erase:
		n = csb->csb_blr_reader.getByte();
		if (n >= csb->csb_rpt.getCount() || !(csb->csb_rpt[n].csb_flags & csb_used))
			error(csb, Arg::Gds(isc_ctxnotdef));
		node->nod_arg[e_erase_stream] = (jrd_nod*) (IPTR) csb->csb_rpt[n].csb_stream;
		break;

	case blr_modify:
	case blr_modify2:
		node = par_modify(tdbb, csb, blr_operator);
		break;

	case blr_exec_proc:
	case blr_exec_pid:
		node = par_exec_proc(tdbb, csb, blr_operator);
		break;

	case blr_pid:
	case blr_procedure:
		node = par_procedure(tdbb, csb, blr_operator);
		break;

	case blr_function:
		node = par_function(tdbb, csb);
		break;

	case blr_index:
		node->nod_arg[0] = PAR_parse_node(tdbb, csb, sub_type);
		node->nod_arg[1] = par_args(tdbb, csb, sub_type);
		break;

	case blr_for:
		if (csb->csb_blr_reader.peekByte() == (UCHAR) blr_stall)
			node->nod_arg[e_for_stall] = PAR_parse_node(tdbb, csb, STATEMENT);

		if (csb->csb_blr_reader.peekByte() == (UCHAR) blr_rse ||
			csb->csb_blr_reader.peekByte() == (UCHAR) blr_singular)
		{
			node->nod_arg[e_for_re] = PAR_parse_node(tdbb, csb, TYPE_RSE);
		}
		else
			node->nod_arg[e_for_re] = par_rse(tdbb, csb, blr_operator);
		node->nod_arg[e_for_statement] = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_dcl_cursor:
		{
			node->nod_arg[e_dcl_cursor_number] = (jrd_nod*) (IPTR) csb->csb_blr_reader.getWord();
			node->nod_arg[e_dcl_cursor_rse] = PAR_parse_node(tdbb, csb, TYPE_RSE);
			n = csb->csb_blr_reader.getWord();
			jrd_nod* temp = PAR_make_node(tdbb, n);
			temp->nod_type = nod_list;
			for (jrd_nod** ptr = temp->nod_arg; n; n--) {
				*ptr++ = PAR_parse_node(tdbb, csb, VALUE);
			}
			node->nod_arg[e_dcl_cursor_refs] = temp;
		}
		break;

	case blr_cursor_stmt:
		n = csb->csb_blr_reader.getByte();
		node->nod_arg[e_cursor_stmt_op] = (jrd_nod*) (IPTR) n;
		node->nod_arg[e_cursor_stmt_number] = (jrd_nod*) (IPTR) csb->csb_blr_reader.getWord();
		switch (n)
		{
		case blr_cursor_open:
		case blr_cursor_close:
			break;
		case blr_cursor_fetch:
#ifdef SCROLLABLE_CURSORS
			if (csb->csb_blr_reader.peekByte() == blr_seek)
				node->nod_arg[e_cursor_stmt_seek] = PAR_parse_node(tdbb, csb, STATEMENT);
#endif
			csb->csb_g_flags |= csb_reuse_context;
			node->nod_arg[e_cursor_stmt_into] = PAR_parse_node(tdbb, csb, STATEMENT);
			csb->csb_g_flags &= ~csb_reuse_context;
			break;
		default:
			PAR_syntax_error(csb, "cursor operation clause");
		}
		break;

	case blr_rse:
	case blr_rs_stream:
		node = par_rse(tdbb, csb, blr_operator);
		break;

	case blr_singular:
		node = PAR_parse_node(tdbb, csb, TYPE_RSE);
		((RecordSelExpr*) node)->nod_flags |= rse_singular;
		break;

	case blr_relation:
	case blr_rid:
	case blr_relation2:
	case blr_rid2:
		node = par_relation(tdbb, csb, blr_operator, true);
		break;

	case blr_union:
		node = par_union(tdbb, csb, false);
		break;

	case blr_recurse:
		node = par_union(tdbb, csb, true);
		break;

	case blr_aggregate:
		node->nod_arg[e_agg_stream] = (jrd_nod*) (IPTR) par_context(csb, 0);
		fb_assert((int) (IPTR)node->nod_arg[e_agg_stream] <= MAX_STREAMS);
		node->nod_arg[e_agg_rse] = PAR_parse_node(tdbb, csb, TYPE_RSE);
		node->nod_arg[e_agg_group] = PAR_parse_node(tdbb, csb, OTHER);
		node->nod_arg[e_agg_map] = par_map(tdbb, csb, (USHORT)(IPTR) node->nod_arg[e_agg_stream]);
		break;

	case blr_group_by:
		node = par_sort(tdbb, csb, false);
		return node->nod_count ? node : NULL;

	case blr_field:
	case blr_fid:
		node = par_field(tdbb, csb, blr_operator);
		if (node->nod_type == nod_domain_validation || node->nod_type == nod_null)
			set_type = false;	// to not change nod->nod_type to nod_field
		break;

	case blr_derived_expr:
	{
		const UCHAR streamCount = csb->csb_blr_reader.getByte();
		USHORT* streamList = FB_NEW(*tdbb->getDefaultPool()) USHORT[streamCount];
		for (UCHAR i = 0; i < streamCount; ++i)
		{
			streamList[i] = csb->csb_blr_reader.getByte();
			streamList[i] = csb->csb_rpt[streamList[i]].csb_stream;
		}

		node->nod_arg[e_derived_expr_stream_list] = (jrd_nod*) streamList;
		node->nod_arg[e_derived_expr_stream_count] = (jrd_nod*)(IPTR) streamCount;
		node->nod_arg[e_derived_expr_expr] = PAR_parse_node(tdbb, csb, sub_type);
		node->nod_count = e_derived_expr_count;
		break;
	}

	case blr_gen_id:
	case blr_set_generator:
		{
			Firebird::MetaName name;

			par_name(csb, name);
			const SLONG tmp = MET_lookup_generator(tdbb, name);
			if (tmp < 0) {
				error(csb, Arg::Gds(isc_gennotdef) << Arg::Str(name));
			}
			node->nod_arg[e_gen_id] = (jrd_nod*) (IPTR) tmp;
			node->nod_arg[e_gen_value] = PAR_parse_node(tdbb, csb, VALUE);

            /* CVC: There're thousand ways to go wrong, but I don't see any value
               in posting dependencies with set generator since it's DDL, so I will
               track only gen_id() in both dialects. */
            if ((blr_operator == blr_gen_id) && (csb->csb_g_flags & csb_get_dependencies))
			{
                jrd_nod* dep_node = PAR_make_node (tdbb, e_dep_length);
                dep_node->nod_type = nod_dependency;
                dep_node->nod_arg [e_dep_object] = (jrd_nod*) (IPTR) tmp;
                dep_node->nod_arg [e_dep_object_type] = (jrd_nod*)(IPTR) obj_generator;
                csb->csb_dependencies.push(dep_node);
            }

		}
		break;

	case blr_record_version:
	case blr_dbkey:
		n = csb->csb_blr_reader.getByte();
		if (n >= csb->csb_rpt.getCount() || !(csb->csb_rpt[n].csb_flags & csb_used))
			error(csb, Arg::Gds(isc_ctxnotdef));
		node->nod_arg[0] = (jrd_nod*) (IPTR) csb->csb_rpt[n].csb_stream;
		break;

	case blr_fetch:
		par_fetch(tdbb, csb, node);
		break;

	case blr_send:
	case blr_receive:
		n = csb->csb_blr_reader.getByte();
		node->nod_arg[e_send_message] = csb->csb_rpt[n].csb_message;
		node->nod_arg[e_send_statement] = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_message:
		node = par_message(tdbb, csb);
		break;

	case blr_literal:
		node = par_literal(tdbb, csb);
		break;

	case blr_cast:
		node = par_cast(tdbb, csb);
		break;

	case blr_extract:
	    // This forced conversion looks strange, but extract_part fits in a byte
		node->nod_arg[e_extract_part] = (jrd_nod*)(U_IPTR) csb->csb_blr_reader.getByte();
		node->nod_arg[e_extract_value] = PAR_parse_node(tdbb, csb, sub_type);
		node->nod_count = e_extract_count;
		break;

	case blr_strlen:
	    // This forced conversion looks strange, but length_type fits in a byte
		node->nod_arg[e_strlen_type] = (jrd_nod*)(U_IPTR) csb->csb_blr_reader.getByte();
		node->nod_arg[e_strlen_value] = PAR_parse_node(tdbb, csb, sub_type);
		node->nod_count = e_strlen_count;
		break;

	case blr_dcl_variable:
		{
			dsc* desc = (dsc*) (node->nod_arg + e_dcl_desc);

			ItemInfo itemInfo;

			n = csb->csb_blr_reader.getWord();
			node->nod_arg[e_dcl_id] = (jrd_nod*) (IPTR) n;
			PAR_desc(tdbb, csb, desc, &itemInfo);
			csb->csb_variables =
				vec<jrd_nod*>::newVector(*tdbb->getDefaultPool(), csb->csb_variables, n + 1);

			if (itemInfo.isSpecial())
			{
				csb->csb_dbg_info.varIndexToName.get(n, itemInfo.name);
				csb->csb_map_item_info.put(Item(nod_variable, n), itemInfo);
			}

			if (itemInfo.explicitCollation)
			{
				jrd_nod* dep_node = PAR_make_node (tdbb, e_dep_length);
				dep_node->nod_type = nod_dependency;
				dep_node->nod_arg [e_dep_object] = (jrd_nod*)(IPTR) INTL_TEXT_TYPE(*desc);
				dep_node->nod_arg [e_dep_object_type] = (jrd_nod*)(IPTR) obj_collation;
				csb->csb_dependencies.push(dep_node);
			}
		}
		break;

	case blr_variable:
		{
			n = csb->csb_blr_reader.getWord();
			node->nod_arg[e_var_id] = (jrd_nod*)(IPTR) n;
			vec<jrd_nod*>* vector = csb->csb_variables;
			if (!vector || n >= vector->count())
				PAR_syntax_error(csb, "variable identifier");
		}
		break;

	case blr_parameter:
	case blr_parameter2:
	case blr_parameter3:
		{
			jrd_nod* message = NULL;
			n = (USHORT) csb->csb_blr_reader.getByte();
			if (n >= csb->csb_rpt.getCount() || !(message = csb->csb_rpt[n].csb_message))
			{
				error(csb, Arg::Gds(isc_badmsgnum));
			}
			node->nod_arg[e_arg_message] = message;
			n = csb->csb_blr_reader.getWord();
			node->nod_arg[e_arg_number] = (jrd_nod*) (IPTR) n;
			const Format* format = (Format*) message->nod_arg[e_msg_format];
			if (n >= format->fmt_count)
				error(csb, Arg::Gds(isc_badparnum));
			if (blr_operator != blr_parameter) {
				jrd_nod* temp = PAR_make_node(tdbb, e_arg_length);
				node->nod_arg[e_arg_flag] = temp;
				node->nod_count = 1;
				temp->nod_count = 0;
				temp->nod_type = nod_argument;
				temp->nod_arg[e_arg_message] = message;
				n = csb->csb_blr_reader.getWord();
				temp->nod_arg[e_arg_number] = (jrd_nod*) (IPTR) n;
				if (n >= format->fmt_count)
					error(csb, Arg::Gds(isc_badparnum));
			}
			if (blr_operator == blr_parameter3) {
				jrd_nod* temp = PAR_make_node(tdbb, e_arg_length);
				node->nod_arg[e_arg_indicator] = temp;
				node->nod_count = 2;
				temp->nod_count = 0;
				temp->nod_type = nod_argument;
				temp->nod_arg[e_arg_message] = message;
				n = csb->csb_blr_reader.getWord();
				temp->nod_arg[e_arg_number] = (jrd_nod*) (IPTR) n;
				if (n >= format->fmt_count)
					error(csb, Arg::Gds(isc_badparnum));
			}
		}
		break;

	case blr_stall:
		break;

	case blr_select:
	case blr_begin:
		{
			NodeStack stack;

			while (csb->csb_blr_reader.peekByte() != (UCHAR) blr_end) {
				if (blr_operator == blr_select && csb->csb_blr_reader.peekByte() != blr_receive)
					PAR_syntax_error(csb, "blr_receive");
				stack.push(PAR_parse_node(tdbb, csb, sub_type));
			}
			csb->csb_blr_reader.getByte(); // skip blr_end
			node = PAR_make_list(tdbb, stack);
		}
		break;

	case blr_block:
		{
			NodeStack stack;

			node->nod_arg[e_blk_action] = PAR_parse_node(tdbb, csb, sub_type);
			while (csb->csb_blr_reader.peekByte() != (UCHAR) blr_end)
			{
				stack.push(PAR_parse_node(tdbb, csb, sub_type));
			}
			csb->csb_blr_reader.getByte(); // skip blr_end
			node->nod_arg[e_blk_handlers] = PAR_make_list(tdbb, stack);
		}
		break;

	case blr_error_handler:
		node->nod_arg[e_err_conditions] = (jrd_nod*) par_conditions(tdbb, csb);
		node->nod_arg[e_err_action] = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_abort:
		{
			const bool flag = (csb->csb_blr_reader.peekByte() == blr_exception_msg);
			node->nod_arg[e_xcp_desc] = (jrd_nod*) par_condition(tdbb, csb);
			if (flag)
			{
				node->nod_arg[e_xcp_msg] = PAR_parse_node(tdbb, csb, sub_type);
			}
			break;
		}

	case blr_if:
		node->nod_arg[e_if_boolean] = PAR_parse_node(tdbb, csb, TYPE_BOOL);
		node->nod_arg[e_if_true] = PAR_parse_node(tdbb, csb, sub_type);
		if (csb->csb_blr_reader.peekByte() == (UCHAR) blr_end) {
			node->nod_count = 2;
			csb->csb_blr_reader.getByte(); // skip blr_end
			break;
		}
		node->nod_arg[e_if_false] = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_label:
		node->nod_arg[e_lbl_label] = (jrd_nod*) (IPTR) csb->csb_blr_reader.getByte();
		node->nod_arg[e_lbl_statement] = PAR_parse_node(tdbb, csb, sub_type);
		break;

	case blr_leave:
		node->nod_arg[0] = (jrd_nod*) (IPTR) csb->csb_blr_reader.getByte();
		break;


	case blr_maximum:
	case blr_minimum:
	case blr_count:
/* count2
    case blr_count2:
*/
	case blr_average:
	case blr_total:
	case blr_from:
	case blr_via:
		node->nod_arg[e_stat_rse] = PAR_parse_node(tdbb, csb, TYPE_RSE);
		if (blr_operator != blr_count)
			node->nod_arg[e_stat_value] = PAR_parse_node(tdbb, csb, VALUE);
		if (blr_operator == blr_via)
			node->nod_arg[e_stat_default] = PAR_parse_node(tdbb, csb, VALUE);
		break;

#ifdef SCROLLABLE_CURSORS
	case blr_seek:
		node->nod_arg[e_seek_direction] = PAR_parse_node(tdbb, csb, VALUE);
		node->nod_arg[e_seek_offset] = PAR_parse_node(tdbb, csb, VALUE);
		break;
#endif

	case blr_init_variable:
		{
			n = csb->csb_blr_reader.getWord();
			node->nod_arg[e_init_var_id] = (jrd_nod*)(U_IPTR) n;
			vec<jrd_nod*>* vector = csb->csb_variables;
			if (!vector || n >= vector->count())
				PAR_syntax_error(csb, "variable identifier");
		}
		break;

	case blr_sys_function:
		node = par_sys_function(tdbb, csb);
		break;

	case blr_stmt_expr:
		node->nod_arg[e_stmt_expr_stmt] = PAR_parse_node(tdbb, csb, STATEMENT);
		node->nod_arg[e_stmt_expr_expr] = PAR_parse_node(tdbb, csb, VALUE);
		break;

	default:
		if ((nod_t)(USHORT) blr_table[(int) blr_operator] == nod_class_node_jrd)
		{
			fb_assert(blr_parsers[blr_operator]);

			node->nod_arg[0] = reinterpret_cast<jrd_nod*>(
				blr_parsers[blr_operator](tdbb, *tdbb->getDefaultPool(), csb));
		}
		else
			PAR_syntax_error(csb, elements[expected]);
	}

	if (set_type)
	{
		if (csb->csb_g_flags & csb_blr_version4)
			node->nod_type = (nod_t)(USHORT) blr_table4[(int) blr_operator];
		else
			node->nod_type = (nod_t)(USHORT) blr_table[(int) blr_operator];
	}

	size_t pos = 0;
	if (csb->csb_dbg_info.blrToSrc.find(blr_offset, pos))
	{
		Firebird::MapBlrToSrcItem& i = csb->csb_dbg_info.blrToSrc[pos];
		jrd_nod* node_src = PAR_make_node(tdbb, e_src_info_length);

		node_src->nod_type = nod_src_info;
		node_src->nod_arg[e_src_info_line] = (jrd_nod*) (IPTR) i.mbs_src_line;
		node_src->nod_arg[e_src_info_col] = (jrd_nod*) (IPTR) i.mbs_src_col;
		node_src->nod_arg[e_src_info_node] = node;

		return node_src;
	}

	return node;
}


void PAR_syntax_error(CompilerScratch* csb, const TEXT* string)
{
/**************************************
 *
 *	P A R _ s y n t a x _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Post a syntax error message.
 *
 **************************************/

	csb->csb_blr_reader.seekBackward(1);

	error(csb, Arg::Gds(isc_syntaxerr) << Arg::Str(string) <<
										  Arg::Num(csb->csb_blr_reader.getOffset()) <<
										  Arg::Num(csb->csb_blr_reader.peekByte()));
}


static void warning(const Arg::StatusVector& v)
{
/**************************************
 *
 *	w a r n i n g
 *
 **************************************
 *
 * Functional description
 *      This is for GBAK so that we can pass warning messages
 *      back to the client.  DO NOT USE this function until we
 *      fully implement warning at the engine level.
 *
 *	We will use the status vector like a warning vector.  What
 *	we are going to do is leave the [1] position of the vector
 *	as 0 so that this will not be treated as an error, and we
 *	will place our warning message in the consecutive positions.
 *	It will be up to the caller to check these positions for
 *	the message.
 *
 **************************************/
	fb_assert(v.value()[0] == isc_arg_warning);

	thread_db* tdbb = JRD_get_thread_data();

/* Make sure that the [1] position is 0
   indicating that no error has occurred */
	Arg::Gds p(FB_SUCCESS);

/* Now place your warning messages */
	p.append(v);

/* Save into tdbb */
	p.copyTo(tdbb->tdbb_status_vector);
	ERR_make_permanent(tdbb->tdbb_status_vector);
}
