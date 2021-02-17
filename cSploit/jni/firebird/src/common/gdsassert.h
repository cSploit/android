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

#ifndef COMMON_GDSASSERT_H
#define COMMON_GDSASSERT_H

#include "../yvalve/gds_proto.h"

#ifdef DEV_BUILD

#include <stdlib.h>		// abort()
#include <stdio.h>

#ifdef WIN_NT
#include <io.h> 		// isatty()
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>		// isatty()
#endif

inline void fb_assert_impl(const char* msg, const char* file, int line, bool do_abort)
{
	const char* const ASSERT_FAILURE_STRING = "Assertion (%s) failure: %s %"LINEFORMAT"\n";

	if (isatty(2))
		fprintf(stderr, ASSERT_FAILURE_STRING, msg, file, line);
	else
		gds__log(ASSERT_FAILURE_STRING, msg, file, line);

	if (do_abort)
		abort();
}

#if !defined(fb_assert)

#define fb_assert(ex) \
	((void) ( !(ex) && (fb_assert_impl(#ex, __FILE__, __LINE__, true), 1) ))

#define fb_assert_continue(ex) \
	((void) ( !(ex) && (fb_assert_impl(#ex, __FILE__, __LINE__, false), 1) ))

#endif	// fb_assert

#else	// DEV_BUILD

#define fb_assert(ex) \
	((void) 0)

#define fb_assert_continue(ex) \
	((void) 0)

#endif	// DEV_BUILD

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

#endif // COMMON_GDSASSERT_H
