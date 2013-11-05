/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		jrd_pwd.h
 *	DESCRIPTION:	User information database name
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
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.02.02 Dmitry Yemanov: Implemented cached security database connection
 */

#ifndef JRD_PWD_H
#define JRD_PWD_H

#include "../jrd/ibase.h"
#include "../common/utils_proto.h"
#include "../jrd/sha.h"
#include "gen/iberror.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <time.h>

const size_t MAX_PASSWORD_ENC_LENGTH = 12;		// passed by remote protocol
const size_t MAX_PASSWORD_LENGTH = 64;			// used to store passwords internally
static const char* const PASSWORD_SALT = "9z";	// for old ENC_crypt()
const size_t SALT_LENGTH = 12;					// measured after base64 coding

namespace Jrd {

class SecurityDatabase
{
	struct user_record
	{
		SLONG gid;
		SLONG uid;
		SSHORT flag;
		SCHAR password[MAX_PASSWORD_LENGTH + 2];
	};

public:
	static void getPath(TEXT* path_buffer)
	{
		static const char* USER_INFO_NAME = "security2.fdb";
		Firebird::PathName name = fb_utils::getPrefix(fb_utils::FB_DIR_SECDB, USER_INFO_NAME);
		name.copyTo(path_buffer, MAXPATHLEN);
	}

	static void initialize();
	static void shutdown();
	static void verifyUser(Firebird::string&, const TEXT*, const TEXT*, const TEXT*,
		int*, int*, int*, const Firebird::string&);

	static void hash(Firebird::string& h, const Firebird::string& userName, const TEXT* passwd)
	{
		Firebird::string salt;
		Jrd::CryptSupport::random(salt, SALT_LENGTH);
		hash(h, userName, passwd, salt);
	}

	static void hash(Firebird::string& h,
					 const Firebird::string& userName,
					 const TEXT* passwd,
					 const Firebird::string& oldHash)
	{
		Firebird::string salt(oldHash);
		salt.resize(SALT_LENGTH, '=');
		Firebird::string allData(salt);
		allData += userName;
		allData += passwd;
		Jrd::CryptSupport::hash(h, allData);
		h = salt + h;
	}

private:
	static const UCHAR PWD_REQUEST[256];
	static const UCHAR TPB[4];

	Firebird::Mutex mutex;

	ISC_STATUS_ARRAY status;

	isc_db_handle lookup_db;
	isc_req_handle lookup_req;

	int counter;
	bool server_shutdown;

	void fini();
	void init();
	void closeDatabase();
	void onShutdown();
	bool lookupUser(const TEXT*, int*, int*, TEXT*);
	void prepare();
	void checkStatus(const char* callName, ISC_STATUS userError = isc_psw_db_error);

	static int onShutdown(const int, const int, void*);

public:
	explicit SecurityDatabase(Firebird::MemoryPool&)
		: lookup_db(0), lookup_req(0), counter(0), server_shutdown(false)
	{}

public:
	// Shuts SecurityDatabase in case of errors during attach or create.
	// When attachment is created, control is passed to it using clear.
	class InitHolder
	{
	public:
		InitHolder()
			: shutdown(true)
		{
			SecurityDatabase::initialize();
		}

		void clear()
		{
			shutdown = false;
		}

		~InitHolder()
		{
			if (shutdown)
			{
				SecurityDatabase::shutdown();
			}
		}

	private:
		bool shutdown;
	};
};

class DelayFailedLogin : public Firebird::Exception
{
private:
	int seconds;

public:
	explicit DelayFailedLogin(int sec) throw() : Exception(), seconds(sec) { }

	virtual ISC_STATUS stuff_exception(ISC_STATUS* const status_vector) const throw();
	virtual const char* what() const throw() { return "Jrd::DelayFailedLogin"; }
	static void raise(int sec);
	void sleep() const;
};


} // namespace Jrd

#endif // JRD_PWD_H
