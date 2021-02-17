/*
 *	MODULE:		traceswi.h
 *	DESCRIPTION:
 *
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "../../jrd/constants.h"

const int IN_SW_TRACE_START			= 1;
const int IN_SW_TRACE_STOP			= 2;
const int IN_SW_TRACE_SUSPEND		= 3;
const int IN_SW_TRACE_RESUME		= 4;
const int IN_SW_TRACE_LIST			= 5;
const int IN_SW_TRACE_CONFIG		= 6;
const int IN_SW_TRACE_NAME			= 7;
const int IN_SW_TRACE_ID			= 8;
const int IN_SW_TRACE_USERNAME		= 9;
const int IN_SW_TRACE_PASSWORD		= 10;
const int IN_SW_TRACE_TRUSTED_USER	= 11;
const int IN_SW_TRACE_TRUSTED_ROLE	= 12;
const int IN_SW_TRACE_SERVICE_NAME	= 13;
const int IN_SW_TRACE_FETCH_PWD		= 14;
const int IN_SW_TRACE_TRUSTED_AUTH	= 15;
const int IN_SW_TRACE_VERSION		= 16;


// list of possible actions (services) for use with trace services
static const struct Switches::in_sw_tab_t trace_action_in_sw_table [] =
{
	{IN_SW_TRACE_LIST,		isc_action_svc_trace_list,		"LIST", 	0, 0, 0, false,	0,	1, NULL},
	{IN_SW_TRACE_RESUME,	isc_action_svc_trace_resume,	"RESUME", 	0, 0, 0, false,	0,	1, NULL},
	{IN_SW_TRACE_STOP,		isc_action_svc_trace_stop,		"STOP", 	0, 0, 0, false,	0,	3, NULL},
	{IN_SW_TRACE_START,		isc_action_svc_trace_start,		"START",	0, 0, 0, false,	0,	3, NULL},
	{IN_SW_TRACE_SUSPEND,	isc_action_svc_trace_suspend,	"SUSPEND",	0, 0, 0, false,	0,	2, NULL},
	{IN_SW_TRACE_VERSION,	0,								"Z",		0, 0, 0, false, 0,	1, NULL},
	{0,						0,								NULL,		0, 0, 0, false, 0,	0, NULL}	// End of List
};


// list of actions (services) parameters
static const struct Switches::in_sw_tab_t trace_option_in_sw_table [] =
{
	{IN_SW_TRACE_CONFIG,	isc_spb_trc_cfg,	"CONFIG", 	0, 0, 0, false,	0,	1, NULL},
	{IN_SW_TRACE_ID,		isc_spb_trc_id,		"ID",		0, 0, 0, false,	0,	1, NULL},
	{IN_SW_TRACE_NAME,		isc_spb_trc_name,	"NAME", 	0, 0, 0, false,	0,	1, NULL},
	{0,						0,					NULL,		0, 0, 0, false, 0,	0, NULL}	// End of List
};

// authentication switches, common for all utils (services)
static const struct Switches::in_sw_tab_t trace_auth_in_sw_table [] =
{
	{IN_SW_TRACE_FETCH_PWD,		0,	"FETCH_PASSWORD",		0, 0, 0, false,	0,	2, NULL},
	{IN_SW_TRACE_PASSWORD,		0,	PASSWORD_SWITCH,		0, 0, 0, false,	0,	1, NULL},
	{IN_SW_TRACE_SERVICE_NAME,	0,	"SERVICE",				0, 0, 0, false,	0,	2, NULL},
	{IN_SW_TRACE_TRUSTED_AUTH,	0,	"TRUSTED",				0, 0, 0, false,	0,	1, NULL},
//	{IN_SW_TRACE_TRUSTED_USER,	0,	TRUSTED_USER_SWITCH,	0, 0, 0, false,	0,	TRUSTED_USER_SWITCH_LEN, NULL},
//	{IN_SW_TRACE_TRUSTED_ROLE,	0,	TRUSTED_ROLE_SWITCH,	0, 0, 0, false,	0,	TRUSTED_ROLE_SWITCH_LEN, NULL},
	{IN_SW_TRACE_USERNAME,		0,	USERNAME_SWITCH,		0, 0, 0, false,	0,	1, NULL},
	{0,							0,	NULL,					0, 0, 0, false, 0,	0, NULL}	// End of List
};
