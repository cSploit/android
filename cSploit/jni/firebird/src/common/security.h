/*
 *	PROGRAM:	Security data base manager
 *	MODULE:		secur_proto.h
 *	DESCRIPTION:	Prototype header file for security.epp
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef UTILITIES_SECUR_PROTO_H
#define UTILITIES_SECUR_PROTO_H

#include "firebird/Auth.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/classes/GetPlugins.h"
#include "../common/classes/array.h"

namespace Auth {

class CharField : public Firebird::AutoIface<ICharUserField, FB_AUTH_CHAR_FIELD_VERSION>
{
public:
	CharField()
		: e(0), s(0), value(*getDefaultMemoryPool())
	{ }

	// ICharUserField implementation
	int FB_CARG entered()
	{
		return e;
	}

	int FB_CARG specified()
	{
		return s;
	}

	void FB_CARG setEntered(int newValue)
	{
		e = newValue;
	}

	void setSpecified(int newValue)
	{
		s = newValue;
		if (s)
		{
			value = "";
		}
	}

	const char* FB_CARG get()
	{
		return value.c_str();
	}

	void FB_CARG set(const char* newValue)
	{
		value = newValue ? newValue : "";
	}

	void FB_CARG set(const char* newValue, size_t len)
	{
		value.assign(newValue, len);
	}

	void clear()
	{
		e = s = 0;
		value = "";
	}

private:
	int e, s;
	Firebird::string value;
};

class IntField : public Firebird::AutoIface<IIntUserField, FB_AUTH_INT_FIELD_VERSION>
{
public:
	IntField()
		: e(0), s(0), value(0)
	{ }

	// IIntUserField implementation
	int FB_CARG entered()
	{
		return e;
	}

	int FB_CARG specified()
	{
		return s;
	}

	void FB_CARG setEntered(int newValue)
	{
		e = newValue;
	}

	void setSpecified(int newValue)
	{
		s = newValue;
		if (s)
		{
			value = 0;
		}
	}

	int FB_CARG get()
	{
		return value;
	}

	void FB_CARG set(int newValue)
	{
		value = newValue;
	}

	void clear()
	{
		e = s = 0;
		value = 0;
	}

private:
	int e, s;
	int value;
};

class UserData : public IUser
{
public:
	UserData()
		: op(0), trustedAuth(0), authenticationBlock(*getDefaultMemoryPool())
	{ }

	// IUser implementation
	int FB_CARG operation()
	{
		return op;
	}

	ICharUserField* FB_CARG userName()
	{
		return &user;
	}

	ICharUserField* FB_CARG password()
	{
		return &pass;
	}

	ICharUserField* FB_CARG firstName()
	{
		return &first;
	}

	ICharUserField* FB_CARG lastName()
	{
		return &last;
	}

	ICharUserField* FB_CARG middleName()
	{
		return &middle;
	}

	ICharUserField* FB_CARG comment()
	{
		return &com;
	}

	ICharUserField* FB_CARG attributes()
	{
		return &attr;
	}

	IIntUserField* FB_CARG admin()
	{
		return &adm;
	}

	IIntUserField* FB_CARG active()
	{
		return &act;
	}

	void FB_CARG clear();

	typedef Firebird::Array<UCHAR> AuthenticationBlock;

	int op, trustedAuth;
	CharField user, pass, first, last, middle, com, attr;
	IntField adm, act;
	CharField database, dba, dbaPassword, role;
	AuthenticationBlock authenticationBlock;

	// deprecated
	CharField group;
	IntField u, g;
};

class StackUserData FB_FINAL : public Firebird::AutoIface<UserData, FB_AUTH_USER_VERSION>
{
};

class DynamicUserData FB_FINAL : public Firebird::VersionedIface<UserData, FB_AUTH_USER_VERSION>
{
public:

#ifdef DEBUG_GDS_ALLOC
	void* operator new(size_t size, Firebird::MemoryPool& pool, const char* fileName, int line)
	{
		return pool.allocate(size, fileName, line);
	}
#else	// DEBUG_GDS_ALLOC
	void* operator new(size_t size, Firebird::MemoryPool& pool)
	{
		return pool.allocate(size);
	}
#endif	// DEBUG_GDS_ALLOC
};

class Get : public Firebird::GetPlugins<Auth::IManagement>
{
public:
	Get(Config* firebirdConf);
};

int setGsecCode(int code, IUser* iUser);

} // namespace Auth

#endif // UTILITIES_SECUR_PROTO_H
