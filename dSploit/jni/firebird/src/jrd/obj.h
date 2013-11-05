/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		obj.h
 *	DESCRIPTION:	Object types in meta-data
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

#ifndef JRD_OBJ_H
#define JRD_OBJ_H

/* Object types used in RDB$DEPENDENCIES and RDB$USER_PRIVILEGES */
/* Note: some values are hard coded in grant.gdl */

const int obj_relation			= 0;
const int obj_view				= 1;
const int obj_trigger			= 2;
const int obj_computed			= 3;
const int obj_validation		= 4;
const int obj_procedure			= 5;
const int obj_expression_index	= 6;
const int obj_exception			= 7;
const int obj_user				= 8;
const int obj_field				= 9;
const int obj_index				= 10;
// const int obj_count				= 11;
const int obj_user_group		= 12;
const int obj_sql_role			= 13;
const int obj_generator			= 14;
const int obj_udf				= 15;
const int obj_blob_filter		= 16;
const int obj_collation			= 17;

// keep this last !
const int obj_type_MAX			= 18;

#endif /* JRD_OBJ_H */

