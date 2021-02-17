/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			StatusArg.cpp
 *	DESCRIPTION:	Build status vector with variable number of elements
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
 *  Copyright (c) 2008 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"
#include "../common/StatusArg.h"
#include "../common/utils_proto.h"

#include "../common/classes/fb_string.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/alloc.h"
#include "fb_exception.h"
#include "gen/iberror.h"
#include "firebird/Interface.h"
#include "../jrd/msg_encode.h"

#ifdef WIN_NT
#include <windows.h>
#else
#include <errno.h>
#endif

namespace {
	// Didn't want to bring dyn.h and friends here.
	const int DYN_MSG_FAC		= 8;
}

namespace Firebird {

namespace Arg {

Base::Base(ISC_STATUS k, ISC_STATUS c) :
	implementation(FB_NEW(*getDefaultMemoryPool()) ImplBase(k, c))
{
}

StatusVector::ImplStatusVector::ImplStatusVector(const ISC_STATUS* s) throw() : Base::ImplBase(0, 0)
{
	fb_assert(s);

	clear();
	// special case - empty initialized status vector, no warnings
	if (s[0] != isc_arg_gds || s[1] != 0 || s[2] != 0)
	{
		append(s, FB_NELEM(m_status_vector) - 1);
	}
}

StatusVector::StatusVector(ISC_STATUS k, ISC_STATUS c) :
	Base(FB_NEW(*getDefaultMemoryPool()) ImplStatusVector(k, c))
{
	operator<<(*(static_cast<Base*>(this)));
}

StatusVector::StatusVector(const ISC_STATUS* s) :
	Base(FB_NEW(*getDefaultMemoryPool()) ImplStatusVector(s))
{
}

StatusVector::StatusVector() :
	Base(FB_NEW(*getDefaultMemoryPool()) ImplStatusVector(0, 0))
{
}

void StatusVector::ImplStatusVector::clear() throw()
{
	m_length = 0;
	m_warning = 0;
	m_status_vector[0] = isc_arg_end;
}

bool StatusVector::ImplStatusVector::compare(const StatusVector& v) const throw()
{
	return m_length == v.length() &&
		   memcmp(m_status_vector, v.value(), m_length * sizeof(ISC_STATUS)) == 0;
}

void StatusVector::ImplStatusVector::makePermanent() throw()
{
	makePermanentVector(m_status_vector);
}

void StatusVector::ImplStatusVector::assign(const Exception& ex) throw()
{
	clear();
	ex.stuff_exception(m_status_vector);
	m_length = fb_utils::statusLength(m_status_vector);
}

void StatusVector::ImplStatusVector::append(const StatusVector& v) throw()
{
	ImplStatusVector newVector(getKind(), getCode());

	if (newVector.appendErrors(this))
	{
		if (newVector.appendErrors(v.implementation))
		{
			if (newVector.appendWarnings(this))
				newVector.appendWarnings(v.implementation);
		}
	}

	*this = newVector;
}

bool StatusVector::ImplStatusVector::appendErrors(const ImplBase* const v) throw()
{
	return append(v->value(), v->firstWarning() ? v->firstWarning() : v->length());
}

bool StatusVector::ImplStatusVector::appendWarnings(const ImplBase* const v) throw()
{
	if (! v->firstWarning())
		return true;
	return append(v->value() + v->firstWarning(), v->length() - v->firstWarning());
}

bool StatusVector::ImplStatusVector::append(const ISC_STATUS* const from, const unsigned int count) throw()
{
	// CVC: I didn't expect count to be zero but it's, in some calls
	fb_assert(count >= 0 && count <= ISC_STATUS_LENGTH);
	if (!count)
		return true; // not sure it's the best option here

	unsigned int copied =
		fb_utils::copyStatus(&m_status_vector[m_length], FB_NELEM(m_status_vector) - m_length, from, count);
	m_length += copied;

	return copied == count;
}

void StatusVector::ImplStatusVector::shiftLeft(const Base& arg) throw()
{
	if (m_length < FB_NELEM(m_status_vector) - 2)
	{
		m_status_vector[m_length++] = arg.getKind();
		m_status_vector[m_length++] = arg.getCode();
		m_status_vector[m_length] = isc_arg_end;
	}
}

void StatusVector::ImplStatusVector::shiftLeft(const Warning& arg) throw()
{
	const int cur = m_warning ? 0 : m_length;
	shiftLeft(*static_cast<const Base*>(&arg));
	if (cur && m_status_vector[cur] == isc_arg_warning)
		m_warning = cur;
}

void StatusVector::ImplStatusVector::shiftLeft(const char* text) throw()
{
	shiftLeft(Str(text));
}

void StatusVector::ImplStatusVector::shiftLeft(const AbstractString& text) throw()
{
	shiftLeft(Str(text));
}

void StatusVector::ImplStatusVector::shiftLeft(const MetaName& text) throw()
{
	shiftLeft(Str(text));
}

void StatusVector::raise() const
{
	if (hasData())
	{
		status_exception::raise(*this);
	}
	status_exception::raise(Gds(isc_random) << Str("Attempt to raise empty exception"));
}

ISC_STATUS StatusVector::ImplStatusVector::copyTo(ISC_STATUS* dest) const throw()
{
	if (hasData())
	{
		memcpy(dest, value(), (length() + 1u) * sizeof(ISC_STATUS));
	}
	else
	{
		dest[0] = isc_arg_gds;
		dest[1] = FB_SUCCESS;
		dest[2] = isc_arg_end;
	}
	return dest[1];
}

ISC_STATUS StatusVector::ImplStatusVector::copyTo(IStatus* dest) const throw()
{
	if (hasData())
	{
		dest->set(length() + 1u, value());
	}
	else
	{
		ISC_STATUS t[3] = {isc_arg_gds, FB_SUCCESS, isc_arg_end};
		dest->set(3, t);
	}
	return dest->get()[1];
}

Gds::Gds(ISC_STATUS s) throw() :
	StatusVector(isc_arg_gds, s) { }

PrivateDyn::PrivateDyn(ISC_STATUS codeWithoutFacility) throw() :
	Gds(ENCODE_ISC_MSG(codeWithoutFacility, DYN_MSG_FAC)) { }

Num::Num(ISC_STATUS s) throw() :
	Base(isc_arg_number, s) { }

Interpreted::Interpreted(const char* text) throw() :
	StatusVector(isc_arg_interpreted, (ISC_STATUS)(IPTR) text) { }

Interpreted::Interpreted(const AbstractString& text) throw() :
	StatusVector(isc_arg_interpreted, (ISC_STATUS)(IPTR) text.c_str()) { }

Unix::Unix(ISC_STATUS s) throw() :
	Base(isc_arg_unix, s) { }

Mach::Mach(ISC_STATUS s) throw() :
	Base(isc_arg_next_mach, s) { }

Windows::Windows(ISC_STATUS s) throw() :
	Base(isc_arg_win32, s) { }

Warning::Warning(ISC_STATUS s) throw() :
	StatusVector(isc_arg_warning, s) { }

// Str overloading.
Str::Str(const char* text) throw() :
	Base(isc_arg_string, (ISC_STATUS)(IPTR) text) { }

Str::Str(const AbstractString& text) throw() :
	Base(isc_arg_string, (ISC_STATUS)(IPTR) text.c_str()) { }

Str::Str(const MetaName& text) throw() :
	Base(isc_arg_string, (ISC_STATUS)(IPTR) text.c_str()) { }

SqlState::SqlState(const char* text) throw() :
	Base(isc_arg_sql_state, (ISC_STATUS)(IPTR) text) { }

SqlState::SqlState(const AbstractString& text) throw() :
	Base(isc_arg_sql_state, (ISC_STATUS)(IPTR) text.c_str()) { }

OsError::OsError() throw() :
#ifdef WIN_NT
	Base(isc_arg_win32, GetLastError()) { }
#else
	Base(isc_arg_unix, errno) { }
#endif

OsError::OsError(ISC_STATUS s) throw() :
#ifdef WIN_NT
	Base(isc_arg_win32, s) { }
#else
	Base(isc_arg_unix, s) { }
#endif
} // namespace Arg

} // namespace Firebird
