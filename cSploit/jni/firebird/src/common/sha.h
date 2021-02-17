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

#ifndef COMMON_SHA_H
#define COMMON_SHA_H

#include "firebird.h"
#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"

namespace Firebird {

class Sha1 : public GlobalStorage
{
public:
	Sha1();

	static const unsigned int HASH_SIZE = 20;
	static const unsigned int BLOCK_SIZE = 64;
	typedef unsigned long LONG;

	struct ShaInfo
	{
		LONG digest[5];				// message digest
		LONG count_lo, count_hi;	// 64-bit bit count
		UCHAR data[BLOCK_SIZE];		// SHA data buffer
		size_t local;			// unprocessed amount in data
	};

	void process(size_t length, const void* bytes);

	void process(const UCharBuffer& bytes)
	{
		process(bytes.getCount(), bytes.begin());
	}

	void process(const string& str)
	{
		process(str.length(), str.c_str());
	}

	void process(const char* str)
	{
		process(strlen(str), str);
	}

	void getHash(UCharBuffer& h);
	void reset();
	~Sha1();

	// return base64-coded values
	static void hashBased64(Firebird::string& hashBase64, const Firebird::string& data);

private:
	void clear();

	ShaInfo handle;
	bool active;
};

} // namespace Firebird

#endif // COMMON_SHA_H
