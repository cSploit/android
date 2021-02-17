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

#ifndef COMMON_NEST_CONST_H
#define COMMON_NEST_CONST_H

// Encloses a pointer to cascade parent object constness.
template <typename T>
class NestConst
{
public:
	NestConst(T* aPtr = NULL)
		: ptr(aPtr)
	{
	}

	NestConst<T>& operator =(T* aPtr)
	{
		ptr = aPtr;
		return *this;
	}

	T** getAddress() { return &ptr; }
	const T* const* getAddress() const { return &ptr; }

	T* getObject() { return ptr; }
	const T* getObject() const { return ptr; }

	operator T*() { return ptr; }
	operator const T*() const { return ptr; }

	T* operator ->() { return ptr; }
	const T* operator ->() const { return ptr; }

private:
	T* ptr;
};

#endif	// COMMON_NEST_CONST_H
