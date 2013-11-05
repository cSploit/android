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

#include <stdlib.h>
#include "ib_udf.h"
#include "firebird.h"
#include "../jrd/common.h"

typedef void* VoidPtr;


// initialized by the engine
static void* (*allocFunc)(long) = NULL;

extern "C" void FB_DLL_EXPORT ib_util_init(void* (*aAllocFunc)(long))
{
	allocFunc = aAllocFunc;
}

extern "C" VoidPtr FB_DLL_EXPORT ib_util_malloc(long size)
{
	return allocFunc ? allocFunc(size) : malloc(size);
}
