/*
 *	PROGRAM:	Firebird utilities
 *	MODULE:		nbkswi.h
 *	DESCRIPTION:	nbackup switches
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Alex Peshkov <peshkoff at mail dot ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef NBACKUP_NBKSWI_H
#define NBACKUP_NBKSWI_H

//#include "../common/common.h"
#include "../jrd/constants.h"

// Switch handling constants

const int IN_SW_NBK_0				= 0;
const int IN_SW_NBK_LOCK			= 1;
const int IN_SW_NBK_UNLOCK			= 2;
const int IN_SW_NBK_FIXUP			= 3;
const int IN_SW_NBK_BACKUP			= 4;
const int IN_SW_NBK_RESTORE			= 5;
const int IN_SW_NBK_NODBTRIG		= 6;
const int IN_SW_NBK_USER_NAME		= 7;
const int IN_SW_NBK_PASSWORD		= 8;
const int IN_SW_NBK_SIZE			= 9;
const int IN_SW_NBK_FETCH			= 10;
const int IN_SW_NBK_VERSION			= 11;
const int IN_SW_NBK_TRUSTED_USER	= 12;
const int IN_SW_NBK_TRUSTED_ROLE	= 13;
const int IN_SW_NBK_HELP			= 14;
const int IN_SW_NBK_DIRECT			= 15;


static const struct Switches::in_sw_tab_t nbackup_in_sw_table [] =
{
	{IN_SW_NBK_NODBTRIG,	isc_spb_nbk_no_triggers,	"T",		0, 0, 0, false,	0,	1, NULL},
	{IN_SW_NBK_DIRECT,		isc_spb_nbk_direct,			"DIRECT",	0, 0, 0, false, 0,  1, NULL},
	{IN_SW_NBK_0,			0,							NULL,		0, 0, 0, false,	0,	0, NULL}	// End of List
};

enum NbakOptionType { nboGeneral, nboSpecial, nboExclusive };

static const struct Switches::in_sw_tab_t nbackup_action_in_sw_table [] =
{
	{IN_SW_NBK_LOCK,		0,						"LOCK",				0, 0, 0, false,	8,	1,	NULL ,nboExclusive},
	{IN_SW_NBK_UNLOCK,		0,						"N",				0, 0, 0, false,	0,	1,	NULL, nboExclusive},
	{IN_SW_NBK_UNLOCK,		0,						"UNLOCK",			0, 0, 0, false,	9,	2,	NULL, nboExclusive},
	{IN_SW_NBK_FIXUP,		0,						"FIXUP",			0, 0, 0, false,	10,	1,	NULL, nboExclusive},
	{IN_SW_NBK_BACKUP,		isc_action_svc_nbak,	"BACKUP",			0, 0, 0, false,	11,	1,	NULL, nboExclusive},
	{IN_SW_NBK_RESTORE,		isc_action_svc_nrest,	"RESTORE",			0, 0, 0, false,	12,	1,	NULL, nboExclusive},
	{IN_SW_NBK_DIRECT,		0,						"DIRECT",			0, 0, 0, false, 70,	1,	NULL, nboSpecial},
	{IN_SW_NBK_SIZE,		0,						"SIZE",				0, 0, 0, false,	17,	1,	NULL, nboSpecial},
	{IN_SW_NBK_NODBTRIG,	0,						"T",				0, 0, 0, false,	0,	1,	NULL, nboGeneral},
	{IN_SW_NBK_NODBTRIG,	0,						"NODBTRIGGERS",		0, 0, 0, false,	16,	3,	NULL, nboGeneral},
	{IN_SW_NBK_USER_NAME,	0,						"USER",				0, 0, 0, false,	13,	1,	NULL, nboGeneral},
	{IN_SW_NBK_PASSWORD,	0,						"PASSWORD",			0, 0, 0, false,	14,	1,	NULL, nboGeneral},
	{IN_SW_NBK_FETCH,		0,						"FETCH_PASSWORD",	0, 0, 0, false,	15,	2,	NULL, nboGeneral},
	{IN_SW_NBK_VERSION, 	0,						"Z",				0, 0, 0, false,	18,	1,	NULL, nboGeneral},
//	{IN_SW_NBK_TRUSTED_USER,	0,				TRUSTED_USER_SWITCH,	0, 0, 0, false,	0,	TRUSTED_USER_SWITCH_LEN,	NULL, nboGeneral},
//	{IN_SW_NBK_TRUSTED_ROLE,	0, 				TRUSTED_ROLE_SWITCH,	0, 0, 0, false,	0,	TRUSTED_ROLE_SWITCH_LEN,	NULL, nboGeneral},
	{IN_SW_NBK_HELP,		0,						"?",				0, 0, 0, false,	0,	1,	NULL, 0},
	{IN_SW_NBK_0,			0,						NULL,				0, 0, 0, false,	0,	0,	NULL, 0}	// End of List
};
#endif // NBACKUP_NBKSWI_H
