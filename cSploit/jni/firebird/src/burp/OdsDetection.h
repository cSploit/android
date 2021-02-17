/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		OdsDetection.h
 *	DESCRIPTION:	Detecting the backup (source) or restore (target) ODS
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

#ifndef BURP_ODSDETECTION_H
#define BURP_ODSDETECTION_H

/* Description of currently supported database formats:
DDL8			= 80,	// stored procedures & exceptions & constraints
DDL9			= 90,	// SQL roles
DDL10			= 100,	// FIELD_PRECISION
DDL11			= 110,	// rdb$description in rdb$roles and rdb$generators
						// rdb$base_collation_name and rdb$specific_attributes in rdb$collations
						// rdb$message enlarged to 1021.
DDL11_1			= 111,	// rdb$relation_type in rdb$relations
						// rdb$procedure_type in rdb$procedures
						// rdb$valid_blr in rdb$triggers
						// rdb$valid_blr in rdb$procedures
						// rdb$default_value, rdb$default_source, rdb$collation_id,
						// rdb$null_flag and rdb$parameter_mechanism in rdb$procedure_parameters
DDL11_2			= 112,	// rdb$field_name and rdb$relation_name in rdb$procedure_parameters
						// rdb$admin system role
						// rdb$message enlarged to 1023.
DDL12_0			= 120	// rdb$engine_name and rdb$entrypoint in rdb$triggers
						// rdb$package_name in rdb$dependencies
						// rdb$engine_name, rdb$package_name and rdb$private_flag
						// in rdb$functions
						// rdb$package_name in rdb$function_arguments
						// rdb$engine_name, rdb$entry_point, rdb$package_name
						// and rdb$private_flag in rdb$procedures
						// rdb$package_name in rdb$procedure_parameters
						// rdb$package_name in mon$call_stack
						// Table rdb$packages
						// Type of rdb$triggers.rdb$trigger_type changed from SMALLINT to BIGINT

ASF: Engine that works with ODS11.1 and newer supports access to non-existent system fields.
Reads return NULL and writes do nothing.
*/

//const int DB_VERSION_DDL4		= 40;  // ods4 db
//const int DB_VERSION_DDL5		= 50;  // ods5 db
const int DB_VERSION_DDL8		= 80;  // ods8 db, IB4
const int DB_VERSION_DDL9		= 90;  // ods9 db, IB5
const int DB_VERSION_DDL10		= 100; // ods10 db, IB6, FB1, FB1.5
const int DB_VERSION_DDL11		= 110; // ods11 db, FB2
const int DB_VERSION_DDL11_1	= 111; // ods11.1 db, FB2.1
const int DB_VERSION_DDL11_2	= 112; // ods11.2 db, FB2.5
const int DB_VERSION_DDL12		= 120; // ods12.0 db, FB3.0

const int DB_VERSION_OLDEST_SUPPORTED = DB_VERSION_DDL8;  // IB4.0 is ods8

void detectRuntimeODS();

#endif // BURP_ODSDETECTION_H
