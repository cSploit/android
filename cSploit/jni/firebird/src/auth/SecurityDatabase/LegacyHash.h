/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		LegacyHash.h
 *	DESCRIPTION:	Firebird 2.X style hash
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.02.02 Dmitry Yemanov: Implemented cached security database connection
 */

#ifndef AUTH_LEGACY_HASH_H
#define AUTH_LEGACY_HASH_H

#include "../common/utils_proto.h"
#include "../common/sha.h"
#include "../common/os/guid.h"
#include "../common/utils_proto.h"

namespace Auth {

const size_t MAX_LEGACY_PASSWORD_LENGTH = 64;			// used to store passwords internally
static const char* const LEGACY_PASSWORD_SALT = "9z";	// for old ENC_crypt()
const size_t SALT_LENGTH = 12;					// measured after base64 coding

class LegacyHash
{
public:
	static void hash(Firebird::string& h, const Firebird::string& userName, const TEXT* passwd)
	{
		Firebird::string salt;
		fb_utils::random64(salt, SALT_LENGTH);
		hash(h, userName, passwd, salt);
	}

	static void hash(Firebird::string& h,
					 const Firebird::string& userName,
					 const Firebird::string& passwd,
					 const Firebird::string& oldHash)
	{
		Firebird::string salt(oldHash);
		salt.resize(SALT_LENGTH, '=');
		Firebird::string allData(salt);
		allData += userName;
		allData += passwd;
		Firebird::Sha1::hashBased64(h, allData);
		h = salt + h;
	}
};

} // namespace Auth

#endif // AUTH_LEGACY_SERVER_H
