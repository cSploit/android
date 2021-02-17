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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_NULLABLE_H
#define CLASSES_NULLABLE_H

#include "firebird.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"


// Auxiliary template to build an empty value.
template <typename T>	// Generic NullableClear
class NullableClear
{
public:
	static void clear(T& v)
	{
		v = 0;
	}
};


// Nullable support without constructor, to allow usage in unions (used in the parser).
template <typename T> class BaseNullable
{
public:
	static BaseNullable<T> val(const T& v)
	{
		BaseNullable<T> nullable;
		nullable.value = v;
		nullable.specified = true;
		return nullable;
	}

	static BaseNullable<T> empty()
	{
		BaseNullable<T> nullable;
		NullableClear<T>::clear(nullable.value);
		nullable.specified = false;
		return nullable;
	}

	bool operator ==(const BaseNullable<T>& o) const
	{
		return (!specified && !o.specified) || (specified == o.specified && value == o.value);
	}

public:
	T value;
	bool specified;
};


// NullableClear specializations.

template <>
class NullableClear<Firebird::string>	// string especialization for NullableClear
{
public:
	static void clear(Firebird::string& v)
	{
		v = "";
	}
};

template <>
class NullableClear<Firebird::MetaName>	// MetaName especialization for NullableClear
{
public:
	static void clear(Firebird::MetaName& v)
	{
		v = "";
	}
};

template <typename T>
class NullableClear<BaseNullable<T> >
{
public:
	static void clear(BaseNullable<T>& v)
	{
		v.specified = false;
	}
};


// Actual Nullable template.
template <typename T> class Nullable : public BaseNullable<T>
{
public:
	explicit Nullable<T>(const T& v)
	{
		this->value = v;
		this->specified = true;
	}

	Nullable<T>(const Nullable<T>& o)
	{
		this->value = o.value;
		this->specified = o.specified;
	}

	Nullable<T>()
	{
		NullableClear<T>::clear(this->value);
		this->specified = false;
	}

	void operator =(const BaseNullable<T>& o)
	{
		this->value = o.value;
		this->specified = o.specified;
	}

	void operator =(const T& v)
	{
		this->value = v;
		this->specified = true;
	}
};


#endif	// CLASSES_NULLABLE_H
