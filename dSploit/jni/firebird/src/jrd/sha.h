/*
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
 * Created by: Alex Peshkov <AlexPeshkov@users.sourceforge.net>
 * and all contributors, signed below.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef JRD_OS_SHA_H
#define JRD_OS_SHA_H

#include "firebird.h"
#include "../common/classes/fb_string.h"

namespace Jrd
{
	class CryptSupport
	{
	private:
		CryptSupport() {}
		~CryptSupport() {}
	public:
		// hash and random return base64-coded values
		static void hash(Firebird::string& hashValue, const Firebird::string& data);
		static void random(Firebird::string& randomValue, size_t length);
	};
}

#endif //JRD_OS_SHA_H
