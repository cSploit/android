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
#include "../jrd/JrdStatement.h"
#include "../jrd/Attachment.h"
#include "../jrd/intl_classes.h"
#include "../jrd/acl.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../jrd/val.h"
#include "../jrd/align.h"
#include "../dsql/Nodes.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/Function.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/Collation.h"

using namespace Firebird;
using namespace Jrd;


template <typename T> static void makeSubRoutines(thread_db* tdbb, JrdStatement* statement,
	CompilerScratch* csb, T& subs);


// Start to turn a parsed scratch into a statement. This is completed by makeStatement.
JrdStatement::JrdStatement(thread_db* tdbb, MemoryPool* p, CompilerScratch* csb)
	: pool(p),
	  rpbsSetup(*p),
	  requests(*p),
	  externalList(*p),
	  accessList(*p),
	  resources(*p),
	  triggerName(*p),
	  parentStatement(NULL),
	  subStatements(*p),
	  fors(*p),
	  invariants(*p),
	  blr(*p),
	  mapFieldInfo(*p),
	  mapItemInfo(*p),
	  interface(NULL)
{
	try
	{
		makeSubRoutines(tdbb, this, csb, csb->subProcedures);
		makeSubRoutines(tdbb, this, csb, csb->subFunctions);

		topNode = (csb->csb_node->kind == DmlNode::KIND_STATEMENT) ?
			static_cast<StmtNode*>(csb->csb_node) : NULL;

		accessList = csb->csb_access;
		externalList = csb->csb_external;
		mapFieldInfo.takeOwnership(csb->csb_map_field_info);
		resources = csb->csb_resources; // Assign array contents
		impureSize = csb->csb_impure;

		//if (csb->csb_g_flags & csb_blr_version4)
		//	flags |= FLAG_VERSION4;
		blrVersion = csb->blrVersion;

		// Take out existence locks on resources used in statement. This is
		// a little complicated since relation locks MUST be taken before
		// index locks.

		for (Resource* resource = resources.begin(); resource != resources.end(); ++resource)
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
				case Resource::rsc_function:
				{
					Routine* routine = resource->rsc_routine;
					routine->addRef();

#ifdef DEBUG_PROCS
					string buffer;
					buffer.printf(
						"Called from JrdStatement::makeRequest:\n\t Incrementing use count of %s\n",
						routine->getName()->toString().c_str());
					JRD_print_procedure_info(tdbb, buffer.c_str());
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

		// make a vector of all used RSEs
		fors = csb->csb_fors;

		// make a vector of all invariant-type nodes, so that we will
		// be able to easily reinitialize them when we restart the request
		invariants.join(csb->csb_invariants);

		rpbsSetup.grow(csb->csb_n_stream);

		CompilerScratch::csb_repeat* tail = csb->csb_rpt.begin();
		const CompilerScratch::csb_repeat* const streams_end = tail + csb->csb_n_stream;

		for (record_param* rpb = rpbsSetup.begin(); tail < streams_end; ++rpb, ++tail)
		{
			// fetch input stream for update if all booleans matched against indices
			if ((tail->csb_flags & csb_update) && !(tail->csb_flags & csb_unmatched))
				 rpb->rpb_stream_flags |= RPB_s_update;

			// if no fields are referenced and this stream is not intended for update,
			// mark the stream as not requiring record's data
			if (!tail->csb_fields && !(tail->csb_flags & csb_update))
				 rpb->rpb_stream_flags |= RPB_s_no_data;

			rpb->rpb_relation = tail->csb_relation;

			delete tail->csb_fields;
			tail->csb_fields = NULL;
		}
	}
	catch (Exception&)
	{
		for (JrdStatement** subStatement = subStatements.begin();
			 subStatement != subStatements.end();
			 ++subStatement)
		{
			(*subStatement)->release(tdbb);
		}

		throw;
	}
}

// Turn a parsed scratch into a statement.
JrdStatement* JrdStatement::makeStatement(thread_db* tdbb, CompilerScratch* csb, bool internalFlag)
{
	DEV_BLKCHK(csb, type_csb);
	SET_TDBB(tdbb);

	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);

	jrd_req* const old_request = tdbb->getRequest();
	tdbb->setRequest(NULL);

	JrdStatement* statement = NULL;

	try
	{
		// Once any expansion required has been done, make a pass to assign offsets
		// into the impure area and throw away any unnecessary crude. Execution
		// optimizations can be performed here.

		DmlNode::doPass1(tdbb, csb, &csb->csb_node);

		// CVC: I'm going to allocate the map before the loop to avoid alloc/dealloc calls.
		AutoPtr<StreamType, ArrayDelete<StreamType> > localMap(FB_NEW(*tdbb->getDefaultPool())
			StreamType[STREAM_MAP_LENGTH]);

		// Copy and compile (pass1) domains DEFAULT and constraints.
		MapFieldInfo::Accessor accessor(&csb->csb_map_field_info);

		for (bool found = accessor.getFirst(); found; found = accessor.getNext())
		{
			FieldInfo& fieldInfo = accessor.current()->second;
			//StreamType local_map[MAP_LENGTH];

			AutoSetRestore<USHORT> autoRemapVariable(&csb->csb_remap_variable,
				(csb->csb_variables ? csb->csb_variables->count() : 0) + 1);

			fieldInfo.defaultValue = NodeCopier::copy(tdbb, csb, fieldInfo.defaultValue, localMap);

			csb->csb_remap_variable = (csb->csb_variables ? csb->csb_variables->count() : 0) + 1;

			if (fieldInfo.validationExpr)
			{
				NodeCopier copier(csb, localMap);
				fieldInfo.validationExpr = copier.copy(tdbb, fieldInfo.validationExpr);
			}

			DmlNode::doPass1(tdbb, csb, fieldInfo.defaultValue.getAddress());
			DmlNode::doPass1(tdbb, csb, fieldInfo.validationExpr.getAddress());
		}

		if (csb->csb_node->kind == DmlNode::KIND_STATEMENT)
			StmtNode::doPass2(tdbb, csb, reinterpret_cast<StmtNode**>(&csb->csb_node), NULL);
		else
			ExprNode::doPass2(tdbb, csb, &csb->csb_node);

		// Compile (pass2) domains DEFAULT and constraints
		for (bool found = accessor.getFirst(); found; found = accessor.getNext())
		{
			FieldInfo& fieldInfo = accessor.current()->second;
			ExprNode::doPass2(tdbb, csb, fieldInfo.defaultValue.getAddress());
			ExprNode::doPass2(tdbb, csb, fieldInfo.validationExpr.getAddress());
		}

		if (csb->csb_impure > MAX_REQUEST_SIZE)
			IBERROR(226);			// msg 226 request size limit exceeded

		// Build the statement and the final request block.

		MemoryPool* const pool = tdbb->getDefaultPool();

		statement = FB_NEW(*pool) JrdStatement(tdbb, pool, csb);

		tdbb->setRequest(old_request);
	} // try
	catch (const Exception& ex)
	{
		if (statement)
		{
			// Release sub statements.
			for (JrdStatement** subStatement = statement->subStatements.begin();
				 subStatement != statement->subStatements.end();
				 ++subStatement)
			{
				(*subStatement)->release(tdbb);
			}
		}

		ex.stuff_exception(tdbb->tdbb_status_vector);
		tdbb->setRequest(old_request);
		ERR_punt();
	}

	if (internalFlag)
		statement->flags |= FLAG_INTERNAL;
	else
		tdbb->bumpStats(RuntimeStatistics::STMT_PREPARES);

	return statement;
}

// Turn a parsed scratch into an executable request.
jrd_req* JrdStatement::makeRequest(thread_db* tdbb, CompilerScratch* csb, bool internalFlag)
{
	JrdStatement* statement = makeStatement(tdbb, csb, internalFlag);
	return statement->getRequest(tdbb, 0);
}

// Returns function or procedure routine.
const Routine* JrdStatement::getRoutine() const
{
	fb_assert(!(procedure && function));

	if (procedure)
		return procedure;

	return function;
}

// Determine if any request of this statement are active.
bool JrdStatement::isActive() const
{
	for (const jrd_req* const* request = requests.begin(); request != requests.end(); ++request)
	{
		if (*request && ((*request)->req_flags & req_in_use))
			return true;
	}

	return false;
}

jrd_req* JrdStatement::findRequest(thread_db* tdbb)
{
	SET_TDBB(tdbb);
	Attachment* const attachment = tdbb->getAttachment();

	if (!this)
		BUGCHECK(167);	/* msg 167 invalid SEND request */

	// Search clones for one request in use by this attachment.
	// If not found, return first inactive request.

	jrd_req* clone = NULL;
	USHORT count = 0;
	const USHORT clones = requests.getCount();
	USHORT n;

	for (n = 0; n < clones; ++n)
	{
		jrd_req* next = getRequest(tdbb, n);

		if (next->req_attachment == attachment)
		{
			if (!(next->req_flags & req_in_use))
			{
				clone = next;
				break;
			}

			++count;
		}
		else if (!(next->req_flags & req_in_use) && !clone)
			clone = next;
	}

	if (count > MAX_CLONES)
		ERR_post(Arg::Gds(isc_req_max_clones_exceeded));

	if (!clone)
		clone = getRequest(tdbb, n);

	clone->setAttachment(attachment);
	clone->req_stats.reset();
	clone->req_base_stats.reset();
	clone->req_flags |= req_in_use;

	return clone;
}

jrd_req* JrdStatement::getRequest(thread_db* tdbb, USHORT level)
{
	SET_TDBB(tdbb);

	Jrd::Attachment* const attachment = tdbb->getAttachment();
	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);

	if (level < requests.getCount() && requests[level])
		return requests[level];

	requests.grow(level + 1);

	MemoryStats* const parentStats = (flags & FLAG_INTERNAL) ?
		&dbb->dbb_memory_stats : &attachment->att_memory_stats;

	// Create the request.
	jrd_req* const request = FB_NEW(*pool) jrd_req(attachment, this, parentStats);
	request->req_id = dbb->generateStatementId(tdbb);

	requests[level] = request;

	return request;
}

// Check that we have enough rights to access all resources this request touches including
// resources it used indirectly via procedures or triggers.
void JrdStatement::verifyAccess(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	ExternalAccessList external;
	buildExternalAccess(tdbb, external);

	for (ExternalAccess* item = external.begin(); item != external.end(); ++item)
	{
		const Routine* routine = NULL;
		int aclType;

		if (item->exa_action == ExternalAccess::exa_procedure)
		{
			routine = MET_lookup_procedure_id(tdbb, item->exa_prc_id, false, false, 0);
			aclType = id_procedure;
		}
		else if (item->exa_action == ExternalAccess::exa_function)
		{
			routine = Function::lookup(tdbb, item->exa_fun_id, false, false, 0);
			aclType = id_function;
		}
		else
		{
			jrd_rel* relation = MET_lookup_relation_id(tdbb, item->exa_rel_id, false);
			jrd_rel* view = NULL;
			if (item->exa_view_id)
				view = MET_lookup_relation_id(tdbb, item->exa_view_id, false);

			if (!relation)
				continue;

			switch (item->exa_action)
			{
				case ExternalAccess::exa_insert:
					verifyTriggerAccess(tdbb, relation, relation->rel_pre_store, view);
					verifyTriggerAccess(tdbb, relation, relation->rel_post_store, view);
					break;
				case ExternalAccess::exa_update:
					verifyTriggerAccess(tdbb, relation, relation->rel_pre_modify, view);
					verifyTriggerAccess(tdbb, relation, relation->rel_post_modify, view);
					break;
				case ExternalAccess::exa_delete:
					verifyTriggerAccess(tdbb, relation, relation->rel_pre_erase, view);
					verifyTriggerAccess(tdbb, relation, relation->rel_post_erase, view);
					break;
				default:
					fb_assert(false);
			}

			continue;
		}

		fb_assert(routine);
		if (!routine->getStatement())
			continue;

		for (const AccessItem* access = routine->getStatement()->accessList.begin();
			 access != routine->getStatement()->accessList.end();
			 ++access)
		{
			const SecurityClass* sec_class = SCL_get_class(tdbb, access->acc_security_name.c_str());

			if (routine->getName().package.isEmpty())
			{
				SCL_check_access(tdbb, sec_class, access->acc_view_id, aclType,
					routine->getName().identifier, access->acc_mask, access->acc_type,
					true, access->acc_name, access->acc_r_name);
			}
			else
			{
				SCL_check_access(tdbb, sec_class, access->acc_view_id,
					id_package, routine->getName().package,
					access->acc_mask, access->acc_type,
					true, access->acc_name, access->acc_r_name);
			}
		}
	}

	// Inherit privileges of caller stored procedure or trigger if and only if
	// this request is called immediately by caller (check for empty req_caller).
	// Currently (in v2.5) this rule will work for EXECUTE STATEMENT only, as
	// tra_callback_count incremented only by it.
	// In v3.0, this rule also works for external procedures and triggers.
	jrd_tra* transaction = tdbb->getTransaction();
	const bool useCallerPrivs = transaction && transaction->tra_callback_count;

	for (const AccessItem* access = accessList.begin(); access != accessList.end(); ++access)
	{
		const SecurityClass* sec_class = SCL_get_class(tdbb, access->acc_security_name.c_str());

		MetaName objName;
		SLONG objType = 0;

		if (useCallerPrivs)
		{
			switch (transaction->tra_caller_name.type)
			{
				case obj_trigger:
					objType = id_trigger;
					break;
				case obj_procedure:
					objType = id_procedure;
					break;
				case obj_udf:
					objType = id_function;
					break;
				case obj_package_header:
					objType = id_package;
					break;
				case obj_type_MAX:	// CallerName() constructor
					fb_assert(transaction->tra_caller_name.name.isEmpty());
					break;
				default:
					fb_assert(false);
			}

			objName = transaction->tra_caller_name.name;
		}

		SCL_check_access(tdbb, sec_class, access->acc_view_id, objType, objName,
			access->acc_mask, access->acc_type, true, access->acc_name, access->acc_r_name);
	}
}

// Release a statement.
void JrdStatement::release(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	// Release sub statements.
	for (JrdStatement** subStatement = subStatements.begin();
		 subStatement != subStatements.end();
		 ++subStatement)
	{
		(*subStatement)->release(tdbb);
	}

	// Release existence locks on references.

	for (Resource* resource = resources.begin(); resource != resources.end(); ++resource)
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
			case Resource::rsc_function:
				resource->rsc_routine->release(tdbb);
				break;

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

	for (jrd_req** instance = requests.begin(); instance != requests.end(); ++instance)
		EXE_release(tdbb, *instance);

	sqlText = NULL;

	// Sub statement pool is the same of the main statement, so don't delete it.
	if (!parentStatement)
	{
		Jrd::Attachment* const att = tdbb->getAttachment();
		att->deletePool(pool);
	}
}

// Check that we have enough rights to access all resources this list of triggers touches.
void JrdStatement::verifyTriggerAccess(thread_db* tdbb, jrd_rel* ownerRelation,
	trig_vec* triggers, jrd_rel* view)
{
	if (!triggers)
		return;

	SET_TDBB(tdbb);

	for (size_t i = 0; i < triggers->getCount(); i++)
	{
		Trigger& t = (*triggers)[i];
		t.compile(tdbb);
		if (!t.statement)
			continue;

		for (const AccessItem* access = t.statement->accessList.begin();
			 access != t.statement->accessList.end(); ++access)
		{
			// If this is not a system relation, we don't post access check if:
			//
			// - The table being checked is the owner of the trigger that's accessing it.
			// - The field being checked is owned by the same table than the trigger
			//   that's accessing the field.
			// - Since the trigger name comes in the triggers vector of the table and each
			//   trigger can be owned by only one table for now, we know for sure that
			//   it's a trigger defined on our target table.

			if (!(ownerRelation->rel_flags & REL_system))
			{
				if (access->acc_type == SCL_object_table &&
					(ownerRelation->rel_name == access->acc_name))
				{
					continue;
				}
				if (access->acc_type == SCL_object_column &&
					(ownerRelation->rel_name == access->acc_r_name))
				{
					continue;
				}
			}

			// a direct access to an object from this trigger
			const SecurityClass* sec_class = SCL_get_class(tdbb, access->acc_security_name.c_str());
			SCL_check_access(tdbb, sec_class,
				(access->acc_view_id) ? access->acc_view_id : (view ? view->rel_id : 0),
				id_trigger, t.statement->triggerName, access->acc_mask,
				access->acc_type, true, access->acc_name, access->acc_r_name);
		}
	}
}

// Invoke buildExternalAccess for triggers in vector
inline void JrdStatement::triggersExternalAccess(thread_db* tdbb, ExternalAccessList& list,
	trig_vec* tvec)
{
	if (!tvec)
		return;

	for (size_t i = 0; i < tvec->getCount(); i++)
	{
		Trigger& t = (*tvec)[i];
		t.compile(tdbb);

		if (t.statement)
			t.statement->buildExternalAccess(tdbb, list);
	}
}

// Recursively walk external dependencies (procedures, triggers) for request to assemble full
// list of requests it depends on.
void JrdStatement::buildExternalAccess(thread_db* tdbb, ExternalAccessList& list)
{
	for (ExternalAccess* item = externalList.begin(); item != externalList.end(); ++item)
	{
		size_t i;
		if (list.find(*item, i))
			continue;

		list.insert(i, *item);

		// Add externals recursively
		if (item->exa_action == ExternalAccess::exa_procedure)
		{
			jrd_prc* const procedure = MET_lookup_procedure_id(tdbb, item->exa_prc_id, false, false, 0);
			if (procedure && procedure->getStatement())
				procedure->getStatement()->buildExternalAccess(tdbb, list);
		}
		else if (item->exa_action == ExternalAccess::exa_function)
		{
			Function* const function = Function::lookup(tdbb, item->exa_fun_id, false, false, 0);
			if (function && function->getStatement())
				function->getStatement()->buildExternalAccess(tdbb, list);
		}
		else
		{
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

			triggersExternalAccess(tdbb, list, vec1);
			triggersExternalAccess(tdbb, list, vec2);
		}
	}
}


// Make sub routines.
template <typename T> static void makeSubRoutines(thread_db* tdbb, JrdStatement* statement,
	CompilerScratch* csb, T& subs)
{
	typename T::Accessor subAccessor(&subs);

	for (bool found = subAccessor.getFirst(); found; found = subAccessor.getNext())
	{
		typename T::ValueType subNode = subAccessor.current()->second;
		Routine* subRoutine = subNode->routine;
		CompilerScratch*& subCsb = subNode->subCsb;

		JrdStatement* subStatement = JrdStatement::makeStatement(tdbb, subCsb, true);
		subStatement->parentStatement = statement;
		subRoutine->setStatement(subStatement);

		switch (subRoutine->getObjectType())
		{
		case obj_procedure:
			subStatement->procedure = static_cast<jrd_prc*>(subRoutine);
			break;

		case obj_udf:
			subStatement->function = static_cast<Function*>(subRoutine);
			break;

		default:
			fb_assert(false);
			break;
		}

		// Move dependencies and permissions from the sub routine to the parent.

		for (CompilerScratch::Dependency* dependency = subCsb->csb_dependencies.begin();
			 dependency != subCsb->csb_dependencies.end();
			 ++dependency)
		{
			csb->csb_dependencies.push(*dependency);
		}

		for (ExternalAccess* access = subCsb->csb_external.begin();
			 access != subCsb->csb_external.end();
			 ++access)
		{
			size_t i;
			if (!csb->csb_external.find(*access, i))
				csb->csb_external.insert(i, *access);
		}

		for (AccessItem* access = subCsb->csb_access.begin();
			 access != subCsb->csb_access.end();
			 ++access)
		{
			size_t i;
			if (!csb->csb_access.find(*access, i))
				csb->csb_access.insert(i, *access);
		}

		delete subCsb;
		subCsb = NULL;

		statement->subStatements.add(subStatement);
	}
}
