/*
 *	PROGRAM:	string class definition
 *	MODULE:		fb_string.cpp
 *	DESCRIPTION:	Provides almost that same functionality,
 *			that STL::basic_string<char> does,
 *			but behaves MemoryPools friendly.
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../common/classes/fb_string.h"

#include <ctype.h>
#include <stdarg.h>

#ifdef HAVE_STRCASECMP
#define STRNCASECMP strncasecmp
#else
#ifdef HAVE_STRICMP
#define STRNCASECMP strnicmp
#else
namespace {
	int StringIgnoreCaseCompare(const char* s1, const char* s2, unsigned int l)
	{
		while (l--) {
			const int delta = toupper(*s1++) - toupper(*s2++);
			if (delta) {
				return delta;
			}
		}
		return 0;
	}
} // namespace
#define STRNCASECMP StringIgnoreCaseCompare
#endif // HAVE_STRICMP
#endif // HAVE_STRCASECMP

namespace {
	class strBitMask
	{
	private:
		char m[32];
	public:
		strBitMask(Firebird::AbstractString::const_pointer s, Firebird::AbstractString::size_type l)
		{
			memset(m, 0, sizeof(m));
			if (l == Firebird::AbstractString::npos) {
				l = strlen(s);
			}
			Firebird::AbstractString::const_pointer end = s + l;
			while (s < end) {
				const unsigned char uc = static_cast<unsigned char>(*s++);
				m[uc >> 3] |= (1 << (uc & 7));
			}
		}
		inline bool Contains(const char c) const
		{
			const unsigned char uc = static_cast<unsigned char>(c);
			return m[uc >> 3] & (1 << (uc & 7));
		}
	};
} // namespace

namespace Firebird {
	const AbstractString::size_type AbstractString::npos = (AbstractString::size_type)(~0);

	AbstractString::AbstractString(const AbstractString& v)
	{
		initialize(v.length());
		memcpy(stringBuffer, v.c_str(), v.length());
	}

	AbstractString::AbstractString(const size_type sizeL, const_pointer dataL)
	{
		initialize(sizeL);
		memcpy(stringBuffer, dataL, sizeL);
	}

	AbstractString::AbstractString(const_pointer p1, const size_type n1,
				 const_pointer p2, const size_type n2)
	{
		// CVC: npos must be maximum size_type value for all platforms.
		// fb_assert(n2 < npos - n1 && n1 + n2 <= max_length());
		if (n2 > npos - n1)
		{
			Firebird::fatal_exception::raise("String length overflow");
		}
		// checkLength(n1 + n2); redundant: initialize() will check.
		initialize(n1 + n2);
		memcpy(stringBuffer, p1, n1);
		memcpy(stringBuffer + n1, p2, n2);
	}

	AbstractString::AbstractString(const size_type sizeL, char_type c)
	{
		initialize(sizeL);
		memset(stringBuffer, c, sizeL);
	}

	void AbstractString::adjustRange(const size_type length, size_type& pos, size_type& n)
	{
		if (pos == npos) {
			pos = length > n ? length - n : 0;
		}
		if (pos >= length) {
			pos = length;
			n = 0;
		}
		else if (pos + n > length || n == npos) {
			n = length - pos;
		}
	}

	AbstractString::pointer AbstractString::baseAssign(const size_type n)
	{
		reserveBuffer(n);
		stringLength = n;
		stringBuffer[stringLength] = 0;
		shrinkBuffer(); // Shrink buffer if it is unneeded anymore
		return stringBuffer;
	}

	AbstractString::pointer AbstractString::baseAppend(const size_type n)
	{
		reserveBuffer(stringLength + n);
		stringLength += n;
		stringBuffer[stringLength] = 0; // Set null terminator inside the new buffer
		return stringBuffer + stringLength - n;
	}

	AbstractString::pointer AbstractString::baseInsert(const size_type p0, const size_type n)
	{
		if (p0 >= length()) {
			return baseAppend(n);
		}
		reserveBuffer(stringLength + n);
		// Do not forget to move null terminator, too
		memmove(stringBuffer + p0 + n, stringBuffer + p0, stringLength - p0 + 1);
		stringLength += n;
		return stringBuffer + p0;
	}

	void AbstractString::baseErase(size_type p0, size_type n)
	{
		adjustRange(length(), p0, n);
		memmove(stringBuffer + p0, stringBuffer + p0 + n, stringLength - (p0 + n) + 1);
		stringLength -= n;
		shrinkBuffer();
	}

	void AbstractString::reserve(size_type n)
	{
		// Do not allow to reserve huge buffers
		if (n > max_length())
			n = max_length();

		reserveBuffer(n);
	}

	void AbstractString::resize(const size_type n, char_type c)
	{
		if (n == length()) {
			return;
		}
		if (n > stringLength) {
			reserveBuffer(n);
			memset(stringBuffer + stringLength, c, n - stringLength);
		}
		stringLength = n;
		stringBuffer[n] = 0;
		shrinkBuffer();
	}

	AbstractString::size_type AbstractString::rfind(const_pointer s, const size_type pos) const
	{
		const size_type l = strlen(s);
		int lastpos = length() - l;
		if (lastpos < 0) {
			return npos;
		}
		if (pos < static_cast<size_type>(lastpos)) {
			lastpos = pos;
		}
		const_pointer start = c_str();
		for (const_pointer endL = &start[lastpos]; endL >= start; --endL)
		{
			if (memcmp(endL, s, l) == 0) {
				return endL - start;
			}
		}
		return npos;
	}

	AbstractString::size_type AbstractString::rfind(char_type c, const size_type pos) const
	{
		int lastpos = length() - 1;
		if (lastpos < 0) {
			return npos;
		}
		if (pos < static_cast<size_type>(lastpos)) {
			lastpos = pos;
		}
		const_pointer start = c_str();
		for (const_pointer endL = &start[lastpos]; endL >= start; --endL)
		{
			if (*endL == c) {
				return endL - start;
			}
		}
		return npos;
	}

	AbstractString::size_type AbstractString::find_first_of(const_pointer s, size_type pos, size_type n) const
	{
		const strBitMask sm(s, n);
		const_pointer p = &c_str()[pos];
		while (pos < length()) {
			if (sm.Contains(*p++)) {
				return pos;
			}
			++pos;
		}
		return npos;
	}

	AbstractString::size_type AbstractString::find_last_of(const_pointer s, const size_type pos, size_type n) const
	{
		const strBitMask sm(s, n);
		int lpos = length() - 1;
		if (static_cast<int>(pos) < lpos && pos != npos) {
			lpos = pos;
		}
		const_pointer p = &c_str()[lpos];
		while (lpos >= 0) {
			if (sm.Contains(*p--)) {
				return lpos;
			}
			--lpos;
		}
		return npos;
	}

	AbstractString::size_type AbstractString::find_first_not_of(const_pointer s, size_type pos, size_type n) const
	{
		const strBitMask sm(s, n);
		const_pointer p = &c_str()[pos];
		while (pos < length()) {
			if (! sm.Contains(*p++)) {
				return pos;
			}
			++pos;
		}
		return npos;
	}

	AbstractString::size_type AbstractString::find_last_not_of(const_pointer s, const size_type pos, size_type n) const
	{
		const strBitMask sm(s, n);
		int lpos = length() - 1;
		if (static_cast<int>(pos) < lpos && pos != npos) {
			lpos = pos;
		}
		const_pointer p = &c_str()[lpos];
		while (lpos >= 0) {
			if (! sm.Contains(*p--)) {
				return lpos;
			}
			--lpos;
		}
		return npos;
	}

	bool AbstractString::LoadFromFile(FILE* file)
	{
		baseErase(0, length());
		if (! file)
			return false;

		bool rc = false;
		int c;
		while ((c = getc(file)) != EOF) {
			rc = true;
			if (c == '\n') {
				break;
			}
			*baseAppend(1) = c;
		}
		return rc;
	}

#ifdef WIN_NT
// The following is used instead of including huge windows.h
extern "C" {
	__declspec(dllimport) unsigned long __stdcall CharUpperBuffA
		(char* lpsz, unsigned long cchLength);
	__declspec(dllimport) unsigned long __stdcall CharLowerBuffA
		(char* lpsz, unsigned long cchLength);
}
#endif // WIN_NT

	void AbstractString::upper()
	{
#ifdef WIN_NT
		CharUpperBuffA(modify(), length());
#else  // WIN_NT
		for (pointer p = modify(); *p; p++) {
			*p = toupper(*p);
		}
#endif // WIN_NT
	}

	void AbstractString::lower()
	{
#ifdef WIN_NT
		CharLowerBuffA(modify(), length());
#else  // WIN_NT
		for (pointer p = modify(); *p; p++) {
			*p = tolower(*p);
		}
#endif // WIN_NT
	}

	void AbstractString::baseTrim(const TrimType whereTrim, const_pointer toTrim)
	{
		const strBitMask sm(toTrim, strlen(toTrim));
		const_pointer b = c_str();
		const_pointer e = &c_str()[length() - 1];
		if (whereTrim != TrimRight) {
			while (b <= e) {
				if (! sm.Contains(*b)) {
					break;
				}
				++b;
			}
		}
		if (whereTrim != TrimLeft) {
			while (b <= e) {
				if (! sm.Contains(*e)) {
					break;
				}
				--e;
			}
		}
		const size_type NewLength = e - b + 1;

		if (NewLength == length())
			return;

		if (b != c_str())
		{
			memmove(stringBuffer, b, NewLength);
		}
		stringLength = NewLength;
		stringBuffer[NewLength] = 0;
		shrinkBuffer();
	}

	void AbstractString::printf(const char* format,...)
	{
		va_list params;
		va_start(params, format);
		vprintf(format, params);
		va_end(params);
	}

// Need macros here - va_copy()/va_end() should be called in SAME function
#ifdef HAVE_VA_COPY
#define FB_VA_COPY(to, from) va_copy(to, from)
#define FB_CLOSE_VACOPY(to) va_end(to)
#else
#define FB_VA_COPY(to, from) to = from
#define FB_CLOSE_VACOPY(to)
#endif

	void AbstractString::vprintf(const char* format, va_list params)
	{
#ifndef HAVE_VSNPRINTF
#error NS: I am lazy to implement version of this routine based on plain vsprintf.
#error Please find an implementation of vsnprintf function for your platform.
#error For example, consider importing library from http://www.ijs.si/software/snprintf/
#error to Firebird extern repository
#endif
		enum { tempsize = 256 };
		char temp[tempsize];
		va_list paramsCopy;
		FB_VA_COPY(paramsCopy, params);
		int l = VSNPRINTF(temp, tempsize, format, paramsCopy);
		FB_CLOSE_VACOPY(paramsCopy);
		if (l < 0)
		{
			size_type n = sizeof(temp);
			while (true) {
				n *= 2;
				if (n > max_length())
					n = max_length();
				FB_VA_COPY(paramsCopy, params);
				l = VSNPRINTF(baseAssign(n), n + 1, format, paramsCopy);
				FB_CLOSE_VACOPY(paramsCopy);
				if (l >= 0)
					break;
				if (n >= max_length()) {
					stringBuffer[max_length()] = 0;
					return;
				}
			}
			resize(l);
			return;
		}
		temp[tempsize - 1] = 0;
		if (l < tempsize) {
			memcpy(baseAssign(l), temp, l);
		}
		else {
			resize(l);
			FB_VA_COPY(paramsCopy, params);
			VSNPRINTF(begin(), l + 1, format, paramsCopy);
			FB_CLOSE_VACOPY(paramsCopy);
		}
	}

	unsigned int AbstractString::hash(const_pointer string, const size_t tableSize)
	{
		unsigned int value = 0;
		unsigned char c;

		while ((c = *string++))
		{
			c = toupper(c);
			value = value * 11 + c;
		}

		return value % tableSize;
	}

	int PathNameComparator::compare(AbstractString::const_pointer s1, AbstractString::const_pointer s2,
		const AbstractString::size_type n)
	{
		if (CASE_SENSITIVITY)
			return memcmp(s1, s2, n);

		return STRNCASECMP(s1, s2, n);
	}

	bool AbstractString::equalsNoCase(AbstractString::const_pointer string) const
	{
		size_t l = strlen(string);
		if (l > length())
		{
			l = length();
		}
		return (STRNCASECMP(c_str(), string, ++l) == 0);
	}
}	// namespace Firebird
