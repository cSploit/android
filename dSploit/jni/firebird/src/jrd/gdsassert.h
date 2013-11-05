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
 * CVC: Do not override local fb_assert like the ones in gpre and dsql.
 */

#ifndef JRD_GDSASSERT_H
#define JRD_GDSASSERT_H

#include "../jrd/gds_proto.h"

#ifdef DEV_BUILD

#include <stdlib.h>		// abort()
#include <stdio.h>

#define FB_ASSERT_FAILURE_STRING	"Assertion (%s) failure: %s %"LINEFORMAT"\n"

#ifdef SUPERCLIENT

#if !defined(fb_assert)
#define fb_assert(ex)	{if (!(ex)) {fprintf(stderr, FB_ASSERT_FAILURE_STRING, #ex, __FILE__, __LINE__); abort();}}
#define fb_assert_continue(ex)	{if (!(ex)) {fprintf(stderr, FB_ASSERT_FAILURE_STRING, #ex, __FILE__, __LINE__);}}
#endif

#else	// !SUPERCLIENT

#if !defined(fb_assert)
#define fb_assert(ex)	{if (!(ex)) {gds__log(FB_ASSERT_FAILURE_STRING, #ex, __FILE__, __LINE__); abort();}}
#define fb_assert_continue(ex)	{if (!(ex)) {gds__log(FB_ASSERT_FAILURE_STRING, #ex, __FILE__, __LINE__);}}
#endif

#endif	// SUPERCLIENT

#else	// DEV_BUILD

#define fb_assert(ex)				// nothing
#define fb_assert_continue(ex)		// nothing

#endif // DEV_BUILD

namespace DtorException {
	inline void devHalt()
	{
		// If any guard's dtor is executed during exception processing,
		// (remember - this guards live on the stack), exception
		// in leave() causes std::terminate() to be called, therefore
		// losing original exception information. Not good for us.
		// Therefore ignore in release and abort in debug.
#ifdef DEV_BUILD
		abort();
#endif
	}
}

#endif // JRD_GDSASSERT_H

