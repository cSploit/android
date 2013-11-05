/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceUnicodeUtils.h
 *	DESCRIPTION:	Unicode support for trace needs
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
 *  Copyright (c) 2010 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef TRACE_UNICODE_UTILS_H
#define TRACE_UNICODE_UTILS_H

#include "firebird.h"
#include "../../common/classes/fb_string.h"
#include "../../jrd/intl_classes.h"
#include "../../jrd/TextType.h"
#include "../../jrd/unicode_util.h"

class UnicodeCollationHolder
{
private:
	charset *cs;
	texttype *tt;
	Firebird::AutoPtr<Jrd::CharSet> charSet;
	Firebird::AutoPtr<Jrd::TextType> textType;

public:
	UnicodeCollationHolder(Firebird::MemoryPool& pool);
	~UnicodeCollationHolder();

	Jrd::TextType* getTextType() { return textType; };
};

#endif // TRACE_UNICODE_UTILS_H
