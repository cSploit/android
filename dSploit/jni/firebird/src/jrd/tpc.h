/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tpc.h
 *	DESCRIPTION:	Transaction Inventory Page Cache
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

#ifndef JRD_TPC_H
#define JRD_TPC_H

namespace Jrd {

class TxPageCache : public pool_alloc_rpt<SCHAR, type_tpc>
{
public:
	TxPageCache*	tpc_next;
	SLONG			tpc_base;				/* id of first transaction in this block */
	UCHAR			tpc_transactions[1];	/* two bits per transaction */
};

} //namespace Jrd

#endif /* JRD_TPC_H */
