/*
 *	PROGRAM:		Firebird utilities interface
 *	MODULE:			UtilSvc.h
 *	DESCRIPTION:	Interface making it possible to use same code
 *					as both utility or service
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
 *  Copyright (c) 2007 Alex Peshkov <peshkoff at mail dot ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_UTILFACE
#define FB_UTILFACE

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"

namespace MsgFormat {
	class SafeArg;
}

namespace Firebird {

const TEXT SVC_TRMNTR = '\377';	// ASCII 255

class ClumpletWriter;

class UtilSvc : public Firebird::GlobalStorage
{
public:
	typedef Firebird::HalfStaticArray<const char*, 20> ArgvType;

public:
	UtilSvc() : argv(getPool()), usvcDataMode(false) { }

	virtual bool isService() = 0;
	virtual void started() = 0;
	virtual void finish() = 0;
	virtual void outputVerbose(const char* text) = 0;
	virtual void outputError(const char* text) = 0;
	virtual void outputData(const char* text) = 0;
    virtual void printf(bool err, const SCHAR* format, ...) = 0;
    virtual void putLine(char, const char*) = 0;
    virtual void putSLong(char, SLONG) = 0;
	virtual void putChar(char, char) = 0;
	virtual void putBytes(const UCHAR*, size_t) = 0;
	virtual ULONG getBytes(UCHAR*, ULONG) = 0;
	virtual void setServiceStatus(const ISC_STATUS*) = 0;
	virtual void setServiceStatus(const USHORT, const USHORT, const MsgFormat::SafeArg&) = 0;
	virtual const ISC_STATUS* getStatus() = 0;
	virtual void initStatus() = 0;
	virtual void checkService() = 0;
	virtual void hidePasswd(ArgvType&, int) = 0;
	virtual void getAddressPath(Firebird::ClumpletWriter& dpb) = 0;
	virtual bool finished() = 0;

	virtual ~UtilSvc() { }

	static UtilSvc* createStandalone(int ac, char** argv);

	static inline void addStringWithSvcTrmntr(const Firebird::string& str, Firebird::string& switches)
	{
		// All string parameters are delimited by SVC_TRMNTR.
		// This is done to ensure that paths with spaces are handled correctly
		// when creating the argc / argv parameters for the service.
		// SVC_TRMNTRs inside the string are duplicated.

		switches += SVC_TRMNTR;
		for (size_t i = 0; i < str.length(); ++i)
		{
			if (str[i] == SVC_TRMNTR)
			{
				switches += SVC_TRMNTR;
			}
			switches += str[i];
		}
		switches += SVC_TRMNTR;
		switches += ' ';
	}

	void setDataMode(bool value)
	{
		usvcDataMode = value;
	}

public:
	ArgvType argv;
	bool usvcDataMode;
};


} // namespace Firebird


#endif // FB_UTILFACE
