/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			BigInteger.h
 *	DESCRIPTION:	Integer of unlimited precision. Uses libtommath.
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
 *  Copyright (c) 2011 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_COMMON_CLASSES_BIG_INTEGER
#define FB_COMMON_CLASSES_BIG_INTEGER

#include <tommath.h>

#include "../common/classes/fb_string.h"
#include "../common/classes/array.h"

namespace Firebird {

class BigInteger
{
public:
	BigInteger();
	explicit BigInteger(const char* text, unsigned int radix = 16u);
	BigInteger(unsigned int count, const unsigned char* bytes);
	explicit BigInteger(const Firebird::UCharBuffer& val);
	BigInteger(const BigInteger& val);
//	BigInteger(int numBits, Random& r);
	~BigInteger();

	BigInteger& operator= (const BigInteger& val);
	void random(int numBytes);
	void assign(unsigned int count, const unsigned char* bytes);

	BigInteger operator+ (const BigInteger& val) const;
	BigInteger operator- (const BigInteger& val) const;
	BigInteger operator* (const BigInteger& val) const;
	BigInteger operator/ (const BigInteger& val) const;
	BigInteger operator% (const BigInteger& val) const;

	BigInteger& operator+= (const BigInteger& val);
	BigInteger& operator-= (const BigInteger& val);
	BigInteger& operator*= (const BigInteger& val);
	BigInteger& operator/= (const BigInteger& val);
	BigInteger& operator%= (const BigInteger& val);

	bool operator== (const BigInteger& val) const;

	void getBytes(Firebird::UCharBuffer& bytes) const;
	unsigned int length() const;
	void getText(Firebird::string& str, unsigned int radix = 16u) const;

	BigInteger modPow(const BigInteger& pow, const BigInteger& mod) const;

private:
	mp_int t;
};

} // namespace Firebird

#endif // FB_COMMON_CLASSES_BIG_INTEGER
