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
 */

#ifndef JRD_FUNCTION_H
#define JRD_FUNCTION_H

#include "../jrd/Routine.h"
#include "../common/classes/array.h"
#include "../common/dsc.h"
#include "../common/classes/NestConst.h"
#include "../jrd/val.h"
#include "../dsql/Nodes.h"

namespace Jrd
{
	class ValueListNode;

	class Function : public Routine
	{
		static const char* const EXCEPTION_MESSAGE;

	public:
		static Function* lookup(thread_db* tdbb, USHORT id, bool return_deleted, bool noscan, USHORT flags);
		static Function* lookup(thread_db* tdbb, const Firebird::QualifiedName& name, bool noscan);

		void releaseLocks(thread_db* tdbb);

		explicit Function(MemoryPool& p)
			: Routine(p),
			  fun_entrypoint(NULL),
			  fun_inputs(0),
			  fun_return_arg(0),
			  fun_temp_length(0),
			  fun_exception_message(p),
			  fun_deterministic(false),
			  fun_external(NULL)
		{
		}

		static Function* loadMetadata(thread_db* tdbb, USHORT id, bool noscan, USHORT flags);
		static int blockingAst(void*);

	public:
		virtual int getObjectType() const
		{
			return obj_udf;
		}

		virtual SLONG getSclType() const
		{
			return SCL_object_function;
		}

		virtual bool checkCache(thread_db* tdbb) const;
		virtual void clearCache(thread_db* tdbb);

	public:
		int (*fun_entrypoint)();				// function entrypoint
		USHORT fun_inputs;						// input arguments
		USHORT fun_return_arg;					// return argument
		ULONG fun_temp_length;					// temporary space required

		Firebird::string fun_exception_message;	// message containing the exception error message

		bool fun_deterministic;
		const ExtEngineManager::Function* fun_external;
	};
}

#endif // JRD_FUNCTION_H
