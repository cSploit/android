/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceUnicodeUtils.cpp
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


#include "TraceUnicodeUtils.h"

using namespace Firebird;

UnicodeCollationHolder::UnicodeCollationHolder(MemoryPool& pool)
{
	cs = FB_NEW(pool) charset;
	tt = FB_NEW(pool) texttype;

	IntlUtil::initUtf8Charset(cs);

	string collAttributes("ICU-VERSION=");
	collAttributes += Jrd::UnicodeUtil::DEFAULT_ICU_VERSION;
	IntlUtil::setupIcuAttributes(cs, collAttributes, "", collAttributes);

	UCharBuffer collAttributesBuffer;
	collAttributesBuffer.push(reinterpret_cast<const UCHAR*>(collAttributes.c_str()),
		collAttributes.length());

	if (!IntlUtil::initUnicodeCollation(tt, cs, "UNICODE", 0, collAttributesBuffer, string()))
		fatal_exception::raiseFmt("cannot initialize UNICODE collation to use in trace plugin");


	charSet = Jrd::CharSet::createInstance(pool, 0, cs);
	textType = FB_NEW(pool) Jrd::TextType(0, tt, charSet);
}

UnicodeCollationHolder::~UnicodeCollationHolder()
{
	fb_assert(tt->texttype_fn_destroy);

	if (tt->texttype_fn_destroy)
		tt->texttype_fn_destroy(tt);

	// cs should be deleted by texttype_fn_destroy call above
	delete tt;
}
