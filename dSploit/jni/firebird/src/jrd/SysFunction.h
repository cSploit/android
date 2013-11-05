/*
 *	PROGRAM:	JRD System Functions
 *	MODULE:		SysFunctions.h
 *	DESCRIPTION:	System Functions
 *
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
 *  for the Firebird Open Source RDBMS project, based on work done
 *  in Yaffil by Oleg Loa <loa@mail.ru> and Alexey Karyakin <aleksey.karyakin@mail.ru>
 *
 *  Copyright (c) 2007 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *    Oleg Loa <loa@mail.ru>
 *    Alexey Karyakin <aleksey.karyakin@mail.ru>
 *
 */

#ifndef JRD_SYSFUNCTION_H
#define JRD_SYSFUNCTION_H

#include "../common/classes/MetaName.h"
#include "../jrd/DataTypeUtil.h"
#include "../jrd/dsc.h"

namespace Jrd
{
	class thread_db;
	class jrd_nod;
	struct impure_value;
}


class SysFunction
{
public:
	typedef void (*SetParamsFunc)(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int, dsc**);
	typedef void (*MakeFunc)(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc*, int, const dsc**);
	typedef dsc* (*EvlFunc)(Jrd::thread_db*, const SysFunction* function, Jrd::jrd_nod*, Jrd::impure_value*);

	const Firebird::MetaName name;
	int minArgCount;
	int maxArgCount;	// -1 for no limit
	SetParamsFunc setParamsFunc;
	MakeFunc makeFunc;
	EvlFunc evlFunc;
	void* misc;

	static const SysFunction* lookup(const Firebird::MetaName& name);
	static dsc* substring(Jrd::thread_db* tdbb, Jrd::impure_value* impure,
		dsc* value, const dsc* offset_value, const dsc* length_value);

	void checkArgsMismatch(int count) const;

private:
	const static SysFunction functions[];
};


#endif	// JRD_SYSFUNCTION_H

