/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			IntlParametersBlock.h
 *	DESCRIPTION:	Convert strings in parameters block to/from UTF8.
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
 *  Copyright (c) 2012 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_COMMON_INTL_PAR_BLOCK
#define FB_COMMON_INTL_PAR_BLOCK

#include "../common/classes/fb_string.h"

namespace Firebird {

class ClumpletWriter;

class IntlParametersBlock
{
public:
	enum TagType { TAG_SKIP, TAG_STRING, TAG_COMMAND_LINE };
	typedef void ProcessString(string& s);

	virtual TagType checkTag(UCHAR tag) = 0;

	void toUtf8(ClumpletWriter& pb, UCHAR utf8Tag);
	void fromUtf8(ClumpletWriter& pb, UCHAR utf8Tag);

private:
	void processParametersBlock(ProcessString* processString, ClumpletWriter& pb);
};

class IntlDpb : public IntlParametersBlock
{
public:
	TagType checkTag(UCHAR tag);
};

class IntlSpb : public IntlParametersBlock
{
public:
	TagType checkTag(UCHAR tag);
};

class IntlSpbStart : public IntlParametersBlock
{
public:
	IntlSpbStart()
		: mode(0)
	{ }

	TagType checkTag(UCHAR tag);

private:
	UCHAR mode;
};

} // namespace Firebird

#endif // FB_COMMON_INTL_PAR_BLOCK
