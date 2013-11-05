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

#include "../jrd/common.h"
#include "../jrd/constants.h"

// Switch handling constants

const int IN_SW_NBK_0				= 0;
//const int IN_SW_NBK_LOCK			= 1;
//const int IN_SW_NBK_UNLOCK		= 2;
//const int IN_SW_NBK_FIXUP			= 3;
const int IN_SW_NBK_BACKUP			= 4;
const int IN_SW_NBK_RESTORE			= 5;
const int IN_SW_NBK_NODBTRIG		= 6;
//const int IN_SW_NBK_USER_NAME		= 7;
//const int IN_SW_NBK_PASSWORD		= 8;
//const int IN_SW_NBK_SIZE			= 9;
const int IN_SW_NBK_DIRECT			= 10;

// taking into an account that this is used only by services, describe only interesting switches

static const struct in_sw_tab_t nbackup_in_sw_table [] =
{
    {IN_SW_NBK_NODBTRIG,	isc_spb_nbk_no_triggers,	"T",	0, 0, 0, false,	0,	1, NULL},
    {IN_SW_NBK_DIRECT,		isc_spb_nbk_direct,			"D",	0, 0, 0, false,	0,	1, NULL},
    {IN_SW_NBK_0,		0,				NULL,		0, 0, 0, false,	0,	0, NULL}		// End of List
};

static const struct in_sw_tab_t nbackup_action_in_sw_table [] =
{
    {IN_SW_NBK_BACKUP,	isc_action_svc_nbak,	"B",	0, 0, 0, false,	0,	1, NULL},
    {IN_SW_NBK_RESTORE,	isc_action_svc_nrest,	"R",	0, 0, 0, false,	0,	1, NULL},
    {IN_SW_NBK_0,		0,				NULL,		0, 0, 0, false,	0,	0, NULL}		// End of List
};
#endif // NBACKUP_NBKSWI_H

