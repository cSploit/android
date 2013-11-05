/*
 *	PROGRAM:	Alice (All Else) Utility
 *	MODULE:		alice_meta.h
 *	DESCRIPTION:	Prototype header file for alice_meta.epp
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

#ifndef ALICE_ALICE_META_H
#define ALICE_ALICE_META_H

void	MET_disable_wal(ISC_STATUS*, isc_db_handle);
void	MET_get_state(ISC_STATUS*, tdr*);
tdr*	MET_get_transaction(ISC_STATUS*, isc_db_handle, SLONG);
void	MET_set_capabilities(ISC_STATUS*, tdr*);

#endif	// ALICE_ALICE_META_H

