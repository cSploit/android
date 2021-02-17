/*
 *	PROGRAM:	Firebird International support
 *	MODULE:		lc_icu.h
 *	DESCRIPTION:	Collations of ICU character sets
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef INTL_LC_ICU_H
#define INTL_LC_ICU_H

#include "../common/classes/fb_string.h"

bool LCICU_setup_attributes(const ASCII* name, const ASCII* charSetName, const ASCII* configInfo,
	const Firebird::string& specificAttributes, Firebird::string& newSpecificAttributes);
bool LCICU_texttype_init(texttype* tt,
						 const ASCII* name,
						 const ASCII* charSetName,
						 USHORT attributes,
						 const UCHAR* specificAttributes,
						 ULONG specificAttributesLength,
						 const ASCII* configInfo);

#endif	// INTL_LC_ICU_H
