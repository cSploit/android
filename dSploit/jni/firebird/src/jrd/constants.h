/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		constants.h
 *	DESCRIPTION:	Misc system constants
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
 * 2001.10.08 Claudio Valderrama: fb_sysflag enum with numbering
 *   for automatically created triggers that aren't system triggers.
 */

#ifndef JRD_CONSTANTS_H
#define JRD_CONSTANTS_H

/* BLOb Subtype definitions */

/* Subtypes < 0  are user defined
 * Subtype  0    means "untyped"
 * Subtypes > 0  are Firebird defined
 */

// BRS 29-Apr-2004
// replace those constants with public defined ones isc_blob_*
//
//const int BLOB_untyped	= 0;
//
//const int BLOB_text		= 1;
//const int BLOB_blr		= 2;
//const int BLOB_acl		= 3;
//const int BLOB_ranges	= 4;
//const int BLOB_summary	= 5;
//const int BLOB_format	= 6;
//const int BLOB_tra		= 7;
//const int BLOB_extfile	= 8;
//const int BLOB_max_predefined_subtype = 9;
//

/* Column Limits */

const ULONG MAX_COLUMN_SIZE	= 32767;	/* Bytes */

/* Misc constant values */

const unsigned int USERNAME_LENGTH	= 31;	// Characters; beware that USER_NAME_LEN = 133 in gsec.h

const size_t MAX_SQL_IDENTIFIER_SIZE = 32;
const size_t MAX_SQL_IDENTIFIER_LEN = MAX_SQL_IDENTIFIER_SIZE - 1;
typedef TEXT SqlIdentifier[MAX_SQL_IDENTIFIER_SIZE];

const char* const NULL_STRING_MARK = "*** null ***";
const char* const UNKNOWN_STRING_MARK = "*** unknown ***";

const char* const ISC_USER = "ISC_USER";
const char* const ISC_PASSWORD = "ISC_PASSWORD";

const char* const NULL_ROLE = "NONE";
const char* const ADMIN_ROLE = "RDB$ADMIN";

// User name assigned to any user granted USR_locksmith rights.
// If this name is changed, modify also the trigger in
// jrd/grant.gdl (which turns into jrd/trig.h.
const char* const SYSDBA_USER_NAME = "SYSDBA";

// This temporary set of flags is needed to implement minimum form of
// ALTER ROLE RDB$ADMIN ADD/DROP SYSTEM_NAME "Domain Admins".
// Value 1 is skipped because rdb$system_flag = 1 is used in all other cases.
const SSHORT ROLE_FLAG_MAY_TRUST	= 2;
const SSHORT ROLE_FLAG_DBO			= 4;

const char* const PRIMARY_KEY		= "PRIMARY KEY";
const char* const FOREIGN_KEY		= "FOREIGN KEY";
const char* const UNIQUE_CNSTRT		= "UNIQUE";
const char* const CHECK_CNSTRT		= "CHECK";
const char* const NOT_NULL_CNSTRT	= "NOT NULL";

const char* const REL_SCOPE_PERSISTENT		= "persistent table \"%s\"";
const char* const REL_SCOPE_GTT_PRESERVE	= "global temporary table \"%s\" of type ON COMMIT PRESERVE ROWS";
const char* const REL_SCOPE_GTT_DELETE		= "global temporary table \"%s\" of type ON COMMIT DELETE ROWS";

/* literal strings in rdb$ref_constraints to be used to identify
   the cascade actions for referential constraints. Used
   by isql/show and isql/extract for now. */

const char* const RI_ACTION_CASCADE = "CASCADE";
const char* const RI_ACTION_NULL    = "SET NULL";
const char* const RI_ACTION_DEFAULT = "SET DEFAULT";
const char* const RI_ACTION_NONE    = "NO ACTION";
const char* const RI_RESTRICT       = "RESTRICT";

// Automatically created domains for fields with direct data type.
// Also, automatically created indices that are unique or non-unique, but not PK.
const char* const IMPLICIT_DOMAIN_PREFIX = "RDB$";
const int IMPLICIT_DOMAIN_PREFIX_LEN = 4;

// Automatically created indices for PKs.
const char* const IMPLICIT_PK_PREFIX = "RDB$PRIMARY";
const int IMPLICIT_PK_PREFIX_LEN = 11;

// Automatically created security classes for SQL objects.
// Keep in sync with trig.h
const char* const DEFAULT_CLASS				= "SQL$DEFAULT";
const char* const SQL_SECCLASS_GENERATOR	= "RDB$SECURITY_CLASS";
const char* const SQL_SECCLASS_PREFIX		= "SQL$";
const int SQL_SECCLASS_PREFIX_LEN			= 4;
const char* const SQL_FLD_SECCLASS_PREFIX	= "SQL$GRANT";
const int SQL_FLD_SECCLASS_PREFIX_LEN		= 9;

// Automatically created check constraints for unnamed PRIMARY and UNIQUE declarations.
const char* const IMPLICIT_INTEGRITY_PREFIX = "INTEG_";
const int IMPLICIT_INTEGRITY_PREFIX_LEN = 6;


/******************************************/
/* System flag meaning - mainly Firebird. */
/******************************************/

enum fb_sysflag {
	fb_sysflag_user = 0,
	fb_sysflag_system = 1,
	fb_sysflag_qli = 2,
	fb_sysflag_check_constraint = 3,
	fb_sysflag_referential_constraint = 4,
	fb_sysflag_view_check = 5
};


/* UDF Arguments are numbered from 0 to MAX_UDF_ARGUMENTS --
   argument 0 is reserved for the return-type of the UDF */

const int MAX_UDF_ARGUMENTS	= 10;

// Maximum length of single line returned from pretty printer
const int PRETTY_BUFFER_SIZE = 1024;

const int MAX_INDEX_SEGMENTS = 16;

// Maximum index key length
// AB: If the maximum key-size will change, don't forget dyn.h and dba.epp
// which cannot use these defines.
const ULONG MAX_KEY			= 4096;		// Maximum page size possible divide by 4 (16384 / 4)
const int MAX_KEY_PRE_ODS11	= 255;		// Max key-size before ODS11

const USHORT SQL_MATCH_1_CHAR		= '_';	/* Not translatable */
const USHORT SQL_MATCH_ANY_CHARS	= '%';	/* Not translatable */

const size_t MAX_CONTEXT_VARS	= 1000;		// Maximum number of context variables allowed for a single object

// Time precision limits and defaults for TIME/TIMESTAMP values.
// Currently they're applied to CURRENT_TIME[STAMP] expressions only.

// Should be more than 6 as per SQL spec, but we don't support more than 3 yet
const size_t MAX_TIME_PRECISION			= 3;
// Consistent with the SQL spec
const size_t DEFAULT_TIME_PRECISION		= 0;
// Should be 6 as per SQL spec
const size_t DEFAULT_TIMESTAMP_PRECISION	= 3;

const size_t MAX_ARRAY_DIMENSIONS = 16;

const size_t MAX_SORT_ITEMS = 255; // ORDER BY f1,...,f255

const int MAX_TABLE_VERSIONS = 255; // maybe this should be in ods.h.

const size_t MAX_DB_PER_TRANS = 256; // A multi-db txn can span up to 256 dbs

// relation types

enum rel_t {
	rel_persistent = 0,
	rel_view = 1,
	rel_external = 2,
	rel_virtual = 3,
	rel_global_temp_preserve = 4,
	rel_global_temp_delete = 5
};

// procedure types

enum prc_t {
	prc_legacy = 0,
	prc_selectable = 1,
	prc_executable = 2
};

// procedure parameter mechanism

enum prm_mech_t {
	prm_mech_normal = 0,
	prm_mech_type_of = 1
};

// states

enum mon_state_t {
	mon_state_idle = 0,
	mon_state_active = 1,
	mon_state_stalled = 2
};

// shutdown modes

enum shut_mode_t {
	shut_mode_online = 0,
	shut_mode_multi = 1,
	shut_mode_single = 2,
	shut_mode_full = 3
};

// backup states

enum backup_state_t {
	backup_state_unknown = -1,
	backup_state_normal = 0,
	backup_state_stalled = 1,
	backup_state_merge = 2
};

// transaction isolation levels

enum tra_iso_mode_t {
	iso_mode_consistency = 0,
	iso_mode_concurrency = 1,
	iso_mode_rc_version = 2,
	iso_mode_rc_no_version = 3
};

// statistics groups

enum stat_group_t {
	stat_database = 0,
	stat_attachment = 1,
	stat_transaction = 2,
	stat_statement = 3,
	stat_call = 4
};

const int TRIGGER_TYPE_SHIFT		= 13;
const int TRIGGER_TYPE_MASK			= (0x3 << TRIGGER_TYPE_SHIFT);

const int TRIGGER_TYPE_DML			= (0 << TRIGGER_TYPE_SHIFT);
const int TRIGGER_TYPE_DB			= (1 << TRIGGER_TYPE_SHIFT);
//const int TRIGGER_TYPE_DDL		= (2 << TRIGGER_TYPE_SHIFT);

const int DB_TRIGGER_CONNECT		= 0;
const int DB_TRIGGER_DISCONNECT		= 1;
const int DB_TRIGGER_TRANS_START	= 2;
const int DB_TRIGGER_TRANS_COMMIT	= 3;
const int DB_TRIGGER_TRANS_ROLLBACK	= 4;
const int DB_TRIGGER_MAX			= 5;

// that's how database trigger action types are encoded
//    (TRIGGER_TYPE_DB | type)

// switches for username and password used when an username and/or password
// is specified by the client application
#define USERNAME_SWITCH "USER"
#define PASSWORD_SWITCH "PASSWORD"
#define TRUSTED_USER_SWITCH "TRUSTED_SVC"
#define TRUSTED_USER_SWITCH_LEN (sizeof(TRUSTED_USER_SWITCH) - 1)
#define TRUSTED_ROLE_SWITCH "TRUSTED_ROLE"
#define TRUSTED_ROLE_SWITCH_LEN (sizeof(TRUSTED_ROLE_SWITCH) - 1)

#endif // JRD_CONSTANTS_H
