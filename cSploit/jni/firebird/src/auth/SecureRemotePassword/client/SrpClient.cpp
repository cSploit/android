/*
 *	PROGRAM:		Firebird authentication.
 *	MODULE:			SrpClient.cpp
 *	DESCRIPTION:	SPR authentication plugin.
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
 *  Copyright (c) 2011 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"

#include "../auth/SecureRemotePassword/client/SrpClient.h"
#include "../auth/SecureRemotePassword/srp.h"
#include "../common/classes/ImplementHelper.h"

using namespace Firebird;

namespace Auth {

class SrpClient FB_FINAL : public StdPlugin<IClient, FB_AUTH_CLIENT_VERSION>
{
public:
	explicit SrpClient(IPluginConfig*)
		: client(NULL), data(getPool()),
		  sessionKey(getPool())
	{ }

	// IClient implementation
	int FB_CARG authenticate(IStatus*, IClientBlock* cb);
    int FB_CARG release();

private:
	RemotePassword* client;
	string data;
	UCharBuffer sessionKey;
};

int SrpClient::authenticate(IStatus* status, IClientBlock* cb)
{
	try
	{
		if (sessionKey.hasData())
		{
			// Why are we called when auth is completed?
			(Arg::Gds(isc_random) << "Auth sync failure - SRP's authenticate called more times than supported").raise();
		}

		if (!client)
		{
			HANDSHAKE_DEBUG(fprintf(stderr, "Cli: SRP phase1: login=%s password=%s\n",
				cb->getLogin(), cb->getPassword()));
			if (!(cb->getLogin() && cb->getPassword()))
			{
				return AUTH_CONTINUE;
			}

			client = new RemotePassword;
			client->genClientKey(data);
			dumpIt("Clnt: clientPubKey", data);
			cb->putData(status, data.length(), data.begin());
			return status->isSuccess() ? AUTH_MORE_DATA : AUTH_FAILED;
		}

		HANDSHAKE_DEBUG(fprintf(stderr, "Cli: SRP phase2\n"));
		unsigned length;
		const unsigned char* saltAndKey = cb->getData(&length);
		if (!saltAndKey || length == 0)
		{
			Arg::Gds(isc_auth_data).raise();
		}
		const unsigned expectedLength =
			(RemotePassword::SRP_SALT_SIZE + RemotePassword::SRP_KEY_SIZE + 2) * 2;
		if (length > expectedLength)
		{
			(Arg::Gds(isc_auth_datalength) << Arg::Num(length) <<
				Arg::Num(expectedLength) << "data").raise();
		}

		string salt, key;
		unsigned charSize = *saltAndKey++;
		charSize += ((unsigned) *saltAndKey++) << 8;
		if (charSize > RemotePassword::SRP_SALT_SIZE * 2)
		{
			(Arg::Gds(isc_auth_datalength) << Arg::Num(charSize) <<
				Arg::Num(RemotePassword::SRP_SALT_SIZE * 2) << "salt").raise();
		}
		salt.assign(saltAndKey, charSize);
		dumpIt("Clnt: salt", salt);
		saltAndKey += charSize;
		length -= (charSize + 2);

		charSize = *saltAndKey++;
		charSize += ((unsigned) *saltAndKey++) << 8;
		if (charSize != length - 2)
		{
			(Arg::Gds(isc_auth_datalength) << Arg::Num(charSize) <<
				Arg::Num(length - 2) << "key").raise();
		}
		key.assign(saltAndKey, charSize);
		dumpIt("Clnt: key(srvPub)", key);

		dumpIt("Clnt: login", string(cb->getLogin()));
		dumpIt("Clnt: pass", string(cb->getPassword()));
		client->clientSessionKey(sessionKey, cb->getLogin(), salt.c_str(), cb->getPassword(), key.c_str());
		dumpIt("Clnt: sessionKey", sessionKey);

		BigInteger cProof = client->clientProof(cb->getLogin(), salt.c_str(), sessionKey);
		cProof.getText(data);

		cb->putData(status, data.length(), data.c_str());
		if (!status->isSuccess())
		{
			return AUTH_FAILED;
		}

		// output the key
		FbCryptKey cKey = {"Symmetric", sessionKey.begin(), NULL, sessionKey.getCount(), 0};
		cb->putKey(status, &cKey);
		if (!status->isSuccess())
		{
			return AUTH_FAILED;
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
		return AUTH_FAILED;
	}

	return AUTH_SUCCESS;
}

int SrpClient::release()
{
	if (--refCounter == 0)
	{
		delete this;
		return 0;
	}
	return 1;
}

namespace
{
	SimpleFactory<SrpClient> factory;
}

void registerSrpClient(IPluginManager* iPlugin)
{
	iPlugin->registerPluginFactory(PluginType::AuthClient, RemotePassword::plugName, &factory);
}

} // namespace Auth
