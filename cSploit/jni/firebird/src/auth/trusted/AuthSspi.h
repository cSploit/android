/*
 *	PROGRAM:		Firebird authentication
 *	MODULE:			AuthSspi.h
 *	DESCRIPTION:	Windows trusted authentication
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
 *  Copyright (c) 2006 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */
#ifndef AUTH_SSPI_H
#define AUTH_SSPI_H

#include <firebird.h>

// This is old versions backward compatibility
#define FB_PREDEFINED_GROUP "Predefined_Group"
#define FB_DOMAIN_ANY_RID_ADMINS "DOMAIN_ANY_RID_ADMINS"

#ifdef TRUSTED_AUTH

#include <../common/classes/fb_string.h>
#include <../common/classes/array.h>
#include "../common/classes/ImplementHelper.h"
#include <../jrd/ibase.h>
#include "firebird/Auth.h"

#define SECURITY_WIN32

#include <windows.h>
#include <Security.h>
#include <stdio.h>

namespace Auth {

class AuthSspi
{
private:
	enum {BUFSIZE = 4096};

	SecHandle secHndl;
	bool hasCredentials;
	CtxtHandle ctxtHndl;
	bool hasContext;
	Firebird::string ctName;
	bool wheel;

	// Handle of library
	static HINSTANCE library;

	// declare entries, required from secur32.dll
	ACQUIRE_CREDENTIALS_HANDLE_FN_A fAcquireCredentialsHandle;
	DELETE_SECURITY_CONTEXT_FN fDeleteSecurityContext;
	FREE_CREDENTIALS_HANDLE_FN fFreeCredentialsHandle;
	QUERY_CONTEXT_ATTRIBUTES_FN_A fQueryContextAttributes;
	FREE_CONTEXT_BUFFER_FN fFreeContextBuffer;
	INITIALIZE_SECURITY_CONTEXT_FN_A fInitializeSecurityContext;
	ACCEPT_SECURITY_CONTEXT_FN fAcceptSecurityContext;

	bool checkAdminPrivilege(PCtxtHandle phContext) const;
	bool initEntries();

public:
	typedef Firebird::Array<unsigned char> DataHolder;

	AuthSspi();
	~AuthSspi();

	// true when has non-empty security context,
	// ready to be sent to the other side
	bool isActive() const
	{
		return hasContext;
	}

	// prepare security context to be sent to the server (used by client)
	bool request(DataHolder& data);

	// accept security context from the client (used by server)
	bool accept(DataHolder& data);

	// returns Windows user name, matching accepted security context
	bool getLogin(Firebird::string& login, bool& wh);
};

class WinSspiServer : public Firebird::StdPlugin<IServer, FB_AUTH_SERVER_VERSION>
{
public:
	// IServer implementation
	int FB_CARG authenticate(Firebird::IStatus* status, IServerBlock* sBlock, IWriter* writerInterface);
    int FB_CARG release();

	WinSspiServer(Firebird::IPluginConfig*);

private:
	AuthSspi::DataHolder sspiData;
	AuthSspi sspi;
};

class WinSspiClient : public Firebird::StdPlugin<IClient, FB_AUTH_CLIENT_VERSION>
{
public:
	// IClient implementation
	int FB_CARG authenticate(Firebird::IStatus* status, IClientBlock* sBlock);
    int FB_CARG release();

	WinSspiClient(Firebird::IPluginConfig*);

private:
	AuthSspi::DataHolder sspiData;
	AuthSspi sspi;
};

void registerTrustedClient(Firebird::IPluginManager* iPlugin);
void registerTrustedServer(Firebird::IPluginManager* iPlugin);

} // namespace Auth

#endif // TRUSTED_AUTH
#endif // AUTH_SSPI_H
