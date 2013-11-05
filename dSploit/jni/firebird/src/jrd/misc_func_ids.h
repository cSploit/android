/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_MISC_FUNC_IDS_H
#define JRD_MISC_FUNC_IDS_H

enum internal_info_id
{
	internal_unknown = 0,
	internal_connection_id = 1,
	internal_transaction_id = 2,
	internal_gdscode = 3,
	internal_sqlcode = 4,
	internal_rows_affected = 5,
	internal_trigger_action = 6,
	internal_sqlstate = 7,
	max_internal_id
};

#endif // JRD_MISC_FUNC_IDS_H
