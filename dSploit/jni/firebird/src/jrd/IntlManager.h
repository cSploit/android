/*
 *	PROGRAM:	JRD International support
 *	MODULE:		IntlManager.h
 *	DESCRIPTION:	INTL Manager
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

#ifndef JRD_INTLMANAGER_H
#define JRD_INTLMANAGER_H

#include "../common/classes/fb_string.h"
#include "../config/ConfObj.h"

struct charset;
struct texttype;

namespace Jrd {

class IntlManager
{
public:
	static bool initialize();

	static bool collationInstalled(const Firebird::string& collationName,
								   const Firebird::string& charSetName);

	static bool lookupCharSet(const Firebird::string& charSetName, charset* cs);

	static bool lookupCollation(const Firebird::string& collationName,
								const Firebird::string& charSetName,
								USHORT attributes, const UCHAR* specificAttributes,
								ULONG specificAttributesLen, bool ignoreAttributes,
								texttype* tt);

	static bool setupCollationAttributes(
		const Firebird::string& collationName, const Firebird::string& charSetName,
		const Firebird::string& specificAttributes, Firebird::string& newSpecificAttributes);

public:
	struct CharSetDefinition
	{
		const char* name;
		UCHAR id;
		USHORT maxBytes;
	};

	struct CharSetAliasDefinition
	{
		const char* name;
		UCHAR charSetId;
	};

	struct CollationDefinition
	{
		UCHAR charSetId;
		UCHAR collationId;
		const char* name;
		const char* baseName;
		USHORT attributes;
		const char* specificAttributes;
	};

	const static CharSetDefinition defaultCharSets[];
	const static CharSetAliasDefinition defaultCharSetAliases[];
	const static CollationDefinition defaultCollations[];

private:
	static Firebird::string getConfigInfo(const ConfObj& confObj);

	static bool registerCharSetCollation(const Firebird::string& name,
		const Firebird::PathName& filename, const Firebird::string& externalName,
		const Firebird::string& configInfo);

	static bool validateCharSet(const Firebird::string& charSetName, charset* cs);
};

}	// namespace Jrd

#endif	// JRD_INTLMANAGER_H
