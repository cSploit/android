/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		perf_proto.h
 *	DESCRIPTION:	Prototype header file for perf.cpp
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

#ifndef JRD_PERF_PROTO_H
#define JRD_PERF_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

int		API_ROUTINE perf_format(const struct perf*, const struct perf*,
								const SCHAR*, SCHAR*, SSHORT*);
void	API_ROUTINE perf_get_info(FB_API_HANDLE*, struct perf*);
void	API_ROUTINE perf_report(const struct perf*, const struct perf*, SCHAR*, SSHORT*);


int		API_ROUTINE perf64_format(const struct perf64*, const struct perf64*,
								const SCHAR*, SCHAR*, SSHORT*);
void	API_ROUTINE perf64_get_info(FB_API_HANDLE*, struct perf64*);
void	API_ROUTINE perf64_report(const struct perf64*, const struct perf64*, SCHAR*, SSHORT*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // JRD_PERF_PROTO_H
