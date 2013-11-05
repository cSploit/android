/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pwd.cpp
 *	DESCRIPTION:	User information database access
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
 * 2003.02.02 Dmitry Yemanov: Implemented cached security database connection
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/enc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/scl.h"
#include "../common/config/config.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/init.h"
#include "../common/classes/ClumpletWriter.h"

using namespace Jrd;
using namespace Firebird;

// BLR to search database for user name record

const UCHAR SecurityDatabase::PWD_REQUEST[] =
{
	blr_version5,
	blr_begin,
	blr_message, 1, 4, 0,
	blr_long, 0,
	blr_long, 0,
	blr_short, 0,
	blr_text, UCHAR(MAX_PASSWORD_LENGTH + 2), 0,
	blr_message, 0, 1, 0,
	blr_cstring, 129, 0,
	blr_receive, 0,
	blr_begin,
	blr_for,
	blr_rse, 1,
	blr_relation, 9, 'R', 'D', 'B', '$', 'U', 'S', 'E', 'R', 'S', 0,
	blr_first,
	blr_literal, blr_short, 0, 1, 0,
	blr_boolean,
	blr_eql,
	blr_field, 0, 13, 'R', 'D', 'B', '$', 'U', 'S', 'E', 'R', '_', 'N', 'A', 'M', 'E',
	blr_parameter, 0, 0, 0,
	blr_end,
	blr_send, 1,
	blr_begin,
	blr_assignment,
	blr_field, 0, 7, 'R', 'D', 'B', '$', 'G', 'I', 'D',
	blr_parameter, 1, 0, 0,
	blr_assignment,
	blr_field, 0, 7, 'R', 'D', 'B', '$', 'U', 'I', 'D',
	blr_parameter, 1, 1, 0,
	blr_assignment,
	blr_literal, blr_short, 0, 1, 0,
	blr_parameter, 1, 2, 0,
	blr_assignment,
	blr_field, 0, 10, 'R', 'D', 'B', '$', 'P', 'A', 'S', 'S', 'W', 'D',
	blr_parameter, 1, 3, 0,
	blr_end,
	blr_send, 1,
	blr_assignment,
	blr_literal, blr_short, 0, 0, 0,
	blr_parameter, 1, 2, 0,
	blr_end,
	blr_end,
	blr_eoc
};

// Transaction parameter buffer

const UCHAR SecurityDatabase::TPB[4] =
{
	isc_tpb_version1,
	isc_tpb_read,
	isc_tpb_concurrency,
	isc_tpb_wait
};

#ifndef EMBEDDED
namespace {
	// Static instance of the database

	GlobalPtr<SecurityDatabase> instance;

	// Disable attempts to brute-force logins/passwords
	class FailedLogin
	{
	public:
		string login;
		int	failCount;
		time_t lastAttempt;

		explicit FailedLogin(const string& l)
			: login(l), failCount(1), lastAttempt(time(0))
		{}

		FailedLogin(MemoryPool& p, const FailedLogin& fl)
			: login(p, fl.login), failCount(fl.failCount), lastAttempt(fl.lastAttempt)
		{}

		static const string* generate(const void* /*sender*/, const FailedLogin* f)
		{
			return &f->login;
		}
	};

	const size_t MAX_CONCURRENT_FAILURES = 16;
	const int MAX_FAILED_ATTEMPTS = 4;
	const int FAILURE_DELAY = 8; // seconds

	class FailedLogins : private SortedObjectsArray<FailedLogin,
		InlineStorage<FailedLogin*, MAX_CONCURRENT_FAILURES>,
		const string, FailedLogin>
	{
	private:
		// as long as we have voluntary threads scheduler,
		// this mutex should be entered AFTER that scheduler entered!
		Mutex fullAccess;

		typedef SortedObjectsArray<FailedLogin,
			InlineStorage<FailedLogin*, MAX_CONCURRENT_FAILURES>,
			const string, FailedLogin> inherited;

	public:
		explicit FailedLogins(MemoryPool& p)
			: inherited(p)
		{}

		void loginFail(const string& login)
		{
			MutexLockGuard guard(fullAccess);

			const time_t t = time(0);

			size_t pos;
			if (find(login, pos))
			{
				FailedLogin& l = (*this)[pos];
				if (t - l.lastAttempt >= FAILURE_DELAY)
				{
					l.failCount = 0;
				}
				l.lastAttempt = t;
				if (++l.failCount >= MAX_FAILED_ATTEMPTS)
				{
					l.failCount = 0;
					Jrd::DelayFailedLogin::raise(FAILURE_DELAY);
				}
				return;
			}

			if (getCount() >= MAX_CONCURRENT_FAILURES)
			{
				// try to perform old entries collection
				for (iterator i = begin(); i != end(); )
				{
					if (t - i->lastAttempt >= FAILURE_DELAY)
					{
						remove(i);
					}
					else
					{
						++i;
					}
				}
			}
			if (getCount() >= MAX_CONCURRENT_FAILURES)
			{
				// it seems we are under attack - too many wrong logins !!!
				Jrd::DelayFailedLogin::raise(FAILURE_DELAY);
			}

			add(FailedLogin(login));
		}

		void loginSuccess(const string& login)
		{
			MutexLockGuard guard(fullAccess);
			size_t pos;
			if (find(login, pos))
			{
				remove(pos);
			}
		}
	};

	InitInstance<FailedLogins> usernameFailedLogins;
	InitInstance<FailedLogins> remoteFailedLogins;
}
#endif //EMBEDDED

/******************************************************************************
 *
 *	Private interface
 */

void SecurityDatabase::fini()
{
#ifndef EMBEDDED
	MutexLockGuard guard(mutex);

	if (server_shutdown)
	{
		return;
	}

	fb_assert(counter > 0);

	if (--counter <= 0)
	{
		closeDatabase();
	}
#endif //EMBEDDED
}

void SecurityDatabase::init()
{
#ifndef EMBEDDED
	MutexLockGuard guard(mutex);

	if (server_shutdown)
	{
		return;
	}

	if (counter++ == 0)
	{
		if (fb_shutdown_callback(status, onShutdown, fb_shut_preproviders, this))
		{
			status_exception::raise(status);
		}
	}
#endif //EMBEDDED
}

void SecurityDatabase::closeDatabase()
{
#ifndef EMBEDDED
	// assumes mutex is locked

	if (lookup_req)
	{
		isc_release_request(status, &lookup_req);
		checkStatus("isc_release_request");
	}

	if (lookup_db)
	{
		isc_detach_database(status, &lookup_db);
		checkStatus("isc_detach_database");
	}
#endif //EMBEDDED
}

void SecurityDatabase::onShutdown()
{
#ifndef EMBEDDED
	isc_db_handle tmp = 0;
	try {
		MutexLockGuard guard(mutex);

		if (server_shutdown)
		{
			return;
		}

		server_shutdown = true;
		tmp = lookup_db;
		lookup_db = 0;
		closeDatabase();
	}
	catch (const Firebird::Exception&)
	{
		if (tmp)
		{
			isc_detach_database(status, &tmp);
		}
	}

	if (tmp)
	{
		isc_detach_database(status, &tmp);
		checkStatus("isc_detach_database");
	}
#endif //EMBEDDED
}

bool SecurityDatabase::lookupUser(const TEXT* user_name, int* uid, int* gid, TEXT* pwd)
{
	bool found = false;		// user found flag
	TEXT uname[129];		// user name buffer
	user_record user;		// user record

	// Start by clearing the output data

	if (uid)
		*uid = 0;
	if (gid)
		*gid = 0;
	if (pwd)
		*pwd = '\0';

	strncpy(uname, user_name, sizeof uname);
	uname[sizeof uname - 1] = 0;

	MutexLockGuard guard(mutex);

	if (server_shutdown)
	{
		return false;
	}

	// Attach database and compile request

	prepare();

	// Lookup

	isc_tr_handle lookup_trans = 0;

	isc_start_transaction(status, &lookup_trans, 1, &lookup_db, sizeof(TPB), TPB);
	checkStatus("isc_start_transaction", isc_psw_start_trans);

	isc_start_and_send(status, &lookup_req, &lookup_trans, 0, sizeof(uname), uname, 0);
	checkStatus("isc_start_and_send");

	while (true)
	{
		isc_receive(status, &lookup_req, 1, sizeof(user), &user, 0);
		checkStatus("isc_receive");

		if (!user.flag || status[1])
			break;
		found = true;
		if (uid)
			*uid = user.uid;
		if (gid)
			*gid = user.gid;
		if (pwd)
		{
			strncpy(pwd, user.password, MAX_PASSWORD_LENGTH);
			pwd[MAX_PASSWORD_LENGTH] = 0;
		}
	}

	isc_rollback_transaction(status, &lookup_trans);
	checkStatus("isc_rollback_transaction");

	return found;
}

void SecurityDatabase::prepare()
{
	TEXT user_info_name[MAXPATHLEN];

	if (lookup_db)
	{
		return;
	}

	lookup_db = lookup_req = 0;

	// Initialize the database name
	getPath(user_info_name);

	// Perhaps build up a dpb
	ClumpletWriter dpb(ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);

	// Attachment is for the security database
	dpb.insertByte(isc_dpb_sec_attach, TRUE);

	// Attach as SYSDBA
	dpb.insertString(isc_dpb_trusted_auth, SYSDBA_USER_NAME, strlen(SYSDBA_USER_NAME));

	isc_attach_database(status, 0, user_info_name, &lookup_db,
		dpb.getBufferLength(), reinterpret_cast<const char*>(dpb.getBuffer()));
	checkStatus("isc_attach_database", isc_psw_attach);

	isc_compile_request(status, &lookup_db, &lookup_req, sizeof(PWD_REQUEST),
		reinterpret_cast<const char*>(PWD_REQUEST));
	if (status[1])
	{
		ISC_STATUS_ARRAY localStatus;
		// ignore status returned in order to keep first error
		isc_detach_database(localStatus, &lookup_db);
	}
	checkStatus("isc_compile_request", isc_psw_attach);
}

/******************************************************************************
 *
 *	Public interface
 */

void SecurityDatabase::initialize()
{
#ifndef EMBEDDED
	instance->init();
#endif //EMBEDDED
}

void SecurityDatabase::shutdown()
{
#ifndef EMBEDDED
	instance->fini();
#endif //EMBEDDED
}

int SecurityDatabase::onShutdown(const int, const int, void* me)
{
#ifndef EMBEDDED
	((SecurityDatabase*) me)->onShutdown();
#endif //EMBEDDED
	return FB_SUCCESS;
}

void SecurityDatabase::verifyUser(string& name,
								  const TEXT* user_name,
								  const TEXT* password,
								  const TEXT* password_enc,
								  int* uid,
								  int* gid,
								  int* node_id,
								  const string& remoteId)
{
	if (user_name)
	{
		name = user_name;
		for (unsigned int n = 0; n < name.length(); ++n)
		{
			name[n] = UPPER7(name[n]);
		}
	}

#ifndef EMBEDDED
	else
	{
		remoteFailedLogins().loginFail(remoteId);
		status_exception::raise(Arg::Gds(isc_login));
	}

	static AmCache useNative = AM_UNKNOWN;
	if (useNative == AM_UNKNOWN)
	{
		// We use PathName for string comparison using platform filename comparison
		// rules (case-sensitive or case-insensitive).
		const PathName authMethod(Config::getAuthMethod());
		useNative = (authMethod == AmNative || authMethod == AmMixed) ? AM_ENABLED : AM_DISABLED;
	}
	if (useNative == AM_DISABLED)
	{
		remoteFailedLogins().loginFail(remoteId);
		status_exception::raise(Arg::Gds(isc_login));
	}

	// Look up the user name in the userinfo database and use the parameters
	// found there. This means that another database must be accessed, and
	// that means the current context must be saved and restored.

	TEXT pw1[MAX_PASSWORD_LENGTH + 1];
	const bool found = instance->lookupUser(name.c_str(), uid, gid, pw1);
	pw1[MAX_PASSWORD_LENGTH] = 0;
	string storedHash(pw1, MAX_PASSWORD_LENGTH);
	storedHash.rtrim();

	// Punt if the user has specified neither a raw nor an encrypted password,
	// or if the user has specified both a raw and an encrypted password,
	// or if the user name specified was not in the password database
	// (or if there was no password database - it's still not found)

	if ((!password && !password_enc) || (password && password_enc) || !found)
	{
		usernameFailedLogins().loginFail(name);
		remoteFailedLogins().loginFail(remoteId);
		status_exception::raise(Arg::Gds(isc_login));
	}

	TEXT pwt[MAX_PASSWORD_LENGTH + 2];
	if (password)
	{
		ENC_crypt(pwt, sizeof pwt, password, PASSWORD_SALT);
		password_enc = pwt + 2;
	}

	string newHash;
	hash(newHash, name, password_enc, storedHash);
	if (newHash != storedHash)
	{
		bool legacyHash = Config::getLegacyHash();
		if (legacyHash)
		{
			newHash.resize(MAX_PASSWORD_LENGTH + 2);
			ENC_crypt(newHash.begin(), newHash.length(), password_enc, PASSWORD_SALT);
			newHash.recalculate_length();
			newHash.erase(0, 2);
			legacyHash = newHash == storedHash;
		}
		if (! legacyHash)
		{
			usernameFailedLogins().loginFail(name);
			remoteFailedLogins().loginFail(remoteId);
			status_exception::raise(Arg::Gds(isc_login));
		}
	}

	usernameFailedLogins().loginSuccess(name);
	remoteFailedLogins().loginSuccess(remoteId);
#endif

	*node_id = 0;
}

void SecurityDatabase::checkStatus(const char* callName, ISC_STATUS userError)
{
	if (status[1] == 0)
	{
		return;
	}

	string message;
	message.printf("Error in %s() API call when working with security database", callName);
	iscLogStatus(message.c_str(), status);

#ifdef DEV_BUILD
	// throw original status error
	status_exception::raise(status);
#else
	// showing real problems with security database to users is not good idea
	// from security POV - therefore some generic message is used
	Arg::Gds(userError).raise();
#endif
}


void DelayFailedLogin::raise(int sec)
{
	throw DelayFailedLogin(sec);
}

ISC_STATUS DelayFailedLogin::stuff_exception(ISC_STATUS* const status_vector) const throw()
{
	Arg::Gds(isc_login).copyTo(status_vector);

	return status_vector[1];
}

void DelayFailedLogin::sleep() const
{
	THREAD_SLEEP(1000 * seconds);
}
