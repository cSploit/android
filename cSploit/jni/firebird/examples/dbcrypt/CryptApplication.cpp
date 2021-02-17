/*
 *	PROGRAM:		Firebird samples.
 *	MODULE:			CryptApplication.cpp
 *	DESCRIPTION:	Sample of passing a key to crypt plugin
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
 *  Copyright (c) 2012 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ibase.h>
#include <firebird/Crypt.h>
#include <firebird/Provider.h>

using namespace Firebird;

class CryptKey : public ICryptKeyCallback
{
public:
	unsigned int FB_CARG callback(unsigned int, const void*, unsigned int length, void* buffer)
	{
		if (length > 0 && buffer)
		{
			char k = 0x5a;
			memcpy(buffer, &k, 1);
			fprintf(stderr, "\nTransfered key to server\n");
		}
		return 1;
	}

	int FB_CARG getVersion()
	{
		return FB_CRYPT_CALLBACK_VERSION;
	}

	IPluginModule* FB_CARG getModule()
	{
		return NULL;		// OK for application, not for plugin
	}
};

class App
{
public:
	App() :
		master(fb_get_master_interface()), status(master->getStatus()),
		p(NULL), att(NULL), tra(NULL)
	{ }

	~App()
	{
		if (tra)
		{
			tra->rollback(status);
			if (!status->isSuccess())
			{
				print("rollback");
				tra->release();
			}
		}
		if (att)
		{
			att->detach(status);
			if (!status->isSuccess())
			{
				print("detach");
				att->release();
			}
		}
		if (p)
		{
			p->release();
		}
		status->dispose();
	}

	enum Action {NONE, ENC, DEC};

	void execute(const char* dbName, const Action a)
	{
		status->init();

		p = master->getDispatcher();

		p->setDbCryptCallback(status, &key);
		if (!status->isSuccess())
			throw "setDbCryptCallback";

		char s[256];
		sprintf(s, "localhost:%s", dbName);
		att = p->attachDatabase(status, s, 0, NULL);
		if (!status->isSuccess())
			throw "attachDatabase";

		if (a != NONE)
		{
			tra = att->startTransaction(status, 0, NULL);
			if (!status->isSuccess())
				throw "startTransaction";
		}

		if (a == ENC)
		{
			att->execute(status, tra, 0,
				"ALTER DATABASE ENCRYPT WITH \"DbCrypt_example\"", 3, NULL, NULL, NULL, NULL);
			if (!status->isSuccess())
				throw "execute";
		}
		if (a == DEC)
		{
			att->execute(status, tra, 0, "ALTER DATABASE DECRYPT", 3, NULL, NULL, NULL, NULL);
			if (!status->isSuccess())
				throw "execute";
		}

		if (tra)
		{
			tra->commit(status);
			if (!status->isSuccess())
				throw "commit";
			tra = NULL;
		}

		printf("Providing key for crypt plugin - press enter to continue ...");
		getchar();

		att->detach(status);
		if (!status->isSuccess())
			throw "detach";
		att = NULL;

		p->release();
		p = NULL;
	}

	void print(const char* where)
	{
		fprintf(stderr, "Error in %s: ", where);
		isc_print_status(status->get());
	}

private:
	IMaster* master;
	IStatus* status;
	IProvider* p;
	IAttachment* att;
	ITransaction* tra;

	CryptKey key;
};

int usage()
{
	fprintf(stderr, "Usage: CryptApplication [ -e | -d ] { db-name }\n");
	return 2;
}

int main(int ac, char** av)
{
	App::Action act = App::NONE;

	if (ac < 2 || ac > 3)
		return usage();

	if (ac == 3)
	{
		if (av[1][0] != '-')
			return usage();

		switch(av[1][1])
		{
		case 'e':
			act = App::ENC;
			break;
		case 'd':
			act = App::DEC;
			break;
		default:
			return usage();
		}
		av++;
	}

	setenv("ISC_USER", "sysdba", 0);
	setenv("ISC_PASSWORD", "masterkey", 0);

	App app;
	try
	{
		app.execute(av[1], act);
	}
	catch (const char* where)
	{
		app.print(where);
		return 1;
	}

	return 0;
}
