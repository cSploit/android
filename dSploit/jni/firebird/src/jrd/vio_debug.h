/* 	PROGRAM:	JRD Access Method
 *	MODULE:		vio_debug.h
 *	DESCRIPTION:	Definitions for tracing VIO activity
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

#ifndef JRD_VIO_DEBUG_H
#define JRD_VIO_DEBUG_H

const int DEBUG_WRITES			= 1;
const int DEBUG_WRITES_INFO		= 2;
const int DEBUG_READS			= 3;
const int DEBUG_READS_INFO		= 4;
const int DEBUG_TRACE			= 5;
const int DEBUG_TRACE_INFO		= 6;
const int DEBUG_TRACE_ALL		= 7;
const int DEBUG_TRACE_ALL_INFO	= 8;

static int debug_flag;

#endif /* JRD_VIO_DEBUG_H */

