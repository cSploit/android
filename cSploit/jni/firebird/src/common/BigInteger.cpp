/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			BigInteger.cpp
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

#include "firebird.h"
#include "gen/iberror.h"

#include <stdlib.h>

#include "../common/BigInteger.h"
#include "../common/os/guid.h"

#define CHECK_MP(a) check(a, #a)

namespace Firebird
{

	static inline void check(int rc, const char* function)
	{
		switch (rc)
		{
		case MP_OKAY:
			return;
		case MP_MEM:
			BadAlloc::raise();
		default:
			// Libtommath error code @1 in function @2
			(Arg::Gds(isc_libtommath_generic) << Arg::Num(rc) << function).raise();
		}
	}

	BigInteger::BigInteger()
	{
		CHECK_MP(mp_init(&t));
	}

	BigInteger::BigInteger(const char* text, unsigned int radix)
	{
		CHECK_MP(mp_init(&t));
		CHECK_MP(mp_read_radix(&t, text, radix));
	}

	BigInteger::BigInteger(unsigned int count, const unsigned char* bytes)
	{
		CHECK_MP(mp_init(&t));
		assign(count, bytes);
	}

	BigInteger::BigInteger(const Firebird::UCharBuffer& val)
	{
		CHECK_MP(mp_init(&t));
		assign(val.getCount(), val.begin());
	}

	BigInteger::BigInteger(const BigInteger& val)
	{
		CHECK_MP(mp_init_copy(&t, const_cast<mp_int*>(&val.t) ));
	}

	BigInteger::~BigInteger()
	{
		mp_clear(&t);
	}

	BigInteger& BigInteger::operator= (const BigInteger& val)
	{
		CHECK_MP(mp_copy(const_cast<mp_int*>(&val.t), &t));
		return *this;
	}

	void BigInteger::random(int numBytes)
	{
		Firebird::UCharBuffer b;
		Firebird::GenerateRandomBytes(b.getBuffer(numBytes), numBytes);
		assign(numBytes, b.begin());
	}

	void BigInteger::assign(unsigned int count, const unsigned char* bytes)
	{
		CHECK_MP(mp_read_unsigned_bin(&t, bytes, count));
	}

	BigInteger BigInteger::operator+ (const BigInteger& val) const
	{
		BigInteger rc;
		CHECK_MP(mp_add(const_cast<mp_int*>(&t), const_cast<mp_int*>(&val.t), &rc.t));
		return rc;
	}

	BigInteger BigInteger::operator- (const BigInteger& val) const
	{
		BigInteger rc;
		CHECK_MP(mp_sub(const_cast<mp_int*>(&t), const_cast<mp_int*>(&val.t), &rc.t));
		return rc;
	}

	BigInteger BigInteger::operator* (const BigInteger& val) const
	{
		BigInteger rc;
		CHECK_MP(mp_mul(const_cast<mp_int*>(&t), const_cast<mp_int*>(&val.t), &rc.t));
		return rc;
	}

	BigInteger BigInteger::operator/ (const BigInteger& val) const
	{
		BigInteger rc;
		CHECK_MP(mp_div(const_cast<mp_int*>(&t), const_cast<mp_int*>(&val.t), &rc.t, NULL));
		return rc;
	}

	BigInteger BigInteger::operator% (const BigInteger& val) const
	{
		BigInteger rc;
		CHECK_MP(mp_mod(const_cast<mp_int*>(&t), const_cast<mp_int*>(&val.t), &rc.t));
		return rc;
	}

	BigInteger& BigInteger::operator+= (const BigInteger& val)
	{
		CHECK_MP(mp_add(&t, const_cast<mp_int*>(&val.t), &t));
		return *this;
	}

	BigInteger& BigInteger::operator-= (const BigInteger& val)
	{
		CHECK_MP(mp_sub(&t, const_cast<mp_int*>(&val.t), &t));
		return *this;
	}

	BigInteger& BigInteger::operator*= (const BigInteger& val)
	{
		CHECK_MP(mp_mul(&t, const_cast<mp_int*>(&val.t), &t));
		return *this;
	}

	BigInteger& BigInteger::operator/= (const BigInteger& val)
	{
		CHECK_MP(mp_div(&t, const_cast<mp_int*>(&val.t), &t, NULL));
		return *this;
	}

	BigInteger& BigInteger::operator%= (const BigInteger& val)
	{
		CHECK_MP(mp_mod(&t, const_cast<mp_int*>(&val.t), &t));
		return *this;
	}

	bool BigInteger::operator== (const BigInteger& val) const
	{
		return mp_cmp(const_cast<mp_int*>(&t), const_cast<mp_int*>(&val.t)) == 0;
	}

	void BigInteger::getBytes(Firebird::UCharBuffer& bytes) const
	{
		CHECK_MP(mp_to_unsigned_bin(const_cast<mp_int*>(&t), bytes.getBuffer(length())));
	}

	unsigned int BigInteger::length() const
	{
		int rc = mp_unsigned_bin_size(const_cast<mp_int*>(&t));
		if (rc < 0)
		{
			check(rc, "mp_unsigned_bin_size(&t)");
		}
		return static_cast<unsigned int>(rc);
	}

	void BigInteger::getText(string& str, unsigned int radix) const
	{
		int size;
		CHECK_MP(mp_radix_size(const_cast<mp_int*>(&t), radix, &size));
		str.resize(size - 1, ' ');
		CHECK_MP(mp_toradix(const_cast<mp_int*>(&t), str.begin(), radix));
	}

	BigInteger BigInteger::modPow(const BigInteger& pow, const BigInteger& mod) const
	{
		BigInteger rc;
		CHECK_MP(mp_exptmod(const_cast<mp_int*>(&t), const_cast<mp_int*>(&pow.t),
							const_cast<mp_int*>(&mod.t), &rc.t));
		return rc;
	}

}

#undef CHECK_MP
