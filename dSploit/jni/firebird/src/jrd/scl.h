/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		scl.h
 *	DESCRIPTION:	Security class definitions
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

#ifndef JRD_SCL_H
#define JRD_SCL_H

#include "../common/classes/MetaName.h"
#include "../common/classes/tree.h"

namespace Jrd {

const size_t ACL_BLOB_BUFFER_SIZE = MAX_USHORT; /* used to read/write acl blob */

// Security class definition

class SecurityClass
{
public:
    typedef USHORT flags_t;

	SecurityClass(Firebird::MemoryPool &pool, const Firebird::MetaName& name)
		: scl_flags(0), scl_name(pool, name)
	{}

	flags_t scl_flags;			// Access permissions
	const Firebird::MetaName scl_name;

	static const Firebird::MetaName& generate(const void*, const SecurityClass* item)
	{
		return item->scl_name;
	}
};

typedef Firebird::BePlusTree<
	SecurityClass*,
	Firebird::MetaName,
	Firebird::MemoryPool,
	SecurityClass
> SecurityClassList;


const SecurityClass::flags_t SCL_read			= 1;		/* Read access */
const SecurityClass::flags_t SCL_write			= 2;		/* Write access */
const SecurityClass::flags_t SCL_delete			= 4;		/* Delete access */
const SecurityClass::flags_t SCL_control		= 8;		/* Control access */
const SecurityClass::flags_t SCL_grant			= 16;		/* Grant privileges */
const SecurityClass::flags_t SCL_exists			= 32;		/* At least ACL exists */
const SecurityClass::flags_t SCL_scanned		= 64;		/* But we did look */
const SecurityClass::flags_t SCL_protect		= 128;		/* Change protection */
const SecurityClass::flags_t SCL_corrupt		= 256;		/* ACL does look too good */
const SecurityClass::flags_t SCL_sql_insert		= 512;
const SecurityClass::flags_t SCL_sql_delete		= 1024;
const SecurityClass::flags_t SCL_sql_update		= 2048;
const SecurityClass::flags_t SCL_sql_references	= 4096;
const SecurityClass::flags_t SCL_execute		= 8192;



/* information about the user */

const USHORT USR_locksmith	= 1;		/* User has great karma */
const USHORT USR_dba		= 2;		/* User has DBA privileges */
const USHORT USR_owner		= 4;		/* User owns database */
const USHORT USR_trole		= 8;		/* Role was set by trusted auth */


class UserId
{
public:
	Firebird::string	usr_user_name;		/* User name */
	Firebird::string	usr_sql_role_name;	/* Role name */
	Firebird::string	usr_project_name;	/* Project name */
	Firebird::string	usr_org_name;		/* Organization name */
	USHORT				usr_user_id;		/* User id */
	USHORT				usr_group_id;		/* Group id */
	USHORT				usr_node_id;		/* Node id */
	USHORT				usr_flags;			/* Misc. crud */
	bool				usr_fini_sec_db;	/* Security database was initialized for it */

	bool locksmith() const
	{
		return usr_flags & (USR_locksmith | USR_owner | USR_dba);
	}

	UserId()
		: usr_user_id(0), usr_group_id(0), usr_node_id(0), usr_flags(0), usr_fini_sec_db(false)
	{ }

	UserId(Firebird::MemoryPool& p, const UserId& ui)
		: usr_user_name(p, ui.usr_user_name),
		  usr_sql_role_name(p, ui.usr_sql_role_name),
		  usr_project_name(p, ui.usr_project_name),
		  usr_org_name(p, ui.usr_org_name),
		  usr_user_id(ui.usr_user_id),
		  usr_group_id(ui.usr_group_id),
		  usr_node_id(ui.usr_node_id),
		  usr_flags(ui.usr_flags),
		  usr_fini_sec_db(false)
	{ }

	UserId(const UserId& ui)
		: usr_user_name(ui.usr_user_name),
		  usr_sql_role_name(ui.usr_sql_role_name),
		  usr_project_name(ui.usr_project_name),
		  usr_org_name(ui.usr_org_name),
		  usr_user_id(ui.usr_user_id),
		  usr_group_id(ui.usr_group_id),
		  usr_node_id(ui.usr_node_id),
		  usr_flags(ui.usr_flags),
		  usr_fini_sec_db(false)
	{ }

	UserId& operator=(const UserId& ui)
	{
		usr_user_name = ui.usr_user_name;
		usr_sql_role_name = ui.usr_sql_role_name;
		usr_project_name = ui.usr_project_name;
		usr_org_name = ui.usr_org_name;
		usr_user_id = ui.usr_user_id;
		usr_group_id = ui.usr_group_id;
		usr_node_id = ui.usr_node_id;
		usr_flags = ui.usr_flags;

		return *this;
	}

	~UserId();
};

const char* const object_database	= "DATABASE";
const char* const object_table		= "TABLE";
const char* const object_procedure	= "PROCEDURE";
const char* const object_column		= "COLUMN";

} //namespace Jrd

#endif // JRD_SCL_H

