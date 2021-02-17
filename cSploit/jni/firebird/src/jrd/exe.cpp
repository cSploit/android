/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe.cpp
 *	DESCRIPTION:	Statement execution
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
 * 2001.6.21 Claudio Valderrama: Allow inserting strings into blob fields.
 * 2001.6.28 Claudio Valderrama: Move code to cleanup_rpb() as directed
 * by Ann Harrison and cleanup of new record in store() routine.
 * 2001.10.11 Claudio Valderrama: Fix SF Bug #436462: From now, we only
 * count real store, modify and delete operations either in an external
 * file or in a table. Counting on a view caused up to three operations
 * being reported instead of one.
 * 2001.12.03 Claudio Valderrama: new visit to the same issue: views need
 * to count virtual operations, not real I/O on the underlying tables.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 *
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 * Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "../jrd/ibsetjmp.h"
#include "../common/classes/VaryStr.h"
#include <string.h>
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "../jrd/extds/ExtDS.h"
#include "../jrd/tra.h"
#include "gen/iberror.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/rse.h"
#include "../jrd/lck.h"
#include "../jrd/intl.h"
#include "../jrd/sbm.h"
#include "../jrd/blb.h"
#include "../jrd/blr.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/rlck_proto.h"

#include "../jrd/tra_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/isc_s_proto.h"

#include "../dsql/dsql_proto.h"
#include "../jrd/rpb_chain.h"
#include "../jrd/RecordSourceNodes.h"
#include "../jrd/VirtualTable.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceJrdHelpers.h"

#include "../dsql/Nodes.h"
#include "../jrd/recsrc/RecordSource.h"
#include "../jrd/recsrc/Cursor.h"
#include "../jrd/Function.h"


using namespace Jrd;
using namespace Firebird;

// AffectedRows class implementation

AffectedRows::AffectedRows()
{
	clear();
}

void AffectedRows::clear()
{
	writeFlag = false;
	fetchedRows = modifiedRows = 0;
}

void AffectedRows::bumpFetched()
{
	fetchedRows++;
}

void AffectedRows::bumpModified(bool increment)
{
	if (increment) {
		modifiedRows++;
	}
	else {
		writeFlag = true;
	}
}

int AffectedRows::getCount() const
{
	return writeFlag ? modifiedRows : fetchedRows;
}

// StatusXcp class implementation

StatusXcp::StatusXcp()
{
	clear();
}

void StatusXcp::clear()
{
	fb_utils::init_status(status);
}

void StatusXcp::init(const ISC_STATUS* vector)
{
	memcpy(status, vector, sizeof(ISC_STATUS_ARRAY));
}

void StatusXcp::copyTo(ISC_STATUS* vector) const
{
	memcpy(vector, status, sizeof(ISC_STATUS_ARRAY));
}

bool StatusXcp::success() const
{
	return (status[1] == FB_SUCCESS);
}

SLONG StatusXcp::as_gdscode() const
{
	return status[1];
}

SLONG StatusXcp::as_sqlcode() const
{
	return gds__sqlcode(status);
}

void StatusXcp::as_sqlstate(char* sqlstate) const
{
	fb_sqlstate(sqlstate, status);
}

static void execute_looper(thread_db*, jrd_req*, jrd_tra*, const StmtNode*, jrd_req::req_s);
static void looper_seh(thread_db*, jrd_req*, const StmtNode*);
static void release_blobs(thread_db*, jrd_req*);
static void release_proc_save_points(jrd_req*);
static void trigger_failure(thread_db*, jrd_req*);
static void stuff_stack_trace(const jrd_req*);

const size_t MAX_STACK_TRACE = 2048;


// Perform an assignment.
void EXE_assignment(thread_db* tdbb, const AssignmentNode* node)
{
	DEV_BLKCHK(node, type_nod);

	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();

	// Get descriptors of src field/parameter/variable, etc.
	request->req_flags &= ~req_null;
	dsc* from_desc = EVL_expr(tdbb, request, node->asgnFrom);

	EXE_assignment(tdbb, node->asgnTo, from_desc, (request->req_flags & req_null),
		node->missing, node->missing2);
}

// Perform an assignment.
void EXE_assignment(thread_db* tdbb, const ValueExprNode* source, const ValueExprNode* target)
{
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();

	// Get descriptors of src field/parameter/variable, etc.
	request->req_flags &= ~req_null;
	dsc* from_desc = EVL_expr(tdbb, request, source);

	EXE_assignment(tdbb, target, from_desc, (request->req_flags & req_null), NULL, NULL);
}

// Perform an assignment.
void EXE_assignment(thread_db* tdbb, const ValueExprNode* to, dsc* from_desc, bool from_null,
	const ValueExprNode* missing_node, const ValueExprNode* missing2_node)
{
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();

	// Get descriptors of receiving and sending fields/parameters, variables, etc.

	dsc* missing = NULL;
	if (missing_node)
		missing = EVL_expr(tdbb, request, missing_node);

	// Get descriptor of target field/parameter/variable, etc.
	DSC* to_desc = EVL_assign_to(tdbb, to);

	request->req_flags &= ~req_null;

	// NS: If we are assigning to NULL, we finished.
	// This functionality is currently used to allow calling UDF routines
	// without assigning resulting value anywhere.
	if (!to_desc)
		return;

	SSHORT null = from_null ? -1 : 0;

	if (!null && missing && MOV_compare(missing, from_desc) == 0)
		null = -1;

	USHORT* impure_flags = NULL;
	const ParameterNode* toParam;
	const VariableNode* toVar;

	if ((toParam = ExprNode::as<ParameterNode>(to)))
	{
		const MessageNode* message = toParam->message;

		if (toParam->argInfo)
		{
			EVL_validate(tdbb, Item(Item::TYPE_PARAMETER, message->messageNumber, toParam->argNumber),
				toParam->argInfo, from_desc, null == -1);
		}

		impure_flags = request->getImpure<USHORT>(
			message->impureFlags + (sizeof(USHORT) * toParam->argNumber));
	}
	else if ((toVar = ExprNode::as<VariableNode>(to)))
	{
		if (toVar->varInfo)
		{
			EVL_validate(tdbb, Item(Item::TYPE_VARIABLE, toVar->varId),
				toVar->varInfo, from_desc, null == -1);
		}

		impure_flags = &request->getImpure<impure_value>(
			toVar->varDecl->impureOffset)->vlu_flags;
	}

	if (impure_flags != NULL)
		*impure_flags |= VLU_checked;

	// If the value is non-missing, move/convert it.  Otherwise fill the
	// field with appropriate nulls.
	dsc temp;

	if (!null)
	{
		// if necessary and appropriate, use the indicator variable

		if (toParam && toParam->argIndicator)
		{
			dsc* indicator = EVL_assign_to(tdbb, toParam->argIndicator);
			temp.dsc_dtype = dtype_short;
			temp.dsc_length = sizeof(SSHORT);
			temp.dsc_scale = 0;
			temp.dsc_sub_type = 0;

			SSHORT len;

			if ((from_desc->dsc_dtype <= dtype_varying) && (to_desc->dsc_dtype <= dtype_varying) &&
				(TEXT_LEN(from_desc) > TEXT_LEN(to_desc)))
			{
				len = TEXT_LEN(from_desc);
			}
			else
				len = 0;

			temp.dsc_address = (UCHAR *) &len;
			MOV_move(tdbb, &temp, indicator);

			if (len)
			{
				temp = *from_desc;
				temp.dsc_length = TEXT_LEN(to_desc);

				if (temp.dsc_dtype == dtype_cstring)
					temp.dsc_length += 1;
				else if (temp.dsc_dtype == dtype_varying)
					temp.dsc_length += 2;

				from_desc = &temp;
			}
		}

		// Validate range for datetime values

		if (DTYPE_IS_DATE(from_desc->dsc_dtype))
		{
			switch (from_desc->dsc_dtype)
			{
				case dtype_sql_date:
					if (!Firebird::TimeStamp::isValidDate(*(GDS_DATE*) from_desc->dsc_address))
					{
						ERR_post(Arg::Gds(isc_date_range_exceeded));
					}
					break;

				case dtype_sql_time:
					if (!Firebird::TimeStamp::isValidTime(*(GDS_TIME*) from_desc->dsc_address))
					{
						ERR_post(Arg::Gds(isc_time_range_exceeded));
					}
					break;

				case dtype_timestamp:
					if (!Firebird::TimeStamp::isValidTimeStamp(*(GDS_TIMESTAMP*) from_desc->dsc_address))
					{
						ERR_post(Arg::Gds(isc_datetime_range_exceeded));
					}
					break;

				default:
					fb_assert(false);
			}
		}

		if (DTYPE_IS_BLOB_OR_QUAD(from_desc->dsc_dtype) || DTYPE_IS_BLOB_OR_QUAD(to_desc->dsc_dtype))
		{
			// ASF: Don't let MOV_move call blb::move because MOV
			// will not pass the destination field to blb::_move.
			blb::move(tdbb, from_desc, to_desc, to);
		}
		else if (!DSC_EQUIV(from_desc, to_desc, false))
		{
			MOV_move(tdbb, from_desc, to_desc);
		}
		else if (from_desc->dsc_dtype == dtype_short)
		{
			*((SSHORT*) to_desc->dsc_address) = *((SSHORT*) from_desc->dsc_address);
		}
		else if (from_desc->dsc_dtype == dtype_long)
		{
			*((SLONG*) to_desc->dsc_address) = *((SLONG*) from_desc->dsc_address);
		}
		else if (from_desc->dsc_dtype == dtype_int64)
		{
			*((SINT64*) to_desc->dsc_address) = *((SINT64*) from_desc->dsc_address);
		}
		else
		{
			memcpy(to_desc->dsc_address, from_desc->dsc_address, from_desc->dsc_length);
		}

		to_desc->dsc_flags &= ~DSC_null;
	}
	else
	{
		if (missing2_node && (missing = EVL_expr(tdbb, request, missing2_node)))
			MOV_move(tdbb, missing, to_desc);
		else
			memset(to_desc->dsc_address, 0, to_desc->dsc_length);

		to_desc->dsc_flags |= DSC_null;
	}

	// Handle the null flag as appropriate for fields and message arguments.


	const FieldNode* toField = ExprNode::as<FieldNode>(to);
	if (toField)
	{
		Record* record = request->req_rpb[toField->fieldStream].rpb_record;

		if (null)
			record->setNull(toField->fieldId);
		else
			record->clearNull(toField->fieldId);
	}
	else if (toParam && toParam->argFlag)
	{
		to_desc = EVL_assign_to(tdbb, toParam->argFlag);

		// If the null flag is a string with an effective length of one,
		// then -1 will not fit.  Therefore, store 1 instead.

		if (null && to_desc->dsc_dtype <= dtype_varying)
		{
			USHORT minlen;

			switch (to_desc->dsc_dtype)
			{
			case dtype_text:
				minlen = 1;
				break;
			case dtype_cstring:
				minlen = 2;
				break;
			case dtype_varying:
				minlen = 3;
				break;
			}

			if (to_desc->dsc_length <= minlen)
				null = 1;
		}

		temp.dsc_dtype = dtype_short;
		temp.dsc_length = sizeof(SSHORT);
		temp.dsc_scale = 0;
		temp.dsc_sub_type = 0;
		temp.dsc_address = (UCHAR*) &null;
		MOV_move(tdbb, &temp, to_desc);

		if (null && toParam->argIndicator)
		{
			to_desc = EVL_assign_to(tdbb, toParam->argIndicator);
			MOV_move(tdbb, &temp, to_desc);
		}
	}
}


void EXE_execute_db_triggers(thread_db* tdbb, jrd_tra* transaction, jrd_req::req_ta trigger_action)
{
/**************************************
 *
 *	E X E _ e x e c u t e _ d b _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *	Execute database triggers
 *
 **************************************/
	Jrd::Attachment* attachment = tdbb->getAttachment();

 	// Do nothing if user doesn't want database triggers.
	if (attachment->att_flags & ATT_no_db_triggers)
		return;

	int type = 0;

	switch (trigger_action)
	{
		case jrd_req::req_trigger_connect:
			type = DB_TRIGGER_CONNECT;
			break;

		case jrd_req::req_trigger_disconnect:
			type = DB_TRIGGER_DISCONNECT;
			break;

		case jrd_req::req_trigger_trans_start:
			type = DB_TRIGGER_TRANS_START;
			break;

		case jrd_req::req_trigger_trans_commit:
			type = DB_TRIGGER_TRANS_COMMIT;
			break;

		case jrd_req::req_trigger_trans_rollback:
			type = DB_TRIGGER_TRANS_ROLLBACK;
			break;

		default:
			fb_assert(false);
			return;
	}

	if (attachment->att_triggers[type])
	{
		jrd_tra* old_transaction = tdbb->getTransaction();
		tdbb->setTransaction(transaction);

		try
		{
			EXE_execute_triggers(tdbb, &attachment->att_triggers[type],
				NULL, NULL, trigger_action, StmtNode::ALL_TRIGS);
			tdbb->setTransaction(old_transaction);
		}
		catch (...)
		{
			tdbb->setTransaction(old_transaction);
			throw;
		}
	}
}


// Execute DDL triggers.
void EXE_execute_ddl_triggers(thread_db* tdbb, jrd_tra* transaction, bool preTriggers, int action)
{
	Jrd::Attachment* attachment = tdbb->getAttachment();

	// Our caller verifies (ATT_no_db_triggers) if DDL triggers should not run.

	if (attachment->att_ddl_triggers)
	{
		jrd_tra* const oldTransaction = tdbb->getTransaction();
		tdbb->setTransaction(transaction);

		try
		{
			trig_vec triggers;
			trig_vec* triggersPtr = &triggers;

			for (trig_vec::iterator i = attachment->att_ddl_triggers->begin();
				 i != attachment->att_ddl_triggers->end();
				 ++i)
			{
				if ((i->type & (1LL << action)) &&
					((preTriggers && (i->type & 0x1) == 0) || (!preTriggers && (i->type & 0x1) == 0x1)))
				{
					triggers.add() = *i;
				}
			}

			EXE_execute_triggers(tdbb, &triggersPtr, NULL, NULL, jrd_req::req_trigger_ddl,
				StmtNode::ALL_TRIGS);

			tdbb->setTransaction(oldTransaction);
		}
		catch (...)
		{
			tdbb->setTransaction(oldTransaction);
			throw;
		}
	}
}


void EXE_receive(thread_db* tdbb,
				 jrd_req* request,
				 USHORT msg,
				 ULONG length,
				 UCHAR* buffer,
				 bool top_level)
{
/**************************************
 *
 *	E X E _ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Move a message from JRD to the host program.  This corresponds to
 *	a JRD BLR/Stmtode* send.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(request, type_req);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_tra* transaction = request->req_transaction;

	if (!(request->req_flags & req_active)) {
		ERR_post(Arg::Gds(isc_req_sync));
	}

	if (request->req_flags & req_proc_fetch)
	{
		/* request->req_proc_sav_point stores all the request savepoints.
		   When going to continue execution put request save point list
		   into transaction->tra_save_point so that it is used in looper.
		   When we come back to EXE_receive() restore
		   transaction->tra_save_point and merge all work done under
		   stored procedure savepoints into the current transaction
		   savepoint, which is the savepoint for fetch. */

		Savepoint* const save_sav_point = transaction->tra_save_point;
		transaction->tra_save_point = request->req_proc_sav_point;
		request->req_proc_sav_point = save_sav_point;

		if (!transaction->tra_save_point) {
			VIO_start_save_point(tdbb, transaction);
		}
	}

	try
	{

	const JrdStatement* statement = request->getStatement();

	if (StmtNode::is<StallNode>(request->req_message))
		execute_looper(tdbb, request, transaction, request->req_next, jrd_req::req_sync);

	if (!(request->req_flags & req_active) || request->req_operation != jrd_req::req_send)
		ERR_post(Arg::Gds(isc_req_sync));

	const MessageNode* message = StmtNode::as<MessageNode>(request->req_message);
	const Format* format = message->format;

	if (msg != message->messageNumber)
		ERR_post(Arg::Gds(isc_req_sync));

	if (length != format->fmt_length)
		ERR_post(Arg::Gds(isc_port_len) << Arg::Num(length) << Arg::Num(format->fmt_length));

	memcpy(buffer, request->getImpure<UCHAR>(message->impureOffset), length);

	// ASF: temporary blobs returned to the client should not be released
	// with the request, but in the transaction end.
	if (top_level)
	{
		for (int i = 0; i < format->fmt_count; ++i)
		{
			const DSC* desc = &format->fmt_desc[i];

			if (desc->isBlob())
			{
				const bid* id = (bid*) (buffer + (ULONG)(IPTR)desc->dsc_address);

				if (transaction->tra_blobs->locate(id->bid_temp_id()))
				{
					BlobIndex* current = &transaction->tra_blobs->current();

					if (current->bli_request &&
						current->bli_request->req_blobs.locate(id->bid_temp_id()))
					{
						current->bli_request->req_blobs.fastRemove();
						current->bli_request = NULL;
					}
				}
			}
		}
	}

	execute_looper(tdbb, request, transaction, request->req_next, jrd_req::req_proceed);

	}	//try
	catch (const Firebird::Exception&)
	{
		if (request->req_flags & req_proc_fetch)
		{
			Savepoint* const save_sav_point = transaction->tra_save_point;
			transaction->tra_save_point = request->req_proc_sav_point;
			request->req_proc_sav_point = save_sav_point;
			release_proc_save_points(request);
		}
		throw;
	}

	if (request->req_flags & req_proc_fetch)
	{
		Savepoint* const save_sav_point = transaction->tra_save_point;
		transaction->tra_save_point = request->req_proc_sav_point;
		request->req_proc_sav_point = save_sav_point;
		VIO_merge_proc_sav_points(tdbb, transaction, &request->req_proc_sav_point);
	}
}


// Release a request instance.
void EXE_release(thread_db* tdbb, jrd_req* request)
{
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);

	EXE_unwind(tdbb, request);

	// system requests are released after all attachments gone and with
	// req_attachment not cleared

	const Jrd::Attachment* attachment = tdbb->getAttachment();

	if (request->req_attachment && request->req_attachment == attachment)
	{
		size_t pos;
		if (request->req_attachment->att_requests.find(request, pos))
			request->req_attachment->att_requests.remove(pos);

		request->req_attachment = NULL;
	}
}


void EXE_send(thread_db* tdbb, jrd_req* request, USHORT msg, ULONG length, const UCHAR* buffer)
{
/**************************************
 *
 *	E X E _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Send a message from the host program to the engine.
 *	This corresponds to a blr_receive or blr_select statement.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(request, type_req);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	if (!(request->req_flags & req_active))
		ERR_post(Arg::Gds(isc_req_sync));

	const StmtNode* message = NULL;
	const StmtNode* node;

	if (request->req_operation != jrd_req::req_receive)
		ERR_post(Arg::Gds(isc_req_sync));
	node = request->req_message;

	jrd_tra* transaction = request->req_transaction;
	const JrdStatement* statement = request->getStatement();

	const SelectNode* selectNode;

	if (StmtNode::is<MessageNode>(node))
		message = node;
	else if ((selectNode = StmtNode::as<SelectNode>(node)))
	{
		const NestConst<StmtNode>* ptr = selectNode->statements.begin();

		for (const NestConst<StmtNode>* end = selectNode->statements.end(); ptr != end; ++ptr)
		{
			const ReceiveNode* receiveNode = (*ptr)->as<ReceiveNode>();
			message = receiveNode->message;

			if (message->as<MessageNode>()->messageNumber == msg)
			{
				request->req_next = *ptr;
				break;
			}
		}
	}
	else
		BUGCHECK(167);	// msg 167 invalid SEND request

	const Format* format = StmtNode::as<MessageNode>(message)->format;

	if (msg != StmtNode::as<MessageNode>(message)->messageNumber)
		ERR_post(Arg::Gds(isc_req_sync));

	if (length != format->fmt_length)
		ERR_post(Arg::Gds(isc_port_len) << Arg::Num(length) << Arg::Num(format->fmt_length));

	memcpy(request->getImpure<UCHAR>(message->impureOffset), buffer, length);

	for (USHORT i = 0; i < format->fmt_count; ++i)
	{
		const DSC* desc = &format->fmt_desc[i];

		// ASF: I'll not test for dtype_cstring because usage is only internal
		if (desc->dsc_dtype == dtype_text || desc->dsc_dtype == dtype_varying)
		{
			const UCHAR* p = request->getImpure<UCHAR>(message->impureOffset +
				(ULONG)(IPTR) desc->dsc_address);
			USHORT len;

			switch (desc->dsc_dtype)
			{
				case dtype_text:
					len = desc->dsc_length;
					break;

				case dtype_varying:
					len = reinterpret_cast<const vary*>(p)->vary_length;
					p += sizeof(USHORT);
					break;
			}

			CharSet* charSet = INTL_charset_lookup(tdbb, DSC_GET_CHARSET(desc));

			if (!charSet->wellFormed(len, p))
				ERR_post(Arg::Gds(isc_malformed_string));
		}
		else if (desc->isBlob())
		{
			if (desc->getCharSet() != CS_NONE && desc->getCharSet() != CS_BINARY)
			{
				const Jrd::bid* bid = request->getImpure<Jrd::bid>(
					message->impureOffset + (ULONG)(IPTR) desc->dsc_address);

				if (!bid->isEmpty())
				{
					AutoBlb blob(tdbb, blb::open(tdbb, transaction/*tdbb->getTransaction()*/, bid));
					blob.getBlb()->BLB_check_well_formed(tdbb, desc);
				}
			}
		}
	}

	execute_looper(tdbb, request, transaction, request->req_next, jrd_req::req_proceed);
}


void EXE_start(thread_db* tdbb, jrd_req* request, jrd_tra* transaction)
{
/**************************************
 *
 *	E X E _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start an execution running.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	BLKCHK(request, type_req);
	BLKCHK(transaction, type_tra);

	if (request->req_flags & req_active)
		ERR_post(Arg::Gds(isc_req_sync) << Arg::Gds(isc_reqinuse));

	if (transaction->tra_flags & TRA_prepared)
		ERR_post(Arg::Gds(isc_req_no_trans));

	JrdStatement* statement = request->getStatement();
	const jrd_prc* proc = statement->procedure;

	/* Post resources to transaction block.  In particular, the interest locks
	on relations/indices are copied to the transaction, which is very
	important for (short-lived) dynamically compiled requests.  This will
	provide transaction stability by preventing a relation from being
	dropped after it has been referenced from an active transaction. */

	TRA_post_resources(tdbb, transaction, statement->resources);

	TRA_attach_request(transaction, request);
	request->req_flags &= req_in_use;
	request->req_flags |= req_active;
	request->req_flags &= ~req_reserved;

	// set up to count records affected by request

	request->req_records_selected = 0;
	request->req_records_updated = 0;
	request->req_records_inserted = 0;
	request->req_records_deleted = 0;

	request->req_records_affected.clear();

	// CVC: set up to count virtual operations on SQL views.

	request->req_view_flags = 0;
	request->req_top_view_store = NULL;
	request->req_top_view_modify = NULL;
	request->req_top_view_erase = NULL;

	// Store request start time for timestamp work
	request->req_timestamp.validate();

	// Set all invariants to not computed.
	const ULONG* const* ptr, * const* end;
	for (ptr = statement->invariants.begin(), end = statement->invariants.end();
		 ptr < end; ++ptr)
	{
		impure_value* impure = request->getImpure<impure_value>(**ptr);
		impure->vlu_flags = 0;
	}

	if (statement->sqlText)
		tdbb->bumpStats(RuntimeStatistics::STMT_EXECUTES);

	request->req_src_line = 0;
	request->req_src_column = 0;

	execute_looper(tdbb, request, transaction,
				   request->getStatement()->topNode,
				   jrd_req::req_evaluate);
}


void EXE_unwind(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	E X E _ u n w i n d
 *
 **************************************
 *
 * Functional description
 *	Unwind a request, maybe active, maybe not.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);

	if (request->req_flags & req_active)
	{
		const JrdStatement* statement = request->getStatement();

		if (statement->fors.getCount() || request->req_ext_stmt)
		{
			Jrd::ContextPoolHolder context(tdbb, request->req_pool);
			jrd_req* old_request = tdbb->getRequest();
			jrd_tra* old_transaction = tdbb->getTransaction();
			try {
				tdbb->setRequest(request);
				tdbb->setTransaction(request->req_transaction);

				for (const RecordSource* const* ptr = statement->fors.begin();
					 ptr != statement->fors.end(); ++ptr)
				{
					(*ptr)->close(tdbb);
				}

				if (request->req_ext_resultset)
					delete request->req_ext_resultset;

				while (request->req_ext_stmt)
					request->req_ext_stmt->close(tdbb);
			}
			catch (const Firebird::Exception&)
			{
				tdbb->setRequest(old_request);
				tdbb->setTransaction(old_transaction);
				throw;
			}

			tdbb->setRequest(old_request);
			tdbb->setTransaction(old_transaction);
		}
		release_blobs(tdbb, request);
	}

	request->req_sorts.unlinkAll();

	if (request->req_proc_sav_point && (request->req_flags & req_proc_fetch))
		release_proc_save_points(request);

	TRA_detach_request(request);

	request->req_flags &= ~(req_active | req_proc_fetch | req_reserved);
	request->req_flags |= req_abort | req_stall;
	request->req_timestamp.invalidate();
	request->req_proc_inputs = NULL;
	request->req_proc_caller = NULL;
}


static void execute_looper(thread_db* tdbb,
						   jrd_req* request,
						   jrd_tra* transaction,
						   const StmtNode* node,
						   jrd_req::req_s next_state)
{
/**************************************
 *
 *	e x e c u t e _ l o o p e r
 *
 **************************************
 *
 * Functional description
 *	Wrapper around looper. This will execute
 *	looper with the save point mechanism.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	// Ensure the cancellation lock can be triggered

	Lock* const lock = attachment->att_cancel_lock;
	if (lock && lock->lck_logical == LCK_none)
		LCK_lock(tdbb, lock, LCK_SR, LCK_WAIT);

	// Start a save point

	if (!(request->req_flags & req_proc_fetch) && request->req_transaction)
	{
		if (transaction && (transaction != attachment->getSysTransaction()))
			VIO_start_save_point(tdbb, transaction);
	}

	request->req_flags &= ~req_stall;
	request->req_operation = next_state;

	looper_seh(tdbb, request, node);

	// If any requested modify/delete/insert ops have completed, forget them

	if (!(request->req_flags & req_proc_fetch) && request->req_transaction)
	{
		if (transaction && (transaction != attachment->getSysTransaction()) &&
			transaction->tra_save_point &&
			!(transaction->tra_save_point->sav_flags & SAV_user) &&
			!transaction->tra_save_point->sav_verb_count)
		{
			// Forget about any undo for this verb
			VIO_verb_cleanup(tdbb, transaction);
		}
	}
}


void EXE_execute_triggers(thread_db* tdbb,
								trig_vec** triggers,
								record_param* old_rpb,
								record_param* new_rpb,
								jrd_req::req_ta trigger_action, StmtNode::WhichTrigger which_trig)
{
/**************************************
 *
 *	e x e c u t e _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *	Execute group of triggers.  Return pointer to failing trigger
 *	if any blow up.
 *
 **************************************/
	if (!*triggers)
		return;

	SET_TDBB(tdbb);

	jrd_req* const request = tdbb->getRequest();
	jrd_tra* const transaction = request ? request->req_transaction : tdbb->getTransaction();

	trig_vec* vector = *triggers;
	Record* const old_rec = old_rpb ? old_rpb->rpb_record : NULL;
	Record* const new_rec = new_rpb ? new_rpb->rpb_record : NULL;

	AutoPtr<Record> null_rec;

	const bool is_db_trigger = (!old_rec && !new_rec);

	if (!is_db_trigger && (!old_rec || !new_rec))
	{
		const Record* record = old_rec ? old_rec : new_rec;
		fb_assert(record && record->rec_format);
		// copy the record
		null_rec = FB_NEW_RPT(record->rec_pool, record->rec_length) Record(record->rec_pool);
		null_rec->rec_length = record->rec_length;
		null_rec->rec_format = record->rec_format;
		// initialize all fields to missing
		null_rec->nullify();
	}

	const Firebird::TimeStamp timestamp =
		request ? request->req_timestamp : Firebird::TimeStamp::getCurrentTimeStamp();

	jrd_req* trigger = NULL;

	try
	{
		for (trig_vec::iterator ptr = vector->begin(); ptr != vector->end(); ++ptr)
		{
			ptr->compile(tdbb);

			trigger = ptr->statement->findRequest(tdbb);

			if (!is_db_trigger)
			{
				if (trigger->req_rpb.getCount() > 0)
				{
					trigger->req_rpb[0].rpb_record = old_rec ? old_rec : null_rec.get();

					if (old_rec)
					{
						trigger->req_rpb[0].rpb_number = old_rpb->rpb_number;
						trigger->req_rpb[0].rpb_number.setValid(true);
					}
					else
						trigger->req_rpb[0].rpb_number.setValid(false);
				}

				if (which_trig == StmtNode::PRE_TRIG &&
					trigger_action == jrd_req::req_trigger_update)
				{
					new_rpb->rpb_number = old_rpb->rpb_number;
				}

				if (trigger->req_rpb.getCount() > 1)
				{
					trigger->req_rpb[1].rpb_record = new_rec ? new_rec : null_rec.get();

					if (new_rec)
					{
						trigger->req_rpb[1].rpb_number = new_rpb->rpb_number;
						trigger->req_rpb[1].rpb_number.setValid(true);
					}
					else
						trigger->req_rpb[1].rpb_number.setValid(false);
				}
			}

			trigger->req_timestamp = timestamp;
			trigger->req_trigger_action = trigger_action;

			TraceTrigExecute trace(tdbb, trigger, which_trig);

			EXE_start(tdbb, trigger, transaction);

			const bool ok = (trigger->req_operation != jrd_req::req_unwind);
			trace.finish(ok ? res_successful : res_failed);

			EXE_unwind(tdbb, trigger);
			trigger->req_attachment = NULL;
			trigger->req_flags &= ~req_in_use;

			if (!ok)
				trigger_failure(tdbb, trigger);

			trigger = NULL;
		}

		if (vector != *triggers)
			MET_release_triggers(tdbb, &vector);
	}
	catch (const Firebird::Exception& ex)
	{
		if (vector != *triggers)
			MET_release_triggers(tdbb, &vector);

		if (trigger)
		{
			EXE_unwind(tdbb, trigger);
			trigger->req_attachment = NULL;
			trigger->req_flags &= ~req_in_use;

			ex.stuff_exception(tdbb->tdbb_status_vector);
			trigger_failure(tdbb, trigger);
		}

		throw;
	}
}


static void stuff_stack_trace(const jrd_req* request)
{
	Firebird::string sTrace;
	bool isEmpty = true;

	for (const jrd_req* req = request; req; req = req->req_caller)
	{
		const JrdStatement* statement = req->getStatement();
		Firebird::string name;

		if (statement->triggerName.length())
		{
			name = "At trigger '";
			name += statement->triggerName.c_str();
		}
		else if (statement->procedure)
		{
			name = statement->parentStatement ? "At sub procedure '" : "At procedure '";
			name += statement->procedure->getName().toString().c_str();
		}
		else if (statement->function)
		{
			name = statement->parentStatement ? "At sub function '" : "At function '";
			name += statement->function->getName().toString().c_str();
		}

		if (! name.isEmpty())
		{
			name.trim();

			if (sTrace.length() + name.length() + 2 > MAX_STACK_TRACE)
				break;

			if (isEmpty)
			{
				isEmpty = false;
				sTrace += name + "'";
			}
			else {
				sTrace += "\n" + name + "'";
			}

			if (req->req_src_line)
			{
				Firebird::string src_info;
				src_info.printf(" line: %"ULONGFORMAT", col: %"ULONGFORMAT,
								req->req_src_line, req->req_src_column);

				if (sTrace.length() + src_info.length() > MAX_STACK_TRACE)
					break;

				sTrace += src_info;
			}
		}
	}

	if (!isEmpty)
		ERR_post_nothrow(Arg::Gds(isc_stack_trace) << Arg::Str(sTrace));
}


const StmtNode* EXE_looper(thread_db* tdbb, jrd_req* request, const StmtNode* node)
{
/**************************************
 *
 *	E X E _ l o o p e r
 *
 **************************************
 *
 * Functional description
 *	Cycle thru request execution tree.  Return next node for
 *	execution on stall or request complete.
 *
 **************************************/
	if (!request->req_transaction)
		ERR_post(Arg::Gds(isc_req_no_trans));

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	jrd_tra* sysTransaction = attachment->getSysTransaction();
	Database* dbb = tdbb->getDatabase();

	if (!node || node->kind != DmlNode::KIND_STATEMENT)
		BUGCHECK(147);

	// Save the old pool and request to restore on exit
	StmtNode::ExeState exeState(tdbb);
	Jrd::ContextPoolHolder context(tdbb, request->req_pool);

	tdbb->setRequest(request);
	tdbb->setTransaction(request->req_transaction);

	fb_assert(request->req_caller == NULL);
	request->req_caller = exeState.oldRequest;

	const SLONG save_point_number = (request->req_transaction->tra_save_point) ?
		request->req_transaction->tra_save_point->sav_number : 0;

	tdbb->tdbb_flags &= ~(TDBB_stack_trace_done | TDBB_sys_error);

	// Execute stuff until we drop

	const JrdStatement* statement = request->getStatement();

	while (node && !(request->req_flags & req_stall))
	{
	try {
		if (request->req_operation == jrd_req::req_evaluate)
		{
			if (--tdbb->tdbb_quantum < 0)
				JRD_reschedule(tdbb, 0, true);

			if (node->hasLineColumn)
			{
				request->req_src_line = node->line;
				request->req_src_column = node->column;
			}
		}

		node = node->execute(tdbb, request, &exeState);

		if (exeState.exit)
			return node;
	}	// try
	catch (const Firebird::Exception& ex)
	{

		ex.stuff_exception(tdbb->tdbb_status_vector);

		request->adjustCallerStats();

		// Ensure the transaction hasn't disappeared in the meantime
		fb_assert(request->req_transaction);

		// Skip this handling for errors coming from the nested looper calls,
		// as they're already handled properly. The only need is to undo
		// our own savepoints.
		if (exeState.catchDisabled)
		{
			tdbb->setTransaction(exeState.oldTransaction);
			tdbb->setRequest(exeState.oldRequest);

			if (request->req_transaction != sysTransaction)
			{
				for (const Savepoint* save_point = request->req_transaction->tra_save_point;
					((save_point) && (save_point_number <= save_point->sav_number));
					save_point = request->req_transaction->tra_save_point)
				{
					++request->req_transaction->tra_save_point->sav_verb_count;
					EXE_verb_cleanup(tdbb, request->req_transaction);
				}
			}

			ERR_punt();
		}

		// If the database is already bug-checked, then get out
		if (dbb->dbb_flags & DBB_bugcheck) {
			Firebird::status_exception::raise(tdbb->tdbb_status_vector);
		}

		// Since an error happened, the current savepoint needs to be undone
		if (request->req_transaction != sysTransaction &&
			request->req_transaction->tra_save_point)
		{
			++request->req_transaction->tra_save_point->sav_verb_count;
			EXE_verb_cleanup(tdbb, request->req_transaction);
		}

		exeState.errorPending = true;
		exeState.catchDisabled = true;
		request->req_operation = jrd_req::req_unwind;
		request->req_label = 0;

		if (!(tdbb->tdbb_flags & TDBB_stack_trace_done) && !(tdbb->tdbb_flags & TDBB_sys_error))
		{
			stuff_stack_trace(request);
			tdbb->tdbb_flags |= TDBB_stack_trace_done;
		}
	}
	} // while()

	request->adjustCallerStats();

	fb_assert(request->req_auto_trans.getCount() == 0);

	// If there is no node, assume we have finished processing the
	// request unless we are in the middle of processing an
	// asynchronous message

	if (!node)
	{
		// Close active cursors
		for (const Cursor* const* ptr = request->req_cursors.begin();
			 ptr < request->req_cursors.end(); ++ptr)
		{
			if (*ptr)
				(*ptr)->close(tdbb);
		}

		request->req_flags &= ~(req_active | req_reserved);
		request->req_timestamp.invalidate();
		release_blobs(tdbb, request);
	}

	request->req_next = node;
	tdbb->setTransaction(exeState.oldTransaction);
	tdbb->setRequest(exeState.oldRequest);

	fb_assert(request->req_caller == exeState.oldRequest);
	request->req_caller = NULL;

	// Ensure the transaction hasn't disappeared in the meantime
	fb_assert(request->req_transaction);

	// In the case of a pending error condition (one which did not
	// result in a exception to the top of looper), we need to
	// delete the last savepoint

	if (exeState.errorPending)
	{
		if (request->req_transaction != sysTransaction)
		{
			for (const Savepoint* save_point = request->req_transaction->tra_save_point;
				((save_point) && (save_point_number <= save_point->sav_number));
				 save_point = request->req_transaction->tra_save_point)
			{
				++request->req_transaction->tra_save_point->sav_verb_count;
				EXE_verb_cleanup(tdbb, request->req_transaction);
			}
		}

		ERR_punt();
	}

	// if the request was aborted, assume that we have already
	// longjmp'ed to the top of looper, and therefore that the
	// last savepoint has already been deleted

	if (request->req_flags & req_abort) {
		ERR_post(Arg::Gds(isc_req_sync));
	}

	return node;
}


// Start looper under Windows SEH (Structured Exception Handling) control
static void looper_seh(thread_db* tdbb, jrd_req* request, const StmtNode* node)
{
#ifdef WIN_NT
	START_CHECK_FOR_EXCEPTIONS(NULL);
#endif
	// TODO:
	// 1. Try to fix the problem with MSVC C++ runtime library, making
	// even C++ exceptions that are implemented in terms of Win32 SEH
	// getting catched by the SEH handler below.
	// 2. Check if it really is correct that only Win32 catches CPU
	// exceptions (such as SEH) here. Shouldn't any platform capable
	// of handling signals use this stuff?
	// (see jrd/ibsetjmp.h for implementation of these macros)

	EXE_looper(tdbb, request, node);

#ifdef WIN_NT
	END_CHECK_FOR_EXCEPTIONS(NULL);
#endif
}


static void release_blobs(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	r e l e a s e _ b l o b s
 *
 **************************************
 *
 * Functional description
 *	Release temporary blobs assigned by this request.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(request, type_req);

	jrd_tra* transaction = request->req_transaction;
	if (transaction)
	{
		DEV_BLKCHK(transaction, type_tra);
		transaction = transaction->getOuter();

		// Release blobs bound to this request

		if (request->req_blobs.getFirst())
		{
			while (true)
			{
				const ULONG blob_temp_id = request->req_blobs.current();
				if (transaction->tra_blobs->locate(blob_temp_id))
				{
					BlobIndex *current = &transaction->tra_blobs->current();
					if (current->bli_materialized)
					{
						request->req_blobs.fastRemove();
						current->bli_request = NULL;
					}
					else
					{
						// Blob was created by request, is accounted for internal needs,
						// but is not materialized. Get rid of it.
						current->bli_blob_object->BLB_cancel(tdbb);
						// Since the routine above modifies req_blobs
						// we need to reestablish accessor position
					}

					if (request->req_blobs.locate(Firebird::locGreat, blob_temp_id))
						continue;

					break;
				}

				// Blob accounting inconsistent, only detected in DEV_BUILD.
				fb_assert(false);

				if (!request->req_blobs.getNext())
					break;
			}
		}

		request->req_blobs.clear();

		// Release arrays assigned by this request

		for (ArrayField** array = &transaction->tra_arrays; *array;)
		{
			DEV_BLKCHK(*array, type_arr);
			if ((*array)->arr_request == request)
				blb::release_array(*array);
			else
				array = &(*array)->arr_next;
		}
	}
}


static void release_proc_save_points(jrd_req* request)
{
/**************************************
 *
 *	r e l e a s e _ p r o c _ s a v e _ p o i n t s
 *
 **************************************
 *
 * Functional description
 *	Release savepoints used by this request.
 *
 **************************************/
	Savepoint* sav_point = request->req_proc_sav_point;

	if (request->req_transaction)
	{
		while (sav_point)
		{
			Savepoint* const temp_sav_point = sav_point->sav_next;
			delete sav_point;
			sav_point = temp_sav_point;
		}
	}
	request->req_proc_sav_point = NULL;
}


static void trigger_failure(thread_db* tdbb, jrd_req* trigger)
{
/**************************************
 *
 *	t r i g g e r _ f a i l u r e
 *
 **************************************
 *
 * Functional description
 *	Trigger failed, report error.
 *
 **************************************/

	SET_TDBB(tdbb);

	if (trigger->req_flags & req_leave)
	{
		trigger->req_flags &= ~req_leave;
		string msg;
		MET_trigger_msg(tdbb, msg, trigger->getStatement()->triggerName, trigger->req_label);
		if (msg.hasData())
		{
			if (trigger->getStatement()->flags & JrdStatement::FLAG_SYS_TRIGGER)
			{
				ISC_STATUS code = PAR_symbol_to_gdscode(msg);
				if (code)
				{
					ERR_post(Arg::Gds(isc_integ_fail) << Arg::Num(trigger->req_label) <<
							 Arg::Gds(code));
				}
			}
			ERR_post(Arg::Gds(isc_integ_fail) << Arg::Num(trigger->req_label) <<
					 Arg::Gds(isc_random) << Arg::Str(msg));
		}
		else
		{
			ERR_post(Arg::Gds(isc_integ_fail) << Arg::Num(trigger->req_label));
		}
	}
	else
	{
		ERR_punt();
	}
}


void EXE_verb_cleanup(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	E X E _ v e r b _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *  If an error happens during the backout of a savepoint, then the transaction
 *  must be marked 'dead' because that is the only way to clean up after a
 *  failed backout. The easiest way to do this is to kill the application
 *  by calling bugcheck.
 *
 **************************************/
	try
	{
		VIO_verb_cleanup(tdbb, transaction);
	}
	catch (const Firebird::Exception&)
	{
		if (tdbb->getDatabase()->dbb_flags & DBB_bugcheck)
			Firebird::status_exception::raise(tdbb->tdbb_status_vector);
		BUGCHECK(290); // msg 290 error during savepoint backout
	}
}
