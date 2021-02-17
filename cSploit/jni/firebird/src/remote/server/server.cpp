/*
 *	PROGRAM:	JRD Remote Server
 *	MODULE:		server.cpp
 *	DESCRIPTION:	Remote server
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
 * 2002.02.27 Claudio Valderrama: Fix SF Bug #509995.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/ibase.h"			// fb_shutdown_callback() is used from it
#include "../common/gdsassert.h"
#ifdef UNIX
#include "../common/file_params.h"
#endif
#include "../remote/remote.h"
#include "../common/file_params.h"
#include "../common/ThreadStart.h"
#include "../jrd/license.h"
#include "../common/classes/timestamp.h"
#include "../remote/merge_proto.h"
#include "../remote/parse_proto.h"
#include "../remote/remot_proto.h"
#include "../remote/server/serve_proto.h"
#include "../common/xdr_proto.h"
#ifdef WIN_NT
#include "../../remote/server/os/win32/cntl_proto.h"
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "../common/isc_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/constants.h"
#include "../jrd/inf_pub.h"
#include "../common/classes/init.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"
#ifdef DEBUG
#include "gen/iberror.h"
#endif
#include "../common/classes/FpeControl.h"
#include "../remote/proto_proto.h"	// xdr_protocol_overhead()
#include "../common/classes/DbImplementation.h"
#include "../common/Auth.h"
#include "../common/classes/GetPlugins.h"
#include "../common/StatementMetadata.h"
#include "../common/isc_f_proto.h"
#include "../auth/SecurityDatabase/LegacyHash.h"
#include "../common/enc_proto.h"
#include "../common/classes/InternalMessageBuffer.h"

using namespace Firebird;


struct server_req_t : public GlobalStorage
{
	server_req_t*	req_next;
	server_req_t*	req_chain;
	RemPortPtr		req_port;
	PACKET			req_send;
	PACKET			req_receive;
public:
	server_req_t() : req_next(0), req_chain(0) { }
};

struct srvr : public GlobalStorage
{
	srvr* const srvr_next;
	const rem_port*	const srvr_parent_port;
	const rem_port::rem_port_t srvr_port_type;
	const USHORT srvr_flags;

public:
	srvr(srvr* servers, rem_port* port, USHORT flags) :
		srvr_next(servers), srvr_parent_port(port),
		srvr_port_type(port->port_type), srvr_flags(flags)
	{ }
};

namespace {

const USHORT PARSER_VERSION = 2;

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

	static const string* generate(const FailedLogin* f)
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
	Mutex fullAccess;

	typedef SortedObjectsArray<FailedLogin,
		InlineStorage<FailedLogin*, MAX_CONCURRENT_FAILURES>,
		const string, FailedLogin> inherited;

public:
	explicit FailedLogins(MemoryPool& p)
		: inherited(p)
	{}

	bool loginFail(const string& login)
	{
		if (!login.hasData())
		{
			return false;
		}

		MutexLockGuard guard(fullAccess, FB_FUNCTION);
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
				return true;
			}
			return false;
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
			// it seems we are under attack - too many wrong logins !!
			return true;
		}

		add(FailedLogin(login));
		return false;
	}

	void loginSuccess(const string& login)
	{
		if (!login.hasData())
		{
			return;
		}

		MutexLockGuard guard(fullAccess, FB_FUNCTION);

		size_t pos;
		if (find(login, pos))
		{
			remove(pos);
		}
	}
};

GlobalPtr<FailedLogins> usernameFailedLogins;
GlobalPtr<FailedLogins> remoteFailedLogins;

void loginFail(const string& login, const string& remId)
{
	// do not remove variables - both functions should be called
	bool f1 = usernameFailedLogins->loginFail(login);
	bool f2 = remoteFailedLogins->loginFail(remId);
	if (f1 || f2)
	{
		// Ahh, someone is too active today
		THREAD_SLEEP(FAILURE_DELAY * 1000);
	}
}

void loginSuccess(const string& login, const string& remId)
{
	usernameFailedLogins->loginSuccess(login);
	remoteFailedLogins->loginSuccess(remId);
}


MakeUpgradeInfo<> upInfo;

template <typename T>
static void getMultiPartConnectParameter(T& putTo, Firebird::ClumpletReader& id, UCHAR param)
{
	// This array is needed only to make sure that all parts of specific data are present
	UCHAR checkBytes[256];
	memset(checkBytes, 0, sizeof(checkBytes));
	UCHAR top = 0;

	for (id.rewind(); !id.isEof(); id.moveNext())
	{
		if (id.getClumpTag() == param)
		{
			const UCHAR* specData = id.getBytes();
			size_t len = id.getClumpLength();
			fb_assert(len <= 255);

			if (len > 1)
			{
				--len;
				unsigned offset = specData[0];
				if (offset + 1 > top)
					top = offset + 1;
				if (checkBytes[offset])
				{
					(Arg::Gds(isc_random) << "Invalid CNCT block: repeated data").raise();	// print offset No here
				}
				checkBytes[offset] = 1;

				offset *= 254;
				++specData;
				putTo.grow(offset + len);
				memcpy(&putTo[offset], specData, len);
			}
		}
	}

	for (UCHAR segment = 0; segment < top; ++segment)
	{
		if (!checkBytes[segment])
			(Arg::Gds(isc_multi_segment) << Arg::Num(segment)).raise();
	}

	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: getMultiPartConnectParameter: loaded tag %d length %" SIZEFORMAT "\n",
		param, putTo.getCount()));
}


// delayed authentication block for auth callback
class ServerAuth : public GlobalStorage, public ServerAuthBase
{
public:
	virtual void accept(PACKET* send, Auth::WriterImplementation* authBlock) = 0;

	ServerAuth(ClumpletReader* aPb, const ParametersSet& aTags,
			   rem_port* port, bool multiPartData = false)
		: authItr(NULL),
		  userName(getPool()),
		  authServer(NULL),
		  tags(&aTags),
		  hopsCount(0),
		  authPort(port)
	{
		if (!authPort->port_srv_auth_block)
		{
			authPort->port_srv_auth_block = new SrvAuthBlock(authPort);
		}

		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth()\n"));

		if (aPb->find(tags->user_name))
		{
			aPb->getString(userName);
			userName.upper();
			if (authPort->port_srv_auth_block->getLogin() &&
				userName != authPort->port_srv_auth_block->getLogin())
			{
				(Arg::Gds(isc_login) << Arg::Gds(isc_login_changed)).raise();
			}
			authPort->port_srv_auth_block->setLogin(userName);
			HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth(): user name=%s\n", userName.c_str()));
		}

		UCharBuffer u;
		if (port->port_protocol >= PROTOCOL_VERSION13)
		{
			string x;

			if (aPb->find(tags->plugin_name))
			{
				aPb->getString(x);
				authPort->port_srv_auth_block->setPluginName(x);
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth(): plugin name=%s\n", x.c_str()));
			}

			if (aPb->find(tags->plugin_list))
			{
				aPb->getString(x);
				authPort->port_srv_auth_block->setPluginList(x);
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth(): plugin list=%s\n", x.c_str()));
			}

			if (tags->specific_data && aPb->find(tags->specific_data))
			{
				if (multiPartData)
					getMultiPartConnectParameter(u, *aPb, tags->specific_data);
				else
					aPb->getData(u);
				authPort->port_srv_auth_block->setDataForPlugin(u);
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth(): plugin data is %" SIZEFORMAT " len\n", u.getCount()));
			}
			else
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth(): miss data with tag %d\n", tags->specific_data));
		}
		else if (authPort->port_srv_auth_block->getLogin() &&
			(aPb->find(tags->password_enc) || aPb->find(tags->password)))
		{
			authPort->port_srv_auth_block->setPluginName("Legacy_Auth");
			authPort->port_srv_auth_block->setPluginList("Legacy_Auth");
			aPb->getData(u);
			if (aPb->getClumpTag() == tags->password)
			{
				TEXT pwt[Auth::MAX_LEGACY_PASSWORD_LENGTH + 2];
				u.push(0);
				ENC_crypt(pwt, sizeof pwt, reinterpret_cast<TEXT*>(u.begin()),
					Auth::LEGACY_PASSWORD_SALT);
				const size_t len = strlen(&pwt[2]);
				memcpy(u.getBuffer(len), &pwt[2], len);
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ServerAuth(): CALLED des locally\n"));
			}
			authPort->port_srv_auth_block->setDataForPlugin(u);
		}
#ifdef WIN_NT
		else if (aPb->find(tags->trusted_auth) && port->port_protocol >= PROTOCOL_VERSION11)
		{
			authPort->port_srv_auth_block->setPluginName("Win_Sspi");
			authPort->port_srv_auth_block->setPluginList("Win_Sspi");
			aPb->getData(u);
			authPort->port_srv_auth_block->setDataForPlugin(u);
		}
#endif
	}

	~ServerAuth()
	{ }

	bool authenticate(PACKET* send, AuthenticateFlags flags)
	{
#ifdef DEV_BUILD
		if (++hopsCount > 10)
#else
		if (++hopsCount > 100)
#endif
		{
			(Arg::Gds(isc_login) << Arg::Gds(isc_auth_handshake_limit)).raise();
		}

		if (authPort->port_srv_auth_block->authCompleted())
		{
			accept(send, &authPort->port_srv_auth_block->authBlockWriter);
			return true;
		}

		bool working = true;
		LocalStatus st;

		authPort->port_srv_auth_block->createPluginsItr();
		authItr = authPort->port_srv_auth_block->plugins;
		if (!authItr)
		{
			if (authPort->port_protocol >= PROTOCOL_VERSION13)
			{
				send->p_operation = op_cont_auth;
				authPort->port_srv_auth_block->extractDataFromPluginTo(&send->p_auth_cont);
				authPort->send(send);
				memset(&send->p_auth_cont, 0, sizeof send->p_auth_cont);
				return false;
			}
		}

		while (authItr && working && authItr->hasData())
		{
			if (!authServer)
			{
				authServer = authItr->plugin();
				authPort->port_srv_auth_block->authBlockWriter.setPlugin(authItr->name());
			}

			// if we asked for more data but received nothing switch to next plugin
			const bool forceNext = (flags & CONT_AUTH) && (!authPort->port_srv_auth_block->hasDataForPlugin());
			HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: ServerAuth calls plug %s\n",
				forceNext ? "forced-NEXT" : authItr->name()));
			int authResult = forceNext ? Auth::AUTH_CONTINUE :
				authServer->authenticate(&st, authPort->port_srv_auth_block,
					&authPort->port_srv_auth_block->authBlockWriter);
			authPort->port_srv_auth_block->setPluginName(authItr->name());

			cstring* s;

			switch (authResult)
			{
			case Auth::AUTH_SUCCESS:
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: Ahh - success\n"));
				loginSuccess(userName, authPort->getRemoteId());
				authServer = NULL;
				authPort->port_srv_auth_block->authCompleted(true);
				accept(send, &authPort->port_srv_auth_block->authBlockWriter);
				return true;

			case Auth::AUTH_CONTINUE:
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: Next plug suggested\n"));
				authItr->next();
				authServer = NULL;
				continue;

			case Auth::AUTH_MORE_DATA:
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: plugin wants more data\n"));
				if (authPort->port_protocol < PROTOCOL_VERSION11)
				{
					authServer = NULL;
					working = false;
					break;
				}

				if (authPort->port_protocol >= PROTOCOL_VERSION13)
				{
					if (flags & USE_COND_ACCEPT)
					{
						HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: send op_cond_accept\n"));
						send->p_operation = op_cond_accept;
						authPort->port_srv_auth_block->extractDataFromPluginTo(&send->p_acpd);
						authPort->port_srv_auth_block->extractNewKeys(&send->p_acpd.p_acpt_keys);
					}
					else
					{
						HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: send op_cont_auth\n"));
						send->p_operation = op_cont_auth;
						authPort->port_srv_auth_block->extractDataFromPluginTo(&send->p_auth_cont);
						authPort->port_srv_auth_block->extractNewKeys(&send->p_auth_cont.p_keys);
					}
				}
				else
				{
					if (REMOTE_legacy_auth(authItr->name(), authPort->port_protocol))
					{
						// compatibility with FB 2.1 trusted
						send->p_operation = op_trusted_auth;

						s = &send->p_trau.p_trau_data;
						s->cstr_allocated = 0;
						authPort->port_srv_auth_block->extractDataFromPluginTo(&send->p_trau.p_trau_data);
					}
					else
					{
						authServer = NULL;
						working = false;
						break;
					}
				}

				authPort->send(send);
				memset(&send->p_auth_cont, 0, sizeof send->p_auth_cont);
				return false;

			case Auth::AUTH_FAILED:
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: authenticate: No luck today - status:\n"));
				HANDSHAKE_DEBUG(isc_print_status(st.get()));
				authServer = NULL;
				working = false;
				break;
			}
		}

		// no success - perform failure processing
		loginFail(userName, authPort->getRemoteId());

		Arg::Gds loginError(isc_login);
#ifndef DEV_BUILD
		if (st.get()[1] == isc_missing_data_structures)
#endif
		{
			loginError << Arg::StatusVector(st.get());
		}
		status_exception::raise(loginError.value());
		return false;	// compiler warning silencer
	}

private:
	AuthServerPlugins* authItr;
	string userName;
	Auth::IServer* authServer;
	const ParametersSet* tags;
	unsigned int hopsCount;

protected:
	rem_port* authPort;
};


class DatabaseAuth : public ServerAuth
{
public:
	DatabaseAuth(rem_port* port, const PathName& db, ClumpletWriter* dpb, P_OP op)
		: ServerAuth(dpb, dpbParam, port),
		  dbName(getPool(), db),
		  pb(dpb),
		  operation(op)
	{ }

	void accept(PACKET* send, Auth::WriterImplementation* authBlock);

private:
	PathName dbName;
	AutoPtr<ClumpletWriter> pb;
	P_OP operation;
};


class ServiceAttachAuth : public ServerAuth
{
public:
	ServiceAttachAuth(rem_port* port, const PathName& pmanagerName, ClumpletWriter* spb)
		: ServerAuth(spb, spbParam, port),
		  managerName(getPool(), pmanagerName),
		  pb(spb)
	{ }

	void accept(PACKET* send, Auth::WriterImplementation* authBlock);

private:
	PathName managerName;
	AutoPtr<ClumpletWriter> pb;
};


#ifdef WIN_NT
class GlobalPortLock
{
public:
	explicit GlobalPortLock(int id)
		: handle(INVALID_HANDLE_VALUE)
	{
		if (id)
		{
			TEXT mutex_name[MAXPATHLEN];
			fb_utils::snprintf(mutex_name, sizeof(mutex_name), "FirebirdPortMutex%d", id);
			fb_utils::prefix_kernel_object_name(mutex_name, sizeof(mutex_name));

			if (!(handle = CreateMutex(ISC_get_security_desc(), FALSE, mutex_name)))
			{
				// MSDN: if the caller has limited access rights, the function will fail with
				// ERROR_ACCESS_DENIED and the caller should use the OpenMutex function.
				if (GetLastError() == ERROR_ACCESS_DENIED)
					system_call_failed::raise("CreateMutex - cannot open existing mutex");
				else
					system_call_failed::raise("CreateMutex");
			}

			if (WaitForSingleObject(handle, INFINITE) == WAIT_FAILED)
			{
				system_call_failed::raise("WaitForSingleObject");
			}
		}
	}

	~GlobalPortLock()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			if (!ReleaseMutex(handle))
			{
				system_call_failed::raise("ReleaseMutex");
			}

			CloseHandle(handle);
		}
	}

private:
	HANDLE handle;
};
#else
class GlobalPortLock
{
public:
	explicit GlobalPortLock(int id)
		: fd(-1)
	{
		if (id)
		{
			mtx->enter(FB_FUNCTION);

			string firebirdPortMutex;
			firebirdPortMutex.printf(PORT_FILE, id);
			TEXT filename[MAXPATHLEN];
			gds__prefix_lock(filename, firebirdPortMutex.c_str());
			while ((fd = open(filename, O_WRONLY | O_CREAT, 0666)) < 0)
			{
				if (errno != EINTR)
				{
					system_call_failed::raise("open");
				}
			}

			struct flock lock;
			lock.l_type = F_WRLCK;
			lock.l_whence = 0;
			lock.l_start = 0;
			lock.l_len = 0;
			while (fcntl(fd, F_SETLKW, &lock) == -1)
			{
				if (errno != EINTR)
				{
					system_call_failed::raise("fcntl");
				}
			}
		}
	}

	~GlobalPortLock()
	{
		if (fd != -1)
		{
			struct flock lock;
			lock.l_type = F_UNLCK;
			lock.l_whence = 0;
			lock.l_start = 0;
			lock.l_len = 0;
			while (fcntl(fd, F_SETLK, &lock) == -1)
			{
				if (errno != EINTR)
				{
					system_call_failed::raise("fcntl");
				}
			}

			close(fd);

			mtx->leave();
		}
	}

private:
	int fd;
	static GlobalPtr<Mutex> mtx;
};

GlobalPtr<Mutex> GlobalPortLock::mtx;
#endif

class Callback FB_FINAL : public RefCntIface<IEventCallback, FB_EVENT_CALLBACK_VERSION>
{
public:
	explicit Callback(Rvnt* aevent)
		: event(aevent)
	{ }

	// IEventCallback implementation
	virtual void FB_CARG eventCallbackFunction(unsigned int length, const UCHAR* items)
	/**************************************
	 *
	 *	s e r v e r _ a s t
	 *
	 **************************************
	 *
	 * Functional description
	 *	Send an asynchronous event packet back to client.
	 *
	 **************************************/
	{
		Rdb* rdb = event->rvnt_rdb;

		rem_port* port = rdb->rdb_port->port_async;
		if (!port)
		{
			return;
		}

		RefMutexGuard portGuard(*port->port_sync, FB_FUNCTION);

		PACKET packet;
		packet.p_operation = op_event;
		P_EVENT* p_event = &packet.p_event;
		p_event->p_event_database = rdb->rdb_id;
		p_event->p_event_items.cstr_length = length;
		p_event->p_event_items.cstr_address = items;
		p_event->p_event_rid = event->rvnt_id;

		event->rvnt_iface = NULL;

		port->send(&packet);
	}

	virtual int FB_CARG release()
	{
		if (--refCounter == 0)
		{
			delete this;
			return 0;
		}

		return 1;
	}

private:
	Rvnt* event;
};

class CryptKeyTypeManager : public PermanentStorage
{
	class CryptKeyType : public PermanentStorage
	{
	public:
		explicit CryptKeyType(MemoryPool& p)
			: PermanentStorage(p), keyType(getPool()), plugins(getPool())
		{ }

		bool operator==(const PathName& t) const
		{
			return keyType == t;
		}

		void set(const PathName& t, const PathName& p)
		{
			fb_assert(keyType.isEmpty() && plugins.isEmpty());
			keyType = t;
			plugins.add() = p;
		}

		void add(const PathName& p)
		{
			plugins.add() = p;		// Here we assume that init code runs once, i.e. plugins do not repeat
		}

		void value(PathName& to) const
		{
			REMOTE_makeList(to, plugins);
		}

	private:
		PathName keyType;
		Remote::ParsedList plugins;
	};

public:
	explicit CryptKeyTypeManager(MemoryPool& p)
		: PermanentStorage(p), knownTypes(getPool())
	{
		LocalStatus st;
		for (GetPlugins<IWireCryptPlugin> cpItr(PluginType::WireCrypt, FB_WIRECRYPT_PLUGIN_VERSION,
												upInfo); cpItr.hasData(); cpItr.next())
		{
			const char* list = cpItr.plugin()->getKnownTypes(&st);
			if (! st.isSuccess())
			{
				status_exception::raise(st.get());
			}

			fb_assert(list);
			Remote::ParsedList newTypes;
			REMOTE_parseList(newTypes, PathName(list));

			PathName plugin(cpItr.name());
			for (unsigned i = 0; i < newTypes.getCount(); ++i)
			{
				bool found = false;
				for (unsigned j = 0; j < knownTypes.getCount(); ++j)
				{
					if (knownTypes[j] == newTypes[i])
					{
						knownTypes[j].add(plugin);
						found = true;
						break;
					}
				}

				if (!found)
				{
					knownTypes.add().set(newTypes[i], plugin);
				}
			}
		}
	}

	PathName operator[](const PathName& keyType) const
	{
		for (unsigned j = 0; j < knownTypes.getCount(); ++j)
		{
			if (knownTypes[j] == keyType)
			{
				PathName rc;
				knownTypes[j].value(rc);
				return rc;
			}
		}

		return "";
	}

private:
	ObjectsArray<CryptKeyType> knownTypes;
};

InitInstance<CryptKeyTypeManager> knownCryptKeyTypes;

class CryptKeyCallback : public VersionedIface<ICryptKeyCallback, FB_CRYPT_CALLBACK_VERSION>
{
public:
	explicit CryptKeyCallback(rem_port* prt)
		: port(prt), l(0), d(NULL)
	{ }

	unsigned int FB_CARG callback(unsigned int dataLength, const void* data,
		unsigned int bufferLength, void* buffer)
	{
		Reference r(*port);

		PACKET p;
		p.p_operation = op_crypt_key_callback;
		p.p_cc.p_cc_data.cstr_length = dataLength;
		p.p_cc.p_cc_data.cstr_address = (UCHAR*) data;
		p.p_cc.p_cc_reply = bufferLength;
		port->send(&p);

		sem.enter();
		if (bufferLength > l)
			bufferLength = l;
		memcpy(buffer, d, bufferLength);
		if (l)
			sem2.release();

		return l;
	}

	void wakeup(unsigned int length, const void* data)
	{
		l = length;
		d = data;
		sem.release();
		if (l)
			sem2.enter();
	}

private:
	rem_port* port;
	Semaphore sem, sem2;
	unsigned int l;
	const void* d;
};

class ServerCallback : public ServerCallbackBase, public GlobalStorage
{
public:
	explicit ServerCallback(rem_port* prt)
		: cryptCallback(prt)
	{ }

	~ServerCallback()
	{ }

	void wakeup(unsigned int length, const void* data)
	{
		cryptCallback.wakeup(length, data);
	}

	ICryptKeyCallback* getInterface()
	{
		return &cryptCallback;
	}

private:
	CryptKeyCallback cryptCallback;
};

} // anonymous

static void		free_request(server_req_t*);
static server_req_t* alloc_request();
static bool		link_request(rem_port*, server_req_t*);

static bool		accept_connection(rem_port*, P_CNCT*, PACKET*);
static ISC_STATUS	allocate_statement(rem_port*, /*P_RLSE*,*/ PACKET*);
static void		append_request_chain(server_req_t*, server_req_t**);
static void		append_request_next(server_req_t*, server_req_t**);
static void		attach_database(rem_port*, P_OP, P_ATCH*, PACKET*);
static void		attach_service(rem_port*, P_ATCH*, PACKET*);
static void		trusted_auth(rem_port*, const P_TRAU*, PACKET*);
static void		continue_authentication(rem_port*, const p_auth_continue*, PACKET*);

static void		aux_request(rem_port*, /*P_REQ*,*/ PACKET*);
static bool		bad_port_context(IStatus*, IRefCounted*, const ISC_STATUS);
static ISC_STATUS	cancel_events(rem_port*, P_EVENT*, PACKET*);
static void		addClumplets(ClumpletWriter*, const ParametersSet&, const rem_port*);

static void		cancel_operation(rem_port*, USHORT);

static bool		check_request(Rrq*, USHORT, USHORT);
static USHORT	check_statement_type(Rsr*);

static bool		get_next_msg_no(Rrq*, USHORT, USHORT*);
static Rtr*		make_transaction(Rdb*, ITransaction*);
static void		ping_connection(rem_port*, PACKET*);
static bool		process_packet(rem_port* port, PACKET* sendL, PACKET* receive, rem_port** result);
static void		release_blob(Rbl*);
static void		release_event(Rvnt*);
static void		release_request(Rrq*);
static void		release_statement(Rsr**);
static void		release_sql_request(Rsr*);
static void		release_transaction(Rtr*);

static void		send_error(rem_port* port, PACKET* apacket, ISC_STATUS errcode);
static void		send_error(rem_port* port, PACKET* apacket, const Firebird::Arg::StatusVector&);
static void		set_server(rem_port*, USHORT);
static int		shut_server(const int, const int, void*);
static THREAD_ENTRY_DECLARE loopThread(THREAD_ENTRY_PARAM);
static void		zap_packet(PACKET*, bool);


inline bool bad_db(IStatus* status_vector, Rdb* rdb)
{
	IRefCounted* iface = NULL;
	if (rdb)
		iface = rdb->rdb_iface;
	return bad_port_context(status_vector, iface, isc_bad_db_handle);
}

inline bool bad_service(IStatus* status_vector, Rdb* rdb)
{
	IRefCounted* iface = NULL;
	if (rdb && rdb->rdb_svc.hasData())
		iface = rdb->rdb_svc->svc_iface;

	return bad_port_context(status_vector, iface, isc_bad_svc_handle);
}


class Worker
{
public:
	static const int MAX_THREADS = MAX_SLONG;
	static const int IDLE_TIMEOUT = 60;

	Worker();
	~Worker();

	bool wait(int timeout = IDLE_TIMEOUT);	// true is success, false if timeout
	static bool wakeUp();

	void setState(const bool active);
	static void start(USHORT flags);

	static int getCount() { return m_cntAll; }

	static bool isShuttingDown() { return shutting_down; }

	static void shutdown();

private:
	Worker* m_next;
	Worker* m_prev;
	Semaphore m_sem;
	bool	m_active;
#ifdef DEV_BUILD
	ThreadId	m_tid;
#endif

	void remove();
	void insert(const bool active);
	static void wakeUpAll();

	static Worker* m_activeWorkers;
	static Worker* m_idleWorkers;
	static GlobalPtr<Mutex> m_mutex;
	static int m_cntAll;
	static int m_cntIdle;
	static bool shutting_down;
};

Worker* Worker::m_activeWorkers = NULL;
Worker* Worker::m_idleWorkers = NULL;
GlobalPtr<Mutex> Worker::m_mutex;
int Worker::m_cntAll = 0;
int Worker::m_cntIdle = 0;
bool Worker::shutting_down = false;


static GlobalPtr<Mutex> request_que_mutex;
static server_req_t* request_que		= NULL;
static server_req_t* free_requests		= NULL;
static server_req_t* active_requests	= NULL;
static int ports_active					= 0;	// length of active_requests
static int ports_pending				= 0;	// length of request_que

static GlobalPtr<Mutex> servers_mutex;
static srvr* servers = NULL;
static AtomicCounter cntServers;
static bool	server_shutdown = false;


static const UCHAR request_info[] =
{
	isc_info_state,
	isc_info_message_number,
	isc_info_end
};

static const UCHAR sql_info[] =
{
	isc_info_sql_stmt_type,
	isc_info_sql_batch_fetch
};


#define GDS_DSQL_ALLOCATE	isc_dsql_allocate_statement
#define GDS_DSQL_EXECUTE	isc_dsql_execute2_m
#define GDS_DSQL_EXECUTE_IMMED	isc_dsql_exec_immed3_m
#define GDS_DSQL_FETCH		isc_dsql_fetch_m
#define GDS_DSQL_FREE		isc_dsql_free_statement
#define GDS_DSQL_INSERT		isc_dsql_insert_m
#define GDS_DSQL_PREPARE	isc_dsql_prepare_m
#define GDS_DSQL_SET_CURSOR	isc_dsql_set_cursor_name
#define GDS_DSQL_SQL_INFO	isc_dsql_sql_info


void SRVR_enum_attachments(ULONG& att_cnt, ULONG& dbs_cnt, ULONG& svc_cnt)
/**************************************
 *
 *	S R V R _ e n u m _ a t t a c h m e n t s
 *
 **************************************
 *
 * Functional description
 *	Queries the service manager to enumerate
 *  active attachments to the server.
 *
 **************************************/
{
	att_cnt = dbs_cnt = svc_cnt = 0;

	// TODO: we need some way to return a number of active services
	// using the service manager. It could be either isc_info_svc_svr_db_info2
	// that adds the third counter, or a completely new information tag.

	DispatcherPtr provider;

	static const UCHAR spb_attach[] =
	{
		isc_spb_version, isc_spb_current_version,
		isc_spb_user_name, 6, 'S', 'Y', 'S', 'D', 'B', 'A'
	};

	LocalStatus status;
	ServService iface(provider->attachServiceManager(&status, "service_mgr",
					  sizeof(spb_attach), spb_attach));

	if (iface.hasData())
	{
		static const UCHAR spb_query[] = {isc_info_svc_svr_db_info};

		UCHAR buffer[BUFFER_XLARGE];
		iface->query(&status, 0, NULL, sizeof(spb_query), spb_query, sizeof(buffer), buffer);
		const UCHAR* p = buffer;

		if (status.isSuccess() && *p++ == isc_info_svc_svr_db_info)
		{
			while (*p != isc_info_flag_end)
			{
				switch (*p++)
				{
				case isc_spb_dbname:
					{
						const USHORT length = (USHORT) gds__vax_integer(p, sizeof(USHORT));
						p += sizeof(USHORT) + length;
					}
					break;
				case isc_spb_num_att:
					att_cnt = (ULONG) gds__vax_integer(p, sizeof(ULONG));
					p += sizeof(ULONG);
					break;
				case isc_spb_num_db:
					dbs_cnt = (ULONG) gds__vax_integer(p, sizeof(ULONG));
					p += sizeof(ULONG);
					break;
				default:
					fb_assert(false);
				}
			}
		}

		iface->detach(&status);
	}
}

void SRVR_main(rem_port* main_port, USHORT flags)
{
/**************************************
 *
 *	S R V R _ m a i n
 *
 **************************************
 *
 * Functional description
 *	Main entrypoint of server.
 *
 **************************************/

	FpeControl::maskAll();

	// Setup context pool for main thread
	ContextPoolHolder mainThreadContext(getDefaultMemoryPool());

	PACKET send, receive;
	zap_packet(&receive, true);
	zap_packet(&send, true);

	set_server(main_port, flags);

	while (true)
	{
		// Note: The following is cloned in server other SRVR_main instances.

		try
		{
			rem_port* port = main_port->receive(&receive);
			if (!port) {
				break;
			}
			if (!process_packet(port, &send, &receive, &port)) {
				break;
			}
		}
		catch(const Exception& ex)
		{
			// even if it's already logged somewhere, repeat it
			iscLogException("SRVR_main", ex);
			break;
		}
	}
}


static void free_request(server_req_t* request)
{
/**************************************
 *
 *	f r e e  _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	MutexLockGuard queGuard(request_que_mutex, FB_FUNCTION);

	request->req_port = 0;
	request->req_next = free_requests;
	free_requests = request;
}


static server_req_t* alloc_request()
{
/**************************************
 *
 *	a l l o c _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Get request block from the free blocks list,
 *	if empty - allocate the new one.
 *
 **************************************/
	MutexEnsureUnlock queGuard(request_que_mutex, FB_FUNCTION);
	queGuard.enter();

	server_req_t* request = free_requests;
#if defined(DEV_BUILD) && defined(DEBUG)
	int request_count = 0;
#endif

	// Allocate a memory block to store the request in
	if (request)
	{
		free_requests = request->req_next;
	}
	else
	{
		// No block on the free list - allocate some new memory
		for (;;)
		{
			try
			{
				request = new server_req_t;
				break;
			}
			catch (const BadAlloc&)
			{ }

#if defined(DEV_BUILD) && defined(DEBUG)
			if (request_count++ > 4)
				BadAlloc::raise();
#endif

			// System is out of memory, let's delay processing this
			// request and hope another thread will free memory or
			// request blocks that we can then use.

			queGuard.leave();
			THREAD_SLEEP(1 * 1000);
			queGuard.enter();
		}
		zap_packet(&request->req_send, true);
		zap_packet(&request->req_receive, true);
#ifdef DEBUG_REMOTE_MEMORY
		printf("alloc_request         allocate request %x\n", request);
#endif
	}

	request->req_next = NULL;
	request->req_chain = NULL;
	return request;
}


static bool link_request(rem_port* port, server_req_t* request)
{
/**************************************
 *
 *	l i n k _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Search for a port in a queue,
 *	if found - append new request to it.
 *
 **************************************/
	const P_OP operation = request->req_receive.p_operation;
	server_req_t* queue;

	MutexLockGuard queGuard(request_que_mutex, FB_FUNCTION);

	bool active = true;
	queue = active_requests;

	while (true)
	{
		for (; queue; queue = queue->req_next)
		{
			if (queue->req_port == port)
			{
				// Don't queue a dummy keepalive packet if there is a request on this port
				if (operation == op_dummy)
				{
					free_request(request);
					return true;
				}

				append_request_chain(request, &queue->req_chain);
#ifdef DEBUG_REMOTE_MEMORY
				printf("link_request %s request_queued %d\n",
					active ? "ACTIVE" : "PENDING", port->port_requests_queued.value());
				fflush(stdout);
#endif
				break;
			}
		}

		if (queue || !active)
			break;

		queue = request_que;
		active = false;
	}

	if (!queue)
		append_request_next(request, &request_que);

	++port->port_requests_queued;

	if (queue)
	{
		if (operation == op_exit || operation == op_disconnect)
			cancel_operation(port, fb_cancel_raise);
		return true;
	}

	return false;
}


void SRVR_multi_thread( rem_port* main_port, USHORT flags)
{
/**************************************
 *
 *	S R V R _ m u l t i _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Multi-threaded flavor of server.
 *
 **************************************/
	server_req_t* request = NULL;
	RemPortPtr port;		// Was volatile PORT port = NULL;

	// hold reference on main_port to free asyncPacket before deletion of port
	RemPortPtr mainPortRef(main_port);
	PACKET asyncPacket;

	zap_packet(&asyncPacket, true);
	++cntServers;

	try
	{
		set_server(main_port, flags);

		// We need to have this error handler as there is so much underlaying code
		// that is depending on it being there.  The expected failure
		// that can occur in the call paths from here is out-of-memory
		// and it is unknown if other failures can get us here
		// Failures can occur from set_server as well as RECEIVE
		//
		// Note that if a failure DOES occur, we reenter the loop and try to continue
		// operations.  This is important as this is the main receive loop for
		// new incoming requests, if we exit here no new requests can come to the
		// server.

		try
		{
			const size_t MAX_PACKET_SIZE = MAX_SSHORT;
			const SSHORT bufSize = MIN(main_port->port_buff_size, MAX_PACKET_SIZE);
			UCharBuffer packet_buffer;
			UCHAR* const buffer = packet_buffer.getBuffer(bufSize);

#ifdef DEV_BUILD
#ifndef WIN_NT
			if (isatty(2))
			{
				fprintf(stderr, "Server started successfully\n");
			}
#endif
#endif
			// When this loop exits, the server will no longer receive requests
			while (true)
			{
				SSHORT dataSize;
				// We have a request block - now get some information to stick into it
				const bool ok = main_port->select_multi(buffer, bufSize, &dataSize, port);
				if (!port)
				{
					if ((main_port->port_server_flags & SRVR_multi_client) && !server_shutdown)
					{
						gds__log("SRVR_multi_thread/RECEIVE: error on main_port, shutting down");
					}
					break;
				}
				if (dataSize)
				{
					const SSHORT asyncSize = port->asyncReceive(&asyncPacket, buffer, dataSize);
					if (asyncSize == dataSize)
					{
						port = NULL;
						continue;
					}
					dataSize -= asyncSize;
					RefMutexGuard queGuard(*port->port_que_sync, FB_FUNCTION);
					memcpy(port->port_queue.add().getBuffer(dataSize), buffer + asyncSize, dataSize);
				}

				RefMutexEnsureUnlock portGuard(*port->port_sync, FB_FUNCTION);
				const bool portLocked = portGuard.tryEnter();
				// Handle bytes received only if port is currently idle and has no requests
				// queued or if it is disconnect (dataSize == 0). Else let loopThread
				// handle these bytes
				if ((portLocked && !port->port_requests_queued.value()) || !dataSize)
				{
					// Allocate a memory block to store the request in
					request = alloc_request();

					if (dataSize)
					{
						fb_assert(portLocked);
						fb_assert(port->port_requests_queued.value() == 0);

						RefMutexGuard queGuard(*port->port_que_sync, FB_FUNCTION);

						const rem_port::RecvQueState recvState = port->getRecvState();
						port->receive(&request->req_receive);

						if (request->req_receive.p_operation == op_partial)
						{
							port->setRecvState(recvState);
							portGuard.leave();

							free_request(request);
							request = 0;
							port = 0;
							continue;
						}
						if (!port->haveRecvData())
						{
							port->clearRecvQue();
						}
					}
					else
					{
						request->req_receive.p_operation = ok ? op_dummy : op_exit;
					}

					request->req_port = port;
					if (portLocked)
					{
						portGuard.leave();
					}

					// link_request will increment port_requests_queued. Port is not locked
					// at this point but it is safe because :
					// - port_requests_queued is atomic counter
					// - the only place where we check its value is at the same thread (see above)
					// - other thread where port_requests_queued is changed is loopThread and
					//	 there port is locked
					// - same port can be accessed no more than by two threads simultaneously -
					//	 this one and some instance of loopThread
					if (!link_request(port, request))
					{
						// Request was assigned to the waiting queue so we need to wake up a
						// thread to handle it
						REMOTE_TRACE(("Enqueue request %p", request));
						request = 0;
#ifdef DEBUG_REMOTE_MEMORY
						printf("SRVR_multi_thread    APPEND_PENDING     request_queued %d\n",
							port->port_requests_queued.value());
						fflush(stdout);
#endif
						Worker::start(flags);
					}
					request = 0;
				}
				port = 0;
			}

			Worker::shutdown();

			// All worker threads are stopped and will never run any more
			// Disconnect remaining ports gracefully
			rem_port* run_port = main_port;
			while (run_port)
			{
				rem_port* current_port = run_port;	// important order of operations
				run_port = run_port->port_next;		// disconnect() can modify linked list of ports
				if (!(current_port->port_flags & PORT_disconnect))
					current_port->disconnect(NULL, NULL);
			}
		}
		catch (const Exception& e)
		{
			gds__log("SRVR_multi_thread: shutting down due to unhandled exception");
			LocalStatus status_vector;
			e.stuffException(&status_vector);
			gds__log_status(0, status_vector.get());

			// If we got as far as having a port allocated before the error, disconnect it
			// gracefully.

			if (port)
			{
/*
#if defined(DEV_BUILD) && defined(DEBUG)
				ConsolePrintf("%%ISERVER-F-NOPORT, no port in a storm\r\n");
#endif
*/
				gds__log("SRVR_multi_thread: forcefully disconnecting a port");

				// To handle recursion within the error handler
				try
				{
					// If we have a port, request really should be non-null, but just in case ...
					if (request != NULL)
					{
						// Send client a real status indication of why we disconnected them
						// Note that send_response() can post errors that wind up in this same handler
/*
#if defined(DEV_BUILD) && defined(DEBUG)
						ConsolePrintf("%%ISERVER-F-NOMEM, virtual memory exhausted\r\n");
#endif
*/
						port->send_response(&request->req_send, 0, 0, &status_vector, false);
						port->disconnect(&request->req_send, &request->req_receive);
					}
					else
					{
						// Can't tell the client much, just make 'em go away.  Their side should detect
						// a network error

						port->disconnect(NULL, NULL);
					}
					port = NULL;

				}	// try
				catch (const Exception&)
				{
					port->disconnect(NULL, NULL);
					port = NULL;
				}
			}

			// There was an error in the processing of the request, if we have allocated
			// a request, free it up and continue.

			if (request != NULL)
			{
				free_request(request);
				request = NULL;
			}
		}

		if (main_port->port_async_receive)
		{
			REMOTE_free_packet(main_port->port_async_receive, &asyncPacket);
		}
	}
	catch (const Exception&)
	{
		// Some kind of unhandled error occurred during server setup.  In lieu
		// of anything we CAN do, log something (and we might be so hosed
		// we can't log anything) and give up.
		// The likely error here is out-of-memory.
		gds__log("SRVR_multi_thread: error during startup, shutting down");
	}
	--cntServers;
}


bool wireEncryption(rem_port* port, ClumpletReader& id)
{
	if (port->port_type == rem_port::XNET)		// local connection
	{
		port->port_required_encryption = false;
		return false;
	}

	int clientCrypt = id.find(CNCT_client_crypt) ? id.getInt() : WIRE_CRYPT_ENABLED;
	switch(clientCrypt)
	{
	case WIRE_CRYPT_REQUIRED:
	case WIRE_CRYPT_ENABLED:
	case WIRE_CRYPT_DISABLED:
		break;
	default:
		clientCrypt = WIRE_CRYPT_ENABLED;
		break;
	}

	int serverCrypt = port->getPortConfig()->getWireCrypt(WC_SERVER);
	if (wcCompatible[clientCrypt][serverCrypt] < 0)
	{
		Arg::Gds(isc_wirecrypt_incompatible).raise();
	}

	port->port_required_encryption = wcCompatible[clientCrypt][serverCrypt] == 2;
	return wcCompatible[clientCrypt][serverCrypt] > 0;
}


class ConnectAuth : public ServerAuth
{
public:
	ConnectAuth(rem_port* port, ClumpletReader& connect)
		: ServerAuth(&connect, connectParam, port, true), useResponse(false)
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ConnectAuth::ConnectAuth()\n"));
	}

	void accept(PACKET* send, Auth::WriterImplementation* authBlock);

	bool useResponse;
};


static void setErrorStatus(IStatus* status)
{
	Arg::Gds loginError(isc_login);
#ifndef DEV_BUILD
	if (status->get()[1] == isc_missing_data_structures)
#endif
	{
		loginError << Arg::StatusVector(status->get());
	}
	status->set(loginError.value());
}

static bool accept_connection(rem_port* port, P_CNCT* connect, PACKET* send)
{
/**************************************
 *
 *	a c c e p t _ c o n n e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Process a connect packet.
 *
 **************************************/

	// Accept the physical connection
	send->p_operation = op_reject;

	if (!port->accept(connect))
	{
		port->send(send);
		return false;
	}

	// Starting with CONNECT_VERSION3, strings inside the op_connect packet are UTF8 encoded

	if (connect->p_cnct_cversion >= CONNECT_VERSION3)
	{
		ISC_utf8ToSystem(port->port_login);
		ISC_utf8ToSystem(port->port_user_name);
		ISC_utf8ToSystem(port->port_peer_name);
	}

	// Select the most appropriate protocol (this will get smarter)
	P_ARCH architecture = arch_generic;
	USHORT version = 0;
	USHORT type = 0;
	bool accepted = false;
	USHORT weight = 0;
	const p_cnct::p_cnct_repeat* protocol = connect->p_cnct_versions;

	for (const p_cnct::p_cnct_repeat* const end = protocol + connect->p_cnct_count;
		protocol < end; protocol++)
	{
		if ((protocol->p_cnct_version == PROTOCOL_VERSION10 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION11 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION12 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION13) &&
			 (protocol->p_cnct_architecture == arch_generic ||
			  protocol->p_cnct_architecture == ARCHITECTURE) &&
			protocol->p_cnct_weight >= weight)
		{
			accepted = true;
			weight = protocol->p_cnct_weight;
			version = protocol->p_cnct_version;
			architecture = protocol->p_cnct_architecture;
			type = MIN(protocol->p_cnct_max_type, ptype_lazy_send);
		}
	}

	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: accept_connection: protoaccept a=%d (v>=13)=%d %d %d\n",
					accepted, version >= PROTOCOL_VERSION13, version, PROTOCOL_VERSION13));

	send->p_acpd.p_acpt_version = port->port_protocol = version;
	send->p_acpd.p_acpt_architecture = architecture;
	send->p_acpd.p_acpt_type = type;
	send->p_acpd.p_acpt_authenticated = 0;

	send->p_acpt.p_acpt_version = port->port_protocol = version;
	send->p_acpt.p_acpt_architecture = architecture;
	send->p_acpt.p_acpt_type = type;

	// modify the version string to reflect the chosen protocol
	string buffer;
	buffer.printf("%s/P%d", port->port_version->str_data, port->port_protocol & FB_PROTOCOL_MASK);
	delete port->port_version;
	port->port_version = REMOTE_make_string(buffer.c_str());

	if (architecture == ARCHITECTURE)
		port->port_flags |= PORT_symmetric;
	if (type != ptype_out_of_band)
		port->port_flags |= PORT_no_oob;
	if (type == ptype_lazy_send)
		port->port_flags |= PORT_lazy;


	Firebird::ClumpletReader id(Firebird::ClumpletReader::UnTagged,
								connect->p_cnct_user_id.cstr_address,
								connect->p_cnct_user_id.cstr_length);

	if (accepted)
	{
		// Setup correct configuration for port
		PathName dbName(connect->p_cnct_file.cstr_address, connect->p_cnct_file.cstr_length);
		RefPtr<Config> dbConfig = REMOTE_get_config(&dbName);
		port->port_security_db = dbConfig->getSecurityDatabase();
		port->port_config = REMOTE_get_config(port->port_security_db.hasData() ?
			&port->port_security_db : NULL);

		// Clear accept data
		send->p_acpd.p_acpt_plugin.cstr_length = 0;
		send->p_acpd.p_acpt_data.cstr_length = 0;
		send->p_acpd.p_acpt_authenticated = 0;
	}

	if (accepted && wireEncryption(port, id))
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "accepted && wireEncryption\n"));
		if (version >= PROTOCOL_VERSION13)
		{
			ConnectAuth* cnctAuth = new ConnectAuth(port, id);
			port->port_srv_auth = cnctAuth;
			if (port->port_srv_auth->authenticate(send, ServerAuth::USE_COND_ACCEPT))
			{
				delete port->port_srv_auth;
				port->port_srv_auth = NULL;
			}

			cnctAuth->useResponse = true;
			return true;
		}

		if (port->port_required_encryption)
		{
			HANDSHAKE_DEBUG(fprintf(stderr, "port_required_encryption, reset accepted\n"));
			accepted = false;
		}
	}

	// We are going to try authentication handshake
	LocalStatus status;
	bool returnData = false;
	if (accepted && version >= PROTOCOL_VERSION13)
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: accept_connection: creates port_srv_auth_block\n"));
		port->port_srv_auth_block = new SrvAuthBlock(port);

		HANDSHAKE_DEBUG(fprintf(stderr,
			"Srv: accept_connection: is going to load data to port_srv_auth_block\n"));
		port->port_srv_auth_block->load(id);
		if (port->port_srv_auth_block->getLogin())
		{
			port->port_login = port->port_srv_auth_block->getLogin();
		}

		HANDSHAKE_DEBUG(fprintf(stderr,
			"Srv: accept_connection: finished with port_srv_auth_block prepare, a=%d\n", accepted));

		if (port->port_srv_auth_block->getPluginName())
		{
			HANDSHAKE_DEBUG(fprintf(stderr, "Srv: accept_connection: calls createPluginsItr\n"));
			port->port_srv_auth_block->createPluginsItr();

			if (port->port_srv_auth_block->plugins)		// We have all required data and iterator was created
			{
				AuthServerPlugins* const plugins = port->port_srv_auth_block->plugins;
				NoCaseString clientPluginName(port->port_srv_auth_block->getPluginName());
				// If there is plugin matching client's one it will be
				HANDSHAKE_DEBUG(fprintf(stderr, "Srv: accept_connection: client plugin='%s' server='%s'\n",
					clientPluginName.c_str(), plugins->name()));

				if (clientPluginName != plugins->name())
				{
					// createPluginsItr() when possible makes current client's plugin first in the list
					// i.e. if names don't match this means we can't use it and client should continue
					// with the next one from us
					port->port_srv_auth_block->setPluginName(plugins->name());
					port->port_srv_auth_block->extractPluginName(&send->p_acpd.p_acpt_plugin);
					returnData = true;
				}
				else
				{
					port->port_srv_auth_block->authBlockWriter.setPlugin(plugins->name());
					switch (plugins->plugin()->authenticate(&status, port->port_srv_auth_block,
						&port->port_srv_auth_block->authBlockWriter))
					{
					case Auth::AUTH_SUCCESS:
						loginSuccess(port->port_login, port->getRemoteId());
						port->port_srv_auth_block->authCompleted(true);
						send->p_acpd.p_acpt_authenticated = 1;
						returnData = true;
						break;

					case Auth::AUTH_CONTINUE:
						// try next plugin
						plugins->next();
						if (!plugins->hasData())
						{
							// failed
							setErrorStatus(&status);
							accepted = false;
							loginFail(port->port_login, port->getRemoteId());
							break;
						}
						port->port_srv_auth_block->setPluginName(plugins->name());
						port->port_srv_auth_block->extractPluginName(&send->p_acpd.p_acpt_plugin);
						break;

					case Auth::AUTH_MORE_DATA:
						port->port_srv_auth_block->extractPluginName(&send->p_acpd.p_acpt_plugin);
						port->port_srv_auth_block->extractDataFromPluginTo(&send->p_acpd.p_acpt_data);
						returnData = true;
						break;

					case Auth::AUTH_FAILED:
						setErrorStatus(&status);
						accepted = false;
						loginFail(port->port_login, port->getRemoteId());
						break;
					}
				}
			}
		}
	}

	// Send off out gracious acceptance or flag rejection
	if (!accepted)
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "!accepted, sending reject\n"));
		if (!status.isSuccess())
			port->send_response(send, 0, 0, status.get(), false);
		else
			port->send(send);
		return false;
	}

	// extractNewKeys() will also send to client list of known plugins
	if (version >= PROTOCOL_VERSION13 &&
		port->port_srv_auth_block->extractNewKeys(&send->p_acpd.p_acpt_keys, returnData))
	{
		returnData = true;
	}

	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: accept_connection: accepted ud=%d protocol=%x\n", returnData, port->port_protocol));

	send->p_operation = returnData ? op_accept_data : op_accept;
	port->send(send);

	return true;
}


void ConnectAuth::accept(PACKET* send, Auth::WriterImplementation*)
{
	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: ConnectAuth::accept: accepted protocol=%x op=%s\n",
		authPort->port_protocol, useResponse ? "response" : "accept"));
	fb_assert(authPort->port_protocol >= PROTOCOL_VERSION13);

	if (useResponse)
	{
		CSTRING* const s = &send->p_resp.p_resp_data;
		authPort->port_srv_auth_block->extractNewKeys(s);
		ISC_STATUS sv[] = {1, 0, 0};
		authPort->send_response(send, 0, s->cstr_length, sv, false);
	}
	else
	{
		send->p_operation = op_accept_data;
		CSTRING* const s = &send->p_acpd.p_acpt_keys;
		authPort->port_srv_auth_block->extractNewKeys(s);
		send->p_acpd.p_acpt_authenticated = 1;
		authPort->send(send);
	}
}


void Rsr::checkIface(ISC_STATUS code)
{
	if (!rsr_iface)
		Arg::Gds(code).raise();
}


void Rsr::checkCursor()
{
	if (!rsr_cursor)
		Arg::Gds(isc_cursor_not_open).raise();
}


static ISC_STATUS allocate_statement( rem_port* port, /*P_RLSE* allocate,*/ PACKET* send)
{
/**************************************
 *
 *	a l l o c a t e _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Allocate a statement handle.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = port->port_context;

	if (bad_db(&status_vector, rdb))
	{
		return port->send_response(send, 0, 0, &status_vector, true);
	}

	OBJCT object = 0;
	// Allocate SQL request block

	Rsr* statement = new Rsr;
	statement->rsr_rdb = rdb;
	statement->rsr_iface = NULL;

	if (statement->rsr_id = port->get_id(statement))
	{
		object = statement->rsr_id;
		statement->rsr_next = rdb->rdb_sql_requests;
		rdb->rdb_sql_requests = statement;
	}
	else
	{
		delete statement;

		status_vector.init();
		(Arg::Gds(isc_too_many_handles)).copyTo(&status_vector);
	}

	return port->send_response(send, object, 0, &status_vector, true);
}


static void append_request_chain(server_req_t* request, server_req_t** que_inst)
{
/**************************************
 *
 *	a p p e n d _ r e q u e s t _ c h a i n
 *
 **************************************
 *
 * Functional description
 *	Traverse using req_chain ptr and append
 *	a request at the end of a que.
 *	Return count of pending requests.
 *
 **************************************/
	MutexLockGuard queGuard(request_que_mutex, FB_FUNCTION);

	while (*que_inst)
		que_inst = &(*que_inst)->req_chain;

	*que_inst = request;
}


static void append_request_next(server_req_t* request, server_req_t** que_inst)
{
/**************************************
 *
 *	a p p e n d _ r e q u e s t _ n e x t
 *
 **************************************
 *
 * Functional description
 *	Traverse using req_next ptr and append
 *	a request at the end of a que.
 *	Return count of pending requests.
 *
 **************************************/
	MutexLockGuard queGuard(request_que_mutex, FB_FUNCTION);

	while (*que_inst)
		que_inst = &(*que_inst)->req_next;

	*que_inst = request;
	ports_pending++;
}


static void addClumplets(ClumpletWriter* dpb_buffer,
						 const ParametersSet& par,
						 const rem_port* port)
{
/**************************************
 *
 *	a d d C l u m p l e t s
 *
 **************************************
 *
 * Functional description
 *	Insert remote connection info into DPB
 *
 **************************************/
	ClumpletWriter address_stack_buffer(ClumpletReader::UnTagged, MAX_UCHAR - 2);
	if (dpb_buffer->find(par.address_path))
	{
		address_stack_buffer.reset(dpb_buffer->getBytes(), dpb_buffer->getClumpLength());
		dpb_buffer->deleteClumplet();
	}

	ClumpletWriter address_record(ClumpletReader::UnTagged, MAX_UCHAR - 2);

	if (port->port_protocol_id.hasData())
		address_record.insertString(isc_dpb_addr_protocol, port->port_protocol_id);

	if (port->port_address.hasData())
		address_record.insertString(isc_dpb_addr_endpoint, port->port_address);

	// We always insert remote address descriptor as a first element
	// of appropriate clumplet so user cannot fake it and engine may somewhat trust it.
	fb_assert(address_stack_buffer.getCurOffset() == 0);
	address_stack_buffer.insertBytes(isc_dpb_address,
		address_record.getBuffer(), address_record.getBufferLength());

	dpb_buffer->insertBytes(par.address_path, address_stack_buffer.getBuffer(),
								address_stack_buffer.getBufferLength());

	// Remove all remaining isc_*pb_address_path clumplets.
	// This is the security feature to prevent user from faking remote address
	// by passing multiple address_path clumplets. Engine assumes that
	// dpb contains no more than one address_path clumplet and for
	// clients coming via remote interface it can trust the first address from
	// address stack. Clients acessing database directly can do whatever they
	// want with the engine including faking the source address, not much we
	// can do about it.

	while (!dpb_buffer->isEof())
	{
		if (dpb_buffer->getClumpTag() == par.address_path)
			dpb_buffer->deleteClumplet();
		else
			dpb_buffer->moveNext();
	}

	// Append information about the remote protocol

	string protocol;
	protocol.printf("P%d", port->port_protocol & FB_PROTOCOL_MASK);

	dpb_buffer->deleteWithTag(par.remote_protocol);
	dpb_buffer->insertString(par.remote_protocol, protocol);

	// Append information about the peer host name and OS user

	dpb_buffer->deleteWithTag(par.host_name);
	if (port->port_peer_name.hasData())
	{
		try
		{
			string peerName(port->port_peer_name);

			ISC_systemToUtf8(peerName);
			ISC_escape(peerName);

			if (!dpb_buffer->find(isc_dpb_utf8_filename))
				ISC_utf8ToSystem(peerName);

			dpb_buffer->insertString(par.host_name, peerName);
		}
		catch (const Exception&)
		{} // no-op
	}

	dpb_buffer->deleteWithTag(par.os_user);
	if (port->port_user_name.hasData())
	{
		try
		{
			string userName(port->port_user_name);

			ISC_systemToUtf8(userName);
			ISC_escape(userName);

			if (!dpb_buffer->find(isc_dpb_utf8_filename))
				ISC_utf8ToSystem(userName);

			dpb_buffer->insertString(par.os_user, userName);
		}
		catch (const Exception&)
		{} // no-op
	}
}


static void attach_database(rem_port* port, P_OP operation, P_ATCH* attach, PACKET* send)
{
/**************************************
 *
 *	a t t a c h _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Process an attach or create packet.
 *
 **************************************/
	WIRECRYPT_DEBUG(fprintf(stderr, "Line encryption %sabled on attach\n", port->port_crypt_complete ? "en" : "dis"));
	if (port->port_required_encryption && !port->port_crypt_complete)
	{
		Arg::Gds(isc_miss_wirecrypt).raise();
	}

	ClumpletWriter* wrt = FB_NEW(*getDefaultMemoryPool()) ClumpletWriter(*getDefaultMemoryPool(),
		ClumpletReader::dpbList, MAX_DPB_SIZE, attach->p_atch_dpb.cstr_address,
		attach->p_atch_dpb.cstr_length);

	port->port_srv_auth = new DatabaseAuth(port, PathName(attach->p_atch_file.cstr_address,
		attach->p_atch_file.cstr_length), wrt, operation);

	if (port->port_srv_auth->authenticate(send))
	{
		delete port->port_srv_auth;
		port->port_srv_auth = NULL;
	}
}


void DatabaseAuth::accept(PACKET* send, Auth::WriterImplementation* authBlock)
{
	DispatcherPtr provider;

	authBlock->store(pb, isc_dpb_auth_block);

	send->p_operation = op_accept;

	for (pb->rewind(); !pb->isEof();)
	{
		switch (pb->getClumpTag())
		{
		// remove trusted auth & trusted role if present (security measure)
		case isc_dpb_trusted_role:
		case isc_dpb_trusted_auth:

		// remove old-style logon parameters
		case isc_dpb_user_name:
		case isc_dpb_password:
		case isc_dpb_password_enc:

		// remove client's config information
		case isc_dpb_config:
			pb->deleteClumplet();
			break;

		default:
			pb->moveNext();
			break;
		}
	}

	// Now insert additional clumplets into dpb
	addClumplets(pb, dpbParam, authPort);

	// See if user has specified parameters relevant to the connection,
	// they will be stuffed in the DPB if so.
	REMOTE_get_timeout_params(authPort, pb);

	const UCHAR* dpb = pb->getBuffer();
	unsigned int dl = (ULONG) pb->getBufferLength();

	if (!authPort->port_server_crypt_callback)
	{
		authPort->port_server_crypt_callback = new ServerCallback(authPort);
	}

	LocalStatus status_vector;
	provider->setDbCryptCallback(&status_vector, authPort->port_server_crypt_callback->getInterface());

	if (status_vector.isSuccess())
	{
		ServAttachment iface(operation == op_attach ?
			provider->attachDatabase(&status_vector, dbName.c_str(), dl, dpb) :
			provider->createDatabase(&status_vector, dbName.c_str(), dl, dpb));

		if (status_vector.isSuccess())
		{
			Rdb* rdb = new Rdb;

			authPort->port_context = rdb;
#ifdef DEBUG_REMOTE_MEMORY
			printf("attach_databases(server)  allocate rdb     %x\n", rdb);
#endif
			rdb->rdb_port = authPort;
			rdb->rdb_iface = iface;
		}
	}

	CSTRING* const s = &send->p_resp.p_resp_data;
	authPort->port_srv_auth_block->extractNewKeys(s);
	authPort->send_response(send, 0, s->cstr_length, &status_vector, false);
}


static void aux_request( rem_port* port, /*P_REQ* request,*/ PACKET* send)
{
/**************************************
 *
 *	a u x _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Other guy wants to establish a secondary connection.
 *	Humor him.
 *
 **************************************/

	try
	{
		LocalStatus status_vector;

		Rdb* const rdb = port->port_context;
		if (bad_db(&status_vector, rdb))
		{
			port->send_response(send, 0, 0, &status_vector, false);
			return;
		}

		// This buffer is used by INET and WNET transports
		// to return the server identification string
		UCHAR buffer[BUFFER_TINY];
		send->p_resp.p_resp_data.cstr_address = buffer;

		// To be retrieved via an overloaded class member once our ports become real classes
		const int aux_port_id = (port->port_type == rem_port::INET) ?
			Config::getDefaultConfig()->getRemoteAuxPort() : 0;
		GlobalPortLock auxPortLock(aux_port_id);

		rem_port* const aux_port = port->request(send);

		port->send_response(send, rdb->rdb_id, send->p_resp.p_resp_data.cstr_length,
							&status_vector, false);

		if (!status_vector.isSuccess())
		{
			return;
		}

		if (aux_port)
		{
			aux_port->port_flags |= PORT_connecting;
			try
			{
				if (aux_port->connect(send))
					aux_port->port_context = rdb;
			}
			catch (const Exception& ex)
			{
				aux_port->port_flags &= ~PORT_connecting;
				iscLogException("", ex);
				fb_assert(port->port_async == aux_port);
				port->port_async = NULL;
				aux_port->disconnect();
			}
			aux_port->port_flags &= ~PORT_connecting;
		}
	}
	catch (const Exception& ex)
	{
		iscLogException("Unhandled exception in server's aux_request():", ex);
	}
}


static bool bad_port_context(IStatus* status_vector, IRefCounted* iface, const ISC_STATUS error)
{
/**************************************
 *
 *	b a d _ p o r t _ c o n t e x t
 *
 **************************************
 *
 * Functional description
 *	Check rdb pointer, in case of error create status vector
 *
 **************************************/
	if (iface)
	{
		return false;
	}
	(Arg::Gds(error)).copyTo(status_vector);
	return true;
}


static ISC_STATUS cancel_events( rem_port* port, P_EVENT * stuff, PACKET* send)
{
/**************************************
 *
 *	c a n c e l _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Cancel events.
 *
 **************************************/
    LocalStatus status_vector;

	// Which database ?

	Rdb* rdb = port->port_context;
	if (bad_db(&status_vector, rdb))
		return port->send_response(send, 0, 0, &status_vector, false);

	// Find the event

	Rvnt* event;
	for (event = rdb->rdb_events; event; event = event->rvnt_next)
	{
		if (event->rvnt_id == stuff->p_event_rid)
			break;
	}

	// If no event found, pretend it was cancelled

	if (!event)
		return port->send_response(send, 0, 0, &status_vector, false);

	// cancel the event

	if (event->rvnt_iface)
		event->rvnt_iface->cancel(&status_vector);

	// zero event info

	event->rvnt_iface = NULL;
	event->rvnt_id = 0L;

	// return response

	return port->send_response(send, 0, 0, &status_vector, false);
}


static void cancel_operation(rem_port* port, USHORT kind)
{
/**************************************
 *
 *	c a n c e l _ o p e r a t i o n
 *
 **************************************
 *
 * Functional description
 *	Flag a running operation for cancel.
 *	Service operations are not currently
 *	able to be canceled.
 *
 **************************************/
	Rdb* rdb;
	if ((port->port_flags & (PORT_async | PORT_disconnect)) || !(rdb = port->port_context))
		return;

	if (rdb->rdb_iface)
	{
		LocalStatus status_vector;
		rdb->rdb_iface->cancelOperation(&status_vector, kind);
	}
}


static bool check_request(Rrq* request, USHORT incarnation, USHORT msg_number)
{
/**************************************
 *
 *	c h e c k _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Check to see if a request is ready to send us a particular
 *	message.  If so, return true, otherwise false.
 *
 **************************************/
	USHORT n;

	if (!get_next_msg_no(request, incarnation, &n))
		return false;

	return msg_number == n;
}


static USHORT check_statement_type( Rsr* statement)
{
/**************************************
 *
 *	c h e c k _ s t a t e m e n t _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Return the type of SQL statement.
 *
 **************************************/
	UCHAR buffer[16];
	LocalStatus local_status;
	USHORT ret = 0;
	bool done = false;

	fb_assert(statement->rsr_iface);
	statement->checkIface();		// this should not happen but...

	statement->rsr_iface->getInfo(&local_status, sizeof(sql_info), sql_info, sizeof(buffer), buffer);

	if (local_status.isSuccess())
	{
		for (const UCHAR* info = buffer; (*info != isc_info_end) && !done;)
		{
			const USHORT l = (USHORT) gds__vax_integer(info + 1, 2);
			const USHORT type = (USHORT) gds__vax_integer(info + 3, l);
			switch (*info)
			{
			case isc_info_sql_stmt_type:
				switch (type)
				{
				case isc_info_sql_stmt_get_segment:
				case isc_info_sql_stmt_put_segment:
					fb_assert(false);
					break;
				case isc_info_sql_stmt_select:
				case isc_info_sql_stmt_select_for_upd:
					ret |= STMT_DEFER_EXECUTE;
					break;
				}
				break;
			case isc_info_sql_batch_fetch:
				if (type == 0)
					ret |= STMT_NO_BATCH;
				break;
			case isc_info_error:
			case isc_info_truncated:
				done = true;
				break;

			}
			info += 3 + l;
		}
	}

	return ret;
}


ISC_STATUS rem_port::compile(P_CMPL* compileL, PACKET* sendL)
{
/**************************************
 *
 *	c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Compile and request.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
	{
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	const UCHAR* blr = compileL->p_cmpl_blr.cstr_address;
	const ULONG blr_length = compileL->p_cmpl_blr.cstr_length;

	ServRequest iface(rdb->rdb_iface->compileRequest(&status_vector, blr_length, blr));

	if (!status_vector.isSuccess())
		return this->send_response(sendL, 0, 0, &status_vector, false);

	// Parse the request to find the messages

	RMessage* message = PARSE_messages(blr, blr_length);
	USHORT max_msg = 0;

	RMessage* next;
	for (next = message; next; next = next->msg_next)
		max_msg = MAX(max_msg, next->msg_number);

	// Allocate block and merge into data structures

	Rrq* requestL = new Rrq(max_msg + 1);
#ifdef DEBUG_REMOTE_MEMORY
	printf("compile(server)           allocate request %x\n", request);
#endif
	requestL->rrq_iface = iface;
	requestL->rrq_rdb = rdb;
	requestL->rrq_max_msg = max_msg;
	OBJCT object = 0;

	if (requestL->rrq_id = this->get_id(requestL))
	{
		object = requestL->rrq_id;
		requestL->rrq_next = rdb->rdb_requests;
		rdb->rdb_requests = requestL;
	}
	else
	{
		requestL->rrq_iface->free(&status_vector);
		delete requestL;
		(Arg::Gds(isc_too_many_handles)).copyTo(&status_vector);
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	while (message)
	{
		next = message->msg_next;
		message->msg_next = message;

		Rrq::rrq_repeat* tail = &requestL->rrq_rpt[message->msg_number];
		tail->rrq_message = message;
		tail->rrq_xdr = message;
		tail->rrq_format = (rem_fmt*) message->msg_address;

		message->msg_address = NULL;
		message = next;
	}

	return this->send_response(sendL, object, 0, &status_vector, false);
}


ISC_STATUS rem_port::ddl(P_DDL* ddlL, PACKET* sendL)
{
/**************************************
 *
 *	d d l
 *
 **************************************
 *
 * Functional description
 *	Execute isc_ddl call.
 *
 **************************************/
	LocalStatus status_vector;
	Rtr* transaction;

	getHandle(transaction, ddlL->p_ddl_transaction);

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
	{
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	const UCHAR* blr = ddlL->p_ddl_blr.cstr_address;
	const ULONG blr_length = ddlL->p_ddl_blr.cstr_length;

	rdb->rdb_iface->executeDyn(&status_vector, transaction->rtr_iface, blr_length, blr);

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


void rem_port::disconnect(PACKET* sendL, PACKET* receiveL)
{
/**************************************
 *
 *	d i s c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	We've lost the connection to the parent.  Stop everything.
 *
 **************************************/
	Rdb* rdb = this->port_context;

	if (this->port_flags & PORT_async)
	{
		if (rdb && rdb->rdb_port && !(rdb->rdb_port->port_flags & PORT_disconnect))
		{
			PACKET *packet = &rdb->rdb_packet;
			packet->p_operation = op_dummy;
			rdb->rdb_port->send(packet);
		}
		return;
	}

	this->port_flags |= PORT_disconnect;

	if (!rdb)
	{
		REMOTE_free_packet(this, sendL);
		REMOTE_free_packet(this, receiveL);
		this->disconnect();
		return;
	}

	// For WNET and XNET we should send dummy op_disconnect packet
	// to wakeup async port handling events on client side.
	// For INET it's not necessary because INET client's async port
	// wakes up while server performs shutdown(socket) call on its async port.
	// See interface.cpp - event_thread().

	PACKET *packet = &rdb->rdb_packet;
	if (this->port_async)
	{
		if ((this->port_type == rem_port::XNET) || (this->port_type == rem_port::PIPE))
		{
			packet->p_operation = op_disconnect;
			this->port_async->send(packet);
		}
		this->port_async->port_flags |= PORT_disconnect;
	}

	LocalStatus status_vector;
	if (rdb->rdb_iface)
	{
		// Prevent a pending or spurious cancel from aborting
		// a good, clean detach from the database.

		rdb->rdb_iface->cancelOperation(&status_vector, fb_cancel_disable);

		while (rdb->rdb_requests)
			release_request(rdb->rdb_requests);

		while (rdb->rdb_sql_requests)
			release_sql_request(rdb->rdb_sql_requests);

		Rtr* transaction;

		while (transaction = rdb->rdb_transactions)
		{
			if (!transaction->rtr_limbo)
				transaction->rtr_iface->rollback(&status_vector);
			else
			{
				// The underlying JRD subsystem will release all
				// memory resources related to a limbo transaction
				// as a side-effect of the database detach call below.
				// However, the y-valve iface must be released.
				transaction->rtr_iface->disconnect(&status_vector);
			}

			release_transaction(rdb->rdb_transactions);
		}

		rdb->rdb_iface->detach(&status_vector);
		rdb->rdb_iface = NULL;

		while (rdb->rdb_events)
			release_event(rdb->rdb_events);

		if (this->port_statement)
			release_statement(&this->port_statement);
	}

	if (rdb->rdb_svc.hasData() && rdb->rdb_svc->svc_iface)
	{
		rdb->rdb_svc->svc_iface->detach(&status_vector);
		rdb->rdb_svc->svc_iface = NULL;
	}

	REMOTE_free_packet(this, sendL);
	REMOTE_free_packet(this, receiveL);

#ifdef DEBUG_REMOTE_MEMORY
	printf("disconnect(server)        free rdb         %x\n", rdb);
#endif
	this->port_context = NULL;
	if (this->port_async)
		this->port_async->port_context = NULL;
	delete rdb;
	if (this->port_connection)
	{
#ifdef DEBUG_REMOTE_MEMORY
		printf("disconnect(server)        free string      %x\n", this->port_connection);
#endif
		delete this->port_connection;
		this->port_connection = NULL;
	}
	if (this->port_version)
	{
#ifdef DEBUG_REMOTE_MEMORY
		printf("disconnect(server)        free string      %x\n", this->port_version);
#endif
		delete this->port_version;
		this->port_version = NULL;
	}
	if (this->port_host)
	{
#ifdef DEBUG_REMOTE_MEMORY
		printf("disconnect(server)        free string      %x\n", this->port_host);
#endif
		delete this->port_host;
		this->port_host = NULL;
	}
	this->disconnect();
}


void rem_port::drop_database(P_RLSE* /*release*/, PACKET* sendL)
{
/**************************************
 *
 *	d r o p _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	End a request.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
	{
		this->send_response(sendL, 0, 0, &status_vector, false);
		return;
	}

	rdb->rdb_iface->dropDatabase(&status_vector);

	if (!status_vector.isSuccess() && (status_vector.get()[1] != isc_drdb_completed_with_errs))
	{
		this->send_response(sendL, 0, 0, &status_vector, false);
		return;
	}

	rdb->rdb_iface = NULL;
	port_flags |= PORT_detached;

	while (rdb->rdb_events)
		release_event(rdb->rdb_events);

	while (rdb->rdb_requests)
		release_request(rdb->rdb_requests);

	while (rdb->rdb_sql_requests)
		release_sql_request(rdb->rdb_sql_requests);

	while (rdb->rdb_transactions)
		release_transaction(rdb->rdb_transactions);

	if (this->port_statement)
		release_statement(&this->port_statement);

	this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::end_blob(P_OP operation, P_RLSE * release, PACKET* sendL)
{
/**************************************
 *
 *	e n d _ b l o b
 *
 **************************************
 *
 * Functional description
 *	End a blob.
 *
 **************************************/
	Rbl* blob;
	LocalStatus status_vector;

	getHandle(blob, release->p_rlse_object);

	if (operation == op_close_blob)
		blob->rbl_iface->close(&status_vector);
	else
		blob->rbl_iface->cancel(&status_vector);

	if (status_vector.isSuccess())
		release_blob(blob);

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::end_database(P_RLSE* /*release*/, PACKET* sendL)
{
/**************************************
 *
 *	e n d _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	End a request.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	rdb->rdb_iface->detach(&status_vector);

	if (!status_vector.isSuccess())
		return this->send_response(sendL, 0, 0, &status_vector, false);

	port_flags |= PORT_detached;
	rdb->rdb_iface = NULL;

	while (rdb->rdb_events)
		release_event(rdb->rdb_events);

	while (rdb->rdb_requests)
		release_request(rdb->rdb_requests);

	while (rdb->rdb_sql_requests)
		release_sql_request(rdb->rdb_sql_requests);

	while (rdb->rdb_transactions)
		release_transaction(rdb->rdb_transactions);

	if (this->port_statement)
		release_statement(&this->port_statement);

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::end_request(P_RLSE * release, PACKET* sendL)
{
/**************************************
 *
 *	e n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	End a request.
 *
 **************************************/
	Rrq* requestL;
	LocalStatus status_vector;

	getHandle(requestL, release->p_rlse_object);

	requestL->rrq_iface->free(&status_vector);

	if (status_vector.isSuccess())
		release_request(requestL);

	return this->send_response(sendL, 0, 0, &status_vector, true);
}


ISC_STATUS rem_port::end_statement(P_SQLFREE* free_stmt, PACKET* sendL)
{
/*****************************************
 *
 *	e n d _ s t a t e m e n t
 *
 *****************************************
 *
 * Functional description
 *	Free a statement.
 *
 *****************************************/
	Rsr* statement;
	LocalStatus status_vector;

	getHandle(statement, free_stmt->p_sqlfree_statement);

	if (free_stmt->p_sqlfree_option & (DSQL_drop | DSQL_unprepare | DSQL_close))
	{
		if (statement->rsr_cursor)
		{
			statement->rsr_cursor->close(&status_vector);
			if (!status_vector.isSuccess())
			{
				return this->send_response(sendL, 0, 0, &status_vector, true);
			}
			statement->rsr_cursor = NULL;
			statement->rsr_cursor_name = "";
			fb_assert(statement->rsr_rtr);
			size_t pos;
			if (!statement->rsr_rtr->rtr_cursors.find(statement, pos))
				fb_assert(false);
			statement->rsr_rtr->rtr_cursors.remove(pos);
		}
		else if (!(free_stmt->p_sqlfree_option & (DSQL_drop | DSQL_unprepare)))
		{
			Arg::Gds(isc_dsql_cursor_close_err).copyTo(&status_vector);
			return this->send_response(sendL, 0, 0, &status_vector, true);
		}
	}

	if (free_stmt->p_sqlfree_option & (DSQL_drop | DSQL_unprepare))
	{
		if (statement->rsr_iface)
		{
			statement->rsr_iface->free(&status_vector);
			if (!status_vector.isSuccess())
			{
				return this->send_response(sendL, 0, 0, &status_vector, true);
			}
			statement->rsr_iface = NULL;
		}
	}

	if (free_stmt->p_sqlfree_option & DSQL_drop)
	{
		release_sql_request(statement);
		statement = NULL;
	}
	else
	{
		statement->rsr_flags.clear(Rsr::FETCHED);
		statement->rsr_rtr = NULL;
		REMOTE_reset_statement(statement);
		statement->rsr_message = statement->rsr_buffer;
	}

	const USHORT object = statement ? statement->rsr_id : INVALID_OBJECT;
	return this->send_response(sendL, object, 0, &status_vector, true);
}


ISC_STATUS rem_port::end_transaction(P_OP operation, P_RLSE * release, PACKET* sendL)
{
/**************************************
 *
 *	e n d _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	End a transaction.
 *
 **************************************/
	Rtr* transaction;
	LocalStatus status_vector;

	getHandle(transaction, release->p_rlse_object);

	switch (operation)
	{
	case op_commit:
		transaction->rtr_iface->commit(&status_vector);
		break;

	case op_rollback:
		transaction->rtr_iface->rollback(&status_vector);
		break;

	case op_rollback_retaining:
		transaction->rtr_iface->rollbackRetaining(&status_vector);
		break;

	case op_commit_retaining:
		transaction->rtr_iface->commitRetaining(&status_vector);
		break;

	case op_prepare:
		transaction->rtr_iface->prepare(&status_vector);
		if (status_vector.isSuccess())
			transaction->rtr_limbo = true;
		break;
	}

	if (status_vector.isSuccess())
	{
		if (operation == op_commit || operation == op_rollback)
		{
			REMOTE_cleanup_transaction(transaction);
			release_transaction(transaction);
		}
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::execute_immediate(P_OP op, P_SQLST * exnow, PACKET* sendL)
{
/*****************************************
 *
 *	e x e c u t e _ i m m e d i a t e
 *
 *****************************************
 *
 * Functional description
 *	process an execute immediate DSQL packet
 *
 *****************************************/
	Rtr* transaction = NULL;
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
	{
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	// Do not call CHECK_HANDLE if this is the start of a transaction
	if (exnow->p_sqlst_transaction) {
		getHandle(transaction, exnow->p_sqlst_transaction);
	}
	ITransaction* tra = NULL;
	if (transaction)
		tra = transaction->rtr_iface;

	ULONG in_msg_length = 0, out_msg_length = 0;
	ULONG in_blr_length = 0, out_blr_length = 0;
	UCHAR* in_msg = NULL;
	UCHAR* out_msg = NULL;
	UCHAR* in_blr;
	UCHAR* out_blr;
	USHORT in_msg_type;

	if (op == op_exec_immediate2)
	{
		in_msg_type = exnow->p_sqlst_message_number;
		if ((SSHORT)in_msg_type == -1)
		{
			return this->send_response(sendL, (OBJCT) (transaction ? transaction->rtr_id : 0),
				 0, &status_vector, false);
		}

		in_blr_length = exnow->p_sqlst_blr.cstr_length;
		in_blr = exnow->p_sqlst_blr.cstr_address;

		if (this->port_statement->rsr_bind_format)
		{
			in_msg_length = this->port_statement->rsr_bind_format->fmt_length;
			RMessage* message = this->port_statement->rsr_message;
			if (!message->msg_address)
			{
				message->msg_address = message->msg_buffer;
			}
			in_msg = message->msg_address;
		}
		out_blr_length = exnow->p_sqlst_out_blr.cstr_length;
		out_blr = exnow->p_sqlst_out_blr.cstr_address;
		if (this->port_statement->rsr_select_format)
		{
			out_msg_length = this->port_statement->rsr_select_format->fmt_length;
			RMessage* message = this->port_statement->rsr_message;
			if (!message->msg_address)
			{
				message->msg_address = message->msg_buffer;
			}
			out_msg = message->msg_address;
		}
	}
	else
	{
		in_blr_length = out_blr_length = 0;
		in_blr = out_blr = NULL;
		in_msg_type = 0;
	}

	// Since the API to GDS_DSQL_EXECUTE_IMMED is public and can not be changed, there needs to
	// be a way to send the parser version to DSQL so that the parser can compare the keyword
	// version to the parser version.  To accomplish this, the parser version is combined with
	// the client dialect and sent across that way.  In dsql8_execute_immediate, the parser version
	// and client dialect are separated and passed on to their final desintations.  The information
	// is combined as follows:
	//     Dialect * 10 + parser_version
	//
	// and is extracted in dsql8_execute_immediate as follows:
	//      parser_version = ((dialect * 10) + parser_version) % 10
	//      client_dialect = ((dialect * 10) + parser_version) / 10
	//
	// For example, parser_version = 1 and client dialect = 1
	//
	//  combined = (1 * 10) + 1 == 11
	//
	//  parser = (combined) % 10 == 1
	//  dialect = (combined) / 10 == 1

	InternalMessageBuffer iMsgBuffer(in_blr_length, in_blr, in_msg_length, in_msg);
	InternalMessageBuffer oMsgBuffer(out_blr_length, out_blr, out_msg_length, out_msg);

	ITransaction* newTra = rdb->rdb_iface->execute(&status_vector, tra,
		exnow->p_sqlst_SQL_str.cstr_length,
		reinterpret_cast<const char*>(exnow->p_sqlst_SQL_str.cstr_address),
		exnow->p_sqlst_SQL_dialect * 10 + PARSER_VERSION,
		iMsgBuffer.metadata, iMsgBuffer.buffer, oMsgBuffer.metadata, oMsgBuffer.buffer);

	if (op == op_exec_immediate2)
	{
		this->port_statement->rsr_format = this->port_statement->rsr_select_format;

		sendL->p_operation = op_sql_response;
		sendL->p_sqldata.p_sqldata_messages = (status_vector.get()[1] || !out_msg) ? 0 : 1;
		this->send_partial(sendL);
	}

	if (status_vector.isSuccess())
	{
		if (transaction && !newTra)
		{
			REMOTE_cleanup_transaction(transaction);
			release_transaction(transaction);
			transaction = NULL;
		}
		else if (!transaction && newTra)
		{
			if (!(transaction = make_transaction(rdb, newTra)))
				(Arg::Gds(isc_too_many_handles)).copyTo(&status_vector);
		}
		else if (newTra && newTra != tra)
			transaction->rtr_iface = newTra;
	}

	return this->send_response(sendL, (OBJCT) (transaction ? transaction->rtr_id : 0),
		0, &status_vector, false);
}


ISC_STATUS rem_port::execute_statement(P_OP op, P_SQLDATA* sqldata, PACKET* sendL)
{
/*****************************************
 *
 *	e x e c u t e _ s t a t e m e n t
 *
 *****************************************
 *
 * Functional description
 *	Execute a non-SELECT dynamic SQL statement.
 *
 *****************************************/
	Rtr* transaction = NULL;

	// Do not call CHECK_HANDLE if this is the start of a transaction
	if (sqldata->p_sqldata_transaction)
	{
		getHandle(transaction, sqldata->p_sqldata_transaction);
	}

	LocalStatus status_vector;
	Rsr* statement;
	getHandle(statement, sqldata->p_sqldata_statement);
	USHORT out_msg_type = (op == op_execute2) ? sqldata->p_sqldata_out_message_number : 0;
	const bool defer = this->haveRecvData();

	if ((SSHORT) out_msg_type == -1)
	{
		return this->send_response(sendL, (OBJCT) (transaction ? transaction->rtr_id : 0),
			0, &status_vector, defer);
	}
	statement->checkIface();

	ULONG in_msg_length = 0, out_msg_length = 0;
	UCHAR* in_msg = NULL;
	UCHAR* out_msg = NULL;
	ULONG out_blr_length;
	UCHAR* out_blr;

	if (statement->rsr_format)
	{
		fb_assert(statement->rsr_format == statement->rsr_bind_format);

		in_msg_length = statement->rsr_format->fmt_length;
		in_msg = statement->rsr_message->msg_address;
	}
	if (op == op_execute2)
	{
		out_blr_length = sqldata->p_sqldata_out_blr.cstr_length;
		out_blr = sqldata->p_sqldata_out_blr.cstr_address;
		if (this->port_statement->rsr_select_format)
		{
			out_msg_length = this->port_statement->rsr_select_format->fmt_length;
			out_msg = this->port_statement->rsr_message->msg_buffer;
		}
	}
	else
	{
		out_blr_length = 0;
		out_msg_type = 0;
		out_blr = NULL;
	}
	statement->rsr_flags.clear(Rsr::FETCHED);

	ITransaction* tra = NULL;
	if (transaction)
		tra = transaction->rtr_iface;

	if (statement->rsr_cursor)
	{
		 Arg::Gds(isc_dsql_cursor_open_err).raise();
	}

	InternalMessageBuffer iMsgBuffer(sqldata->p_sqldata_blr.cstr_length,
		sqldata->p_sqldata_blr.cstr_address, in_msg_length, in_msg);
	InternalMessageBuffer oMsgBuffer(out_blr_length, out_blr, out_msg_length, out_msg);
	ITransaction* newTra = tra;

	unsigned flags = statement->rsr_iface->getFlags(&status_vector);
	if (!status_vector.isSuccess())
	{
		status_exception::raise(status_vector.get());
	}

	if ((flags & IStatement::FLAG_HAS_CURSOR) && (out_msg_length == 0))
	{
		statement->rsr_cursor =
			statement->rsr_iface->openCursor(&status_vector, tra,
											 iMsgBuffer.metadata, iMsgBuffer.buffer,
											 (out_blr_length ? oMsgBuffer.metadata : DELAYED_OUT_FORMAT));
		if (status_vector.isSuccess())
		{
			transaction->rtr_cursors.add(statement);
			statement->rsr_delayed_format = !out_blr_length;

			if (statement->rsr_cursor_name.hasData())
			{
				statement->rsr_cursor->setCursorName(&status_vector, statement->rsr_cursor_name.c_str());

				if (status_vector.isSuccess())
					statement->rsr_cursor_name = "";
			}
		}
	}
	else
	{
		newTra = statement->rsr_iface->execute(&status_vector, tra,
			iMsgBuffer.metadata, iMsgBuffer.buffer, oMsgBuffer.metadata, oMsgBuffer.buffer);
	}

	if (op == op_execute2)
	{
		this->port_statement->rsr_format = this->port_statement->rsr_select_format;

		sendL->p_operation = op_sql_response;
		sendL->p_sqldata.p_sqldata_messages = (!status_vector.isSuccess() || !out_msg) ? 0 : 1;
		this->send_partial(sendL);
	}

	if (status_vector.isSuccess())
	{
		if (transaction && !newTra)
		{
			REMOTE_cleanup_transaction(transaction);
			release_transaction(transaction);
			transaction = NULL;
		}
		else if (!transaction && newTra)
		{
			if (!(transaction = make_transaction(statement->rsr_rdb, newTra)))
				(Arg::Gds(isc_too_many_handles)).copyTo(&status_vector);
		}
		else if (newTra && newTra != tra)
			transaction->rtr_iface = newTra;

		statement->rsr_rtr = transaction;
	}

	return this->send_response(sendL, (OBJCT) (transaction ? transaction->rtr_id : 0),
		0, &status_vector, defer);
}


ISC_STATUS rem_port::fetch(P_SQLDATA * sqldata, PACKET* sendL)
{
/*****************************************
 *
 *	f e t c h
 *
 *****************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor.
 *
 *****************************************/
	LocalStatus status_vector;
	Rsr* statement;
	getHandle(statement, sqldata->p_sqldata_statement);

	const ULONG msg_length = statement->rsr_format ? statement->rsr_format->fmt_length : 0;

	USHORT count = statement->rsr_flags.test(Rsr::NO_BATCH) ? 1 : sqldata->p_sqldata_messages;
	USHORT count2 = statement->rsr_flags.test(Rsr::NO_BATCH) ? 0 : count;

	// On first fetch, clear the end-of-stream flag & reset the message buffers

	if (!statement->rsr_flags.test(Rsr::FETCHED))
	{
		statement->rsr_flags.clear(Rsr::EOF_SET | Rsr::STREAM_ERR);
		statement->clearException();

		RMessage* message = statement->rsr_message;

		if (message != NULL)
		{
			statement->rsr_buffer = message;

			while (true)
			{
				message->msg_address = NULL;
				message = message->msg_next;

				if (message == statement->rsr_message)
					break;
			}
		}
	}


	// If required, call setDelayedOutputFormat()

	statement->checkCursor();
	if (statement->rsr_delayed_format)
	{
		InternalMessageBuffer msgBuffer(sqldata->p_sqldata_blr.cstr_length,
			sqldata->p_sqldata_blr.cstr_address, msg_length, NULL);

		if (!msgBuffer.metadata)
			Arg::Gds(isc_dsql_cursor_open_err).raise();

		statement->rsr_cursor->setDelayedOutputFormat(&status_vector, msgBuffer.metadata);
		if (!status_vector.isSuccess())
			status_exception::raise(status_vector.get());

		statement->rsr_delayed_format = false;
	}


	// Get ready to ship the data out

	P_SQLDATA* response = &sendL->p_sqldata;
	sendL->p_operation = op_fetch_response;
	response->p_sqldata_statement = sqldata->p_sqldata_statement;
	response->p_sqldata_status = 0;
	response->p_sqldata_messages = 1;
	RMessage* message = NULL;

	// Check to see if any messages are already sitting around

	bool rc = true;

	while (true)
	{

		// Have we exhausted the cache & reached cursor EOF?
		if (statement->rsr_flags.test(Rsr::EOF_SET) && !statement->rsr_msgs_waiting)
		{
			statement->rsr_flags.clear(Rsr::EOF_SET);
			rc = false;
			count2 = 0;
			break;
		}

		// Have we exhausted the cache & have a pending error?
		if (statement->rsr_flags.test(Rsr::STREAM_ERR) && !statement->rsr_msgs_waiting)
		{
			fb_assert(statement->rsr_status);
			statement->rsr_flags.clear(Rsr::STREAM_ERR);
			return this->send_response(sendL, 0, 0, statement->rsr_status->value(), false);
		}

		message = statement->rsr_buffer;

		// Make sure message can be de referenced, if not then return false
		if (message == NULL)
			return FALSE;

		// If we don't have a message cached, get one from the access method.

		if (!message->msg_address)
		{
			fb_assert(statement->rsr_msgs_waiting == 0);

			rc = statement->rsr_cursor->fetchNext(&status_vector, message->msg_buffer);

			statement->rsr_flags.set(Rsr::FETCHED);
			if (!status_vector.isSuccess())
			{
				return this->send_response(sendL, 0, 0, &status_vector, false);
			}
			if (!rc)
			{
				count2 = 0;
				break;
			}

			message->msg_address = message->msg_buffer;
		}
		else
		{
			// Take a message from the outqoing queue
			fb_assert(statement->rsr_msgs_waiting >= 1);
			statement->rsr_msgs_waiting--;
		}

		count--;

		// There's a buffer waiting -- send it

		if (!this->send_partial(sendL))
			return FALSE;

		message->msg_address = NULL;

		// If we've sent the requested amount, break out of loop

		if (count <= 0)
			break;
	}

	response->p_sqldata_status = rc ? 0 : 100;
	response->p_sqldata_messages = 0;

	// hvlad: message->msg_address not used in xdr_protocol because of
	// response->p_sqldata_messages set to zero above.
	// It is important to not zero message->msg_address after send because
	// during thread context switch in send we can receive packet with
	// op_free and op_execute (lazy port feature allow this) which itself
	// set message->msg_address to some useful information
	// This fix must be re-thought when real multithreading will be implemented
	if (message) {
		message->msg_address = NULL;
	}

	this->send(sendL);

	// Since we have a little time on our hands while this packet is sent
	// and processed, get the next batch of records.  Start by finding the
	// next free buffer.

	message = statement->rsr_buffer;
	RMessage* next = NULL;

	while (message->msg_address && message->msg_next != statement->rsr_buffer)
		message = message->msg_next;

	for (; count2; --count2)
	{
		if (message->msg_address)
		{
			if (!next)
			{
				next = statement->rsr_buffer;
				while (next->msg_next != message)
					next = next->msg_next;
			}
			message = new RMessage(statement->rsr_fmt_length);
			message->msg_number = next->msg_number;
			message->msg_next = next->msg_next;
			next->msg_next = message;
			next = message;
		}

		rc = statement->rsr_cursor->fetchNext(&status_vector, message->msg_buffer);
		if (!status_vector.isSuccess())
		{
			// If already have an error queued, don't overwrite it
			if (!statement->rsr_flags.test(Rsr::STREAM_ERR))
			{
				statement->rsr_flags.set(Rsr::STREAM_ERR);
				statement->saveException(status_vector.get(), true);
			}
			break;
		}
		if (!rc)
		{
			statement->rsr_flags.set(Rsr::EOF_SET);
			break;
		}

		message->msg_address = message->msg_buffer;
		message = message->msg_next;
		statement->rsr_msgs_waiting++;
	}

	return TRUE;
}


static bool get_next_msg_no(Rrq* request, USHORT incarnation, USHORT * msg_number)
{
/**************************************
 *
 *	g e t _ n e x t _ m s g _ n o
 *
 **************************************
 *
 * Functional description
 *	Return the number of the next message
 *	in the request.
 *
 **************************************/
	LocalStatus status_vector;
	UCHAR info_buffer[128];

	request->rrq_iface->getInfo(&status_vector, incarnation,
		sizeof(request_info), request_info, sizeof(info_buffer), info_buffer);

	if (!status_vector.isSuccess())
		return false;

	bool result = false;
	for (const UCHAR* info = info_buffer; *info != isc_info_end;)
	{
		const USHORT l = (USHORT) gds__vax_integer(info + 1, 2);
		const USHORT n = (USHORT) gds__vax_integer(info + 3, l);

		switch (*info)
		{
		case isc_info_state:
			if (n != isc_info_req_send)
				return false;
			break;

		case isc_info_message_number:
			*msg_number = n;
			result = true;
			break;

		default:
			return false;
		}
		info += 3 + l;
	}

	return result;
}


ISC_STATUS rem_port::get_segment(P_SGMT* segment, PACKET* sendL)
{
/**************************************
 *
 *	g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get a single blob segment.
 *
 **************************************/
	Rbl* blob;

	getHandle(blob, segment->p_sgmt_blob);

	UCHAR temp_buffer[BLOB_LENGTH];
	USHORT buffer_length = segment->p_sgmt_length;
	UCHAR* buffer;
	if (buffer_length <= sizeof(temp_buffer))
		buffer = temp_buffer;
	else
	{
		if (buffer_length > blob->rbl_buffer_length)
		{
			blob->rbl_buffer = blob->rbl_data.getBuffer(buffer_length);
			blob->rbl_buffer_length = buffer_length;
		}
		buffer = blob->rbl_buffer;
	}
#ifdef DEBUG_REMOTE_MEMORY
	printf("get_segment(server)       allocate buffer  %x\n", buffer);
#endif
	sendL->p_resp.p_resp_data.cstr_address = buffer;

	// Gobble up a buffer's worth of segments

	LocalStatus status_vector;

	UCHAR* p = buffer;
	ISC_STATUS state = 0;

	while (buffer_length > 2)
	{
		buffer_length -= 2;
		p += 2;
		const USHORT length = blob->rbl_iface->getSegment(&status_vector, buffer_length, p);

		if (status_vector.get()[1] == isc_segstr_eof)
		{
			state = 2;
			status_vector.init();
			p -= 2;
			break;
		}
		if (!status_vector.isSuccess() && (status_vector.get()[1] != isc_segment))
		{
			p -= 2;
			break;
		}
		p[-2] = (UCHAR) length;
		p[-1] = (UCHAR) (length >> 8);
		p += length;
		buffer_length -= length;
		if (status_vector.get()[1] == isc_segment)
		{
			state = 1;
			status_vector.init();
			break;
		}
	}

	const ISC_STATUS status = this->send_response(sendL, (OBJCT) state, (ULONG) (p - buffer),
		&status_vector, false);

#ifdef DEBUG_REMOTE_MEMORY
	printf("get_segment(server)       free buffer      %x\n", buffer);
#endif

	return status;
}


ISC_STATUS rem_port::get_slice(P_SLC * stuff, PACKET* sendL)
{
/**************************************
 *
 *	g e t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Get an array slice.
 *
 **************************************/
	Rtr* transaction;
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	getHandle(transaction, stuff->p_slc_transaction);

	HalfStaticArray<UCHAR, 4096> temp_buffer;
	UCHAR* slice = 0;

	if (stuff->p_slc_length)
	{
		slice = temp_buffer.getBuffer(stuff->p_slc_length);
		memset(slice, 0, stuff->p_slc_length);
#ifdef DEBUG_REMOTE_MEMORY
		printf("get_slice(server)         allocate buffer  %x\n", slice);
#endif
	}

	P_SLR* response = &sendL->p_slr;

	response->p_slr_length = rdb->rdb_iface->getSlice(&status_vector,
		transaction->rtr_iface, &stuff->p_slc_id, stuff->p_slc_sdl.cstr_length,
		stuff->p_slc_sdl.cstr_address, stuff->p_slc_parameters.cstr_length,
		stuff->p_slc_parameters.cstr_address, stuff->p_slc_length, slice);

	ISC_STATUS status;
	if (!status_vector.isSuccess())
		status = this->send_response(sendL, 0, 0, &status_vector, false);
	else
	{
		sendL->p_operation = op_slice;
		response->p_slr_slice.lstr_address = slice;
		response->p_slr_slice.lstr_length = response->p_slr_length;
		response->p_slr_sdl = stuff->p_slc_sdl.cstr_address;
		response->p_slr_sdl_length = stuff->p_slc_sdl.cstr_length;
		this->send(sendL);
		response->p_slr_sdl = 0;
		status = FB_SUCCESS;
	}

	return status;
}


void rem_port::info(P_OP op, P_INFO* stuff, PACKET* sendL)
{
/**************************************
 *
 *	i n f o
 *
 **************************************
 *
 * Functional description
 *	Get info for a blob, database, request, service,
 *	statement, or transaction.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if ((op == op_service_info) ? bad_service(&status_vector, rdb) : bad_db(&status_vector, rdb))
	{
		this->send_response(sendL, 0, 0, &status_vector, false);
		return;
	}

	const ULONG buffer_length = stuff->p_info_buffer_length;

	// Make sure there is a suitable temporary blob buffer
	Array<UCHAR> buf;
	UCHAR* const buffer = buffer_length ? buf.getBuffer(buffer_length) : NULL;
	memset(buffer, 0, buffer_length);

	HalfStaticArray<UCHAR, 1024> info;
	UCHAR* info_buffer = NULL;
	ULONG info_len;
	HalfStaticArray<UCHAR, 1024> temp;
	UCHAR* temp_buffer = NULL;

	if (op == op_info_database)
	{
		temp_buffer = buffer_length ? temp.getBuffer(buffer_length) : NULL;
	}
	else
	{
		// stuff isc_info_length in front of info items buffer

		CSTRING_CONST* info_string = (op == op_service_info) ?
			&stuff->p_info_recv_items : &stuff->p_info_items;

		info_len = 1 + info_string->cstr_length;
		info_buffer = info.getBuffer(info_len);

		*info_buffer = isc_info_length;
		memmove(info_buffer + 1, info_string->cstr_address, info_len - 1);
	}

	Rbl* blob;
	Rtr* transaction;
	Rsr* statement;
	Svc* service;

	ULONG info_db_len = 0;

	switch (op)
	{
	case op_info_blob:
		getHandle(blob, stuff->p_info_object);
		blob->rbl_iface->getInfo(&status_vector, info_len, info_buffer,
			buffer_length, buffer);
		break;

	case op_info_database:
		rdb->rdb_iface->getInfo(&status_vector,
			stuff->p_info_items.cstr_length, stuff->p_info_items.cstr_address,
			buffer_length, //sizeof(temp)
			temp_buffer); //temp

		if (status_vector.isSuccess())
		{
			string version;
			version.printf("%s/%s%s", FB_VERSION, this->port_version->str_data,
				this->port_crypt_complete ? ":C" : "");
			info_db_len = MERGE_database_info(temp_buffer, //temp
				buffer, buffer_length,
				DbImplementation::current.backwardCompatibleImplementation(), 4, 1,
				reinterpret_cast<const UCHAR*>(version.c_str()),
				reinterpret_cast<const UCHAR*>(this->port_host->str_data));
		}
		break;

	case op_info_request:
		{
			Rrq* requestL;
			getHandle(requestL, stuff->p_info_object);
			requestL->rrq_iface->getInfo(&status_vector, stuff->p_info_incarnation,
				info_len, info_buffer, buffer_length, buffer);
		}
		break;

	case op_info_transaction:
		getHandle(transaction, stuff->p_info_object);
		transaction->rtr_iface->getInfo(&status_vector, info_len, info_buffer,
			buffer_length, buffer);
		break;

	case op_service_info:
		service = rdb->rdb_svc;
		service->svc_iface->query(&status_vector,
			stuff->p_info_items.cstr_length, stuff->p_info_items.cstr_address,
			info_len, info_buffer, buffer_length, buffer);
		break;

	case op_info_sql:
		getHandle(statement, stuff->p_info_object);
		statement->checkIface(isc_info_unprepared_stmt);

		statement->rsr_iface->getInfo(&status_vector, info_len, info_buffer,
			buffer_length, buffer);
		break;
	}

	// Send a response that includes the segment.

	ULONG response_len = info_db_len ? info_db_len : buffer_length;

	SLONG skip_len = 0;
	if (buffer && *buffer == isc_info_length)
	{
		skip_len = gds__vax_integer(buffer + 1, 2);
		const SLONG val = gds__vax_integer(buffer + 3, skip_len);
		fb_assert(val >= 0);
		skip_len += 3;
		if (val && (ULONG) val < response_len)
			response_len = val;
	}

	sendL->p_resp.p_resp_data.cstr_address = buffer + skip_len;

	this->send_response(sendL, stuff->p_info_object, response_len, &status_vector, false);
}


static Rtr* make_transaction (Rdb* rdb, ITransaction* iface)
{
/**************************************
 *
 *	m a k e _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Create a local transaction iface.
 *
 **************************************/
	Rtr* transaction = new Rtr;
	transaction->rtr_rdb = rdb;
	transaction->rtr_iface = iface;
	if (transaction->rtr_id = rdb->rdb_port->get_id(transaction))
	{
		transaction->rtr_next = rdb->rdb_transactions;
		rdb->rdb_transactions = transaction;
	}
	else
	{
		delete transaction;
		transaction = NULL;
	}

	return transaction;
}


static void ping_connection(rem_port* port, PACKET* send)
{
/**************************************
 *
 *	p i n g _ c o n n e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Check the connection for persistent errors.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = port->port_context;
	if (!bad_db(&status_vector, rdb))
		rdb->rdb_iface->ping(&status_vector);

	port->send_response(send, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::open_blob(P_OP op, P_BLOB* stuff, PACKET* sendL)
{
/**************************************
 *
 *	o p e n _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open or create a new blob.
 *
 **************************************/
	Rtr* transaction;
	LocalStatus status_vector;

	getHandle(transaction, stuff->p_blob_transaction);

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
	{
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	ULONG bpb_length = 0;
	const UCHAR* bpb = NULL;

	if (op == op_open_blob2 || op == op_create_blob2)
	{
		bpb_length = stuff->p_blob_bpb.cstr_length;
		bpb = stuff->p_blob_bpb.cstr_address;
	}

	ServBlob iface(op == op_open_blob || op == op_open_blob2 ?
		rdb->rdb_iface->openBlob(&status_vector, transaction->rtr_iface,
			&stuff->p_blob_id, bpb_length, bpb) :
		rdb->rdb_iface->createBlob(&status_vector, transaction->rtr_iface,
			&sendL->p_resp.p_resp_blob_id, bpb_length, bpb));

	USHORT object = 0;
	if (status_vector.isSuccess())
	{
		Rbl* blob = new Rbl;
#ifdef DEBUG_REMOTE_MEMORY
		printf("open_blob(server)         allocate blob    %x\n", blob);
#endif
		blob->rbl_iface = iface;
		blob->rbl_rdb = rdb;
		if (blob->rbl_id = this->get_id(blob))
		{
			object = blob->rbl_id;
			blob->rbl_rtr = transaction;
			blob->rbl_next = transaction->rtr_blobs;
			transaction->rtr_blobs = blob;
		}
		else
		{
			blob->rbl_iface->cancel(&status_vector);
			delete blob;
			(Arg::Gds(isc_too_many_handles)).copyTo(&status_vector);
		}
	}

	return this->send_response(sendL, object, 0, &status_vector, false);
}


ISC_STATUS rem_port::prepare(P_PREP * stuff, PACKET* sendL)
{
/**************************************
 *
 *	p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	End a transaction.
 *
 **************************************/
	Rtr* transaction;
	LocalStatus status_vector;

	getHandle(transaction, stuff->p_prep_transaction);

	transaction->rtr_iface->prepare(&status_vector,
		stuff->p_prep_data.cstr_length, stuff->p_prep_data.cstr_address);
	if (status_vector.isSuccess())
	{
		transaction->rtr_limbo = true;
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::prepare_statement(P_SQLST * prepareL, PACKET* sendL)
{
/*****************************************
 *
 *	p r e p a r e _ s t a t e m e n t
 *
 *****************************************
 *
 * Functional description
 *	Prepare a dynamic SQL statement for execution.
 *
 *****************************************/
	Rtr* transaction = NULL;
	Rsr* statement;

	// Do not call CHECK_HANDLE if this is the start of a transaction
	if (prepareL->p_sqlst_transaction)
	{
		getHandle(transaction, prepareL->p_sqlst_transaction);
	}
	getHandle(statement, prepareL->p_sqlst_statement);

	HalfStaticArray<UCHAR, 1024> local_buffer, info_buffer;
	ULONG infoLength = prepareL->p_sqlst_items.cstr_length;
	UCHAR* const info = info_buffer.getBuffer(infoLength + 1);
	UCHAR* const buffer = local_buffer.getBuffer(prepareL->p_sqlst_buffer_length);

	// stuff isc_info_length in front of info items buffer
	*info = isc_info_length;
	memmove(info + 1, prepareL->p_sqlst_items.cstr_address, infoLength++);
	const unsigned int flags = StatementMetadata::buildInfoFlags(infoLength, info);

	ITransaction* iface = NULL;
	if (transaction)
		iface = transaction->rtr_iface;

	// Since the API to GDS_DSQL_PREPARE is public and can not be changed, there needs to
	// be a way to send the parser version to DSQL so that the parser can compare the keyword
	// version to the parser version.  To accomplish this, the parser version is combined with
	// the client dialect and sent across that way.  In dsql8_prepare_statement, the parser version
	// and client dialect are separated and passed on to their final desintations.  The information
	// is combined as follows:
	//     Dialect * 10 + parser_version
	//
	// and is extracted in dsql8_prepare_statement as follows:
	//      parser_version = ((dialect * 10) + parser_version) % 10
	//      client_dialect = ((dialect * 10) + parser_version) / 10
	//
	// For example, parser_version = 1 and client dialect = 1
	//
	//  combined = (1 * 10) + 1 == 11
	//
	//  parser = (combined) % 10 == 1
	//  dialect = (combined) / 10 == 1

	LocalStatus status_vector;

	if (statement->rsr_iface)
	{
		statement->rsr_iface->free(&status_vector);
		if (!status_vector.isSuccess())
			return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	statement->rsr_cursor_name = "";

	Rdb* rdb = statement->rsr_rdb;
	if (bad_db(&status_vector, rdb))
	{
		return this->send_response(sendL, 0, 0, &status_vector, true);
	}

	statement->rsr_iface = rdb->rdb_iface->prepare(&status_vector,
		iface, prepareL->p_sqlst_SQL_str.cstr_length,
		reinterpret_cast<const char*>(prepareL->p_sqlst_SQL_str.cstr_address),
		prepareL->p_sqlst_SQL_dialect * 10 + PARSER_VERSION, flags);
	if (!status_vector.isSuccess())
		return this->send_response(sendL, 0, 0, &status_vector, false);

	LocalStatus s2;
	statement->rsr_iface->getInfo(&s2, infoLength, info, prepareL->p_sqlst_buffer_length, buffer);
	if (!s2.isSuccess())
		return this->send_response(sendL, 0, 0, &s2, false);

	REMOTE_reset_statement(statement);

	statement->rsr_flags.clear(Rsr::NO_BATCH | Rsr::DEFER_EXECUTE);
	USHORT state = check_statement_type(statement);
	if (state & STMT_NO_BATCH) {
		statement->rsr_flags.set(Rsr::NO_BATCH);
	}
	if ((state & STMT_DEFER_EXECUTE) && (port_flags & PORT_lazy)) {
		statement->rsr_flags.set(Rsr::DEFER_EXECUTE);
	}
	if (!(port_flags & PORT_lazy)) {
		state = 0;
	}

	// Send a response that includes the info requested

	ULONG response_len = prepareL->p_sqlst_buffer_length;
	SLONG skip_len = 0;
	if (*buffer == isc_info_length)
	{
		skip_len = gds__vax_integer(buffer + 1, 2);
		const SLONG val = gds__vax_integer(buffer + 3, skip_len);
		fb_assert(val >= 0);
		skip_len += 3;
		if (val && (ULONG) val < response_len)
			response_len = val;
	}

	sendL->p_resp.p_resp_data.cstr_address = buffer + skip_len;

	const ISC_STATUS status = this->send_response(sendL, state, response_len, &status_vector, false);

	return status;
}


class DecrementRequestsQueued
{
public:
	explicit DecrementRequestsQueued(rem_port* port) :
		m_port(port)
	{}

	~DecrementRequestsQueued()
	{
		--m_port->port_requests_queued;
	}

private:
	RemPortPtr m_port;
};

// Declared in serve_proto.h

static bool process_packet(rem_port* port, PACKET* sendL, PACKET* receive, rem_port** result)
{
/**************************************
 *
 *	p r o c e s s _ p a c k e t
 *
 **************************************
 *
 * Functional description
 *	Given an packet received from client, process it a packet ready to be
 *	sent.
 *
 **************************************/
	RefMutexGuard portGuard(*port->port_sync, FB_FUNCTION);
	DecrementRequestsQueued dec(port);

	try
	{
		const P_OP op = receive->p_operation;
		switch (op)
		{
		case op_connect:
			if (!accept_connection(port, &receive->p_cnct, sendL))
			{
				//const string& s = port->port_user_name;
				/***
				if (s.hasData())
				{								looks like logging rejects is not good idea any more?
					gds__log("SERVER/process_packet: connection rejected for %s", s.c_str());
				}
				***/

				if (port->port_server->srvr_flags & SRVR_multi_client)
					port->port_state = rem_port::BROKEN;
				else
				{
					// gds__log("SERVER/process_packet: connect reject, server exiting");
					port->disconnect(sendL, receive);
					return false;
				}
			}
			break;

		case op_compile:
			port->compile(&receive->p_cmpl, sendL);
			break;

		case op_attach:
		case op_create:
			attach_database(port, op, &receive->p_atch, sendL);
			break;

		case op_service_attach:
			attach_service(port, &receive->p_atch, sendL);
			break;

		case op_trusted_auth:	// PROTOCOL < 13
			trusted_auth(port, &receive->p_trau, sendL);
			break;

		case op_cont_auth:		// PROTOCOL >= 13
			continue_authentication(port, &receive->p_auth_cont, sendL);
			break;

		case op_update_account_info:
		case op_authenticate_user:
			send_error(port, sendL, isc_wish_list);
			break;

		case op_service_start:
			port->service_start(&receive->p_info, sendL);
			break;

		case op_disconnect:
		case op_exit:
			{
				const srvr* server = port->port_server;
				if (!server)
					break;

				if ((server->srvr_flags & SRVR_multi_client) && port != server->srvr_parent_port)
				{

					port->disconnect(sendL, receive);
					port = NULL;
					break;
				}

				if ((server->srvr_flags & SRVR_multi_client) && port == server->srvr_parent_port)
				{
					gds__log("SERVER/process_packet: Multi-client server shutdown");
				}
				port->disconnect(sendL, receive);
				return false;
			}

		case op_receive:
			port->receive_msg(&receive->p_data, sendL);
			break;

		case op_send:
			port->send_msg(&receive->p_data, sendL);
			break;

		case op_start:
		case op_start_and_receive:
			port->start(op, &receive->p_data, sendL);
			break;

		case op_start_and_send:
		case op_start_send_and_receive:
			port->start_and_send(op, &receive->p_data, sendL);
			break;

		case op_transact:
			port->transact_request(&receive->p_trrq, sendL);
			break;

		case op_reconnect:
		case op_transaction:
			port->start_transaction(op, &receive->p_sttr, sendL);
			break;

		case op_prepare:
		case op_rollback:
		case op_rollback_retaining:
		case op_commit:
		case op_commit_retaining:
			port->end_transaction(op, &receive->p_rlse, sendL);
			break;

		case op_detach:
			port->end_database(&receive->p_rlse, sendL);
			break;

		case op_service_detach:
			port->service_end(&receive->p_rlse, sendL);
			break;

		case op_drop_database:
			port->drop_database(&receive->p_rlse, sendL);
			break;

		case op_create_blob:
		case op_open_blob:
		case op_create_blob2:
		case op_open_blob2:
			port->open_blob(op, &receive->p_blob, sendL);
			break;

		case op_batch_segments:
		case op_put_segment:
			port->put_segment(op, &receive->p_sgmt, sendL);
			break;

		case op_get_segment:
			port->get_segment(&receive->p_sgmt, sendL);
			break;

		case op_seek_blob:
			port->seek_blob(&receive->p_seek, sendL);
			break;

		case op_cancel_blob:
		case op_close_blob:
			port->end_blob(op, &receive->p_rlse, sendL);
			break;

		case op_prepare2:
			port->prepare(&receive->p_prep, sendL);
			break;

		case op_release:
			port->end_request(&receive->p_rlse, sendL);
			break;

		case op_info_blob:
		case op_info_database:
		case op_info_request:
		case op_info_transaction:
		case op_service_info:
		case op_info_sql:
			port->info(op, &receive->p_info, sendL);
			break;

		case op_que_events:
			port->que_events(&receive->p_event, sendL);
			break;

		case op_cancel_events:
			cancel_events(port, &receive->p_event, sendL);
			break;

		case op_connect_request:
			aux_request(port, /*&receive->p_req,*/ sendL);
			break;

		case op_ddl:
			port->ddl(&receive->p_ddl, sendL);
			break;

		case op_get_slice:
			port->get_slice(&receive->p_slc, sendL);
			break;

		case op_put_slice:
			port->put_slice(&receive->p_slc, sendL);
			break;

		case op_allocate_statement:
			allocate_statement(port, /*&receive->p_rlse,*/ sendL);
			break;

		case op_execute:
		case op_execute2:
			port->execute_statement(op, &receive->p_sqldata, sendL);
			break;

		case op_exec_immediate:
		case op_exec_immediate2:
			port->execute_immediate(op, &receive->p_sqlst, sendL);
			break;

		case op_fetch:
			port->fetch(&receive->p_sqldata, sendL);
			break;

		case op_free_statement:
			port->end_statement(&receive->p_sqlfree, sendL);
			break;

		case op_prepare_statement:
			port->prepare_statement(&receive->p_sqlst, sendL);
			break;

		case op_set_cursor:
			port->set_cursor(&receive->p_sqlcur, sendL);
			break;

		case op_dummy:
			sendL->p_operation = op_dummy;
			port->send(sendL);
			break;

		case op_cancel:
			cancel_operation(port, receive->p_cancel_op.p_co_kind);
			break;

		case op_ping:
			ping_connection(port, sendL);
			break;

		case op_crypt:
			port->start_crypt(&receive->p_crypt, sendL);
			break;

		///case op_insert:
		default:
			gds__log("SERVER/process_packet: don't understand packet type %d", receive->p_operation);
			port->port_state = rem_port::BROKEN;
			break;
		}

		if (port && port->port_state == rem_port::BROKEN)
		{
			if (!port->port_parent)
			{
				if (!Worker::isShuttingDown() && !(port->port_flags & PORT_rdb_shutdown))
					gds__log("SERVER/process_packet: broken port, server exiting");
				port->disconnect(sendL, receive);
				return false;
			}
			port->disconnect(sendL, receive);
			port = NULL;
		}

	}	// try
	catch (const status_exception& ex)
	{
		// typical case like bad interface passed
		LocalStatus local_status;

		ex.stuffException(&local_status);

		// Send it to the user
		port->send_response(sendL, 0, 0, &local_status, false);
	}
	catch (const Exception& ex)
	{
		// something more serious happened
		LocalStatus local_status;

		// Log the error to the user.
		ex.stuffException(&local_status);
		gds__log_status(0, local_status.get());

		port->send_response(sendL, 0, 0, &local_status, false);
		port->disconnect(sendL, receive);	// Well, how about this...

		return false;
	}

	if (result)
	{
		*result = port;
	}

	return true;
}


static void trusted_auth(rem_port* port, const P_TRAU* p_trau, PACKET* send)
{
/**************************************
 *
 *	t r u s t e d _ a u t h
 *
 **************************************
 *
 * Functional description
 *	Server side of trusted auth handshake.
 *
 **************************************/
	ServerAuthBase* sa = port->port_srv_auth;
	if (! sa)
	{
		send_error(port, send, isc_unavailable);
	}

	if (port->port_protocol < PROTOCOL_VERSION11 || port->port_protocol >= PROTOCOL_VERSION13)
	{
		send_error(port, send, (Arg::Gds(isc_random) << "Operation not supported for network protocol"));
	}

	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: trusted_auth\n"));
	port->port_srv_auth_block->setDataForPlugin(p_trau->p_trau_data);
	if (sa->authenticate(send, ServerAuth::CONT_AUTH))
	{
		delete sa;
		port->port_srv_auth = NULL;
	}
}


static void continue_authentication(rem_port* port, const p_auth_continue* p_auth_c, PACKET* send)
{
/**************************************
 *
 *	c o n t i n u e _ a u t h e n t i c a t i o n
 *
 **************************************
 *
 * Functional description
 *	Server side of multi-hop auth handshake.
 *
 **************************************/
	ServerAuthBase* sa = port->port_srv_auth;
	if (! sa)
	{
		send_error(port, send, isc_unavailable);
	}

	if (port->port_protocol < PROTOCOL_VERSION13)
	{
		send_error(port, send, (Arg::Gds(isc_random) << "Operation not supported for network protocol"));
	}

	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: continue_authentication\n"));
	port->port_srv_auth_block->setDataForPlugin(p_auth_c);
	if (sa->authenticate(send, ServerAuth::CONT_AUTH))
	{
		delete sa;
		port->port_srv_auth = NULL;
	}
}


ISC_STATUS rem_port::put_segment(P_OP op, P_SGMT * segment, PACKET* sendL)
{
/**************************************
 *
 *	p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Write a single blob segment.
 *
 **************************************/
	Rbl* blob;

	getHandle(blob, segment->p_sgmt_blob);

	const UCHAR* p = segment->p_sgmt_segment.cstr_address;
	ULONG length = segment->p_sgmt_segment.cstr_length;

	// Do the signal segment version.  If it failed, just pass on the bad news.

	LocalStatus status_vector;

	if (op == op_put_segment)
	{
		blob->rbl_iface->putSegment(&status_vector, length, p);
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	// We've got a batch of segments.  This is only slightly more complicated

	const UCHAR* const end = p + length;

	while (p < end)
	{
		length = *p++;
		length += *p++ << 8;
		blob->rbl_iface->putSegment(&status_vector, length, p);

		if (!status_vector.isSuccess())
			return this->send_response(sendL, 0, 0, &status_vector, false);
		p += length;
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::put_slice(P_SLC * stuff, PACKET* sendL)
{
/**************************************
 *
 *	p u t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Put an array slice.
 *
 **************************************/
	Rtr* transaction;
	LocalStatus status_vector;

	getHandle(transaction, stuff->p_slc_transaction);

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	sendL->p_resp.p_resp_blob_id = stuff->p_slc_id;
	rdb->rdb_iface->putSlice(&status_vector, transaction->rtr_iface,
		&sendL->p_resp.p_resp_blob_id,
		stuff->p_slc_sdl.cstr_length, stuff->p_slc_sdl.cstr_address,
		stuff->p_slc_parameters.cstr_length, stuff->p_slc_parameters.cstr_address,
		stuff->p_slc_slice.lstr_length, stuff->p_slc_slice.lstr_address);

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::que_events(P_EVENT * stuff, PACKET* sendL)
{
/**************************************
 *
 *	q u e _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	// Find unused event block or, if necessary, a new one

	Rvnt* event;
	for (event = rdb->rdb_events; event; event = event->rvnt_next)
	{
		if (!event->rvnt_iface)
			break;
	}

	if (!event)
	{
		event = new Rvnt;
#ifdef DEBUG_REMOTE_MEMORY
		printf("que_events(server)        allocate event   %x\n", event);
#endif
		event->rvnt_next = rdb->rdb_events;
		rdb->rdb_events = event;
		event->rvnt_callback = new Callback(event);
	}

	event->rvnt_id = stuff->p_event_rid;
	event->rvnt_rdb = rdb;

	event->rvnt_iface = rdb->rdb_iface->queEvents(&status_vector, event->rvnt_callback,
		stuff->p_event_items.cstr_length, stuff->p_event_items.cstr_address);

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::receive_after_start(P_DATA* data, PACKET* sendL, IStatus* status_vector)
{
/**************************************
 *
 *	r e c e i v e _ a f t e r _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Receive a message.
 *
 **************************************/
	Rrq* requestL;

	getHandle(requestL, data->p_data_request);

	const USHORT level = data->p_data_incarnation;
	requestL = REMOTE_find_request(requestL, level);

	// Figure out the number of the message that we're stalled on.

	USHORT msg_number;
	if (!get_next_msg_no(requestL, level, &msg_number))
		return this->send_response(sendL, 0, 0, status_vector, false);

	sendL->p_operation = op_response_piggyback;
	P_RESP* response = &sendL->p_resp;
	response->p_resp_object = msg_number;
	response->p_resp_data.cstr_length = 0;

	if (!sendL->p_resp.p_resp_status_vector)
		sendL->p_resp.p_resp_status_vector = FB_NEW(*getDefaultMemoryPool()) Firebird::DynamicStatusVector();

	sendL->p_resp.p_resp_status_vector->save(status_vector->get());

	this->send_partial(sendL);

	// Fill in packet to fool receive into thinking that it has been
	// called directly by the client.

	const Rrq::rrq_repeat* tail = &requestL->rrq_rpt[msg_number];
	const rem_fmt* format = tail->rrq_format;

	data->p_data_message_number = msg_number;
	data->p_data_messages = REMOTE_compute_batch_size(this,
		(USHORT) xdr_protocol_overhead(op_response_piggyback), op_send, format);

	return this->receive_msg(data, sendL);
}


ISC_STATUS rem_port::receive_msg(P_DATA * data, PACKET* sendL)
{
/**************************************
 *
 *	r e c e i v e _ m s g
 *
 **************************************
 *
 * Functional description
 *	Receive a message.
 *
 **************************************/

	// Find the database, request, message number, and the number of
	// messages the client is willing to cope with.  Then locate the
	// message control block for the request and message type.

	Rrq* requestL;
	getHandle(requestL, data->p_data_request);

	const USHORT level = data->p_data_incarnation;
	requestL = REMOTE_find_request(requestL, level);
	const USHORT msg_number = data->p_data_message_number;
	USHORT count, count2;
	count2 = count = data->p_data_messages;

	LocalStatus status_vector;

	if (msg_number > requestL->rrq_max_msg)
	{
		(Arg::Gds(isc_badmsgnum)).copyTo(&status_vector);
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}
	Rrq::rrq_repeat* tail = &requestL->rrq_rpt[msg_number];
	const rem_fmt* format = tail->rrq_format;

	// Get ready to ship the data out

	sendL->p_operation = op_send;
	sendL->p_data.p_data_request = data->p_data_request;
	sendL->p_data.p_data_message_number = msg_number;
	sendL->p_data.p_data_incarnation = level;
	sendL->p_data.p_data_messages = 1;

	// Check to see if any messages are already sitting around

	RMessage* message = 0;

	while (true)
	{
		message = tail->rrq_xdr;

		// If we don't have a message cached, get one from the next layer down.

		if (!message->msg_address)
		{
			// If we have an error queued for delivery, send it now

			if (!requestL->rrqStatus.isSuccess())
			{
				const ISC_STATUS res =
					this->send_response(sendL, 0, 0, requestL->rrqStatus.value(), false);
				requestL->rrqStatus.clear();
				return res;
			}

			requestL->rrq_iface->receive(&status_vector, level,
				msg_number, format->fmt_length, message->msg_buffer);

			if (!status_vector.isSuccess())
				return this->send_response(sendL, 0, 0, &status_vector, false);

			message->msg_address = message->msg_buffer;
		}

		// If there aren't any buffers remaining, break out of loop

		if (--count <= 0)
			break;

		// There's a buffer waiting -- see if the request is ready to send

		RMessage* next = message->msg_next;

		if ((next == message || !next->msg_address) &&
			!check_request(requestL, data->p_data_incarnation, msg_number))
		{
			// We've reached the end of the RSE - don't prefetch and flush
			// everything we've buffered so far

			count2 = 0;
			break;
		}

		if (!this->send_partial(sendL))
			return FALSE;
		message->msg_address = NULL;
	}

	sendL->p_data.p_data_messages = 0;
	this->send(sendL);
	message->msg_address = NULL;

	// Bump up the message pointer to resync with rrq_xdr (rrq_xdr
	// was incremented by xdr_request in the SEND call).

	tail->rrq_message = message->msg_next;

	// Since we have a little time on our hands while this packet is sent
	// and processed, get the next batch of records.  Start by finding the
	// next free buffer.

	message = tail->rrq_xdr;
	RMessage* prior = NULL;

	while (message->msg_address && message->msg_next != tail->rrq_xdr)
		message = message->msg_next;

	for (; count2 && check_request(requestL, data->p_data_incarnation, msg_number); --count2)
	{
		if (message->msg_address)
		{
			if (!prior)
				prior = tail->rrq_xdr;

			while (prior->msg_next != message)
				prior = prior->msg_next;

			// allocate a new message block and put it in the cache

			message = new RMessage(format->fmt_length);
#ifdef DEBUG_REMOTE_MEMORY
			printf("receive_msg(server)       allocate message %x\n", message);
#endif
			message->msg_number = prior->msg_number;
			message->msg_next = prior->msg_next;

			prior->msg_next = message;
			prior = message;
		}

		// fetch the next record into cache; even for scrollable cursors, we are
		// just doing a simple lookahead continuing on in the last direction specified,
		// so there is no reason to do an isc_receive2()

		requestL->rrq_iface->receive(&status_vector, data->p_data_incarnation,
			msg_number, format->fmt_length, message->msg_buffer);

		// Did we have an error?  If so, save it for later delivery

		if (!status_vector.isSuccess())
		{
			// If already have an error queued, don't overwrite it

			if (requestL->rrqStatus.isSuccess())
				requestL->rrqStatus.save(status_vector.get());

			break;
		}

		message->msg_address = message->msg_buffer;
		message = message->msg_next;
	}

	return TRUE;
}


static void release_blob(Rbl* blob)
{
/**************************************
 *
 *	r e l e a s e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Release a blob block and friends.
 *
 **************************************/
	Rdb* rdb = blob->rbl_rdb;
	Rtr* transaction = blob->rbl_rtr;

	rdb->rdb_port->releaseObject(blob->rbl_id);

	for (Rbl** p = &transaction->rtr_blobs; *p; p = &(*p)->rbl_next)
	{
		if (*p == blob)
		{
			*p = blob->rbl_next;
			break;
		}
	}

#ifdef DEBUG_REMOTE_MEMORY
	printf("release_blob(server)      free blob        %x\n", blob);
#endif
	delete blob;
}


static void release_event( Rvnt* event)
{
/**************************************
 *
 *	r e l e a s e _ e v e n t
 *
 **************************************
 *
 * Functional description
 *	Release an event block.
 *
 **************************************/
	Rdb* rdb = event->rvnt_rdb;

	for (Rvnt** p = &rdb->rdb_events; *p; p = &(*p)->rvnt_next)
	{
		if (*p == event)
		{
			*p = event->rvnt_next;
			break;
		}
	}

	delete event;
}


static void release_request( Rrq* request)
{
/**************************************
 *
 *	r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request block.
 *
 **************************************/
	Rdb* rdb = request->rrq_rdb;
	rdb->rdb_port->releaseObject(request->rrq_id);
	REMOTE_release_request(request);
}


static void release_statement( Rsr** statement)
{
/**************************************
 *
 *	r e l e a s e _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Release a GDML or SQL statement?
 *
 **************************************/
	if ((*statement)->rsr_cursor)
	{
		(*statement)->rsr_cursor = NULL;

		Rtr* const transaction = (*statement)->rsr_rtr;
		fb_assert(transaction);

		size_t pos;
		if (transaction->rtr_cursors.find(*statement, pos))
			transaction->rtr_cursors.remove(pos);
		else
			fb_assert(false);
	}

	delete (*statement)->rsr_select_format;
	delete (*statement)->rsr_bind_format;

	(*statement)->releaseException();
	REMOTE_release_messages((*statement)->rsr_message);

	delete *statement;
	*statement = NULL;
}


static void release_sql_request( Rsr* statement)
{
/**************************************
 *
 *	r e l e a s e _ s q l _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release an SQL request block.
 *
 **************************************/
	Rdb* rdb = statement->rsr_rdb;
	rdb->rdb_port->releaseObject(statement->rsr_id);

	for (Rsr** p = &rdb->rdb_sql_requests; *p; p = &(*p)->rsr_next)
	{
		if (*p == statement)
		{
			*p = statement->rsr_next;
			break;
		}
	}

	release_statement(&statement);
}


static void release_transaction( Rtr* transaction)
{
/**************************************
 *
 *	r e l e a s e _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Release a transaction block and friends.
 *
 **************************************/
	Rdb* rdb = transaction->rtr_rdb;
	rdb->rdb_port->releaseObject(transaction->rtr_id);

	while (transaction->rtr_blobs)
		release_blob(transaction->rtr_blobs);

	while (transaction->rtr_cursors.hasData())
	{
		Rsr* const statement = transaction->rtr_cursors.pop();
		fb_assert(statement->rsr_cursor);
		statement->rsr_cursor = NULL;
		statement->rsr_cursor_name = "";
	}

	for (Rtr** p = &rdb->rdb_transactions; *p; p = &(*p)->rtr_next)
	{
		if (*p == transaction)
		{
			*p = transaction->rtr_next;
			break;
		}
	}

#ifdef DEBUG_REMOTE_MEMORY
	printf("release_transact(server)  free transaction %x\n", transaction);
#endif
	delete transaction;
}


ISC_STATUS rem_port::seek_blob(P_SEEK* seek, PACKET* sendL)
{
/**************************************
 *
 *	s e e k _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Execute a blob seek operation.
 *
 **************************************/
	Rbl* blob;

	getHandle(blob, seek->p_seek_blob);

	const SSHORT mode = seek->p_seek_mode;
	const SLONG offset = seek->p_seek_offset;

	LocalStatus status_vector;
	SLONG result = blob->rbl_iface->seek(&status_vector, mode, offset);

	sendL->p_resp.p_resp_blob_id.gds_quad_low = result;

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::send_msg(P_DATA * data, PACKET* sendL)
{
/**************************************
 *
 *	s e n d _ m s g
 *
 **************************************
 *
 * Functional description
 *	Handle a isc_send operation.
 *
 **************************************/
	LocalStatus status_vector;

	Rrq* requestL;
	getHandle(requestL, data->p_data_request);

	const USHORT number = data->p_data_message_number;
	requestL = REMOTE_find_request(requestL, data->p_data_incarnation);
	if (number > requestL->rrq_max_msg)
	{
		(Arg::Gds(isc_badmsgnum)).copyTo(&status_vector);
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}
	RMessage* message = requestL->rrq_rpt[number].rrq_message;
	const rem_fmt* format = requestL->rrq_rpt[number].rrq_format;

	requestL->rrq_iface->send(&status_vector, data->p_data_incarnation, number,
		format->fmt_length, message->msg_address);

	message->msg_address = NULL;

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::send_response(	PACKET*	sendL,
							OBJCT	object,
							ULONG	length,
							const ISC_STATUS* status_vector,
							bool defer_flag)
{
/**************************************
 *
 *	s e n d _ r e s p o n s e
 *
 **************************************
 *
 * Functional description
 *	Send a response packet.
 *
 **************************************/

	P_RESP* response = &sendL->p_resp;

	// Start by translating the status vector into "generic" form

	Firebird::SimpleStatusVector new_vector;
	const ISC_STATUS* old_vector = status_vector;
	const ISC_STATUS exit_code = old_vector[1];

	char buffer[1024];
	char* p = buffer;
	char* bufferEnd = p + sizeof(buffer);

	for (bool sw = true; *old_vector && sw;)
	{
		switch (*old_vector)
		{
		case isc_arg_warning:
		case isc_arg_gds:
			new_vector.push(*old_vector++);

			// The status codes are converted to their offsets so that they
			// were compatible with the RDB implementation.  This was fine
			// when status codes were restricted to a single facility.  Now
			// that the facility is part of the status code we need to know
			// this on the client side, thus when talking with 6.0 and newer
			// clients, do not decode the status code, just send it to the
			// client.  The same check is made in interface.cpp::check_response
			new_vector.push(*old_vector++);

			for (;;)
			{
				switch (*old_vector)
				{
				case isc_arg_cstring:
					new_vector.push(*old_vector++);
					// fall through ...

				case isc_arg_string:
				case isc_arg_number:
					new_vector.push(*old_vector++);
					new_vector.push(*old_vector++);
					continue;
				} // switch
				break;
			} // for (;;)
			continue;

		case isc_arg_interpreted:
		case isc_arg_sql_state:
			new_vector.push(*old_vector++);
			new_vector.push(*old_vector++);
			continue;
		}

		const int l = (p < bufferEnd) ? fb_interpret(p, bufferEnd - p, &old_vector) : 0;
		if (l == 0)
			break;

		new_vector.push(isc_arg_interpreted);
		new_vector.push((ISC_STATUS)(IPTR) p);

		p += l;
		sw = false;
	}

	new_vector.push(isc_arg_end);

	// Format and send response.  Note: the blob_id and data address fields
	// of the response packet may contain valid data.  Don't trash them.

	if (!response->p_resp_status_vector)
		response->p_resp_status_vector = FB_NEW(*getDefaultMemoryPool()) Firebird::DynamicStatusVector();

	response->p_resp_status_vector->save(new_vector.begin());

	sendL->p_operation = op_response;

	response->p_resp_object = object;
	response->p_resp_data.cstr_length = length;

	const bool defer = (this->port_flags & PORT_lazy) && defer_flag;

	if (defer)
		this->send_partial(sendL);
	else
	{
		this->send(sendL);

		// If database or attachment has been shut down,
		// there's no point in keeping the connection open
		if (exit_code == isc_shutdown || exit_code == isc_att_shutdown)
		{
			this->port_state = rem_port::BROKEN;
			this->port_flags |= PORT_rdb_shutdown;
		}
	}

	return exit_code;
}

// Maybe this can be a member of rem_port?
static void send_error(rem_port* port, PACKET* apacket, ISC_STATUS errcode)
{
	LocalStatus status_vector;
	(Arg::Gds(errcode)).copyTo(&status_vector);
	port->send_response(apacket, 0, 0, &status_vector, false);
}

// Maybe this can be a member of rem_port?
static void send_error(rem_port* port, PACKET* apacket, const Firebird::Arg::StatusVector& err)
{
	LocalStatus status_vector;
	err.copyTo(&status_vector);
	port->send_response(apacket, 0, 0, &status_vector, false);
}


static void attach_service(rem_port* port, P_ATCH* attach, PACKET* sendL)
{
	WIRECRYPT_DEBUG(fprintf(stderr, "Line encryption %sabled on attach svc\n", port->port_crypt_complete ? "en" : "dis"));
	if (port->port_required_encryption && !port->port_crypt_complete)
	{
		Arg::Gds(isc_miss_wirecrypt).raise();
	}

	PathName manager(attach->p_atch_file.cstr_address, attach->p_atch_file.cstr_length);

	ClumpletWriter* wrt = FB_NEW(*getDefaultMemoryPool()) ClumpletWriter(*getDefaultMemoryPool(),
		ClumpletReader::spbList, MAX_DPB_SIZE, attach->p_atch_dpb.cstr_address, attach->p_atch_dpb.cstr_length);
	port->port_srv_auth = new ServiceAttachAuth(port,
		PathName(attach->p_atch_file.cstr_address, attach->p_atch_file.cstr_length), wrt);

	if (port->port_srv_auth->authenticate(sendL))
	{
		delete port->port_srv_auth;
		port->port_srv_auth = NULL;
	}
}


void ServiceAttachAuth::accept(PACKET* sendL, Auth::WriterImplementation* authBlock)
{
	authBlock->store(pb, isc_spb_auth_block);
	authPort->port_srv_auth_block->extractNewKeys(&sendL->p_resp.p_resp_data);

	authPort->service_attach(managerName.c_str(), pb, sendL);
}


ISC_STATUS rem_port::service_attach(const char* service_name,
									ClumpletWriter* spb,
									PACKET* sendL)
{
/**************************************
 *
 *	s e r v i c e _ a t t a c h
 *
 **************************************
 *
 * Functional description
 *	Connect to a Firebird service.
 *
 **************************************/
	sendL->p_operation = op_accept;

    // Now insert additional clumplets into spb
	addClumplets(spb, spbParam, this);

	for (spb->rewind(); !spb->isEof();)
	{
		switch (spb->getClumpTag())
		{
		// remove old-style logon parameters
		case isc_spb_user_name:
		case isc_spb_password:
		case isc_spb_password_enc:

		// remove trusted auth & trusted role if present (security measure)
		case isc_spb_trusted_role:
		case isc_spb_trusted_auth:

		// remove user config info (security measure)
		case isc_spb_config:
			spb->deleteClumplet();
			break;

		default:
			spb->moveNext();
			break;
		}
	}

	// See if user has specified parameters relevant to the connection,
	// they will be stuffed in the SPB if so.
	REMOTE_get_timeout_params(this, spb);

	if (!port_server_crypt_callback)
	{
		port_server_crypt_callback = new ServerCallback(this);
	}

	DispatcherPtr provider;
	LocalStatus status_vector;

	provider->setDbCryptCallback(&status_vector, port_server_crypt_callback->getInterface());
	if (status_vector.isSuccess())
	{
		dumpAuthBlock("rem_port::service_attach()", spb, isc_spb_auth_block);
		ServService iface(provider->attachServiceManager(&status_vector, service_name,
			(ULONG) spb->getBufferLength(), spb->getBuffer()));

		if (status_vector.isSuccess())
		{
			Rdb* rdb = new Rdb;

			this->port_context = rdb;
#ifdef DEBUG_REMOTE_MEMORY
			printf("attach_service(server)  allocate rdb     %x\n", rdb);
#endif
			rdb->rdb_port = this;
			Svc* svc = rdb->rdb_svc = new Svc;
			svc->svc_iface = iface;
		}
	}

	return this->send_response(sendL, 0, sendL->p_resp.p_resp_data.cstr_length, &status_vector,
		false);
}


ISC_STATUS rem_port::service_end(P_RLSE* /*release*/, PACKET* sendL)
{
/**************************************
 *
 *	s e r v i c e _ e n d
 *
 **************************************
 *
 * Functional description
 *	Close down a service.
 *
 **************************************/
    LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_service(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	rdb->rdb_svc->svc_iface->detach(&status_vector);

	if (status_vector.isSuccess())
	{
		port_flags |= PORT_detached;
		rdb->rdb_svc->svc_iface = NULL;
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


void rem_port::service_start(P_INFO * stuff, PACKET* sendL)
{
/**************************************
 *
 *	s e r v i c e _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start a service on the server
 *
 **************************************/
    LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_service(&status_vector, rdb))
	{
		this->send_response(sendL, 0, 0, &status_vector, false);
		return;
	}

	Svc* svc = rdb->rdb_svc;
	svc->svc_iface->start(&status_vector, stuff->p_info_items.cstr_length,
		stuff->p_info_items.cstr_address);
	this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::set_cursor(P_SQLCUR * sqlcur, PACKET* sendL)
{
/*****************************************
 *
 *	s e t _ c u r s o r
 *
 *****************************************
 *
 * Functional description
 *	Set a cursor name for a dynamic request.
 *
 *****************************************/
	Rsr* statement;
	LocalStatus status_vector;
	const char* name = reinterpret_cast<const char*>(sqlcur->p_sqlcur_cursor_name.cstr_address);

	getHandle(statement, sqlcur->p_sqlcur_statement);


	if (statement->rsr_cursor)
		statement->rsr_cursor->setCursorName(&status_vector, name);
	else
	{
		if (statement->rsr_cursor_name.hasData() && statement->rsr_cursor_name != name)
		{
			status_vector.set((Arg::Gds(isc_dsql_decl_err) <<
				Arg::Gds(isc_dsql_cursor_redefined) << statement->rsr_cursor_name).value());
		}
		else
			statement->rsr_cursor_name = name;
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


void rem_port::start_crypt(P_CRYPT * crypt, PACKET* sendL)
/*****************************************
 *
 *	s t a r t _ c r y p t
 *
 *****************************************
 *
 * Functional description
 *	Start to crypt the wire using given plugin and key.
 *	Disconnect on error but send OK on success.
 *
 *****************************************/
{
	try
	{
		FbCryptKey* key = NULL;
		PathName keyName(crypt->p_key.cstr_address, crypt->p_key.cstr_length);
		for (unsigned k = 0; k < port_crypt_keys.getCount(); ++k)
		{
			if (keyName == port_crypt_keys[k]->type)
			{
				key = port_crypt_keys[k];
				break;
			}
		}

		if (! key)
		{
			(Arg::Gds(isc_wirecrypt_key) << keyName).raise();
		}

		PathName plugName(crypt->p_plugin.cstr_address, crypt->p_plugin.cstr_length);
		// Check it's availability
		Remote::ParsedList plugins;
		REMOTE_parseList(plugins, Config::getDefaultConfig()->getPlugins(PluginType::WireCrypt));
		bool found = false;
		for (unsigned n = 0; n < plugins.getCount(); ++n)
		{
			if (plugins[n] == plugName)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			(Arg::Gds(isc_wirecrypt_plugin) << plugName).raise();
		}

		GetPlugins<IWireCryptPlugin> cp(PluginType::WireCrypt, FB_WIRECRYPT_PLUGIN_VERSION, upInfo,
										plugName.c_str());
		if (!cp.hasData())
		{
			(Arg::Gds(isc_wirecrypt_plugin) << plugName).raise();
		}

		// Initialize crypt key
		LocalStatus st;
		cp.plugin()->setKey(&st, key);
		if (! st.isSuccess())
		{
			status_exception::raise(st.get());
		}

		// Install plugin
		port_crypt_plugin = cp.plugin();
		port_crypt_plugin->addRef();
		port_crypt_complete = true;

		send_response(sendL, 0, 0, &st, false);
		WIRECRYPT_DEBUG(fprintf(stderr, "Installed cipher %s key %s\n", cp.name(), key->type));
	}
	catch(const Exception& ex)
	{
		iscLogException("start_crypt:", ex);
		disconnect();
	}
}


void set_server(rem_port* port, USHORT flags)
{
/**************************************
 *
 *	s e t _ s e r v e r
 *
 **************************************
 *
 * Functional description
 *	Look up the server for this type
 *	of port.  If one doesn't exist,
 *	create it.
 *
 **************************************/
	MutexLockGuard srvrGuard(servers_mutex, FB_FUNCTION);
	srvr* server;

	for (server = servers; server; server = server->srvr_next)
	{
		if (port->port_type == server->srvr_port_type)
			break;
	}

	if (!server)
	{
		servers = server = new srvr(servers, port, flags);
		fb_shutdown_callback(0, shut_server, fb_shut_postproviders, 0);
	}

	port->port_server = server;
}


ISC_STATUS rem_port::start(P_OP operation, P_DATA * data, PACKET* sendL)
{
/**************************************
 *
 *	s t a r t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	Rtr* transaction;

	getHandle(transaction, data->p_data_transaction);

	Rrq* requestL;
	getHandle(requestL, data->p_data_request);

	requestL = REMOTE_find_request(requestL, data->p_data_incarnation);
	REMOTE_reset_request(requestL, 0);

	LocalStatus status_vector;

	requestL->rrq_iface->start(&status_vector, transaction->rtr_iface, data->p_data_incarnation);

	if (status_vector.isSuccess())
	{
		requestL->rrq_rtr = transaction;
		if (operation == op_start_and_receive)
			return this->receive_after_start(data, sendL, &status_vector);
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::start_and_send(P_OP operation, P_DATA* data, PACKET* sendL)
{
/**************************************
 *
 *	s t a r t _ a n d _ s e n d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
    LocalStatus status_vector;
	Rtr* transaction;
	getHandle(transaction, data->p_data_transaction);

	Rrq* requestL;
	getHandle(requestL, data->p_data_request);

	requestL = REMOTE_find_request(requestL, data->p_data_incarnation);
	const USHORT number = data->p_data_message_number;

	if (number > requestL->rrq_max_msg)
	{
		(Arg::Gds(isc_badmsgnum)).copyTo(&status_vector);
		return this->send_response(sendL, 0, 0, &status_vector, false);
	}

	RMessage* message = requestL->rrq_rpt[number].rrq_message;
	const rem_fmt* format = requestL->rrq_rpt[number].rrq_format;
	REMOTE_reset_request(requestL, message);

	requestL->rrq_iface->startAndSend(&status_vector,
		transaction->rtr_iface, data->p_data_incarnation,
		number, format->fmt_length, message->msg_address);

	if (status_vector.isSuccess())
	{
		requestL->rrq_rtr = transaction;
		if (operation == op_start_send_and_receive)
			return this->receive_after_start(data, sendL, &status_vector);
	}

	return this->send_response(sendL, 0, 0, &status_vector, false);
}


ISC_STATUS rem_port::start_transaction(P_OP operation, P_STTR * stuff, PACKET* sendL)
{
/**************************************
 *
 *	s t a r t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	LocalStatus status_vector;

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	ServTransaction iface(operation == op_reconnect ?
		rdb->rdb_iface->reconnectTransaction(&status_vector,
			stuff->p_sttr_tpb.cstr_length, stuff->p_sttr_tpb.cstr_address) :
		rdb->rdb_iface->startTransaction(&status_vector,
			stuff->p_sttr_tpb.cstr_length, stuff->p_sttr_tpb.cstr_address));

	OBJCT object = 0;
	if (status_vector.isSuccess())
	{
		Rtr* transaction = make_transaction(rdb, iface);
		if (transaction)
		{
			object = transaction->rtr_id;
			if (operation == op_reconnect)
				transaction->rtr_limbo = true;
#ifdef DEBUG_REMOTE_MEMORY
			printf("start_transaction(server) allocate trans   %x\n", transaction);
#endif
		}
		else
		{
			if (operation != op_reconnect)
			{
				iface->rollback(&status_vector);
			}
			else
			{
				// Note that there is an underlying transaction pool
				// that won't be released until this connection is
				// detached. At least release the y-valve iface.
				iface->disconnect(&status_vector);
			}

			(Arg::Gds(isc_too_many_handles)).copyTo(&status_vector);
		}
	}

	return this->send_response(sendL, object, 0, &status_vector, false);
}


static THREAD_ENTRY_DECLARE loopThread(THREAD_ENTRY_PARAM)
{
/**************************************
 *
 *	t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Execute requests in a happy loop.
 *
 **************************************/

	try {

	FpeControl::maskAll();

	Worker worker;

	while (!Worker::isShuttingDown())
	{
		MutexEnsureUnlock reqQueGuard(request_que_mutex, FB_FUNCTION);
		reqQueGuard.enter();
		server_req_t* request = request_que;
		if (request)
		{
			worker.setState(true);

			REMOTE_TRACE(("Dequeue request %p", request_que));
			request_que = request->req_next;
			ports_pending--;
			reqQueGuard.leave();

			while (request)
			{
				rem_port* port = NULL;

				// Bind a thread to a port.

				if (request->req_port->port_server_flags & SRVR_thread_per_port)
				{
					port = request->req_port;
					free_request(request);

					SRVR_main(port, port->port_server_flags);
					request = 0;
					continue;
				}
				// Splice request into list of active requests, execute request,
				// and unsplice

				{ // scope
					MutexLockGuard queGuard(request_que_mutex, FB_FUNCTION);
					request->req_next = active_requests;
					active_requests = request;
					ports_active++;
				}

				// Validate port.  If it looks ok, process request

				RefMutexEnsureUnlock portQueGuard(*request->req_port->port_que_sync, FB_FUNCTION);
				{ // port_sync scope
					RefMutexGuard portGuard(*request->req_port->port_sync, FB_FUNCTION);

					if (request->req_port->port_state == rem_port::DISCONNECTED ||
						!process_packet(request->req_port, &request->req_send, &request->req_receive, &port))
					{
						port = NULL;
					}

					// With lazy port feature enabled we can have more received and
					// not handled data in receive queue. Handle it now if it contains
					// whole request packet. If it contain partial packet don't clear
					// request queue, restore receive buffer state to state before
					// reading packet and wait until rest of data arrives
					if (port)
					{
						fb_assert(port == request->req_port);

						// It is very important to not release port_que_sync before
						// port_sync, else we can miss data arrived at time between
						// releasing locks and will never handle it. Therefore we
						// can't ise MutexLockGuard here
						portQueGuard.enter();
						if (port->haveRecvData())
						{
							server_req_t* new_request = alloc_request();

							const rem_port::RecvQueState recvState = port->getRecvState();
							port->receive(&new_request->req_receive);

							if (new_request->req_receive.p_operation == op_partial)
							{
								free_request(new_request);
								port->setRecvState(recvState);
							}
							else
							{
								if (!port->haveRecvData())
									port->clearRecvQue();

								new_request->req_port = port;

#ifdef DEV_BUILD
								const bool ok =
#endif
									link_request(port, new_request);
								fb_assert(ok);
							}
						}
					}
				} // port_sync scope

				if (port) {
					portQueGuard.leave();
				}

				{ // request_que_mutex scope
					MutexLockGuard queGuard(request_que_mutex, FB_FUNCTION);

					// Take request out of list of active requests

					for (server_req_t** req_ptr = &active_requests; *req_ptr;
						req_ptr = &(*req_ptr)->req_next)
					{
						if (*req_ptr == request)
						{
							*req_ptr = request->req_next;
							ports_active--;
							break;
						}
					}

					// If this is a explicit or implicit disconnect, get rid of
					// any chained requests

					if (!port)
					{
						server_req_t* next;
						while (next = request->req_chain)
						{
							request->req_chain = next->req_chain;
							free_request(next);
						}
						if (request->req_send.p_operation == op_void &&
							request->req_receive.p_operation == op_void)
						{
							delete request;
							request = 0;
						}
					}
					else
					{
#ifdef DEBUG_REMOTE_MEMORY
						printf("thread    ACTIVE     request_queued %d\n",
								  port->port_requests_queued.value());
						fflush(stdout);
#endif
					}

					// Pick up any remaining chained request, and free current request

					if (request)
					{
						server_req_t* next = request->req_chain;
						free_request(request);
						//request = next;

						// Try to be fair - put new request at the end of waiting
						// requests queue and take request to work on from the
						// head of the queue
						if (next)
						{
							append_request_next(next, &request_que);
							request = request_que;
							request_que = request->req_next;
							ports_pending--;
						}
						else {
							request = NULL;
						}
					}
				} // request_que_mutex scope
			} // while (request)
		}
		else
		{
			worker.setState(false);
			reqQueGuard.leave();

			if (Worker::isShuttingDown())
				break;

			if (!worker.wait())
				break;
		}
	}

	} // try
	catch (const Exception& ex)
	{
		iscLogException("Error while processing the incoming packet", ex);
	}

	return 0;
}


SSHORT rem_port::asyncReceive(PACKET* asyncPacket, const UCHAR* buffer, SSHORT dataSize)
{
/**************************************
 *
 *	a s y n c R e c e i v e
 *
 **************************************
 *
 * Functional description
 *	If possible, accept incoming data asynchronously
 *
 **************************************/
	if (! port_async_receive)
	{
		return 0;
	}
	if (haveRecvData())
	{
		// We have no reliable way to distinguish network packet that start
		// from the beginning of XDR packet or in the middle of it.
		// Therefore try to process asynchronously only if there is no data
		// waiting in port queue. This can lead to fallback to synchronous
		// processing of async command (i.e. with some delay), but reliably
		// protects from spurious protocol breakage.
		return 0;
	}

	switch (xdr_peek_long(&port_async_receive->port_receive, buffer, dataSize))
	{
	case op_cancel:
	case op_abort_aux_connection:
	case op_crypt_key_callback:
		break;
	default:
		return 0;
	}

	{ // scope for guard
		static GlobalPtr<Mutex> mutex;
		MutexLockGuard guard(mutex, FB_FUNCTION);

		port_async_receive->clearRecvQue();
		port_async_receive->port_receive.x_handy = 0;
		memcpy(port_async_receive->port_queue.add().getBuffer(dataSize), buffer, dataSize);

		// It's required, that async packets follow simple rule:
		// xdr packet fits into network packet.
		port_async_receive->receive(asyncPacket);
	}

	const SSHORT asyncSize = dataSize - port_async_receive->port_receive.x_handy;
	fb_assert(asyncSize >= 0);

	switch (asyncPacket->p_operation)
	{
	case op_cancel:
		cancel_operation(this, asyncPacket->p_cancel_op.p_co_kind);
		break;
	case op_abort_aux_connection:
		if (port_async && (port_async->port_flags & PORT_connecting))
		{
			port_async->abort_aux_connection();
		}
		break;
	case op_crypt_key_callback:
		port_server_crypt_callback->wakeup(asyncPacket->p_cc.p_cc_data.cstr_length,
			asyncPacket->p_cc.p_cc_data.cstr_address);
		break;
	default:
		fb_assert(false);
		return 0;
	}

	return asyncSize;
}


ISC_STATUS rem_port::transact_request(P_TRRQ* trrq, PACKET* sendL)
{
/**************************************
 *
 *	t r a n s a c t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	LocalStatus status_vector;
	Rtr* transaction;

	getHandle(transaction, trrq->p_trrq_transaction);

	Rdb* rdb = this->port_context;
	if (bad_db(&status_vector, rdb))
		return this->send_response(sendL, 0, 0, &status_vector, false);

	UCHAR* blr = trrq->p_trrq_blr.cstr_address;
	const ULONG blr_length = trrq->p_trrq_blr.cstr_length;
	Rpr* procedure = this->port_rpr;
	UCHAR* in_msg = (procedure->rpr_in_msg) ? procedure->rpr_in_msg->msg_address : NULL;
	const ULONG in_msg_length =
		(procedure->rpr_in_format) ? procedure->rpr_in_format->fmt_length : 0;
	UCHAR* out_msg = (procedure->rpr_out_msg) ? procedure->rpr_out_msg->msg_address : NULL;
	const ULONG out_msg_length =
		(procedure->rpr_out_format) ? procedure->rpr_out_format->fmt_length : 0;

	rdb->rdb_iface->transactRequest(&status_vector, transaction->rtr_iface,
		blr_length, blr, in_msg_length, in_msg, out_msg_length, out_msg);

	if (!status_vector.isSuccess())
		return this->send_response(sendL, 0, 0, &status_vector, false);

	P_DATA* data = &sendL->p_data;
	sendL->p_operation = op_transact_response;
	data->p_data_messages = 1;
	this->send(sendL);

	return FB_SUCCESS;
}

static void zap_packet(PACKET* packet, bool new_packet)
{
/**************************************
 *
 *	z a p _ p a c k e t
 *
 **************************************
 *
 * Functional description
 *	Zero out a packet block.
 *
 **************************************/

	if (new_packet)
		memset(packet, 0, sizeof(PACKET));
	else
	{
#ifdef DEBUG_XDR_MEMORY
		// Don't trash debug xdr memory allocation table of recycled packets.

		memset(&packet->p_operation, 0, sizeof(PACKET) - OFFSET(PACKET*, p_operation));
#else
		memset(packet, 0, sizeof(PACKET));
#endif
	}
}




Worker::Worker()
{
	m_active = false;
	m_next = m_prev = NULL;
#ifdef DEV_BUILD
	m_tid = getThreadId();
#endif

	MutexLockGuard guard(m_mutex, FB_FUNCTION);
	insert(m_active);
}

Worker::~Worker()
{
	MutexLockGuard guard(m_mutex, FB_FUNCTION);
	remove();
	--m_cntAll;
}


bool Worker::wait(int timeout)
{
	if (m_sem.tryEnter(timeout))
		return true;

	MutexLockGuard guard(m_mutex, FB_FUNCTION);
	if (m_sem.tryEnter(0))
		return true;

	remove();
	return false;
}

void Worker::setState(const bool active)
{
	if (m_active == active)
		return;

	MutexLockGuard guard(m_mutex, FB_FUNCTION);
	remove();
	insert(active);
}

bool Worker::wakeUp()
{
	MutexLockGuard reqQueGuard(request_que_mutex, FB_FUNCTION);

#ifdef _DEBUG
	int cnt = 0;
	for (server_req_t* req = request_que; req; req = req->req_next)
		cnt++;
	fb_assert(cnt == ports_pending);

	cnt = 0;
	for (server_req_t* req = active_requests; req; req = req->req_next)
		cnt++;
	fb_assert(cnt == ports_active);
#endif

	if (!ports_pending)
		return true;

	MutexLockGuard guard(m_mutex, FB_FUNCTION);

	if (m_idleWorkers)
	{
		Worker* idle = m_idleWorkers;
		idle->setState(true);
		idle->m_sem.release();
		return true;
	}

	if (m_cntAll >= ports_active + ports_pending)
		return true;

	return (m_cntAll >= MAX_THREADS);
}

void Worker::wakeUpAll()
{
	MutexLockGuard guard(m_mutex, FB_FUNCTION);
	for (Worker* thd = m_idleWorkers; thd; thd = thd->m_next)
		thd->m_sem.release();
}

void Worker::remove()
{
	if (!m_active && (m_next || m_prev || m_idleWorkers == this)) {
		m_cntIdle--;
	}

	if (m_idleWorkers == this) {
		m_idleWorkers = this->m_next;
	}
	if (m_activeWorkers == this) {
		m_activeWorkers = this->m_next;
	}
	if (m_next) {
		m_next->m_prev = this->m_prev;
	}
	if (m_prev) {
		m_prev->m_next = this->m_next;
	}
	m_prev = m_next = NULL;
}

void Worker::insert(const bool active)
{
	fb_assert(!m_next);
	fb_assert(!m_prev);
	fb_assert(m_idleWorkers != this);
	fb_assert(m_activeWorkers != this);

	Worker** list = active ? &m_activeWorkers : &m_idleWorkers;
	m_next = *list;
	if (*list) {
		(*list)->m_prev = this;
	}
	*list = this;
	m_active = active;
	if (!m_active)
	{
		m_cntIdle++;
		fb_assert(m_idleWorkers == this);
	}
	else
		fb_assert(m_activeWorkers == this);
}

void Worker::start(USHORT flags)
{
	if (!isShuttingDown() && !wakeUp())
	{
		if (isShuttingDown())
			return;

		MutexLockGuard guard(m_mutex, FB_FUNCTION);
		try
		{
			Thread::start(loopThread, (void*)(IPTR) flags, THREAD_medium);
			++m_cntAll;
		}
		catch (const Exception&)
		{
			if (!m_cntAll)
			{
				Arg::Gds(isc_no_threads).raise();
			}
		}
	}
}

void Worker::shutdown()
{
	MutexLockGuard guard(m_mutex, FB_FUNCTION);
	if (shutting_down)
	{
		return;
	}

	shutting_down = true;

	while (getCount())
	{
		wakeUpAll();
		m_mutex->leave();	// we need CheckoutGuard here
		try
		{
			THREAD_SLEEP(100);
		}
		catch (const Exception&)
		{
			m_mutex->enter(FB_FUNCTION);
			throw;
		}
		m_mutex->enter(FB_FUNCTION);
	}
}

static int shut_server(const int, const int, void*)
{
	server_shutdown = true;
	return 0;
}

void SrvAuthBlock::extractDataFromPluginTo(cstring* to)
{
	to->cstr_allocated = 0;
	to->cstr_length = (ULONG) dataFromPlugin.getCount();
	to->cstr_address = dataFromPlugin.begin();
}

bool SrvAuthBlock::authCompleted(bool flag)
{
	if (flag)
		flComplete = true;
	return flComplete;
}

void SrvAuthBlock::setLogin(const Firebird::string& user)
{
	userName = user;
}

void SrvAuthBlock::load(Firebird::ClumpletReader& id)
{
	if (id.find(CNCT_login))
	{
		id.getString(userName);
		userName.upper();
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: AuthBlock: login %s\n", userName.c_str()));
	}

	if (id.find(CNCT_plugin_name))
	{
		id.getPath(pluginName);
		firstTime = false;
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: AuthBlock: plugin %s\n", pluginName.c_str()));
	}

	if (id.find(CNCT_plugin_list))
	{
		id.getPath(pluginList);
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: AuthBlock: plugin list %s\n", pluginList.c_str()));
	}

	dataForPlugin.clear();
	getMultiPartConnectParameter(dataForPlugin, id, CNCT_specific_data);
	if (dataForPlugin.hasData())
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: AuthBlock: data %" SIZEFORMAT "\n", dataForPlugin.getCount()));
	}
}

const char* SrvAuthBlock::getPluginName()
{
	return pluginName.nullStr();
}

void SrvAuthBlock::setPluginName(const Firebird::string& name)
{
	pluginName = name.ToPathName();
}

void SrvAuthBlock::setPluginList(const Firebird::string& list)
{
	if (firstTime)
	{
		pluginList = list.ToPathName();
	}
	if (pluginList.hasData())
	{
		firstTime = false;
	}
}

void SrvAuthBlock::setDataForPlugin(const Firebird::UCharBuffer& data)
{
	dataForPlugin.assign(data.begin(), data.getCount());
}

void SrvAuthBlock::setDataForPlugin(const cstring& data)
{
	dataForPlugin.assign(data.cstr_address, data.cstr_length);
}

void SrvAuthBlock::setDataForPlugin(const p_auth_continue* data)
{
	dataForPlugin.assign(data->p_data.cstr_address, data->p_data.cstr_length);
	HANDSHAKE_DEBUG(fprintf(stderr, "Srv: setDataForPlugin: %d firstTime = %d nm=%d ls=%d login='%s'\n",
						 data->p_data.cstr_length, firstTime, data->p_name.cstr_length,
						 data->p_list.cstr_length, userName.c_str()));
	if (firstTime)
	{
		pluginName.assign(data->p_name.cstr_address, data->p_name.cstr_length);
		pluginList.assign(data->p_list.cstr_address, data->p_list.cstr_length);

		firstTime = false;
	}
}

bool SrvAuthBlock::hasDataForPlugin()
{
	return dataForPlugin.hasData();
}

void SrvAuthBlock::extractDataFromPluginTo(P_AUTH_CONT* to)
{
	extractDataFromPluginTo(&to->p_data);
	extractPluginName(&to->p_name);
}

void SrvAuthBlock::extractDataFromPluginTo(P_ACPD* to)
{
	extractDataFromPluginTo(&to->p_acpt_data);
	extractPluginName(&to->p_acpt_plugin);
}

void SrvAuthBlock::extractPluginName(cstring* to)
{
	to->cstr_length = (ULONG) pluginName.length();
	to->cstr_address = (UCHAR*)(pluginName.c_str());
	to->cstr_allocated = 0;
}

void SrvAuthBlock::createPluginsItr()
{
	if (firstTime || plugins)
	{
		return;
	}

	Remote::ParsedList fromClient;
	REMOTE_parseList(fromClient, pluginList);

	Remote::ParsedList onServer;
	REMOTE_parseList(onServer, port->getPortConfig()->getPlugins(PluginType::AuthServer));

	Remote::ParsedList final;
	for (unsigned s = 0; s < onServer.getCount(); ++s)
	{
		// do not expect too long lists, therefore use double loop
		for (unsigned c = 0; c < fromClient.getCount(); ++c)
		{
			if (onServer[s] == fromClient[c])
			{
				final.push(onServer[s]);
			}
		}
	}

	if (final.getCount() == 0)
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "Srv: createPluginsItr: No matching plugins on server\n"));
		(Arg::Gds(isc_login)
#ifdef DEV_BUILD
							<< Arg::Gds(isc_random) << "No matching plugins on server"
#endif
							).raise();
	}

	// reorder to make it match first, already passed, plugin data
	for (unsigned f = 1; f < final.getCount(); ++f)
	{
		if (final[f] == pluginName)
		{
			final[f] = final[0];
			final[0] = pluginName;
			break;
		}
	}

	// special case - last plugin from the list on the server may be used to check
	// correctness of what previous plugins added to auth parameters block
	bool multiFactor = true;
	for (unsigned sp = 0; sp < final.getCount(); ++sp)
	{
		if (final[sp] == onServer[onServer.getCount() - 1])
		{
			multiFactor = false;
			break;
		}
	}
	if (multiFactor)
	{
		final.push(onServer[onServer.getCount() - 1]);
	}

	REMOTE_makeList(pluginList, final);

	RefPtr<Config> portConf(port->getPortConfig());
	plugins = new AuthServerPlugins(PluginType::AuthServer, FB_AUTH_SERVER_VERSION, upInfo,
									portConf, pluginList.c_str());
}

void SrvAuthBlock::reset()
{
	pluginName = "";
	pluginList = "";
	dataForPlugin.clear();
	dataFromPlugin.clear();
	flComplete = false;
	firstTime = true;

	authBlockWriter.reset();
	delete plugins;
	plugins = NULL;
}

bool SrvAuthBlock::extractNewKeys(CSTRING* to, bool flagPluginsList)
{
	lastExtractedKeys.reset();
	for (unsigned n = 0; n < newKeys.getCount(); ++n)
	{
		const PathName& t = newKeys[n];
		PathName plugins = knownCryptKeyTypes()[t];
		if (plugins.hasData())
		{
			lastExtractedKeys.insertPath(TAG_KEY_TYPE, t);
			lastExtractedKeys.insertPath(TAG_KEY_PLUGINS, plugins);
		}
	}

	if (flagPluginsList && dataFromPlugin.getCount() == 0)
	{
		lastExtractedKeys.insertPath(TAG_KNOWN_PLUGINS, pluginList);
	}

	to->cstr_length = (ULONG) lastExtractedKeys.getBufferLength();
	to->cstr_address = const_cast<UCHAR*>(lastExtractedKeys.getBuffer());
	to->cstr_allocated = 0;

	newKeys.clear();		// Very important - avoids sending same key type more than once

	return to->cstr_length > 0;
}
