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
 *  The Original Code was created by Claudio Valderrama on 12-Mar-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef FB_PTRSENTRY_H
#define FB_PTRSENTRY_H

// Helper class to track allocation of single entities and arrays.
// Hopefully scarcely used as we move to more objects.
// It simply deletes the given pointer when the container (an object of this clas)
// goes out of scope or when we call clean() explicitly.
// It assumes and used C++ memory primitives. It won't work if you used malloc.
// If you delete the pointer "from outside", you have to call forget().

template <typename T>
class PtrSentry
{
public:
	PtrSentry();
	PtrSentry(T* ptr, bool is_array);
	~PtrSentry();
	void clean();
	T* exchange(T* ptr, bool is_array);
	void forget();
private:
	T* m_ptr;
	bool m_is_array;
};


template <typename T>
inline PtrSentry<T>::PtrSentry()
	: m_ptr(0), m_is_array(false)
{
}

template <typename T>
inline PtrSentry<T>::PtrSentry(T* ptr, bool is_array)
	: m_ptr(ptr), m_is_array(is_array)
{
}

template <typename T>
inline PtrSentry<T>::~PtrSentry()
{
	if (m_is_array)
		delete[] m_ptr;
	else
		delete m_ptr;
}

// The code assumes the allocation was with C++ new!
template <typename T>
inline void PtrSentry<T>::clean()
{
	if (m_is_array)
		delete[] m_ptr;
	else
		delete m_ptr;

	// We need this because clean() can be called directly. Otherwise, when
	// the destructor calls it, it would try to deallocate an invalid address.
	m_ptr = 0;
}

// Gives a new pointer to the object. The other is not deleted; simply forgotten.
// The old pointer is returned.
template <typename T>
T* PtrSentry<T>::exchange(T* ptr, bool is_array)
{
	if (m_ptr == ptr) // Don't know what else to do to signal error than to return NULL.
		return 0;

	T* const old_ptr = m_ptr;
	m_ptr = ptr;
	m_is_array = is_array;
	return old_ptr;
}

// Simply forget the current pointer. For example, we discovered the allocated
// memory has to survive to return to the caller.
template <typename T>
void PtrSentry<T>::forget()
{
	exchange(0, false);
}


#endif // FB_PTRSENTRY_H

