/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		fun_proto.h
 *	DESCRIPTION:	Prototype header file for fun.cpp
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

#ifndef JRD_FUN_PROTO_H
#define JRD_FUN_PROTO_H

namespace Jrd
{
	class CompilerScratch;
}

class IbUtil
{
public:
	static void initialize();

	static void* alloc(long size);
	static bool free(void* ptr);
};


void FUN_evaluate(Jrd::thread_db*, Jrd::UserFunction*, Jrd::jrd_nod*, Jrd::impure_value*);
Jrd::UserFunction* FUN_lookup_function(Jrd::thread_db*, const Firebird::MetaName&); //, bool ShowAccessError);
Jrd::UserFunction* FUN_resolve(Jrd::thread_db*, Jrd::CompilerScratch*, Jrd::UserFunction*, Jrd::jrd_nod*);

#endif // JRD_FUN_PROTO_H

