/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		auto.h
 *	DESCRIPTION:	STL::auto_ptr replacement
 *
 *		*** warning ***
 *  Unlike STL's auto_ptr ALWAYS deletes ptr in destructor -
 *  no ownership flag!
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

#ifndef CLASSES_AUTO_PTR_H
#define CLASSES_AUTO_PTR_H

#include <stdio.h>

namespace Firebird {


template <typename What>
class SimpleDelete
{
public:
	static void clear(What *ptr)
	{
		delete ptr;
	}
};

template <typename What>
class ArrayDelete
{
public:
	static void clear(What* ptr)
	{
		delete[] ptr;
	}
};

template <typename Where, typename Clear = SimpleDelete<Where> >
class AutoPtr
{
private:
	Where* ptr;
public:
	AutoPtr<Where, Clear>(Where* v = NULL)
		: ptr(v)
	{}

	~AutoPtr()
	{
		Clear::clear(ptr);
	}

	AutoPtr<Where, Clear>& operator= (Where* v)
	{
		Clear::clear(ptr);
		ptr = v;
		return *this;
	}

	operator Where*()
	{
		return ptr;
	}

	bool operator !() const
	{
		return !ptr;
	}

	Where* operator->()
	{
		return ptr;
	}

	Where* release()
	{
		Where* tmp = ptr;
		ptr = NULL;
		return tmp;
	}

	void reset(Where* v = NULL)
	{
		if (v != ptr) {
			Clear::clear(ptr);
			ptr = v;
		}
	}

private:
	AutoPtr<Where, Clear>(AutoPtr<Where, Clear>&);
	void operator=(AutoPtr<Where, Clear>&);
};


template <typename T>
class AutoSetRestore
{
public:
	AutoSetRestore(T* aValue, T newValue)
		: value(aValue),
		  oldValue(*aValue)
	{
		*value = newValue;
	}

	~AutoSetRestore()
	{
		*value = oldValue;
	}

private:
	// copying is prohibited
	AutoSetRestore(const AutoSetRestore&);
	AutoSetRestore& operator =(const AutoSetRestore&);

	T* value;
	T oldValue;
};

template <typename T, typename T2>
class AutoSetRestore2
{
private:
	typedef T (T2::*Getter)();
	typedef void (T2::*Setter)(T);

public:
	AutoSetRestore2(T2* aPointer, Getter aGetter, Setter aSetter, T newValue)
		: pointer(aPointer),
		  setter(aSetter),
		  oldValue((aPointer->*aGetter)())
	{
		(aPointer->*aSetter)(newValue);
	}

	~AutoSetRestore2()
	{
		(pointer->*setter)(oldValue);
	}

private:
	// copying is prohibited
	AutoSetRestore2(const AutoSetRestore2&);
	AutoSetRestore2& operator =(const AutoSetRestore2&);

private:
	T2* pointer;
	Setter setter;
	T oldValue;
};


// One more typical class for AutoPtr cleanup
class FileClose
{
public:
	static void clear(FILE *f)
	{
		if (f) {
			fclose(f);
		}
	}
};


} //namespace Firebird

#endif // CLASSES_AUTO_PTR_H

