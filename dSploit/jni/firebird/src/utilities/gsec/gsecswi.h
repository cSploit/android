/*
 *	PROGRAM:	Security data base manager
 *	MODULE:		gsecswi.h
 *	DESCRIPTION:	gsec switches
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

#ifndef GSEC_GSECSWI_H
#define GSEC_GSECSWI_H

#include "../jrd/common.h"
#include "../jrd/constants.h"

/* Switch handling constants.  Note that the first IN_SW_DATA_ITEMS
   switch constants refer to data items.  The remaining switch constants
   refer to actual switches. */

const int IN_SW_GSEC_0				= 0;	// not a known switch
const int IN_SW_GSEC_UID			= 1;	// uid is specified
const int IN_SW_GSEC_GID			= 2;	// gid is specified
const int IN_SW_GSEC_SYSU			= 3;	// sys_user_name is specified
const int IN_SW_GSEC_GROUP			= 4;	// group is specified
const int IN_SW_GSEC_PASSWORD		= 5;	// password is specified
const int IN_SW_GSEC_FNAME			= 6;	// first name is specified
const int IN_SW_GSEC_MNAME			= 7;	// middle name/initial is specified
const int IN_SW_GSEC_LNAME			= 8;	// last name is specified
const int IN_SW_GSEC_ADD			= 9;	// add a user
const int IN_SW_GSEC_DEL			= 10;	// delete a user
const int IN_SW_GSEC_DIS			= 11;	// display user(s)
const int IN_SW_GSEC_MOD			= 12;	// modify a user
const int IN_SW_GSEC_QUIT			= 13;	// quit from interactive session
const int IN_SW_GSEC_HELP			= 14;	// help
const int IN_SW_GSEC_Z				= 15;	// version
const int IN_SW_GSEC_DATABASE		= 16;	// database to manage
const int IN_SW_GSEC_DBA_USER_NAME	= 17;	// Database Admin. User name
const int IN_SW_GSEC_DBA_PASSWORD	= 18;	// Database Admin. Password
const int IN_SW_GSEC_SQL_ROLE_NAME	= 19;	// SQL Role to assume
const int IN_SW_GSEC_AMBIG			= 20;	// ambiguous switch
//const int IN_SW_GSEC_USERNAME		= 21;	// placeholder for the username
const int IN_SW_GSEC_DBA_TRUST_USER	= 22;	// Database Admin. Trusted User name
const int IN_SW_GSEC_DBA_TRUST_ROLE	= 23;	// use trusted role for auth
#ifdef TRUSTED_AUTH
const int IN_SW_GSEC_TRUSTED_AUTH	= 24;	// Use trusted authentication
#endif
const int IN_SW_GSEC_FETCH_PASSWORD	= 25;   // Fetch Password (Database Admin.)
const int IN_SW_GSEC_MAPPING		= 26;	// Change auto admin mapping
const int IN_SW_GSEC_ADMIN			= 27;	// Grant/revoke RDB$ADMIN in security database
const int IN_SW_GSEC_DIS_ADM		= 28;   // display user(s) with admin info

static const struct in_sw_tab_t gsec_in_sw_table [] =
{
    {IN_SW_GSEC_ADD,		0,				"ADD",		0, 0, 0, false,	0,	1, NULL},	// add user
    {IN_SW_GSEC_UID,		isc_spb_sec_userid,		"UID",		0, 0, 0, false,	0,	1, NULL},	// user's ID
    {IN_SW_GSEC_GID,		isc_spb_sec_groupid,		"GID",		0, 0, 0, false,	0,	2, NULL},	// user's group ID
    {IN_SW_GSEC_SYSU,		0,				"SYSU",		0, 0, 0, false,	0,	1, NULL},	// sys_user's name
    {IN_SW_GSEC_GROUP,		isc_spb_sec_groupname,		"GROUP",	0, 0, 0, false,	0,	2, NULL},	// user's group name
    {IN_SW_GSEC_PASSWORD,	isc_spb_sec_password,		"PW",		0, 0, 0, false,	0,	2, NULL},	// user's password
    {IN_SW_GSEC_FNAME,		isc_spb_sec_firstname,		"FNAME",	0, 0, 0, false,	0,	1, NULL},	// user's first name
    {IN_SW_GSEC_MNAME,		isc_spb_sec_middlename,		"MNAME",	0, 0, 0, false,	0,	2, NULL},	// user's middle name/initial
    {IN_SW_GSEC_LNAME,		isc_spb_sec_lastname,		"LNAME",	0, 0, 0, false,	0,	1, NULL},	// user's last name
    {IN_SW_GSEC_DEL,		0,				"DELETE",	0, 0, 0, false,	0,	2, NULL},	// delete user
    {IN_SW_GSEC_DIS,		0,			"OLD_DISPLAY",	0, 0, 0, false,	0, 11, NULL},	// display user(s)
    {IN_SW_GSEC_DIS_ADM,	0,				"DISPLAY",	0, 0, 0, false,	0,	2, NULL},	// display user(s) with admin info
    {IN_SW_GSEC_MOD,		0,				"MODIFY",	0, 0, 0, false,	0,	2, NULL},	// modify user
    {IN_SW_GSEC_QUIT,		0,				"QUIT",		0, 0, 0, false,	0,	1, NULL},	// exit command line interface
    {IN_SW_GSEC_HELP,		0,				"HELP",		0, 0, 0, false,	0,	1, NULL},	// print help
    {IN_SW_GSEC_Z,			0,				"Z",		0, 0, 0, false,	0,	1, NULL},	// version
    {IN_SW_GSEC_DATABASE,	isc_spb_dbname,	"DATABASE",	0, 0, 0, false,	0,	2, NULL},	// specify database to use
    {IN_SW_GSEC_DBA_USER_NAME,	0,			"USER",		0, 0, 0, false,	0,	1, NULL},	// Database Admin. User name
    {IN_SW_GSEC_DBA_PASSWORD, 	0,			"PASSWORD",	0, 0, 0, false,	0,	2, NULL},	// Database Admin. Password
    {IN_SW_GSEC_FETCH_PASSWORD,	0,			"FETCH_PASSWORD", 0, 0, 0, false, 0, 2, NULL},	// Fetch Database Admin. Password
    {IN_SW_GSEC_SQL_ROLE_NAME,	isc_spb_sql_role_name,		"ROLE",		0, 0, 0, false,	0,	2, NULL},	// SQL Role to assume
	{IN_SW_GSEC_DBA_TRUST_USER,	0,			TRUSTED_USER_SWITCH,	0, 0, 0, false,	0,
											TRUSTED_USER_SWITCH_LEN, NULL},				// Database Admin. Trusted User name
	{IN_SW_GSEC_DBA_TRUST_ROLE,	0,			TRUSTED_ROLE_SWITCH,	0, 0, 0, false,	0,
											TRUSTED_ROLE_SWITCH_LEN, NULL},				// Use trusted role for auth
#ifdef TRUSTED_AUTH
	{IN_SW_GSEC_TRUSTED_AUTH,	0,			"TRUSTED",	0, 0, 0, false,	0,	1, NULL},	// Database Admin. Trusted User name
#endif
	{IN_SW_GSEC_MAPPING,		0,			"MAPPING",	0, 0, 0, false, 0,	2, NULL},	// Change mapping of domain admins to sysdba in sec. DB
	{IN_SW_GSEC_ADMIN,	isc_spb_sec_admin,	"ADMIN",	0, 0, 0, false, 0,	3, NULL},	// Grant/revoke RDB$ADMIN in security database
    {IN_SW_GSEC_0,				0,			NULL,		0, 0, 0, false,	0,	0, NULL}	// End of List
};

static const struct in_sw_tab_t gsec_action_in_sw_table [] =
{
    {IN_SW_GSEC_ADD,		isc_action_svc_add_user,		"ADD",		0, 0, 0, false,	0,	1, NULL},	// add user
    {IN_SW_GSEC_DEL,		isc_action_svc_delete_user,		"DELETE",	0, 0, 0, false,	0,	2, NULL},	// delete user
    {IN_SW_GSEC_MOD,		isc_action_svc_modify_user,		"MODIFY",	0, 0, 0, false,	0,	2, NULL},	// modify user
    {IN_SW_GSEC_DIS,		isc_action_svc_display_user, "OLD_DISPLAY",	0, 0, 0, false,	0, 11, NULL},	// display user(s) without admin info
    {IN_SW_GSEC_MAPPING,	isc_action_svc_set_mapping,		"MAP SET",	0, 0, 0, false,	0,	2, NULL},	// map admins
    {IN_SW_GSEC_MAPPING,	isc_action_svc_drop_mapping,	"MAP DROP",	0, 0, 0, false,	0,	2, NULL},	// map admins
	{IN_SW_GSEC_DIS_ADM,	isc_action_svc_display_user_adm,"DISPLAY",	0, 0, 0, false,	0,	2, NULL},	// display user(s) with admin info
    {IN_SW_GSEC_0,		0,				NULL,		0, 0, 0, false,	0,	0, NULL}		// End of List
};
#endif // GSEC_GSECSWI_H
