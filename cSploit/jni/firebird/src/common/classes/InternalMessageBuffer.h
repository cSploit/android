/*
 *	PROGRAM:		FB interfaces.
 *	MODULE:			InternalMessageBuffer.h
 *	DESCRIPTION:	Old=>new message style converter.
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
 *  Copyright (c) 2013 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_COMMON_CLASSES_INTERNAL_MESSAGE_BUFFER
#define FB_COMMON_CLASSES_INTERNAL_MESSAGE_BUFFER

#include "firebird/Provider.h"

namespace Firebird {

class InternalMessageBuffer
{
public:
	InternalMessageBuffer(unsigned aBlrLength, const unsigned char* aBlr,
		unsigned aBufferLength, unsigned char* aBuffer);
	~InternalMessageBuffer();

public:
	Firebird::IMessageMetadata* metadata;
	UCHAR* buffer;
};

} // namespace Firebird

#endif // FB_COMMON_CLASSES_INTERNAL_MESSAGE_BUFFER
