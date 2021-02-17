/*
 *	PROGRAM:	string class definition
 *	MODULE:		fb_string.h
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

#ifndef INCLUDE_FB_STRING_H
#define INCLUDE_FB_STRING_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "firebird.h"
#include "fb_types.h"
#include "fb_exception.h"
#include "../common/classes/alloc.h"
#include "../common/classes/RefCounted.h"

namespace Firebird
{
	class AbstractString : private AutoStorage
	{
	public:
		typedef char char_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef char* pointer;
		typedef const char* const_pointer;
		typedef char& reference;
		typedef const char& const_reference;
		typedef char value_type;
		typedef pointer iterator;
		typedef const_pointer const_iterator;
		static const size_type npos;
		enum {INLINE_BUFFER_SIZE = 32, INIT_RESERVE = 16/*, KEEP_SIZE = 512*/};

	protected:
		typedef ULONG internal_size_type; // 32 bits!

	private:
		const internal_size_type max_length;

	protected:
		char_type inlineBuffer[INLINE_BUFFER_SIZE];
		char_type* stringBuffer;
		internal_size_type stringLength, bufferSize;

	private:
		void checkPos(size_type pos) const
		{
			if (pos >= length()) {
				fatal_exception::raise("Firebird::string - pos out of range");
			}
		}

		void checkLength(size_type len)
		{
			if (len > getMaxLength()) {
				fatal_exception::raise("Firebird::string - length exceeds predefined limit");
			}
		}

		// Reserve buffer to allow storing at least newLen characters there
		// (not including null terminator). Existing contents of our string are preserved.
		void reserveBuffer(const size_type newLen)
		{
			size_type newSize = newLen + 1;
			fb_assert(newSize != 0); // This large argument would be a programming error for sure.
			if (newSize > bufferSize)
			{
				// Make sure we do not exceed string length limit
				checkLength(newLen);

				// Order of assignments below is important in case of low memory conditions

				// Grow buffer exponentially to prevent memory fragmentation
				if (newSize / 2 < bufferSize)
					newSize = size_t(bufferSize) * 2u;

				// Do not grow buffer beyond string length limit
				size_type max_length = getMaxLength() + 1;
				if (newSize > max_length)
					newSize = max_length;

				// Allocate new buffer
				char_type *newBuffer = FB_NEW(getPool()) char_type[newSize];

				// Carefully copy string data including null terminator
				memcpy(newBuffer, stringBuffer, sizeof(char_type) * (stringLength + 1u));

				// Deallocate old buffer if needed
				if (stringBuffer != inlineBuffer)
					delete[] stringBuffer;

				stringBuffer = newBuffer;
				bufferSize = static_cast<internal_size_type>(newSize);
			}
		}

		// Make sure our buffer is large enough to store at least <length> characters in it
		// (not including null terminator). Resulting buffer is not initialized.
		// Use it in constructors only when stringBuffer is not assigned yet.
		void initialize(const size_type len)
		{
			if (len < INLINE_BUFFER_SIZE)
			{
				stringBuffer = inlineBuffer;
				bufferSize = INLINE_BUFFER_SIZE;
			}
			else
			{
				stringBuffer = NULL; // Be safe in case of exception
				checkLength(len);

				// Reserve a few extra bytes in the buffer
				size_type newSize = len + 1 + INIT_RESERVE;

				// Do not grow buffer beyond string length limit
				size_type max_length = getMaxLength() + 1;
				if (newSize > max_length)
					newSize = max_length;

				// Allocate new buffer
				stringBuffer = FB_NEW(getPool()) char_type[newSize];
				bufferSize = static_cast<internal_size_type>(newSize);
			}
			stringLength = static_cast<internal_size_type>(len);
			stringBuffer[stringLength] = 0;
		}

		void shrinkBuffer()
		{
			// Shrink buffer if we decide it is beneficial
		}

	protected:
		AbstractString(const size_type limit, const size_type sizeL, const void* datap);

		AbstractString(const size_type limit, const_pointer p1, const size_type n1,
					 const_pointer p2, const size_type n2);

		AbstractString(const size_type limit, const AbstractString& v);

		explicit AbstractString(const size_type limit) :
			max_length(static_cast<internal_size_type>(limit)),
			stringBuffer(inlineBuffer), stringLength(0), bufferSize(INLINE_BUFFER_SIZE)
		{
			stringBuffer[0] = 0;
		}

		AbstractString(const size_type limit, const size_type sizeL, char_type c);

		AbstractString(const size_type limit, MemoryPool& p) : AutoStorage(p),
			max_length(static_cast<internal_size_type>(limit)),
			stringBuffer(inlineBuffer), stringLength(0), bufferSize(INLINE_BUFFER_SIZE)
		{
			stringBuffer[0] = 0;
		}

		AbstractString(const size_type limit, MemoryPool& p, const AbstractString& v)
			: AutoStorage(p), max_length(static_cast<internal_size_type>(limit))
		{
			initialize(v.length());
			memcpy(stringBuffer, v.c_str(), stringLength);
		}

		AbstractString(const size_type limit, MemoryPool& p, const void* s, const size_type l)
			: AutoStorage(p), max_length(static_cast<internal_size_type>(limit))
		{
			initialize(l);
			memcpy(stringBuffer, s, l);
		}

		pointer modify()
		{
			return stringBuffer;
		}

		// Trim the range making sure that it fits inside specified length
		static void adjustRange(const size_type length, size_type& pos, size_type& n);

		pointer baseAssign(const size_type n);

		pointer baseAppend(const size_type n);

		pointer baseInsert(const size_type p0, const size_type n);

		void baseErase(size_type p0, size_type n);

		enum TrimType {TrimLeft, TrimRight, TrimBoth};

		void baseTrim(const TrimType whereTrim, const_pointer toTrim);

		size_type getMaxLength() const
		{
			return max_length;
		}

	public:
		const_pointer c_str() const
		{
			return stringBuffer;
		}
		size_type length() const
		{
			return stringLength;
		}
		size_type getCount() const
		{
			return stringLength;
		}
		// Almost same as c_str(), but return 0, not "",
		// when string has no data. Useful when interacting
		// with old code, which does check for NULL.
		const_pointer nullStr() const
		{
			return stringLength ? stringBuffer : 0;
		}
		// Call it only when you have worked with at() or operator[]
		// in case a null ASCII was inserted in the middle of the string.
		size_type recalculate_length()
		{
		    stringLength = static_cast<internal_size_type>(strlen(stringBuffer));
		    return stringLength;
		}

		void reserve(size_type n = 0);
		void resize(const size_type n, char_type c = ' ');
		void grow(const size_type n)
		{
			resize(n);
		}

		pointer getBuffer(size_t l)
		{
			return baseAssign(l);
		}

		size_type find(const AbstractString& str, size_type pos = 0) const
		{
			return find(str.c_str(), pos);
		}
		size_type find(const_pointer s, size_type pos = 0) const
		{
			const_pointer p = strstr(c_str() + pos, s);
			return p ? p - c_str() : npos;
		}
		size_type find(char_type c, size_type pos = 0) const
		{
			const_pointer p = strchr(c_str() + pos, c);
			return p ? p - c_str() : npos;
		}
		size_type rfind(const AbstractString& str, size_type pos = npos) const
		{
			return rfind(str.c_str(), pos);
		}
		size_type rfind(const_pointer s, const size_type pos = npos) const;
		size_type rfind(char_type c, const size_type pos = npos) const;
		size_type find_first_of(const AbstractString& str, size_type pos = 0) const
		{
			return find_first_of(str.c_str(), pos, str.length());
		}
		size_type find_first_of(const_pointer s, size_type pos, size_type n) const;
		size_type find_first_of(const_pointer s, size_type pos = 0) const
		{
			return find_first_of(s, pos, strlen(s));
		}
		size_type find_first_of(char_type c, size_type pos = 0) const
		{
			return find(c, pos);
		}
		size_type find_last_of(const AbstractString& str, size_type pos = npos) const
		{
			return find_last_of(str.c_str(), pos, str.length());
		}
		size_type find_last_of(const_pointer s, const size_type pos, size_type n = npos) const;
		size_type find_last_of(const_pointer s, size_type pos = npos) const
		{
			return find_last_of(s, pos, strlen(s));
		}
		size_type find_last_of(char_type c, size_type pos = npos) const
		{
			return rfind(c, pos);
		}
		size_type find_first_not_of(const AbstractString& str, size_type pos = 0) const
		{
			return find_first_not_of(str.c_str(), pos, str.length());
		}
		size_type find_first_not_of(const_pointer s, size_type pos, size_type n) const;
		size_type find_first_not_of(const_pointer s, size_type pos = 0) const
		{
			return find_first_not_of(s, pos, strlen(s));
		}
		size_type find_first_not_of(char_type c, size_type pos = 0) const
		{
			const char_type s[2] = {c, 0};
			return find_first_not_of(s, pos, 1);
		}
		size_type find_last_not_of(const AbstractString& str, size_type pos = npos) const
		{
			return find_last_not_of(str.c_str(), pos, str.length());
		}
		size_type find_last_not_of(const_pointer s, const size_type pos, size_type n = npos) const;
		size_type find_last_not_of(const_pointer s, size_type pos = npos) const
		{
			return find_last_not_of(s, pos, strlen(s));
		}
		size_type find_last_not_of(char_type c, size_type pos = npos) const
		{
			const char_type s[2] = {c, 0};
			return find_last_not_of(s, pos, 1);
		}

		iterator begin()
		{
			return modify();
		}
		const_iterator begin() const
		{
			return c_str();
		}
		iterator end()
		{
			return modify() + length();
		}
		const_iterator end() const
		{
			return c_str() + length();
		}
		const_reference at(const size_type pos) const
		{
			checkPos(pos);
			return c_str()[pos];
		}
		reference at(const size_type pos)
		{
			checkPos(pos);
			return modify()[pos];
		}
		const_reference operator[](size_type pos) const
		{
			return at(pos);
		}
		reference operator[](size_type pos)
		{
			return at(pos);
		}
		const_pointer data() const
		{
			return c_str();
		}
		size_type size() const
		{
			return length();
		}
		size_type capacity() const
		{
			return bufferSize - 1u;
		}
		bool empty() const
		{
			return length() == 0;
		}
		bool hasData() const
		{
			return !empty();
		}
		// to satisfy both ways to check for empty string
		bool isEmpty() const
		{
			return empty();
		}

		void upper();
		void lower();
		void ltrim(const_pointer ToTrim = " ")
		{
			baseTrim(TrimLeft, ToTrim);
		}
		void rtrim(const_pointer ToTrim = " ")
		{
			baseTrim(TrimRight, ToTrim);
		}
		void trim(const_pointer ToTrim = " ")
		{
			baseTrim(TrimBoth, ToTrim);
		}
		void alltrim(const_pointer ToTrim = " ")
		{
			baseTrim(TrimBoth, ToTrim);
		}

		bool LoadFromFile(FILE* file);
		void vprintf(const char* Format, va_list params);
		void printf(const char* Format, ...);

		size_type copyTo(pointer to, size_type toSize) const
		{
			fb_assert(to);
			fb_assert(toSize);
			if (--toSize > length())
			{
				toSize = length();
			}
			memcpy(to, c_str(), toSize);
			to[toSize] = 0;
			return toSize;
		}

		static unsigned int hash(const_pointer string, const size_type tableSize);

		unsigned int hash(size_type tableSize) const
		{
			return hash(c_str(), tableSize);
		}

		bool equalsNoCase(const_pointer string) const;

		AbstractString& append(const AbstractString& str)
		{
			fb_assert(&str != this);
			return append(str.c_str(), str.length());
		}
		AbstractString& append(const AbstractString& str, size_type pos, size_type n)
		{
			fb_assert(&str != this);
			adjustRange(str.length(), pos, n);
			return append(str.c_str() + pos, n);
		}
		AbstractString& append(const_pointer s, const size_type n)
		{
			memcpy(baseAppend(n), s, n);
			return *this;
		}
		AbstractString& append(const_pointer s)
		{
			return append(s, strlen(s));
		}
		AbstractString& append(size_type n, char_type c)
		{
			memset(baseAppend(n), c, n);
			return *this;
		}
		AbstractString& append(const_iterator first, const_iterator last)
		{
			return append(first, last - first);
		}

		AbstractString& insert(size_type p0, const AbstractString& str)
		{
			fb_assert(&str != this);
			return insert(p0, str.c_str(), str.length());
		}
		AbstractString& insert(size_type p0, const AbstractString& str, size_type pos,
			size_type n)
		{
			fb_assert(&str != this);
			adjustRange(str.length(), pos, n);
			return insert(p0, &str.c_str()[pos], n);
		}
		AbstractString& insert(size_type p0, const_pointer s, const size_type n)
		{
			if (p0 >= length()) {
				return append(s, n);
			}
			memcpy(baseInsert(p0, n), s, n);
			return *this;
		}
		AbstractString& insert(size_type p0, const_pointer s)
		{
			return insert(p0, s, strlen(s));
		}
		AbstractString& insert(size_type p0, const size_type n, const char_type c)
		{
			if (p0 >= length()) {
				return append(n, c);
			}
			memset(baseInsert(p0, n), c, n);
			return *this;
		}
		// iterator insert(iterator it, char_type c);	// what to return here?
		void insert(iterator it, size_type n, char_type c)
		{
			insert(it - c_str(), n, c);
		}
		void insert(iterator it, const_iterator first, const_iterator last)
		{
			insert(it - c_str(), first, last - first);
		}

		AbstractString& erase(size_type p0 = 0, size_type n = npos)
		{
			baseErase(p0, n);
			return *this;
		}
		iterator erase(iterator it)
		{
			erase(it - c_str(), 1);
			return it;
		}
		iterator erase(iterator first, iterator last)
		{
			erase(first - c_str(), last - first);
			return first;
		}

		AbstractString& replace(size_type p0, size_type n0, const AbstractString& str)
		{
			fb_assert(&str != this);
			return replace(p0, n0, str.c_str(), str.length());
		}
		AbstractString& replace(const size_type p0, const size_type n0,
			const AbstractString& str, size_type pos, size_type n)
		{
			fb_assert(&str != this);
			adjustRange(str.length(), pos, n);
			return replace(p0, n0, &str.c_str()[pos], n);
		}
		AbstractString& replace(const size_type p0, const size_type n0, const_pointer s,
			size_type n)
		{
			erase(p0, n0);
			return insert(p0, s, n);
		}
		AbstractString& replace(size_type p0, size_type n0, const_pointer s)
		{
			return replace(p0, n0, s, strlen(s));
		}
		AbstractString& replace(const size_type p0, const size_type n0, size_type n,
			char_type c)
		{
			erase(p0, n0);
			return insert(p0, n, c);
		}
		AbstractString& replace(iterator first0, iterator last0, const AbstractString& str)
		{
			fb_assert(&str != this);
			return replace(first0 - c_str(), last0 - first0, str);
		}
		AbstractString& replace(iterator first0, iterator last0, const_pointer s,
			size_type n)
		{
			return replace(first0 - c_str(), last0 - first0, s, n);
		}
		AbstractString& replace(iterator first0, iterator last0, const_pointer s)
		{
			return replace(first0 - c_str(), last0 - first0, s);
		}
		AbstractString& replace(iterator first0, iterator last0, size_type n, char_type c)
		{
			return replace(first0 - c_str(), last0 - first0, n, c);
		}
		AbstractString& replace(iterator first0, iterator last0, const_iterator first,
			const_iterator last)
		{
			return replace(first0 - c_str(), last0 - first0, first, last - first);
		}

		~AbstractString()
		{
			if (stringBuffer != inlineBuffer)
				delete[] stringBuffer;
		}
	};

	class StringComparator
	{
	public:
		static int compare(AbstractString::const_pointer s1,
								  AbstractString::const_pointer s2,
								  const AbstractString::size_type n)
		{
			return memcmp(s1, s2, n);
		}

		static AbstractString::size_type getMaxLength()
		{
			return 0xFFFFFFFEu;
		}
	};

	class PathNameComparator
	{
	public:
		static int compare(AbstractString::const_pointer s1,
						   AbstractString::const_pointer s2,
						   const AbstractString::size_type n);

		static AbstractString::size_type getMaxLength()
		{
			return 0xFFFEu;
		}
	};

	class IgnoreCaseComparator
	{
	public:
		static int compare(AbstractString::const_pointer s1,
						   AbstractString::const_pointer s2,
						   const AbstractString::size_type n);

		static AbstractString::size_type getMaxLength()
		{
			return 0xFFFFFFFEu;
		}
	};

	template<typename Comparator>
	class StringBase : public AbstractString
	{
		typedef StringBase StringType;
	protected:
		StringBase(const_pointer p1, size_type n1,
				   const_pointer p2, size_type n2) :
			   AbstractString(Comparator::getMaxLength(), p1, n1, p2, n2) {}
	private:
		StringType add(const_pointer s, size_type n) const
		{
			return StringBase<Comparator>(c_str(), length(), s, n);
		}
	public:
		StringBase() : AbstractString(Comparator::getMaxLength()) {}
		StringBase(const StringType& v) : AbstractString(Comparator::getMaxLength(), v) {}
		StringBase(const void* s, size_type n) : AbstractString(Comparator::getMaxLength(), n, s) {}
		StringBase(const_pointer s) : AbstractString(Comparator::getMaxLength(), strlen(s), s) {}
		explicit StringBase(const unsigned char* s) :
			AbstractString(Comparator::getMaxLength(), strlen((char*)s), (char*)s) {}
		StringBase(size_type n, char_type c) : AbstractString(Comparator::getMaxLength(), n, c) {}
		StringBase(const_iterator first, const_iterator last) :
			AbstractString(Comparator::getMaxLength(), last - first, first) {}
		explicit StringBase(MemoryPool& p) : AbstractString(Comparator::getMaxLength(), p) {}
		StringBase(MemoryPool& p, const AbstractString& v) : AbstractString(Comparator::getMaxLength(), p, v) {}
		StringBase(MemoryPool& p, const char_type* s, size_type l) :
			AbstractString(Comparator::getMaxLength(), p, s, l) {}

		static size_type max_length()
		{
			return Comparator::getMaxLength();
		}

		StringType& assign(const StringType& str)
		{
			fb_assert(&str != this);
			return assign(str.c_str(), str.length());
		}
		StringType& assign(const StringType& str, size_type pos, size_type n)
		{
			fb_assert(&str != this);
			adjustRange(str.length(), pos, n);
			return assign(&str.c_str()[pos], n);
		}
		StringType& assign(const void* s, size_type n)
		{
			memcpy(baseAssign(n), s, n);
			return *this;
		}
		StringType& assign(const_pointer s)
		{
			return assign(s, strlen(s));
		}
		StringType& assign(size_type n, char_type c)
		{
			memset(baseAssign(n), c, n);
			return *this;
		}
		StringType& assign(const_iterator first, const_iterator last)
		{
			return assign(first, last - first);
		}

		StringType& operator=(const StringType& v)
		{
			if (&v == this)
				return *this;
			return assign(v);
		}
		StringType& operator=(const_pointer s)
		{
			return assign(s, strlen(s));
		}
		StringType& operator=(char_type c)
		{
			return assign(&c, 1);
		}
		StringType& operator+=(const StringType& v)
		{
			fb_assert(&v != this);
			append(v);
			return *this;
		}
		StringType& operator+=(const_pointer s)
		{
			append(s);
			return *this;
		}
		StringType& operator+=(char_type c)
		{
			append(1, c);
			return *this;
		}
		StringType operator+(const StringType& v) const
		{
			return add(v.c_str(), v.length());
		}
		StringType operator+(const_pointer s) const
		{
			return add(s, strlen(s));
		}
		StringType operator+(char_type c) const
		{
			return add(&c, 1);
		}

		StringBase<StringComparator> ToString() const
		{
			return StringBase<StringComparator>(c_str());
		}
		StringBase<PathNameComparator> ToPathName() const
		{
			return StringBase<PathNameComparator>(c_str());
		}
		StringBase<IgnoreCaseComparator> ToNoCaseString() const
		{
			return StringBase<IgnoreCaseComparator>(c_str());
		}

		StringType substr(size_type pos = 0, size_type n = npos) const
		{
			adjustRange(length(), pos, n);
			return StringType(&c_str()[pos], n);
		}
		int compare(const StringType& str) const
		{
			return compare(str.c_str(), str.length());
		}
		int compare(size_type p0, size_type n0, const StringType& str)
		{
			return compare(p0, n0, str.c_str(), str.length());
		}
		int compare(size_type p0, size_type n0, const StringType& str, size_type pos, size_type n)
		{
			adjustRange(str.length(), pos, n);
			return compare(p0, n0, &str.c_str()[pos], n);
		}
		int compare(const_pointer s) const
		{
			return compare(s, strlen(s));
		}
		int compare(size_type p0, size_type n0, const_pointer s, const size_type n) const
		{
			adjustRange(length(), p0, n0);
			const size_type ml = n0 < n ? n0 : n;
			const int rc = Comparator::compare(&c_str()[p0], s, ml);
			return rc ? rc : n0 - n;
		}
		int compare(const_pointer s, const size_type n) const
		{
			const size_type ml = length() < n ? length() : n;
			const int rc = Comparator::compare(c_str(), s, ml);
			if (rc)
			{
				return rc;
			}
			const difference_type dl = length() - n;
			return (dl < 0) ? -1 : (dl > 0) ? 1 : 0;
		}

		// These four functions are to speed up the most common comparisons: equality and inequality.
		bool equals(const StringType& str) const
		{
			const size_type n = str.length();
			return (length() != n) ? false : (Comparator::compare(c_str(), str.c_str(), n) == 0);
		}
		bool different(const StringType& str) const
		{
			const size_type n = str.length();
			return (length() != n) ? true : (Comparator::compare(c_str(), str.c_str(), n) != 0);
		}
		bool equals(const_pointer s) const
		{
			const size_type n = strlen(s);
			return (length() != n) ? false : (Comparator::compare(c_str(), s, n) == 0);
		}
		bool different(const_pointer s) const
		{
			const size_type n = strlen(s);
			return (length() != n) ? true : (Comparator::compare(c_str(), s, n) != 0);
		}

		bool operator< (const StringType& str) const {return compare(str) <  0;}
		bool operator<=(const StringType& str) const {return compare(str) <= 0;}
		bool operator==(const StringType& str) const {return equals(str);}
		bool operator>=(const StringType& str) const {return compare(str) >= 0;}
		bool operator> (const StringType& str) const {return compare(str) >  0;}
		bool operator!=(const StringType& str) const {return different(str);}

		bool operator< (const char_type* str) const {return compare(str) <  0;}
		bool operator<=(const char_type* str) const {return compare(str) <= 0;}
		bool operator==(const char_type* str) const {return equals(str);}
		bool operator>=(const char_type* str) const {return compare(str) >= 0;}
		bool operator> (const char_type* str) const {return compare(str) >  0;}
		bool operator!=(const char_type* str) const {return different(str);}
    };

	typedef StringBase<StringComparator> string;
	static inline string operator+(string::const_pointer s, const string& str)
	{
		string rc(s);
		rc += str;
		return rc;
	}
	static inline string operator+(string::char_type c, const string& str)
	{
		string rc(1, c);
		rc += str;
		return rc;
	}

	typedef StringBase<PathNameComparator> PathName;
	static inline PathName operator+(PathName::const_pointer s, const PathName& str)
	{
		PathName rc(s);
		rc += str;
		return rc;
	}
	static inline PathName operator+(PathName::char_type c, const PathName& str)
	{
		PathName rc(1, c);
		rc += str;
		return rc;
	}

	typedef StringBase<IgnoreCaseComparator> NoCaseString;
	static inline NoCaseString operator+(NoCaseString::const_pointer s, const NoCaseString& str)
	{
		NoCaseString rc(s);
		rc += str;
		return rc;
	}
	static inline NoCaseString operator+(NoCaseString::char_type c, const NoCaseString& str)
	{
		NoCaseString rc(1, c);
		rc += str;
		return rc;
	}

	// reference-counted strings
	typedef AnyRef<string> RefString;
	typedef RefPtr<RefString> RefStrPtr;
}


#endif	// INCLUDE_FB_STRING_H
