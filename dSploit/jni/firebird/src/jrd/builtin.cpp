/*
 *	PROGRAM:	InterBase Access Method
 *	MODULE:		builtin.cpp
 *	DESCRIPTION:	Entry points for builtin UDF library
 *
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
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/flu.h"
#include "../jrd/common.h"
#include "../jrd/flu_proto.h"
#include "../jrd/gds_proto.h"

struct FN
{
	const TEXT* fn_module;
	const TEXT* fn_entrypoint;
	FPTR_INT fn_function;
};

static const FN isc_builtin_functions[] =
{
	// Internal functions available for QA testing only
	/*
	"DEBUG_CRASH_TESTS", "TEST1", QATEST_entrypoint,

	and so shall it be, *NEVER* include this in a production build
	removed this ugly security hole
	FSG 18.Dez.2000
	*/


	{NULL, NULL, NULL}			// End of list marker
};


FPTR_INT BUILTIN_entrypoint(const TEXT* module, const TEXT* entrypoint)
{
/**************************************
 *
 *	B U I L T I N _ e n t r y p o i n t
 *
 **************************************
 *
 * Functional description
 *	Lookup function in hardcoded table.  The module and
 *	entrypoint names are null terminated, but may contain
 *	insignificant trailing blanks.
 *
 *	Builtin functions may reside under the Firebird install
 *	location.  The module name may be prefixed with $FIREBIRD.
 *
 **************************************/

	// Strip off any preceeding $FIREBIRD path location from the requested module name.

	const TEXT* modname = module;

	TEXT temp[MAXPATHLEN];
	gds__prefix(temp, "");
	TEXT* p = temp;
	for (p = temp; *p; p++, modname++)
	{
		if (*p != *modname)
			break;
	}

	if (!*p)
		module = modname;

	// Strip off any trailing spaces from module name

	p = temp;

	while (*module && *module != ' ')
		*p++ = *module++;

	*p++ = 0;

	// Strip off any trailing spaces from entrypoint name

	const TEXT* ep = p;

	while (*entrypoint && *entrypoint != ' ')
		*p++ = *entrypoint++;

	*p = 0;

	// Scan the list for a matching (module, entrypoint) name

	for (const FN* function = isc_builtin_functions; function->fn_module; ++function)
	{
		if (!strcmp(temp, function->fn_module) && !strcmp(ep, function->fn_entrypoint))
		{
			return function->fn_function;
		}
	}

	return NULL;
}

