/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		lls.h
 *	DESCRIPTION:	most commonly used in Jrd stacks
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
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_LLS_H
#define JRD_LLS_H

#include "../common/classes/stack.h"

namespace Jrd {
	class Record;
	class jrd_nod;
	class RecordSource;
	class River;
	typedef Firebird::Stack<Record*> RecordStack;
	typedef Firebird::Stack<jrd_nod*> NodeStack;
	typedef Firebird::Stack<SLONG> PageStack;
	typedef Firebird::Stack<UCHAR*> UCharStack;
	typedef Firebird::Stack<RecordSource*> RsbStack;
	typedef Firebird::Stack<River*> RiverStack;
	typedef Firebird::Stack<UCHAR> StreamStack;
}

#endif // JRD_LLS_H
