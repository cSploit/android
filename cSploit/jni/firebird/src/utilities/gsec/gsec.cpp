/*
 *
 *	PROGRAM:	Security data base manager
 *	MODULE:		gsec.cpp
 *	DESCRIPTION:	Main line routine
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

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/msg_encode.h"
#include "../common/isc_f_proto.h"
#include "../utilities/gsec/gsec.h"
#include "../utilities/gsec/gsec_proto.h"
#include "../jrd/license.h"
#include "../jrd/constants.h"
#include "../common/security.h"
#include "../common/classes/Switches.h"
#include "../utilities/gsec/gsecswi.h"
#include "../common/classes/ClumpletWriter.h"

#include "../common/call_service.h"
#include "../common/utils_proto.h"
#include "../common/classes/MsgPrint.h"
#include "../common/StatusHolder.h"

using MsgFormat::SafeArg;
using namespace Auth;


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

//const int MAXARGS	= 20;		// max number of args allowed on command line
const int MAXSTUFF	= 1000;		// longest interactive command line

static void util_output(bool error, const SCHAR*, ...);

static bool get_line(Firebird::UtilSvc::ArgvType&, TEXT*, size_t);
static bool get_switches(Firebird::UtilSvc::ArgvType&, const Switches&, tsec*, bool*);
static SSHORT parse_cmd_line(Firebird::UtilSvc::ArgvType&, tsec*);
static void printhelp();
static void get_security_error(ISC_STATUS*, int);
static void insert_error(ISC_STATUS*, ISC_STATUS);
static void msg_get(USHORT number, TEXT* msg);


int GSEC_main(Firebird::UtilSvc* uSvc)
{
/**********************************************
 *
 *    G S E C _ m a i n
 *
 **********************************************
 * Functional Description:
 *   Entrypoint for GSEC via the services manager
 **********************************************/
	int exit_code = FINI_OK;

	try {
		exit_code = gsec(uSvc);
	}
	catch (const Firebird::Exception& e)
	{
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);
		uSvc->initStatus();
		uSvc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}

	return exit_code;
}


static void setAttr(Firebird::string& attr, ICharUserField* field)
{
	attr = field->get();
}

static void setAttr(Firebird::string& attr, IIntUserField* field)
{
	attr.printf("%d", field->get());
}

template <typename I>
static void setAttr(Firebird::string& attr, const char* name, I* field)
{
	Firebird::string s;
	if (field->entered())
	{
		setAttr(s, field);
	}
	else if (!field->specified())
	{
		return;
	}
	attr += name;
	attr += '=';
	attr += s;
	attr += '\n';
}

static bool fieldSet(IUserField* field)
{
	return field->entered() || field->specified();
}

static void merge(IIntUserField* to, IIntUserField* from)
{
	if (fieldSet(to))
		return;
	if (from->entered())
	{
		to->set(from->get());
		to->setEntered(1);
	}
}


int gsec(Firebird::UtilSvc* uSvc)
{
/**************************************
 *
 *	c o m m o n _ m a i n
 *
 **************************************
 *
 * Functional description
 *	If there is no command line, prompt for one, read it
 *	and make an artificial argv.   Otherwise, pass
 *	the specified argv to SECURITY_exec_line (see below).
 *
 **************************************/
	int exit_code = FINI_OK;
	Firebird::UtilSvc::ArgvType& argv = uSvc->argv;

	TEXT stuff[MAXSTUFF];		// a place to put stuff in interactive mode

	tsec tsecInstance(uSvc);
	tsec* tdsec = &tsecInstance;
	tsec::putSpecific(tdsec);

	StackUserData u;
	tdsec->tsec_user_data = &u;

	const unsigned char* block;
	unsigned int bs = uSvc->getAuthBlock(&block);
	if (bs)
	{
		u.authenticationBlock.add(block, bs);
	}

	try {
	// Perform some special handling when run as a Firebird service.

	tdsec->tsec_throw = true;
	tdsec->tsec_interactive = !uSvc->isService();
	UserData* user_data = tdsec->tsec_user_data;

	//if (!uSvc->isService() && argv.getCount() == 1)
	//	GSEC_error(GsecMsg101); // use gsec -? to get help

	int ret = parse_cmd_line(argv, tdsec);
	if (!uSvc->isService() && ret == -2) // user asked for help
		GSEC_exit();

	Firebird::PathName databaseName;
	const bool databaseNameEntered = user_data->database.entered();
	if (databaseNameEntered)
	{
		databaseName = user_data->database.get();
	}
	else
	{
		const Firebird::RefPtr<Config> defConf(Config::getDefaultConfig());
		databaseName = defConf->getSecurityDatabase();
	}

	const Firebird::string sqlRoleName(user_data->role.entered() ? user_data->role.get() : "");

	Firebird::PathName serverName;
	const bool useServices = !uSvc->isService();

	switch (ISC_extract_host(databaseName, serverName, true))
	{
	case ISC_PROTOCOL_TCPIP:
		serverName += ":";
		break;
	case ISC_PROTOCOL_WLAN:
		serverName = "\\\\" + serverName + "\\";
		break;
	}

	if (!useServices)
	{
		serverName = "";
	}
	user_data->database.set(databaseName.c_str());

	Firebird::RefPtr<IManagement> manager;
	ISC_STATUS_ARRAY status;

	if (!useServices)
	{
		// Get remote address info for management plugin
		Firebird::string network_protocol, remote_address;
		Firebird::ClumpletWriter tmp(Firebird::ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);
		uSvc->fillDpb(tmp);

		if (tmp.find(isc_dpb_address_path))
		{
			Firebird::ClumpletReader address_stack(Firebird::ClumpletReader::UnTagged,
												   tmp.getBytes(), tmp.getClumpLength());

			while (!address_stack.isEof())
			{
				if (address_stack.getClumpTag() != isc_dpb_address)
				{
					address_stack.moveNext();
					continue;
				}

				Firebird::ClumpletReader address(Firebird::ClumpletReader::UnTagged,
												 address_stack.getBytes(), address_stack.getClumpLength());

				while (!address.isEof())
				{
					switch (address.getClumpTag())
					{
						case isc_dpb_addr_protocol:
							address.getString(network_protocol);
							break;
						case isc_dpb_addr_endpoint:
							address.getString(remote_address);
							break;
						default:
							break;
					}

					address.moveNext();
				}

				break;
			}
		}

		// Create config to pass -DATABASE parameter value to plugin
		Firebird::string databaseText;
		databaseText.printf("SecurityDatabase = %s\n", databaseName.c_str());
		ConfigFile gsecDatabase(ConfigFile::USE_TEXT, databaseText.c_str());
		Firebird::RefPtr<Config> defaultConfig(Config::getDefaultConfig());
		Firebird::RefPtr<Config> pseudoConfig(new Config(gsecDatabase, *defaultConfig));

		uSvc->checkService();

		fb_assert(user_data->dba.entered() || user_data->authenticationBlock.hasData());
		if (user_data->dba.entered() || user_data->authenticationBlock.hasData())
		{
			class GsecInfo : public Firebird::AutoIface<ILogonInfo, FB_AUTH_LOGON_INFO_VERSION>
			{
			public:
				GsecInfo(const char* pDba, const char* pRole,
						 const char* pProtocol, const char* pAddress,
						 const UserData::AuthenticationBlock* pAuthBlock)
					: dba(pDba), sqlRole(pRole),
					  protocol(pProtocol), address(pAddress),
					  authBytes(pAuthBlock->getCount() ? pAuthBlock->begin() : NULL),
					  authLength(pAuthBlock->getCount())
				{ }

				// ILogonInfo implementation
				const char* FB_CARG name()
				{
					return dba;
				}

				const char* FB_CARG role()
				{
					return sqlRole;
				}

				const char* FB_CARG networkProtocol()
				{
					return protocol;
				}

				const char* FB_CARG remoteAddress()
				{
					return address;
				}

				unsigned int FB_CARG authBlock(const unsigned char** bytes)
				{
					*bytes = authBytes;
					return authLength;
				}

			private:
				const char* dba;
				const char* sqlRole;
				const char* protocol;
				const char* address;
				const unsigned char* authBytes;
				unsigned int authLength;
			};

			Firebird::LocalStatus st;
			try
			{
				Get getPlugin(pseudoConfig);
				manager = getPlugin.plugin();
				if (!manager)
				{
					GSEC_error_redirect((Firebird::Arg::Gds(isc_random) <<
						"Management plugin is missing or failed to load").value(), GsecMsg15);
				}

				GsecInfo info(user_data->dba.get(), user_data->role.get(),
							  network_protocol.c_str(), remote_address.c_str(), &user_data->authenticationBlock);
				manager->start(&st, &info);
			}
			catch (const Firebird::Exception& ex)
			{
				ex.stuffException(&st);
			}

			if (!st.isSuccess())
			{
				GSEC_error_redirect(st.get(), GsecMsg15);
			}
		}
	}

	isc_svc_handle sHandle = 0;
	if (useServices)
	{
		sHandle = attachRemoteServiceManager(
					status,
					user_data->dba.get(),
					user_data->dbaPassword.get(),
#ifdef TRUSTED_AUTH
					user_data->trustedAuth,
#else
					false,
#endif
					serverName.c_str(),
					!databaseNameEntered);
		if (! sHandle)
		{
			GSEC_print(GsecMsg101); // use gsec -? to get help
			GSEC_error_redirect(status, GsecMsg15);
		}
	}

	class Attributes : public ConfigFile
	{
	public:
		explicit Attributes(IUser* data)
			: ConfigFile(USE_TEXT, data->attributes()->entered() ? data->attributes()->get() : "")
		{ }

		int operator[](const char* name)
		{
			const Parameter* p = findParameter(name);
			return p ? int(p->asInteger()) : 0;
		}

		void set(IIntUserField* field, const char* name)
		{
			const Parameter* p = findParameter(name);
			if (p)
			{
				field->set(p->asInteger());
				field->setEntered(1);
			}
		}
	};

	class Display : public Firebird::AutoIface<IListUsers, FB_AUTH_LIST_USERS_VERSION>
	{
	public:
		explicit Display(tsec* t)
			: tdsec(t), first(true)
		{ }

		// IListUsers implementation
		void FB_CARG list(IUser* data)
		{
			Attributes attr(data);

			if (data->active()->entered() && data->active()->get() == 0)
			{
				// skip inactive users
				return;
			}

			if (tdsec->utilSvc->isService())
			{
				tdsec->utilSvc->putLine(isc_spb_sec_username, data->userName()->get());
				tdsec->utilSvc->putLine(isc_spb_sec_firstname, data->firstName()->entered() ? data->firstName()->get() : "");
				tdsec->utilSvc->putLine(isc_spb_sec_middlename, data->middleName()->entered() ? data->middleName()->get() : "");
				tdsec->utilSvc->putLine(isc_spb_sec_lastname, data->lastName()->entered() ? data->lastName()->get() : "");
				tdsec->utilSvc->putSLong(isc_spb_sec_userid, attr["uid"]);
				tdsec->utilSvc->putSLong(isc_spb_sec_groupid, attr["gid"]);
				if (data->operation() == DIS_OPER)
				{
					tdsec->utilSvc->putSLong(isc_spb_sec_admin, data->admin()->get());
				}
			}
			else
			{
				if (first)
				{
					GSEC_message(GsecMsg26);
					GSEC_message(GsecMsg27);
					// msg26: "    user name                    uid   gid admin     full name"
					// msg27: "-------------------------------------------------------------------------------------------------"
					first = false;
				}

				util_output(false, "%-*.*s %5d %5d %-5.5s     %s %s %s\n",
							USERNAME_LENGTH, USERNAME_LENGTH, data->userName()->get(),
							attr["uid"], attr["gid"], data->admin()->get() ? "admin" : "",
							data->firstName()->get(), data->middleName()->get(), data->lastName()->get());
			}
		}

	private:
		tsec* tdsec;
		bool first;
	};


	class Callback : public Firebird::AutoIface<IListUsers, FB_AUTH_LIST_USERS_VERSION>
	{
	public:
		explicit Callback(StackUserData* pu)
			: u(pu)
		{ }

		// IListUsers implementation
		void FB_CARG list(IUser* data)
		{
			Attributes attr(data);

			attr.set(&u->u, "uid");
			attr.set(&u->g, "gid");
		}

	private:
		StackUserData* u;
	};


	if (!tdsec->tsec_interactive)
	{
		if (ret == 0)
		{
			// Signal the start of the service here ONLY if we are displaying users
			// since the number of users may exceed the service buffer.  This
			// will cause the service to wait for the client to request data.  However,
			// if the server is not signaled, then the client can never request anything.
			if (user_data->operation() == DIS_OPER || user_data->operation() == OLD_DIS_OPER)
				uSvc->started();

			Display disp(tdsec);
			if (! useServices)
			{
				Firebird::LocalStatus st;

				if (user_data->operation() == ADD_OPER)
				{
					user_data->act.set(1);
					user_data->act.setEntered(1);
				}

				if (user_data->operation() == MOD_OPER && user_data->userName()->entered() &&
					(fieldSet(&user_data->u) || fieldSet(&user_data->g) || fieldSet(&user_data->group)))
				{
					StackUserData u;
					u.op = OLD_DIS_OPER;
					u.user.set(user_data->userName()->get());
					u.user.setEntered(1);

					Callback cb(&u);
					ret = manager->execute(&st, &u, &cb);

					if (ret)
					{
						ret = setGsecCode(ret, user_data);		// user_data, not u !
						fb_utils::copyStatus(status, FB_NELEM(status),
											 st.get(), fb_utils::statusLength(st.get()));
						GSEC_print(ret, user_data->userName()->get());
						if (status[1])
						{
							GSEC_print_status(status);
						}
						get_security_error(status, ret);
					}

					if (!st.isSuccess())
					{
						Firebird::status_exception::raise(st.get());
					}

					merge(&user_data->u, &u.u);
					merge(&user_data->g, &u.g);
				}

				Firebird::string attr;
				setAttr(attr, "uid", &user_data->u);
				setAttr(attr, "gid", &user_data->g);
				setAttr(attr, "groupName", &user_data->group);
				user_data->attributes()->set(attr.c_str());
				user_data->attributes()->setEntered(attr.hasData() ? 1 : 0);

				ret = manager->execute(&st, user_data, &disp);

				if (ret)
				{
					ret = setGsecCode(ret, user_data);
					fb_utils::copyStatus(status, FB_NELEM(status),
										 st.get(), fb_utils::statusLength(st.get()));
					GSEC_print(ret, user_data->userName()->get());
					if (status[1])
					{
						GSEC_print_status(status);
					}
					get_security_error(status, ret);
				}

				manager->commit(&st);
				if (!st.isSuccess())
				{
					Firebird::status_exception::raise(st.get());
				}
			}
			else
			{
				callRemoteServiceManager(status, sHandle, *user_data, &disp);
				if (status[1])
				{
					GSEC_print_status(status);
					ret = setGsecCode(-1, user_data);
				}
			}
		}
	}
	else
	{
		Firebird::UtilSvc::ArgvType local_argv;
		for (;;)
		{
			MOVE_CLEAR(status, sizeof(ISC_STATUS_ARRAY));
			// Clear out user data each time through this loop.
			user_data->clear();
			if (get_line(local_argv, stuff, sizeof(stuff)))
				break;
			if (local_argv.getCount() > 1)
			{
				ret = parse_cmd_line(local_argv, tdsec);
				if (ret == 1)
				{
					// quit command
					ret = 0;
					break;
				}
				if (user_data->dba.entered() ||
					user_data->dbaPassword.entered() ||
					user_data->database.entered()
#ifdef TRUSTED_AUTH
					|| user_data->trustedAuth
#endif
					)
				{
					GSEC_print(GsecMsg92);
					continue;
				}

				user_data->database.set(databaseName.c_str());
				user_data->database.setEntered(databaseNameEntered);
				user_data->role.set(sqlRoleName.c_str());
				user_data->role.setEntered(sqlRoleName.hasData());

				if (ret == 0)
				{
					Display disp(tdsec);
					callRemoteServiceManager(status, sHandle, *user_data, &disp);
					if (status[1])
					{
						GSEC_print_status(status);
					}
				}
			}
		}
	}

	if (ret && status[1])
	{
		uSvc->setServiceStatus(status);
	}

	if (sHandle)
	{
		ISC_STATUS_ARRAY status;
		detachRemoteServiceManager(status, sHandle);
		if (status[1]) {
			GSEC_print_status(status);
		}
	}

	if (!exit_code)
	{
		exit_code = ret;
	}
	}	// try
	catch (const Firebird::LongJump&)
	{
		// All error exit calls to GSEC_error() wind up here
		exit_code = tdsec->tsec_exit_code;

		tdsec->tsec_throw = false;
	}
	catch (const Firebird::Exception& e)
	{
		// Real exceptions are coming here
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);

		tdsec->tsec_throw = false;

		GSEC_print_status(status);
		uSvc->initStatus();
		uSvc->setServiceStatus(status);

		exit_code = FINI_ERROR;
	}

	tdsec->utilSvc->started();
	return exit_code;
}


static bool get_line(Firebird::UtilSvc::ArgvType& argv, TEXT* stuff, size_t maxstuff)
{
/**************************************
 *
 *	g e t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Read the current line and put its pieces into an argv
 *	structure.   Reads pieces (argv [0] is
 *	unused), and a max of MAXSTUFF characters, at which point
 *
 **************************************/
	GSEC_print_partial(GsecMsg1);
	argv.clear();
	argv.push("gsec");
	TEXT* cursor = stuff;
	int count = (int) maxstuff - 1;
	bool first = true;

	// for each input character, if it's white space (or any non-printable,
	// non-newline for that matter), ignore it; if it's a newline, we're
	// done; otherwise, put it in the current argument

	while (count > 0)
	{
		TEXT c = getc(stdin);
		if (c > ' ' && c <= '~')
		{
			// note that the first argument gets a '-' appended to the front to fool
			// the switch checker into thinking it came from the command line

			for (argv.push(cursor); count > 0; count--)
			{
				if (first)
				{
					first = false;
					if (c != '?')
					{
						*cursor++ = '-';
						count--;
					}
				}
				*cursor++ = c;
				c = getc(stdin);
				if (c <= ' ' || c > '~')
					break;
			}
			*cursor++ = '\0';
			count--;
		}
		if (c == '\n')
			break;
		if (c == EOF)
		{
			if (SYSCALL_INTERRUPTED(errno))
			{
				errno = 0;
				continue;
			}
			return true;
		}
	}

	*cursor = '\0';
	return false;
}


static bool get_switches(Firebird::UtilSvc::ArgvType& argv,
						 const Switches& switches,
						 tsec* tdsec, bool* quitflag)
{
/**************************************
 *
 *	g e t _ s w i t c h e s
 *
 **************************************
 *
 * Functional description
 *	Parse the input line arguments, saving
 *	interesting switches in a switch table.
 *
 **************************************/

	// look at each argument.   it's either a switch or a parameter.
	// parameters must always follow a switch, but not all switches
	// need parameters.   this is how, for example, parameters are
	// cleared (like a -fname switch followed by no first name
	// parameter).

	UserData* user_data = tdsec->tsec_user_data;
	*quitflag = false;
	USHORT last_sw = IN_SW_GSEC_0;
	tdsec->tsec_sw_version = false;
	for (size_t argc = 1; argc < argv.getCount(); ++argc)
	{
		const char* string = argv[argc];
		if (*string == '?' || strcmp(string, "-?") == 0)
			user_data->op = HELP_OPER;
		else if (*string != '-')
		{
			// this is not a switch, so it must be a parameter for
			// the previous switch, if any
			char quote;
			unsigned int l;
			Firebird::string uname;

			switch (last_sw)
			{
			case IN_SW_GSEC_ADD:
			case IN_SW_GSEC_DEL:
			case IN_SW_GSEC_DIS:
			case IN_SW_GSEC_DIS_ADM:
			case IN_SW_GSEC_MOD:
				quote = ' ';
				uname = "";
				l = 0;
				while (l < MAX_SQL_IDENTIFIER_SIZE && string[l] && string[l] != quote)
				{
					if (l == 0 && (*string == '\'' || *string == '"'))
					{
						quote = *string++;
						continue;
					}
					uname += string[l++];
				}
				if (l == MAX_SQL_IDENTIFIER_SIZE)
				{
					GSEC_diag(GsecMsg76);
					// invalid user name (maximum 31 bytes allowed)
					return false;
				}
				uname.upper();
				user_data->user.set(uname.c_str());
				user_data->user.setEntered(1);
				break;
			case IN_SW_GSEC_PASSWORD:
				uname = string;
				user_data->pass.set(uname.c_str());
				user_data->pass.setEntered(1);
				break;
			case IN_SW_GSEC_UID:
				user_data->u.set(atoi(string));
				user_data->u.setEntered(1);
				break;
			case IN_SW_GSEC_GID:
				user_data->g.set(atoi(string));
				user_data->g.setEntered(1);
				break;
			case IN_SW_GSEC_SYSUSER:
				// ignore it
				break;
			case IN_SW_GSEC_GROUP:
				user_data->group.set(string);
				user_data->group.setEntered(1);
				break;
			case IN_SW_GSEC_FNAME:
				user_data->first.set(string);
				user_data->first.setEntered(1);
				break;
			case IN_SW_GSEC_MNAME:
				user_data->middle.set(string);
				user_data->middle.setEntered(1);
				break;
			case IN_SW_GSEC_LNAME:
				user_data->last.set(string);
				user_data->last.setEntered(1);
				break;
			case IN_SW_GSEC_DATABASE:
				user_data->database.set(string);
				user_data->database.setEntered(1);
				break;
			case IN_SW_GSEC_DBA_USER_NAME:
				user_data->dba.set(string);
				user_data->dba.setEntered(1);
				break;
			case IN_SW_GSEC_DBA_PASSWORD:
				tdsec->utilSvc->hidePasswd(argv, argc);
				user_data->dbaPassword.set(argv[argc]);
				user_data->dbaPassword.setEntered(1);
				break;
			case IN_SW_GSEC_FETCH_PASSWORD:
				{
					const char* passwd = NULL;
					switch (fb_utils::fetchPassword(argv[argc], passwd))
					{
					case fb_utils::FETCH_PASS_OK:
						break;
					default:
						GSEC_diag(GsecMsg96);
						// error fetching password from file
						return false;
					}
					user_data->dbaPassword.set(passwd);
					user_data->dbaPassword.setEntered(1);
					break;
				}
			case IN_SW_GSEC_SQL_ROLE_NAME:
				user_data->role.set(string);
				user_data->role.setEntered(1);
				break;
			case IN_SW_GSEC_MAPPING:
				{
					Firebird::string val(string);
					val.upper();

					if (val == "SET")
					{
						user_data->op = MAP_SET_OPER;
					}
					else if (val == "DROP") {
						user_data->op = MAP_DROP_OPER;
					}
					else
					{
						GSEC_diag(GsecMsg99);
						// gsec - invalid parameter value for -MAPPING, only SET or DROP is accepted
						return false;
					}
				}
				break;
			case IN_SW_GSEC_ADMIN:
				{
					Firebird::string val(string);
					val.upper();

					if (val == "YES")
					{
						user_data->adm.set(1);
					}
					else if (val == "NO") {
						user_data->adm.set(0);
					}
					else
					{
						GSEC_diag(GsecMsg103);
						// invalid parameter for -ADMIN, only YES or NO is accepted
						return false;
					}
					user_data->adm.setEntered(1);
				}
				break;
			case IN_SW_GSEC_Z:
			case IN_SW_GSEC_0:
				GSEC_diag(GsecMsg29);
				// gsec - invalid parameter, no switch defined
				return false;
			}
			last_sw = IN_SW_GSEC_0;
		}
		else
		{
			// leave in_sw = IN_SW_GSEC_AMBIG; out for now
			const Switches::in_sw_tab_t* rc = switches.findSwitch(string);
			const USHORT in_sw = rc ? rc->in_sw : IN_SW_GSEC_0;

			// this checks to make sure that the switch is not a duplicate.   if
			// it is a duplicate, it's an error.   if it's not a duplicate, the
			// appropriate specified flag is set (to later check for duplicates),
			// and the applicable parameter value is set to its null value, in
			// case the user really wants to remove an existing parameter.

			SSHORT err_msg_no;

			switch (in_sw)
			{
			case IN_SW_GSEC_ADD:
			case IN_SW_GSEC_DEL:
			case IN_SW_GSEC_DIS:
			case IN_SW_GSEC_DIS_ADM:
			case IN_SW_GSEC_MOD:
			case IN_SW_GSEC_QUIT:
			case IN_SW_GSEC_HELP:
			case IN_SW_GSEC_MAPPING:
				if (user_data->op)
				{
					GSEC_error(GsecMsg30);
					// gsec - operation already specified
					return false;
				}
				switch (in_sw)
				{
				case IN_SW_GSEC_ADD:
					user_data->op = ADD_OPER;
					break;
				case IN_SW_GSEC_DEL:
					user_data->op = DEL_OPER;
					break;
				case IN_SW_GSEC_DIS_ADM:
					user_data->op = DIS_OPER;
					break;
				case IN_SW_GSEC_DIS:
					user_data->op = OLD_DIS_OPER;
					break;
				case IN_SW_GSEC_MOD:
					user_data->op = MOD_OPER;
					break;
				case IN_SW_GSEC_QUIT:
					user_data->op = QUIT_OPER;
					*quitflag = true;
					break;
				case IN_SW_GSEC_HELP:
					user_data->op = HELP_OPER;
					break;
				}
				user_data->user.clear();
				tdsec->tsec_interactive = false;
				break;
			case IN_SW_GSEC_PASSWORD:
			case IN_SW_GSEC_UID:
			case IN_SW_GSEC_GID:
			case IN_SW_GSEC_SYSUSER:
			case IN_SW_GSEC_GROUP:
			case IN_SW_GSEC_FNAME:
			case IN_SW_GSEC_MNAME:
			case IN_SW_GSEC_LNAME:
			case IN_SW_GSEC_DATABASE:
			case IN_SW_GSEC_DBA_USER_NAME:
			case IN_SW_GSEC_DBA_PASSWORD:
			case IN_SW_GSEC_FETCH_PASSWORD:
			case IN_SW_GSEC_SQL_ROLE_NAME:
				err_msg_no = 0;
				switch (in_sw)
				{
				case IN_SW_GSEC_PASSWORD:
					if (user_data->pass.specified())
					{
						err_msg_no = GsecMsg31;
						break;
					}
					user_data->pass.setSpecified(1);
					break;
				case IN_SW_GSEC_UID:
					if (user_data->u.specified())
					{
						err_msg_no = GsecMsg32;
						break;
					}
					user_data->u.setSpecified(1);
					break;
				case IN_SW_GSEC_GID:
					if (user_data->g.specified())
					{
						err_msg_no = GsecMsg33;
						break;
					}
					user_data->g.setSpecified(1);
					break;
				case IN_SW_GSEC_SYSUSER:
					// ignore it
					break;
				case IN_SW_GSEC_GROUP:
					if (user_data->group.specified())
					{
						err_msg_no = GsecMsg35;
						break;
					}
					user_data->group.setSpecified(1);
					break;
				case IN_SW_GSEC_FNAME:
					if (user_data->first.specified())
					{
						err_msg_no = GsecMsg36;
						break;
					}
					user_data->first.setSpecified(1);
					break;
				case IN_SW_GSEC_MNAME:
					if (user_data->middle.specified())
					{
						err_msg_no = GsecMsg37;
						break;
					}
					user_data->middle.setSpecified(1);
					break;
				case IN_SW_GSEC_LNAME:
					if (user_data->last.specified())
					{
						err_msg_no = GsecMsg38;
						break;
					}
					user_data->last.setSpecified(1);
					break;
				case IN_SW_GSEC_DATABASE:
					if (user_data->database.specified())
					{
						err_msg_no = GsecMsg78;
						break;
					}
					user_data->database.setSpecified(1);
					break;
				case IN_SW_GSEC_DBA_USER_NAME:
					if (user_data->dba.specified())
					{
						err_msg_no = GsecMsg79;
						break;
					}
					user_data->dba.setSpecified(1);
					break;
				case IN_SW_GSEC_DBA_PASSWORD:
				case IN_SW_GSEC_FETCH_PASSWORD:
					if (user_data->dbaPassword.specified())
					{
						err_msg_no = GsecMsg80;
						break;
					}
					user_data->dbaPassword.setSpecified(1);
					break;
				case IN_SW_GSEC_SQL_ROLE_NAME:
					if (user_data->role.specified())
					{
						err_msg_no = GsecMsg81;
						break;
					}
					user_data->role.setSpecified(1);
					break;
				}
				if (err_msg_no)
				{
					GSEC_error(err_msg_no);
					return false;
				}
				break;
			case IN_SW_GSEC_Z:
				if (!tdsec->tsec_sw_version)
				{
					TEXT msg[MSG_LENGTH];
					msg_get(GsecMsg39, msg);
					util_output(true, "%s %s\n", msg, FB_VERSION);
				}
				tdsec->tsec_sw_version = true;
				break;
#ifdef TRUSTED_AUTH
			case IN_SW_GSEC_TRUSTED_AUTH:
				user_data->trustedAuth = true;
				break;
#endif
			case IN_SW_GSEC_0:
				GSEC_diag(GsecMsg40);
				// gsec - invalid switch specified
				return false;
			case IN_SW_GSEC_AMBIG: // Not used for now
				GSEC_diag(GsecMsg41);
				// gsec - ambiguous switch specified
				return false;
			}
			last_sw = in_sw;
		}

		// make sure that the current suite of switches and parameters
		// is valid, and if not, indicate why not

		if (user_data->u.entered() || user_data->g.entered() ||
			user_data->group.entered() ||
			user_data->pass.entered() || user_data->first.entered() ||
			user_data->middle.entered() || user_data->last.entered())
		{
			switch (user_data->operation())
			{
			case 0:
				GSEC_error(GsecMsg42);
				// gsec - no operation specified for parameters
				return false;
			case ADD_OPER:
			case MOD_OPER:
				// any parameter is ok for these operation states
				break;
			case DEL_OPER:
			case DIS_OPER:
			case OLD_DIS_OPER:
			case QUIT_OPER:
			case HELP_OPER:
				GSEC_error(GsecMsg43);
				// gsec - no parameters allowed for this operation
				return false;
			}
		}

		if (*quitflag)
			break;
	}

	return true;
}


static void printhelp()
{
/**************************************
 *
 *	p r i n t h e l p
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	util_output(true, "\n%s", "   ");
	GSEC_print(GsecMsg45);
	// gsec utility - maintains user password database

	util_output(true, "\n%s", "   ");
	GSEC_print(GsecMsg46);
	util_output(true, "%s", "     ");
	GSEC_print_partial(GsecMsg2);
	GSEC_print_partial(GsecMsg82);
	GSEC_print(GsecMsg47);
	// gsec [ <options> ... ] -<command> [ <parameter> ... ]

	util_output(true, "\n%s", "   ");
	GSEC_print(GsecMsg48);
	// interactive usage:

	util_output(true, "%s", "     ");
	GSEC_print_partial(GsecMsg2);
	GSEC_print(GsecMsg82);
	// gsec [ <options> ... ]

	util_output(true, "%s", "     ");
	GSEC_print_partial(GsecMsg1);
	util_output(true, "\n%s", "     ");
	GSEC_print(GsecMsg47);
	// GSEC> <command> [ <parameter> ... ]

	util_output(true, "\n%s", "   ");
	GSEC_print(GsecMsg83);
	// available options:

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg84);
	// -user <database administrator name>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg85);
	// -password <database administrator password>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg95);
	// -fetch_password <fetch database administrator password from file>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg86);
	// -role <database administrator SQL role name>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg91);
	// -trusted (use trusted authentication)

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg87);
	// -database <database to manage>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg88);
	// -nz

	util_output(true, "\n%s", "   ");
	GSEC_print(GsecMsg49);
	// available commands:

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg50);
	// adding a new user:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg51);
	// add <name> [ <parameter> ... ]

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg52);
	// deleting a current user:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg53);
	// delete <name>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg54);
	// displaying all users:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg55);
	// display

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg56);
	// displaying one user:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg57);
	// display <name>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg58);
	// modifying a user's parameters:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg59);
	// modify <name> <parameter> [ <parameter> ... ]

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg98);
	// changing admins mapping to SYSDBA:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg100);
	// -ma(pping) {set|drop}

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg60);
	// help:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg61);
	// ? (interactive only)

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg62);
	// help

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg89);
	// displaying version number:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg90);
	// z (interactive only)

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg63);
	// quit interactive session:

	util_output(true, "%s", "       ");
	GSEC_print(GsecMsg64);
	// quit (interactive only)

	util_output(true, "\n%s", "   ");
	GSEC_print(GsecMsg65);
	// available parameters:

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg66);
	// -pw <password>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg67);
	// -uid <uid>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg68);
	// -gid <gid>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg71);
	// -fname <firstname>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg72);
	// -mname <middlename>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg73);
	// -lname <lastname>

	util_output(true, "%s", "     ");
	GSEC_print(GsecMsg102);
	// -adm(in) {yes|no}
	util_output(true, "\n", NULL);
}


static SSHORT parse_cmd_line(Firebird::UtilSvc::ArgvType& argv, tsec* tdsec)
{
/**************************************
 *
 *	p a r s e _ c m d _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Read the command line
 *	returns 0 on normal completion,
 *	   1 if user chooses to quit
 *	   -1 on error or if user asks for help
 *
 **************************************/
	bool quitflag = false;
	UserData* user_data = tdsec->tsec_user_data;
	user_data->clear();

	// Call a subroutine to process the input line.

	const Switches switches(gsec_in_sw_table, FB_NELEM(gsec_in_sw_table), false, true);

	SSHORT ret = 0;
	if (!get_switches(argv, switches, tdsec, &quitflag))
	{
		GSEC_diag(GsecMsg16);
		// gsec - error in switch specifications
		ret = -1;
	}
	else if (user_data->operation())
	{
		switch (user_data->operation())
		{
		case HELP_OPER:
			printhelp();
			ret = -2;
			break;
		case OLD_DIS_OPER:
		case DIS_OPER:
		case QUIT_OPER:
		case MAP_SET_OPER:
		case MAP_DROP_OPER:
			// nothing
			break;
		default:
			if (!user_data->user.entered())
			{
				GSEC_error(GsecMsg18); // gsec - no user name specified
				ret = -1;
			}
			break;
		}
	}

	if (quitflag)
		ret = 1;

	if (tdsec->tsec_sw_version)
		ret = -1;

	return ret;
}

void GSEC_print_status(const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	U T I L _ p r i n t _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Print error message. Use fb_interpret
 *	to allow redirecting output.
 *
 **************************************/
	if (status_vector)
	{
		const ISC_STATUS* vector = status_vector;
		/*tsec* tdsec = */ tsec::getSpecific();

		SCHAR s[1024];
		while (fb_interpret(s, sizeof(s), &vector))
		{
			const char* nl = (s[0] ? (s[strlen(s) - 1] != '\n') : true) ? "\n" : "";
			util_output(true, "%s%s", s, nl);
		}
	}
}

static void util_output(bool error, const SCHAR* format, ...)
{
/**************************************
 *
 *	u t i l _ o u t p u t
 *
 **************************************
 *
 * Functional description
 *	Platform independent output routine.
 *  Exit on output error
 *
 **************************************/
	va_list arglist;
	va_start(arglist, format);
	tsec* tdsec = tsec::getSpecific();
	if (!tdsec->utilSvc->isService())
	{
		Firebird::string buf;
		buf.vprintf(format, arglist);

		if (error)
		{
			tdsec->utilSvc->outputError(buf.c_str());
		}
		else
		{
			tdsec->utilSvc->outputVerbose(buf.c_str());
		}
	}
	va_end(arglist);
}

void GSEC_error_redirect(const ISC_STATUS* status_vector, USHORT errcode)
{
/**************************************
 *
 *	G S E C _ e r r o r _ r e d i r e c t
 *
 **************************************
 *
 * Functional description
 *	Issue error message. Output messages then abort.
 *
 **************************************/

	tsec* tdsec = tsec::getSpecific();
	if (!tdsec->utilSvc->isService())
	{
		GSEC_print_status(status_vector);
		GSEC_error(errcode);
	}
	else
	{
		GSEC_error(errcode, status_vector);
	}
}

void GSEC_diag(USHORT errcode)
{
/**************************************
 *
 *	 G S E C _ d i a g
 *
 **************************************
 *
 * Functional description
 *	Call error if service, print if utility.
 *
 **************************************/
	tsec* tdsec = tsec::getSpecific();
	if (tdsec->utilSvc->isService())
		GSEC_error(errcode);
	else
		GSEC_print(errcode);
}

void GSEC_error(USHORT errcode, const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	 G S E C _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Format and print an error message, then punt.
 *
 **************************************/
	static const SafeArg dummy;

	tsec* tdsec = tsec::getSpecific();
	tdsec->utilSvc->setServiceStatus(GSEC_MSG_FAC, errcode, dummy);
	if (status_vector)
	{
		tdsec->utilSvc->setServiceStatus(status_vector);
	}
	tdsec->utilSvc->started();

	GSEC_print(errcode);
	// CVC: copy the errcode to exit with a value different than 0
	tdsec->tsec_exit_code = errcode;
	if (tdsec->tsec_throw)
		Firebird::LongJump::raise();
}

//**************************
// G S E C _ e x i t
//**************************
// Exit without error
void GSEC_exit()
{
	tsec* tdsec = tsec::getSpecific();
	tdsec->utilSvc->started();
	if (tdsec->tsec_throw)
		Firebird::LongJump::raise();
}


void GSEC_print(USHORT number, const char* str)
{
/**************************************
 *
 *	G S E C _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it.
 *
 **************************************/
	TEXT buffer[256];

	SafeArg arg;
	if (str)
		arg << str;

	fb_msg_format(0, GSEC_MSG_FAC, number, sizeof(buffer), buffer, arg);
	util_output(true, "%s\n", buffer);
}


void GSEC_message(USHORT number, const char* str)
{
/**************************************
 *
 *	G S E C _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it.
 *
 **************************************/
	TEXT buffer[256];

	SafeArg arg;
	if (str)
		arg << str;

	fb_msg_format(0, GSEC_MSG_FAC, number, sizeof(buffer), buffer, arg);
	util_output(false, "%s\n", buffer);
}


void GSEC_print_partial(USHORT number)
{
/**************************************
 *
 *	G S E C _ p r i n t _ p a r t i a l
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it.
 *
 **************************************/
	static const SafeArg dummy;
	TEXT buffer[256];

	fb_msg_format(0, GSEC_MSG_FAC, number, sizeof(buffer), buffer, dummy);
	util_output(true, "%s ", buffer);
}


static void msg_get(USHORT number, TEXT* msg)
{
/**************************************
 *
 *	m s g _ g e t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file
 *
 **************************************/

	static const SafeArg dummy;
	fb_msg_format(NULL, GSEC_MSG_FAC, number, MSG_LENGTH, msg, dummy);
}


static void insert_error(ISC_STATUS* status, ISC_STATUS isc_err)
{
/**************************************
 *
 *      i n s e r t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *
 *	Adds isc error code to status vector
 **************************************/
	if (status[1])
	{
		memmove(&status[2], &status[0], sizeof(ISC_STATUS_ARRAY) - 2 * sizeof(ISC_STATUS));
	}
	else
	{
		status[2] = isc_arg_end;
	}
	status[0] = isc_arg_gds;
	status[1] = isc_err;
}


static void get_security_error(ISC_STATUS* status, int gsec_err)
{
/**************************************
 *
 *      g e t _ s e c u r i t y _ e r r o r
 *
 **************************************
 *
 * Functional description
 *
 *    Converts the gsec error code to an isc
 *    error code and adds it to the status vector
 **************************************/


	switch (gsec_err)
	{
	case GsecMsg19:			// gsec - add record error
		insert_error(status, isc_error_adding_sec_record);
		return;

	case GsecMsg20:			// gsec - modify record error
		insert_error(status, isc_error_modifying_sec_record);
		return;

	case GsecMsg21:			// gsec - find/modify record error
		insert_error(status, isc_error_modifying_sec_record);
		return;

	case GsecMsg22:			// gsec - record not found for user:
		insert_error(status, isc_usrname_not_found);
		return;

	case GsecMsg23:			// gsec - delete record error
		insert_error(status, isc_error_deleting_sec_record);
		return;

	case GsecMsg24:			// gsec - find/delete record error
		insert_error(status, isc_error_deleting_sec_record);
		return;

	case GsecMsg75:			// gsec error
		insert_error(status, isc_error_updating_sec_db);
		return;

	default:
		return;
	}
}
