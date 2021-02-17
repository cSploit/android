/*
 *      PROGRAM:        Firebird server manager
 *      MODULE:         srvrmgr_proto.h
 *      DESCRIPTION:    Prototype header file for srvrmgr.cpp
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

#ifndef SRVRMGR_PROTO_H
#define SRVRMGR_PROTO_H

void 	SRVRMGR_cleanup (ibmgr_data_t*);
USHORT	SRVRMGR_exec_line (ibmgr_data_t*);
void 	SRVRMGR_msg_get (USHORT, TEXT*);

#endif // SRVRMGR_PROTO_H

