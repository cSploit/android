/*
 *	PROGRAM:		Firebird basic API
 *	MODULE:			firebird/Utl.h
 *	DESCRIPTION:	Misc calls
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

#ifndef FB_UTL_INTERFACE
#define FB_UTL_INTERFACE

#include "./Interface.h"

namespace Firebird {

class IVersionCallback : public IVersioned
{
public:
	virtual void FB_CARG callback(const char* text) = 0;
};
#define FB_VERSION_CALLBACK_VERSION (FB_VERSIONED_VERSION + 1)

class IAttachment;
class ITransaction;

class IUtl : public IVersioned
{
public:
	virtual void FB_CARG getFbVersion(IStatus* status, IAttachment* att,
		IVersionCallback* callback) = 0;
	virtual void FB_CARG loadBlob(IStatus* status, ISC_QUAD* blobId,
		IAttachment* att, ITransaction* tra, const char* file, FB_BOOLEAN txt) = 0;
	virtual void FB_CARG dumpBlob(IStatus* status, ISC_QUAD* blobId,
		IAttachment* att, ITransaction* tra, const char* file, FB_BOOLEAN txt) = 0;
	virtual void FB_CARG getPerfCounters(IStatus* status, IAttachment* att,
		const char* countersSet, ISC_INT64* counters) = 0;
	virtual IAttachment* FB_CARG executeCreateDatabase(Firebird::IStatus* status,
		unsigned stmtLength, const char* creatDBstatement, unsigned dialect,
		FB_BOOLEAN* stmtIsCreateDb = NULL) = 0;
};
#define FB_UTL_VERSION (FB_VERSIONED_VERSION + 5)

} // namespace Firebird

#endif // FB_UTL_INTERFACE
