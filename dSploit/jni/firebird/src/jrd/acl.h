/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		acl.h
 *	DESCRIPTION:	Access control list definitions
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

#ifndef JRD_ACL_H
#define JRD_ACL_H

// CVC: The correct type for these ACL_ and privileges seems to be UCHAR instead
// of int, based on usage, but they aren't coherent either with scl.epp's
// P_NAMES.p_names_acl that's USHORT.

const int ACL_version	= 1;

const int ACL_end		= 0;
const int ACL_id_list	= 1;
const int ACL_priv_list	= 2;

// Privileges to be granted

const int priv_end			= 0;
const int priv_control		= 1;		// Control over ACL
const int priv_grant		= 2;		// Grant subset of rights to others
const int priv_delete		= 3;		// Delete object
const int priv_read			= 4;		// Read object
const int priv_write		= 5;		// Write object
const int priv_protect		= 6;		// Change protection
const int priv_sql_insert	= 7;		// SQL insertion
const int priv_sql_delete	= 8;		// SQL deletion
const int priv_sql_update	= 9;		// SQL update
const int priv_sql_references	= 10;	// SQL references for foreign key
const int priv_execute		= 11;		// Execute (procedure)
const int priv_max			= 12;

// Identification criterias

const int id_end			= 0;
const int id_group			= 1;		// UNIX group id
const int id_user			= 2;		// UNIX user
const int id_person			= 3;		// User name
const int id_project		= 4;		// Project name
const int id_organization	= 5;		// Organization name
const int id_node			= 6;		// Node id
const int id_view			= 7;		// View name
const int id_views			= 8;		// All views
const int id_trigger		= 9;		// Trigger name
const int id_procedure		= 10;		// Procedure name
const int id_sql_role		= 11;		// SQL role
const int id_max			= 12;

/* Format of access control list:

	acl		:=	<ACL_version> [ <acl_element> ]... <0>
	acl_element	:=	<ACL_id_list> <id_list> <ACL_priv_list> <priv_list>
	id_list		:=	[ <id_item> ]... <id_end>
	id_item		:=	<id_criteria> <length> [<name>]
	priv_list	:=	[ <privilege> ]... <priv_end>

*/


// Transaction Description Record
// CVC: This information should match enum tdr_vals in alice.h

const int TDR_VERSION			= 1;
const int TDR_HOST_SITE			= 1;
const int TDR_DATABASE_PATH		= 2;
const int TDR_TRANSACTION_ID	= 3;
const int TDR_REMOTE_SITE		= 4;

#endif // JRD_ACL_H
