/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			StatusHolder.h
 *	DESCRIPTION:	Firebird's exception classes
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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Vlad Khorsun <hvlad at users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_STATUS_HOLDER
#define FB_STATUS_HOLDER

#include "firebird/Provider.h"
#include "../common/utils_proto.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/classes/array.h"

namespace Firebird {

class BaseStatus : public IStatus
{
public:
	// IStatus implementation
	virtual void FB_CARG set(const ISC_STATUS* value)
	{
		set(fb_utils::statusLength(value), value);
	}

	virtual void FB_CARG set(unsigned int length, const ISC_STATUS* value)
	{
		fb_utils::copyStatus(vector, FB_NELEM(vector), value, length);
	}

	virtual void FB_CARG init()
	{
		fb_utils::init_status(vector);
	}

	virtual const ISC_STATUS* FB_CARG get() const
	{
		return vector;
	}

	virtual int FB_CARG isSuccess() const
	{
		return vector[1] == 0;
	}

public:
	BaseStatus()
	{
		init();
	}

	void check()
	{
		if (!isSuccess())
			status_exception::raise(get());
	}

private:
	ISC_STATUS vector[40];	// FixMe - may be a kind of dynamic storage will be better?
};

class LocalStatus : public AutoIface<BaseStatus, FB_STATUS_VERSION>
{
public:
	virtual void FB_CARG dispose()
	{ }
};


// This trivial container is used when we need to grow vector element by element w/o any conversions
typedef HalfStaticArray<ISC_STATUS, ISC_STATUS_LENGTH> SimpleStatusVector;


// DynamicStatusVector owns strings, contained in it
class DynamicStatusVector
{
public:
	explicit DynamicStatusVector(const ISC_STATUS* status = NULL)
		: m_status_vector(*getDefaultMemoryPool())
	{
		ISC_STATUS* s = m_status_vector.getBuffer(ISC_STATUS_LENGTH);
		fb_utils::init_status(s);

		if (status)
		{
			save(status);
		}
	}

	~DynamicStatusVector()
	{
		clear();
	}

	ISC_STATUS save(const ISC_STATUS* status);
	void clear();

	ISC_STATUS getError() const
	{
		return value()[1];
	}

	const ISC_STATUS* value() const
	{
		return m_status_vector.begin();
	}

	bool isSuccess() const
	{
		return getError() == 0;
	}

	ISC_STATUS& operator[](unsigned int index)
	{
		return m_status_vector[index];
	}

private:
	SimpleStatusVector m_status_vector;
};


class StatusHolder
{
public:
	explicit StatusHolder(const ISC_STATUS* status = NULL)
		: m_status_vector(status), m_raised(false)
	{ }

	ISC_STATUS save(const ISC_STATUS* status);
	void clear();
	void raise();

	ISC_STATUS getError()
	{
		return value()[1];
	}

	const ISC_STATUS* value()
	{
		if (m_raised) {
			clear();
		}
		return m_status_vector.value();
	}

	bool isSuccess()
	{
		return getError() == 0;
	}

private:
	DynamicStatusVector m_status_vector;
	bool m_raised;
};


} // namespace Firebird


#endif // FB_STATUS_HOLDER
