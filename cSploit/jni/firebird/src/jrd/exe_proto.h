/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe_proto.h
 *	DESCRIPTION:	Prototype header file for exe.cpp
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
 */

#ifndef JRD_EXE_PROTO_H
#define JRD_EXE_PROTO_H

#include "../jrd/cmp_proto.h"

namespace Jrd {
	class jrd_req;
	class jrd_tra;
	class AssignmentNode;
}

void EXE_assignment(Jrd::thread_db*, const Jrd::AssignmentNode*);
void EXE_assignment(Jrd::thread_db*, const Jrd::ValueExprNode*, const Jrd::ValueExprNode*);
void EXE_assignment(Jrd::thread_db* tdbb, const Jrd::ValueExprNode* to, dsc* from_desc, bool from_null,
	const Jrd::ValueExprNode* missing_node, const Jrd::ValueExprNode* missing2_node);

void EXE_execute_db_triggers(Jrd::thread_db*, Jrd::jrd_tra*, enum Jrd::jrd_req::req_ta);
void EXE_execute_ddl_triggers(Jrd::thread_db* tdbb, Jrd::jrd_tra* transaction,
	bool preTriggers, int action);
const Jrd::StmtNode* EXE_looper(Jrd::thread_db* tdbb, Jrd::jrd_req* request,
	const Jrd::StmtNode* in_node);

void EXE_execute_triggers(Jrd::thread_db*, Jrd::trig_vec**, Jrd::record_param*, Jrd::record_param*,
	Jrd::jrd_req::req_ta, Jrd::StmtNode::WhichTrigger);

void EXE_receive(Jrd::thread_db*, Jrd::jrd_req*, USHORT, ULONG, UCHAR*, bool = false);
void EXE_release(Jrd::thread_db*, Jrd::jrd_req*);
void EXE_send(Jrd::thread_db*, Jrd::jrd_req*, USHORT, ULONG, const UCHAR*);
void EXE_start(Jrd::thread_db*, Jrd::jrd_req*, Jrd::jrd_tra*);
void EXE_unwind(Jrd::thread_db*, Jrd::jrd_req*);
void EXE_verb_cleanup(Jrd::thread_db* tdbb, Jrd::jrd_tra* transaction);

namespace Jrd
{
	// ASF: To make this class MT-safe in SS for v3, it should be AutoCacheRequest::release job to
	// inform CMP that the request is available for subsequent usage.
	class AutoCacheRequest
	{
	public:
		AutoCacheRequest(thread_db* tdbb, USHORT aId, USHORT aWhich)
			: id(aId),
			  which(aWhich),
			  request(tdbb->getAttachment()->findSystemRequest(tdbb, id, which))
		{
		}

		AutoCacheRequest()
			: id(0),
			  which(0),
			  request(NULL)
		{
		}

		~AutoCacheRequest()
		{
			release();
		}

	public:
		void reset(thread_db* tdbb, USHORT aId, USHORT aWhich)
		{
			release();

			id = aId;
			which = aWhich;
			request = tdbb->getAttachment()->findSystemRequest(tdbb, id, which);
		}

		void compile(thread_db* tdbb, const UCHAR* blr, ULONG blrLength)
		{
			if (request)
				return;

			request = CMP_compile2(tdbb, blr, blrLength, true);
			cacheRequest();
		}

		jrd_req* operator ->()
		{
			return request;
		}

		operator jrd_req*()
		{
			return request;
		}

		bool operator !() const
		{
			return !request;
		}

	private:
		inline void release()
		{
			if (request)
			{
				EXE_unwind(JRD_get_thread_data(), request);
				request = NULL;
			}
		}

		inline void cacheRequest()
		{
			Jrd::Attachment* att = JRD_get_thread_data()->getAttachment();

			if (which == IRQ_REQUESTS)
				att->att_internal[id] = request->getStatement();
			else if (which == DYN_REQUESTS)
				att->att_dyn_req[id] = request->getStatement();
			else
			{
				fb_assert(false);
			}
		}

	private:
		USHORT id;
		USHORT which;
		jrd_req* request;
	};

	class AutoRequest
	{
	public:
		AutoRequest()
			: request(NULL)
		{
		}

		~AutoRequest()
		{
			release();
		}

	public:
		void reset()
		{
			release();
		}

		void compile(thread_db* tdbb, const UCHAR* blr, ULONG blrLength)
		{
			if (request)
				return;

			request = CMP_compile2(tdbb, blr, blrLength, true);
		}

		jrd_req* operator ->()
		{
			return request;
		}

		operator jrd_req*()
		{
			return request;
		}

		bool operator !() const
		{
			return !request;
		}

	private:
		inline void release()
		{
			if (request)
			{
				CMP_release(JRD_get_thread_data(), request);
				request = NULL;
			}
		}

	private:
		jrd_req* request;
	};
}

#endif // JRD_EXE_PROTO_H
