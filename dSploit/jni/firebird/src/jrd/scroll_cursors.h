/*
 *	MODULE:		scroll_cursors.h
 *	DESCRIPTION:	OSRI entrypoints and defines for SCROLLABLE_CURSORS
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

#ifndef JRD_SCROLL_CURSORS_H
#define JRD_SCROLL_CURSORS_H

/* ALL THE FOLLOWING DEFINITIONS HAVE BEEN TAKEN OUT OF JRD/IBASE.H
   WHEN SCROLLABLE_CURSORS ARE TOTALLY IMPLEMENTED, THE FOLLOWING
   DEFINITIONS NEED TO BE PUT IN THE PROPER INCLUDE FILE.

   This was done so that IB 5.0 customers do not see any functions
   they are not supposed to see.
   As per Engg. team's decision on 23-Sep-1997
*/

#ifdef SCROLLABLE_CURSORS
ISC_STATUS ISC_EXPORT isc_dsql_fetch2(ISC_STATUS *,
									  isc_stmt_handle *,
									  unsigned short,
									  XSQLDA *,
									  unsigned short, signed long);
#endif

#ifdef SCROLLABLE_CURSORS
ISC_STATUS ISC_EXPORT isc_dsql_fetch2_m(ISC_STATUS*,
										isc_stmt_handle*,
										unsigned short,
										const char*,
										unsigned short,
										unsigned short,
										char*,
										unsigned short, signed long);
#endif

#ifdef SCROLLABLE_CURSORS
ISC_STATUS ISC_EXPORT isc_embed_dsql_fetch2(ISC_STATUS*,
											char*,
											unsigned short,
											XSQLDA*,
											unsigned short, signed long);
#endif

#ifdef SCROLLABLE_CURSORS
ISC_STATUS ISC_EXPORT isc_receive2(ISC_STATUS *,
								   isc_req_handle *,
								   short,
								   short,
								   void *,
								   short, unsigned short, unsigned long);
#endif

/****** Add the following commented lines in the #else part of..
#else                                    __cplusplus || __STDC__
ISC_STATUS  ISC_EXPORT isc_receive2();
******/

/****************************************/
/* Scroll direction for isc_dsql_fetch2 */
/****************************************/

const int isc_fetch_next		= 0;
const int isc_fetch_prior		= 1;
const int isc_fetch_first		= 2;
const int isc_fetch_last		= 3;
const int isc_fetch_absolute	= 4;
const int isc_fetch_relative	= 5;

#endif /* JRD_SCROLL_CURSORS_H */
