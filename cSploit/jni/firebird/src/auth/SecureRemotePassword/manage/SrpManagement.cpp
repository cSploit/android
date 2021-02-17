/*
 *	PROGRAM:		Firebird authentication
 *	MODULE:			SrpManagement.cpp
 *	DESCRIPTION:	Manages security database with SRP
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

#include "../common/classes/ImplementHelper.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/StatusHolder.h"
#include "firebird/Auth.h"
#include "../auth/SecureRemotePassword/srp.h"
#include "../jrd/constants.h"
#include "../utilities/gsec/gsec.h"
#include "../auth/SecureRemotePassword/Message.h"
#include "../common/classes/auto.h"

namespace {

Firebird::MakeUpgradeInfo<> upInfo;
const unsigned int INIT_KEY = ((~0) - 1);
unsigned int secDbKey = INIT_KEY;

const unsigned int SZ_LOGIN = 31;
const unsigned int SZ_NAME = 31;
typedef Field<Varying> Varfield;
typedef Field<Text> Name;
typedef Field<ISC_QUAD> Blob;
typedef Field<FB_BOOLEAN> Boolean;

} // anonymous namespace

namespace Auth {

class SrpManagement FB_FINAL : public Firebird::StdPlugin<IManagement, FB_AUTH_MANAGE_VERSION>
{
public:
	explicit SrpManagement(Firebird::IPluginConfig* par)
		: config(Firebird::REF_NO_INCR, par->getFirebirdConf()), upCount(0), delCount(0)
	{ }

	void prepareDataStructures()
	{
		const char* script[] = {
			"CREATE TABLE PLG$SRP (PLG$USER_NAME SEC$USER_NAME NOT NULL PRIMARY KEY, "
			"PLG$VERIFIER VARCHAR(128) CHARACTER SET OCTETS NOT NULL, "
			"PLG$SALT VARCHAR(32) CHARACTER SET OCTETS NOT NULL, "
			"PLG$COMMENT RDB$DESCRIPTION, PLG$FIRST SEC$NAME_PART, "
			"PLG$MIDDLE SEC$NAME_PART, PLG$LAST SEC$NAME_PART, "
			"PLG$ATTRIBUTES RDB$DESCRIPTION, "
			"PLG$ACTIVE BOOLEAN)"
			,
			"CREATE VIEW PLG$SRP_VIEW AS "
			"SELECT PLG$USER_NAME, PLG$VERIFIER, PLG$SALT, PLG$COMMENT, "
			"   PLG$FIRST, PLG$MIDDLE, PLG$LAST, PLG$ATTRIBUTES, PLG$ACTIVE "
			"FROM PLG$SRP WHERE CURRENT_USER = 'SYSDBA' "
			"   OR CURRENT_ROLE = '" ADMIN_ROLE "' OR CURRENT_USER = PLG$SRP.PLG$USER_NAME"
			,
			"GRANT ALL ON PLG$SRP to VIEW PLG$SRP_VIEW"
			,
			"GRANT SELECT ON PLG$SRP_VIEW to PUBLIC"
			,
			"GRANT UPDATE(PLG$VERIFIER, PLG$SALT, PLG$FIRST, PLG$MIDDLE, PLG$LAST, "
			"   PLG$COMMENT, PLG$ATTRIBUTES) ON PLG$SRP_VIEW TO PUBLIC"
			,
			NULL
		};

		Firebird::LocalStatus s;
		Firebird::ITransaction* ddlTran(att->startTransaction(&s, 0, NULL));

		try
		{
			for (const char** sql = script; *sql; ++sql)
			{
				att->execute(&s, ddlTran, 0, *sql, SQL_DIALECT_V6, NULL, NULL, NULL, NULL);
				check(&s);
			}

			ddlTran->commit(&s);
			check(&s);
		}
		catch (const Firebird::Exception&)
		{
			if (ddlTran)
			{
				ddlTran->rollback(&s);
			}
			throw;
		}
	}

	// IManagement implementation
	void FB_CARG start(Firebird::IStatus* status, ILogonInfo* logonInfo)
	{
		try
		{
			Firebird::MasterInterfacePtr()->upgradeInterface(logonInfo, FB_AUTH_LOGON_INFO_VERSION, upInfo);

			status->init();

			if (att)
			{
				(Firebird::Arg::Gds(isc_random) << "Database is already attached in SRP").raise();
			}

			if (secDbKey == INIT_KEY)
			{
				secDbKey = config->getKey("SecurityDatabase");
			}
			const char* secDbName = config->asString(secDbKey);

			if (!(secDbName && secDbName[0]))
			{
				Firebird::Arg::Gds(isc_secdb_name).raise();
			}

			Firebird::ClumpletWriter dpb(Firebird::ClumpletReader::dpbList, MAX_DPB_SIZE);
			dpb.insertByte(isc_dpb_sec_attach, TRUE);

			const unsigned char* authBlock;
			unsigned int authBlockSize = logonInfo->authBlock(&authBlock);

			if (authBlockSize)
			{
#if SRP_DEBUG > 0
				fprintf(stderr, "SrpManagement: Using authBlock size %d\n", authBlockSize);
#endif
				dpb.insertBytes(isc_dpb_auth_block, authBlock, authBlockSize);
			}
			else
			{
				const char* str = logonInfo->name();
#if SRP_DEBUG > 0
				fprintf(stderr, "SrpManagement: Using name '%s'\n", str);
#endif
				if (str && str[0])
					dpb.insertString(isc_dpb_trusted_auth, str, strlen(str));

				str = logonInfo->role();
				if (str && str[0])
					dpb.insertString(isc_dpb_sql_role_name, str, strlen(str));
			}

			Firebird::DispatcherPtr p;
			att = p->attachDatabase(status, secDbName, dpb.getBufferLength(), dpb.getBuffer());
			check(status);

			tra = att->startTransaction(status, 0, NULL);
			check(status);
		}
		catch (const Firebird::Exception& ex)
		{
			ex.stuffException(status);

			if (att)
			{
				// detach from database
				Firebird::LocalStatus lStatus;
				att->detach(&lStatus);
				att = NULL;
			}
		}
	}

	int FB_CARG execute(Firebird::IStatus* status, IUser* user, IListUsers* callback)
	{
		try
		{
			if (callback)
			{
				Firebird::MasterInterfacePtr()->upgradeInterface(callback, FB_AUTH_LIST_USERS_VERSION, upInfo);
			}
			Firebird::MasterInterfacePtr()->upgradeInterface(user, FB_AUTH_USER_VERSION, upInfo);

			status->init();

			fb_assert(att);
			fb_assert(tra);

			switch(user->operation())
			{
			case MAP_DROP_OPER:
			case MAP_SET_OPER:
				{
					Firebird::string sql;
					sql.printf("ALTER ROLE " ADMIN_ROLE " %s AUTO ADMIN MAPPING",
						user->operation() == MAP_SET_OPER ? "SET" : "DROP");
					att->execute(status, tra, sql.length(), sql.c_str(), SQL_DIALECT_V6, NULL, NULL, NULL, NULL);
					check(status);
				}
				break;

			case ADD_OPER:
				{
					const char* insert =
						"INSERT INTO plg$srp_view(PLG$USER_NAME, PLG$VERIFIER, PLG$SALT, PLG$FIRST, PLG$MIDDLE, PLG$LAST,"
						"PLG$COMMENT, PLG$ATTRIBUTES, PLG$ACTIVE) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)";

					Firebird::IStatement* stmt = NULL;
					try
					{
						for (unsigned repeat = 0; ; ++repeat)
						{
							stmt = att->prepare(status, tra, 0, insert, SQL_DIALECT_V6, Firebird::IStatement::PREPARE_PREFETCH_METADATA);
							if (status->isSuccess())
							{
								break;
							}
							else if (repeat > 0)
							{
								Firebird::status_exception::raise(status->get());
							}

							if (fb_utils::containsErrorCode(status->get(), isc_dsql_relation_err))
							{
								prepareDataStructures();
								tra->commit(status);
								check(status);
								tra = att->startTransaction(status, 0, NULL);
								check(status);
							}
						}

						fb_assert(stmt);

						Meta im(stmt, false);
						Message add(im);
						Name login(add);
						Varfield verifier(add), slt(add);
						Name first(add), middle(add), last(add);
						Blob comment(add), attr(add);
						Boolean active(add);

						setField(login, user->userName());
						setField(first, user->firstName());
						setField(middle, user->middleName());
						setField(last, user->lastName());
						setField(status, comment, user->comment());
						setField(status, attr, user->attributes());
						setField(active, user->active());

#if SRP_DEBUG > 1
						Firebird::BigInteger salt("02E268803000000079A478A700000002D1A6979000000026E1601C000000054F");
#else
						Firebird::BigInteger salt;
						salt.random(RemotePassword::SRP_SALT_SIZE);
#endif
						Firebird::UCharBuffer s;
						salt.getBytes(s);
						slt.set(s.getCount(), s.begin());

						dumpIt("salt", s);
#if SRP_DEBUG > 0
						fprintf(stderr, ">%s< >%s<\n", user->userName()->get(), user->password()->get());
#endif
						Firebird::string s1;
						salt.getText(s1);
						server.computeVerifier(user->userName()->get(), s1, user->password()->get()).getBytes(s);
						dumpIt("verifier", s);
						verifier.set(s.getCount(), s.begin());

						stmt->execute(status, tra, add.getMetadata(), add.getBuffer(), NULL, NULL);
						check(status);

						stmt->free(status);
						check(status);
					}
					catch (const Firebird::Exception&)
					{
						if (stmt)
						{
							stmt->release();
						}
						throw;
					}
				}
				break;

			case MOD_OPER:
				{
					Firebird::string update = "UPDATE plg$srp_view SET ";

					Firebird::AutoPtr<Varfield> verifier, slt;
					if (user->password()->entered())
					{
						update += "PLG$VERIFIER=?,PLG$SALT=?,";
					}

					Firebird::AutoPtr<Name> first, middle, last;
					Firebird::AutoPtr<Blob> comment, attr;
					Firebird::AutoPtr<Boolean> active;
					allocField(user->firstName(), update, "PLG$FIRST");
					allocField(user->middleName(), update, "PLG$MIDDLE");
					allocField(user->lastName(), update, "PLG$LAST");
					allocField(user->comment(), update, "PLG$COMMENT");
					allocField(user->attributes(), update, "PLG$ATTRIBUTES");
					allocField(user->active(), update, "PLG$ACTIVE");

					if (update[update.length() - 1] != ',')
					{
						return 0;
					}
					update.rtrim(",");
					update += " WHERE PLG$USER_NAME=?";

					Firebird::IStatement* stmt = NULL;
					try
					{
						stmt = att->prepare(status, tra, 0, update.c_str(), SQL_DIALECT_V6, Firebird::IStatement::PREPARE_PREFETCH_METADATA);
						check(status);

						Meta im(stmt, false);
						Message up(im);

						if (user->password()->entered())
						{
							verifier = new Varfield(up);
							slt = new Varfield(up);
#if SRP_DEBUG > 1
							Firebird::BigInteger salt("02E268803000000079A478A700000002D1A6979000000026E1601C000000054F");
#else
							Firebird::BigInteger salt;
							salt.random(RemotePassword::SRP_SALT_SIZE);
#endif
							Firebird::UCharBuffer s;
							salt.getBytes(s);
							slt->set(s.getCount(), s.begin());

							dumpIt("salt", s);
#if SRP_DEBUG > 0
							fprintf(stderr, ">%s< >%s<\n", user->userName()->get(), user->password()->get());
#endif
							Firebird::string s1;
							salt.getText(s1);
							server.computeVerifier(user->userName()->get(), s1, user->password()->get()).getBytes(s);
							dumpIt("verifier", s);
							verifier->set(s.getCount(), s.begin());
						}

						allocField(first, up, user->firstName());
						allocField(middle, up, user->middleName());
						allocField(last, up, user->lastName());
						allocField(comment, up, user->comment());
						allocField(attr, up, user->attributes());
						allocField(active, up, user->active());

						Name login(up);

						assignField(first, user->firstName());
						assignField(middle, user->middleName());
						assignField(last, user->lastName());
						assignField(status, comment, user->comment());
						assignField(status, attr, user->attributes());
						assignField(active, user->active());
						setField(login, user->userName());

						stmt->execute(status, tra, up.getMetadata(), up.getBuffer(), NULL, NULL);
						check(status);

						if (!checkCount(status, &upCount, isc_info_update_count))
						{
							stmt->release();
							return GsecMsg22;
						}

						stmt->free(status);
						check(status);
					}
					catch (const Firebird::Exception&)
					{
						if (stmt)
						{
							stmt->release();
						}
						throw;
					}
				}
				break;

			case DEL_OPER:
				{
					const char* del = "DELETE FROM plg$srp_view WHERE PLG$USER_NAME=?";
					Firebird::IStatement* stmt = NULL;
					try
					{
						stmt = att->prepare(status, tra, 0, del, SQL_DIALECT_V6, Firebird::IStatement::PREPARE_PREFETCH_METADATA);
						check(status);

						Meta im(stmt, false);
						Message dl(im);
						Name login(dl);
						setField(login, user->userName());

						stmt->execute(status, tra, dl.getMetadata(), dl.getBuffer(), NULL, NULL);
						check(status);

						if (!checkCount(status, &delCount, isc_info_delete_count))
						{
							stmt->release();
							return GsecMsg22;
						}

						stmt->free(status);
						check(status);
					}
					catch (const Firebird::Exception&)
					{
						if (stmt)
						{
							stmt->release();
						}
						throw;
					}
				}
				break;

			case OLD_DIS_OPER:
			case DIS_OPER:
				{
					Firebird::string disp =	"SELECT PLG$USER_NAME, PLG$FIRST, PLG$MIDDLE, PLG$LAST, PLG$COMMENT, PLG$ATTRIBUTES, "
											"	CASE WHEN RDB$RELATION_NAME IS NULL THEN 0 ELSE 1 END, PLG$ACTIVE "
											"FROM PLG$SRP_VIEW LEFT JOIN RDB$USER_PRIVILEGES "
											"	ON PLG$SRP_VIEW.PLG$USER_NAME = RDB$USER_PRIVILEGES.RDB$USER "
											"		AND RDB$RELATION_NAME = '" ADMIN_ROLE "' "
											"		AND RDB$PRIVILEGE = 'M' ";
					if (user->userName()->entered())
					{
						disp += " WHERE PLG$USER_NAME = ?";
					}

					Firebird::IStatement* stmt = NULL;
					Firebird::IResultSet* rs = NULL;
					try
					{
						stmt = att->prepare(status, tra, 0, disp.c_str(), SQL_DIALECT_V6,
							Firebird::IStatement::PREPARE_PREFETCH_METADATA);
						check(status);

						Meta om(stmt, true);
						Message di(om);
						Name login(di);
						Name first(di), middle(di), last(di);
						Blob comment(di), attr(di);
						Field<SLONG> admin(di);
						Boolean active(di);

						Firebird::AutoPtr<Message> par;
						if (user->userName()->entered())
						{
							Meta im(stmt, false);
							par = new Message(im);
							Name login(*par);
							setField(login, user->userName());
						}

						rs = stmt->openCursor(status, tra, (par ? par->getMetadata() : NULL),
							(par ? par->getBuffer() : NULL), om);
						check(status);

						while (rs->fetchNext(status, di.getBuffer()))
						{
							check(status);

							listField(user->userName(), login);
							listField(user->firstName(), first);
							listField(user->middleName(), middle);
							listField(user->lastName(), last);
							listField(status, user->comment(), comment);
							listField(status, user->attributes(), attr);
							listField(user->active(), active);
							user->admin()->set(admin);

							callback->list(user);
						}
						check(status);

						rs->close(status);
						check(status);

						stmt->free(status);
						check(status);
					}
					catch (const Firebird::Exception&)
					{
						if (stmt)
						{
							stmt->release();
						}
						throw;
					}
				}
				break;

			default:
				return -1;
			}
		}
		catch (const Firebird::Exception& ex)
		{
			ex.stuffException(status);
			return -1;
		}

		return 0;
	}

	void FB_CARG commit(Firebird::IStatus* status)
	{
		if (tra)
		{
			tra->commit(status);
			if (status->isSuccess())
			{
				tra = NULL;
			}
		}
	}

	void FB_CARG rollback(Firebird::IStatus* status)
	{
		if (tra)
		{
			tra->rollback(status);
			if (status->isSuccess())
			{
				tra = NULL;
			}
		}
	}

	int FB_CARG release()
	{
		if (--refCounter == 0)
		{
			Firebird::LocalStatus status;
			rollback(&status);
			if (att)
			{
				att->detach(&status);
				if (status.isSuccess())
				{
					att = NULL;
				}
			}

			if (tra)
			{
				tra->release();
			}

			if (att)
			{
				att->release();
			}

			delete this;
			return 0;
		}

		return 1;
	}

private:
	Firebird::RefPtr<Firebird::IFirebirdConf> config;
	Firebird::RefPtr<Firebird::IAttachment> att;
	Firebird::RefPtr<Firebird::ITransaction> tra;
	RemotePassword server;
	int upCount, delCount;

	bool checkCount(Firebird::IStatus* status, int* count, UCHAR item)
	{
		unsigned char buffer[100];
		att->getInfo(status, 1, &item, sizeof(buffer), buffer);
		check(status);

		if (gds__vax_integer(buffer + 1, 2) != 6)
		{
			return false;
		}

		int newCount = gds__vax_integer(buffer + 5, 4);
		int oldCount = *count;
		*count = newCount;
		return newCount == oldCount + 1;
	}

	static void check(Firebird::IStatus* status)
	{
		if (!status->isSuccess())
		{
			checkStatusVectorForMissingTable(status->get());
			Firebird::status_exception::raise(status->get());
		}
	}

	static void setField(Name& to, Auth::ICharUserField* from)
	{
		if (from->entered())
		{
			to = from->get();
		}
		else
		{
			to.null = FB_TRUE;
		}
	}

	static void setField(Boolean& to, Auth::IIntUserField* from)
	{
		if (from->entered())
		{
			to = from->get() ? FB_TRUE : FB_FALSE;
		}
		else
		{
			to.null = FB_TRUE;
		}
	}

	void setField(Firebird::IStatus* st, Blob& to, Auth::ICharUserField* from)
	{
		if (from->entered())
		{
			blobWrite(st, to, from);
		}
		else
		{
			to.null = FB_TRUE;
		}
	}

	static void allocField(Auth::IUserField* value, Firebird::string& update, const char* name)
	{
		if (value->entered() || value->specified())
		{
			update += ' ';
			update += name;
			update += "=?,";
		}
	}

	template <typename FT>
	static void allocField(Firebird::AutoPtr<FT>& field, Message& up, Auth::IUserField* value)
	{
		if (value->entered() || value->specified())
		{
			field = new FT(up);
		}
	}

	static void assignField(Firebird::AutoPtr<Name>& field, Auth::ICharUserField* name)
	{
		if (field.hasData())
		{
			if (name->entered())
			{
				*field = name->get();
			}
			else
			{
				fb_assert(name->specified());
				field->null = FB_TRUE;
			}
		}
	}

	static void assignField(Firebird::AutoPtr<Boolean>& field, Auth::IIntUserField* name)
	{
		if (field.hasData())
		{
			if (name->entered())
			{
				*field = name->get() ? FB_TRUE : FB_FALSE;
			}
			else
			{
				fb_assert(name->specified());
				field->null = FB_TRUE;
			}
		}
	}

	void assignField(Firebird::IStatus* st, Firebird::AutoPtr<Blob>& field, Auth::ICharUserField* name)
	{
		if (field.hasData())
		{
			if (name->entered())
			{
				blobWrite(st, *field, name);
				field->null = FB_FALSE;
			}
			else
			{
				fb_assert(name->specified());
				field->null = FB_TRUE;
			}
		}
	}

	static void listField(Auth::ICharUserField* to, Name& from)
	{
		to->setEntered(from.null ? 0 : 1);
		if (!from.null)
		{
			to->set(from);
		}
	}

	static void listField(Auth::IIntUserField* to, Boolean& from)
	{
		to->setEntered(from.null ? 0 : 1);
		if (!from.null)
		{
			to->set(from);
		}
	}

	void listField(Firebird::IStatus* st, Auth::ICharUserField* to, Blob& from)
	{
		to->setEntered(from.null ? 0 : 1);
		if (!from.null)
		{
			Firebird::string s;
			Firebird::IBlob* blob = NULL;
			try
			{
				blob = att->openBlob(st, tra, &from);
				check(st);

				char segbuf[256];
				unsigned len;
				while ( (len = blob->getSegment(st, sizeof(segbuf), segbuf)) )
				{
					if (st->get()[1] != isc_segment)
						check(st);
					s.append(segbuf, len);
				}
				if (st->get()[1] != isc_segstr_eof)
					check(st);

				blob->close(st);
				check(st);

				to->set(s.c_str());
			}
			catch (const Firebird::Exception&)
			{
				if (blob)
					blob->release();
				throw;
			}
		}
	}

	void blobWrite(Firebird::IStatus* st, Blob& to, Auth::ICharUserField* from)
	{
		to.null = FB_FALSE;
		const char* ptr = from->get();
		unsigned l = strlen(ptr);

		Firebird::IBlob* blob = NULL;
		try
		{
			blob = att->createBlob(st, tra, &to);
			check(st);

			blob->putSegment(st, l, ptr);
			check(st);

			blob->close(st);
			check(st);
		}
		catch (const Firebird::Exception&)
		{
			if (blob)
				blob->release();
			throw;
		}
	}
};

// register plugin
static Firebird::SimpleFactory<Auth::SrpManagement> factory;

} // namespace Auth

extern "C" void FB_PLUGIN_ENTRY_POINT(Firebird::IMaster* master)
{
	Firebird::CachedMasterInterface::set(master);
	Firebird::PluginManagerInterfacePtr()->registerPluginFactory(Firebird::PluginType::AuthUserManagement, Auth::RemotePassword::plugName, &Auth::factory);
	Firebird::myModule->registerMe();
}
