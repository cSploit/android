/*
 *	PROGRAM:		Firebird authentication
 *	MODULE:			Auth.h
 *	DESCRIPTION:	Implementation of interfaces, passed to plugins
 *					Plugins loader
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
 *  Copyright (c) 2010 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_AUTH_H
#define FB_AUTH_H

#include "firebird/Auth.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/init.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/ImplementHelper.h"

namespace Auth {

class WriterImplementation : public Firebird::AutoIface<IWriter, FB_AUTH_WRITER_VERSION>
{
public:
	WriterImplementation();

	void store(Firebird::ClumpletWriter* to, unsigned char tag);
	void setPlugin(const char* m);

	// IWriter implementation
	void FB_CARG reset();
	void FB_CARG add(const char* name);
	void FB_CARG setType(const char* value);
	void FB_CARG setDb(const char* value);

private:
	Firebird::ClumpletWriter current, result;
	Firebird::string plugin, type;
	unsigned int sequence;

	void putLevel();
};

} // namespace Auth


#endif // FB_AUTH_H
