/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		vector.h
 *	DESCRIPTION:	static array of simple elements
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 *
 */

#ifndef VECTOR_H
#define VECTOR_H

#include "../jrd/gdsassert.h"
#include <string.h>

namespace Firebird {

// Very fast static array of simple types
template <typename T, size_t Capacity>
class Vector
{
public:
	Vector() : count(0) {}

	T& operator[](size_t index)
	{
  		fb_assert(index < count);
  		return data[index];
	}

	const T& operator[](size_t index) const
	{
  		fb_assert(index < count);
  		return data[index];
	}

	T* begin() { return data; }
	T* end() { return data + count; }
	const T* begin() const { return data; }
	const T* end() const { return data + count; }
	size_t getCount() const { return count; }
	size_t getCapacity() const { return Capacity; }
	void clear() { count = 0; }

	void insert(size_t index, const T& item)
	{
	  fb_assert(index <= count);
	  fb_assert(count < Capacity);
	  memmove(data + index + 1, data + index, sizeof(T) * (count++ - index));
	  data[index] = item;
	}

	size_t add(const T& item)
	{
		fb_assert(count < Capacity);
		data[count] = item;
  		return ++count;
	}

	T* remove(size_t index)
	{
  		fb_assert(index < count);
  		memmove(data + index, data + index + 1, sizeof(T) * (--count - index));
		return &data[index];
	}

	void shrink(size_t newCount)
	{
		fb_assert(newCount <= count);
		count = newCount;
	}

	void join(const Vector<T, Capacity>& L)
	{
		fb_assert(count + L.count <= Capacity);
		memcpy(data + count, L.data, sizeof(T) * L.count);
		count += L.count;
	}

	// prepare vector to be used as a buffer of capacity items
	T* getBuffer(size_t capacityL)
	{
		fb_assert(capacityL <= Capacity);
		count = capacityL;
		return data;
	}

	void push(const T& item)
	{
		add(item);
	}

	T pop()
	{
		fb_assert(count > 0);
		count--;
		return data[count];
	}

protected:
	size_t count;
	T data[Capacity];
};

// Template for default value comparsion
template <typename T>
class DefaultComparator
{
public:
	static bool greaterThan(const T& i1, const T& i2)
	{
	    return i1 > i2;
	}
};

// Template to convert value to index directly
template <typename T>
class DefaultKeyValue
{
public:
	static const T& generate(const void* /*sender*/, const T& item) { return item; }
};

// Fast sorted array of simple objects
// It is used for B+ tree nodes lower, but can still be used by itself
template <typename Value, size_t Capacity, typename Key = Value,
	typename KeyOfValue = DefaultKeyValue<Value>,
	typename Cmp = DefaultComparator<Key> >
class SortedVector : public Vector<Value, Capacity>
{
public:
	SortedVector() : Vector<Value, Capacity>() {}
	bool find(const Key& item, size_t& pos) const
	{
		size_t highBound = this->count, lowBound = 0;
		while (highBound > lowBound) {
			const size_t temp = (highBound + lowBound) >> 1;
			if (Cmp::greaterThan(item, KeyOfValue::generate(this, this->data[temp])))
				lowBound = temp + 1;
			else
				highBound = temp;
		}
		pos = lowBound;
		return highBound != this->count &&
			!Cmp::greaterThan(KeyOfValue::generate(this, this->data[lowBound]), item);
	}
	size_t add(const Value& item)
	{
	    size_t pos;
  	    find(KeyOfValue::generate(this, item), pos);
		this->insert(pos, item);
		return pos;
	}
};

} // namespace Firebird

#endif

