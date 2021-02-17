/*
 *	PROGRAM:		Firebird authentication
 *	MODULE:			Auth.h
 *	DESCRIPTION:	Interfaces, used by authentication plugins
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
 *  Copyright (c) 2010 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_AUTH_INTERFACE
#define FB_AUTH_INTERFACE

#include "firebird/Plugin.h"

namespace Firebird {
class IStatus;
}

namespace Auth {

const int AUTH_SUCCESS = 0;
const int AUTH_MORE_DATA = 1;
const int AUTH_CONTINUE = 2;
const int AUTH_FAILED = -1;

class IWriter : public Firebird::IVersioned
{
public:
	virtual void FB_CARG reset() = 0;
	virtual void FB_CARG add(const char* name) = 0;
	virtual void FB_CARG setType(const char* value) = 0;
	virtual void FB_CARG setDb(const char* value) = 0;
};
#define FB_AUTH_WRITER_VERSION (FB_VERSIONED_VERSION + 4)

// Representation of auth-related data, passed to/from server auth plugin
class IServerBlock : public Firebird::IVersioned
{
public:
	virtual const char* FB_CARG getLogin() = 0;
	virtual const unsigned char* FB_CARG getData(unsigned int* length) = 0;
	virtual void FB_CARG putData(Firebird::IStatus* status, unsigned int length, const void* data) = 0;
	virtual void FB_CARG putKey(Firebird::IStatus* status, Firebird::FbCryptKey* cryptKey) = 0;
};
#define FB_AUTH_SERVER_BLOCK_VERSION (FB_VERSIONED_VERSION + 4)

// Representation of auth-related data, passed to/from client auth plugin
class IClientBlock : public Firebird::IRefCounted
{
public:
	virtual const char* FB_CARG getLogin() = 0;
	virtual const char* FB_CARG getPassword() = 0;
	virtual const unsigned char* FB_CARG getData(unsigned int* length) = 0;
	virtual void FB_CARG putData(Firebird::IStatus* status, unsigned int length, const void* data) = 0;
	virtual void FB_CARG putKey(Firebird::IStatus* status, Firebird::FbCryptKey* cryptKey) = 0;
};
#define FB_AUTH_CLIENT_BLOCK_VERSION (FB_REFCOUNTED_VERSION + 5)

// server part of authentication plugin
class IServer : public Firebird::IPluginBase
{
public:
	virtual int FB_CARG authenticate(Firebird::IStatus* status, IServerBlock* sBlock, IWriter* writerInterface) = 0;
};
#define FB_AUTH_SERVER_VERSION (FB_PLUGIN_VERSION + 1)

// .. and corresponding client
class IClient : public Firebird::IPluginBase
{
public:
	virtual int FB_CARG authenticate(Firebird::IStatus* status, IClientBlock* cBlock) = 0;
};
#define FB_AUTH_CLIENT_VERSION (FB_PLUGIN_VERSION + 1)

class IUserField : public Firebird::IVersioned
{
public:
	virtual int FB_CARG entered() = 0;
	virtual int FB_CARG specified() = 0;
	virtual void FB_CARG setEntered(int newValue) = 0;
};
#define FB_AUTH_FIELD_VERSION (FB_VERSIONED_VERSION + 3)

class ICharUserField : public IUserField
{
public:
	virtual const char* FB_CARG get() = 0;
	virtual void FB_CARG set(const char* newValue) = 0;
};
#define FB_AUTH_CHAR_FIELD_VERSION (FB_AUTH_FIELD_VERSION + 2)

class IIntUserField : public IUserField
{
public:
	virtual int FB_CARG get() = 0;
	virtual void FB_CARG set(int newValue) = 0;
};
#define FB_AUTH_INT_FIELD_VERSION (FB_AUTH_FIELD_VERSION + 2)

class IUser : public Firebird::IVersioned
{
public:
	virtual int FB_CARG operation() = 0;

	virtual ICharUserField* FB_CARG userName() = 0;
	virtual ICharUserField* FB_CARG password() = 0;

	virtual ICharUserField* FB_CARG firstName() = 0;
	virtual ICharUserField* FB_CARG lastName() = 0;
	virtual ICharUserField* FB_CARG middleName() = 0;

	virtual ICharUserField* FB_CARG comment() = 0;
	virtual ICharUserField* FB_CARG attributes() = 0;
	virtual IIntUserField* FB_CARG active() = 0;

	virtual IIntUserField* FB_CARG admin() = 0;

	virtual void FB_CARG clear() = 0;
};
#define FB_AUTH_USER_VERSION (FB_VERSIONED_VERSION + 11)

class IListUsers : public Firebird::IVersioned
{
public:
	virtual void FB_CARG list(IUser* user) = 0;
};
#define FB_AUTH_LIST_USERS_VERSION (FB_VERSIONED_VERSION + 1)

class ILogonInfo : public Firebird::IVersioned
{
public:
	virtual const char* FB_CARG name() = 0;
	virtual const char* FB_CARG role() = 0;
	virtual const char* FB_CARG networkProtocol() = 0;
	virtual const char* FB_CARG remoteAddress() = 0;
	virtual unsigned int FB_CARG authBlock(const unsigned char** bytes) = 0;
};
#define FB_AUTH_LOGON_INFO_VERSION (FB_VERSIONED_VERSION + 5)

class IManagement : public Firebird::IPluginBase
{
public:
	virtual void FB_CARG start(Firebird::IStatus* status, ILogonInfo* logonInfo) = 0;
	virtual int FB_CARG execute(Firebird::IStatus* status, IUser* user, IListUsers* callback) = 0;
	virtual void FB_CARG commit(Firebird::IStatus* status) = 0;
	virtual void FB_CARG rollback(Firebird::IStatus* status) = 0;
};
#define FB_AUTH_MANAGE_VERSION (FB_PLUGIN_VERSION + 4)

} // namespace Auth


#endif // FB_AUTH_INTERFACE
