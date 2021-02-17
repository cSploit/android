/*
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
 *  Copyright (c) 2008 Alex Peshkov <alexpeshkoff@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../common/classes/ClumpletWriter.h"
#include "../jrd/UserManagement.h"
#include "../jrd/jrd.h"
#include "../jrd/tra.h"
#include "../jrd/msg_encode.h"
#include "../utilities/gsec/gsec.h"
#include "../common/security.h"
#include "../jrd/met_proto.h"
#include "../jrd/ini.h"
#include "gen/ids.h"

using namespace Jrd;
using namespace Firebird;

static MakeUpgradeInfo<> upInfo;

const Format* UsersTableScan::getFormat(thread_db* tdbb, jrd_rel* relation) const
{
	jrd_tra* const transaction = tdbb->getTransaction();
	return transaction->getUserManagement()->getList(tdbb, relation)->getFormat();
}


bool UsersTableScan::retrieveRecord(thread_db* tdbb, jrd_rel* relation,
									FB_UINT64 position, Record* record) const
{
	jrd_tra* const transaction = tdbb->getTransaction();
	return transaction->getUserManagement()->getList(tdbb, relation)->fetch(position, record);
}


UserManagement::UserManagement(jrd_tra* tra)
	: DataDump(*tra->tra_pool),
	  threadDbb(NULL),
	  commands(*tra->tra_pool),
	  manager(NULL)
{
	Attachment* att = tra->tra_attachment;
	if (!att || !att->att_user)
	{
		(Arg::Gds(isc_random) << "Unknown user name for given transaction").raise();
	}

	fb_assert(att->att_database && att->att_database->dbb_config.hasData());
	Auth::Get getPlugin(att->att_database->dbb_config);
	manager = getPlugin.plugin();
	fb_assert(manager);
	manager->addRef();

	class UserIdInfo : public AutoIface<Auth::ILogonInfo, FB_AUTH_LOGON_INFO_VERSION>
	{
	public:
		explicit UserIdInfo(const Attachment* pAtt)
			: att(pAtt)
		{ }

		// ILogonInfo implementation
		const char* FB_CARG name()
		{
			return att->att_user->usr_user_name.c_str();
		}

		const char* FB_CARG role()
		{
			return att->att_user->usr_sql_role_name.c_str();
		}

		const char* FB_CARG networkProtocol()
		{
			return att->att_network_protocol.c_str();
		}

		const char* FB_CARG remoteAddress()
		{
			return att->att_remote_address.c_str();
		}

		unsigned int FB_CARG authBlock(const unsigned char** bytes)
		{
			const Auth::UserData::AuthenticationBlock& aBlock = att->att_user->usr_auth_block;
			*bytes = aBlock.getCount() ? aBlock.begin() : NULL;
			return aBlock.getCount();
		}

	private:
		const Attachment* att;
	};

	LocalStatus status;
	UserIdInfo idInfo(att);
	manager->start(&status, &idInfo);

	if (!status.isSuccess())
	{
		status_exception::raise(status.get());
	}
}

UserManagement::~UserManagement()
{
	for (ULONG i = 0; i < commands.getCount(); ++i)
	{
		delete commands[i];
	}
	commands.clear();

	if (manager)
	{
		LocalStatus status;
		manager->rollback(&status);
		PluginManagerInterfacePtr()->releasePlugin(manager);
		manager = NULL;

		if (!status.isSuccess())
		{
			status_exception::raise(status.get());
		}
	}
}

void UserManagement::commit()
{
	if (manager)
	{
		LocalStatus status;
		manager->commit(&status);

		if (!status.isSuccess())
		{
			status_exception::raise(status.get());
		}

		PluginManagerInterfacePtr()->releasePlugin(manager);
		manager = NULL;
	}
}

USHORT UserManagement::put(Auth::DynamicUserData* userData)
{
	const size_t ret = commands.getCount();
	if (ret > MAX_USHORT)
	{
		status_exception::raise(Arg::Gds(isc_random) << "Too many user management DDL per transaction)");
	}
	commands.push(userData);
	return ret;
}

void UserManagement::checkSecurityResult(int errcode, Firebird::IStatus* status, const char* userName, Auth::IUser* user)
{
	if (!errcode)
	{
	    return;
	}
	errcode = Auth::setGsecCode(errcode, user);

	Arg::StatusVector tmp;
	tmp << Arg::Gds(ENCODE_ISC_MSG(errcode, GSEC_MSG_FAC));
	if (errcode == GsecMsg22)
	{
		tmp << userName;
	}
	tmp.append(Arg::StatusVector(status->get()));

	tmp.raise();
}

static inline void merge(string& s, ConfigFile::Parameters::const_iterator& p)
{
	if (p->value.hasData())
	{
		string attr;
		attr.printf("%s=%s\n", p->name.c_str(), p->value.c_str());
		s += attr;
	}
}

void UserManagement::execute(USHORT id)
{
	if (id >= commands.getCount())
	{
		status_exception::raise(Arg::Gds(isc_random) << "Wrong job id passed to UserManagement::execute()");
	}

	if (!(manager && commands[id]))
	{
		// Already executed.
		return;
	}

	LocalStatus status;
	Auth::UserData* command = commands[id];
	if (command->attr.entered() || command->op == Auth::ADDMOD_OPER)
	{
		Auth::StackUserData cmd;
		cmd.op = Auth::DIS_OPER;
		cmd.user.set(command->userName()->get());
		cmd.user.setEntered(1);

		class OldAttributes : public Firebird::AutoIface<Auth::IListUsers, FB_AUTH_LIST_USERS_VERSION>
		{
		public:
			OldAttributes()
				: present(false)
			{ }

			// IListUsers implementation
			void FB_CARG list(Auth::IUser* data)
			{
				value = data->attributes()->entered() ? data->attributes()->get() : "";
				present = true;
			}

			string value;
			bool present;
		};

		OldAttributes oldAttributes;
		int ret = manager->execute(&status, &cmd, &oldAttributes);
		checkSecurityResult(ret, &status, command->userName()->get(), command);

		if (command->op == Auth::ADDMOD_OPER)
		{
			command->op = oldAttributes.present ? Auth::MOD_OPER : Auth::ADD_OPER;
		}

		if (command->attr.entered())
		{
			ConfigFile ocf(ConfigFile::USE_TEXT, oldAttributes.value.c_str());
			ConfigFile::Parameters::const_iterator old(ocf.getParameters().begin());
			ConfigFile::Parameters::const_iterator oldEnd(ocf.getParameters().end());

			ConfigFile ccf(ConfigFile::USE_TEXT, command->attr.get());
			ConfigFile::Parameters::const_iterator cur(ccf.getParameters().begin());
			ConfigFile::Parameters::const_iterator curEnd(ccf.getParameters().end());

			string merged;
			while (old != oldEnd && cur != curEnd)
			{
				if (old->name == cur->name)
				{
					merge(merged, cur);
					++old;
					++cur;
				}
				else if (old->name < cur->name)
				{
					merge(merged, old);
					++old;
				}
				else
				{
					merge(merged, cur);
					++cur;
				}
			}

			while (cur != curEnd)
			{
				merge(merged, cur);
				++cur;
			}

			while (old != oldEnd)
			{
				merge(merged, old);
				++old;
			}

			if (merged.hasData())
			{
				command->attr.set(merged.c_str());
			}
			else
			{
				command->attr.setEntered(0);
				command->attr.setSpecified(1);
				command->attr.set("");
			}
		}
	}

	if (command->op == Auth::ADD_OPER)
	{
		if (!command->act.entered())
		{
			command->act.set(1);
			command->act.setEntered(1);
		}
	}

	int errcode = manager->execute(&status, command, NULL);
	checkSecurityResult(errcode, &status, command->userName()->get(), command);

	delete commands[id];
	commands[id] = NULL;
}

void UserManagement::list(Auth::IUser* u)
{
	RecordBuffer* buffer = getData(rel_sec_users);
	Record* record = buffer->getTempRecord();
	record->nullify();

	int charset = CS_METADATA;

	if (u->userName()->entered())
	{
		putField(threadDbb, record,
				 DumpField(f_sec_user_name, VALUE_STRING, strlen(u->userName()->get()), u->userName()->get()),
				 charset);
	}

	if (u->firstName()->entered())
	{
		putField(threadDbb, record,
				 DumpField(f_sec_first_name, VALUE_STRING, strlen(u->firstName()->get()), u->firstName()->get()),
				 charset);
	}

	if (u->middleName()->entered())
	{
		putField(threadDbb, record,
				 DumpField(f_sec_middle_name, VALUE_STRING, strlen(u->middleName()->get()), u->middleName()->get()),
				 charset);
	}

	if (u->lastName()->entered())
	{
		putField(threadDbb, record,
				 DumpField(f_sec_last_name, VALUE_STRING, strlen(u->lastName()->get()), u->lastName()->get()),
				 charset);
	}

	if (u->active()->entered())
	{
		UCHAR v = u->active()->get() ? '\1' : '\0';
		putField(threadDbb, record,
				 DumpField(f_sec_active, VALUE_BOOLEAN, sizeof(v), &v),
				 charset);
	}

	if (u->comment()->entered())
	{
		putField(threadDbb, record,
				 DumpField(f_sec_comment, VALUE_STRING, strlen(u->comment()->get()), u->comment()->get()),
				 charset);
	}

	buffer->store(record);

	if (u->userName()->entered() && u->attributes()->entered())
	{
		buffer = getData(rel_sec_user_attributes);

		ConfigFile cf(ConfigFile::USE_TEXT, u->attributes()->get());
		ConfigFile::Parameters::const_iterator e(cf.getParameters().end());
		for (ConfigFile::Parameters::const_iterator b(cf.getParameters().begin()); b != e; ++b)
		{
			record = buffer->getTempRecord();
			record->nullify();

			putField(threadDbb, record,
					 DumpField(f_sec_attr_user, VALUE_STRING, strlen(u->userName()->get()), u->userName()->get()),
					 charset);

			putField(threadDbb, record,
					 DumpField(f_sec_attr_key, VALUE_STRING, b->name.length(), b->name.c_str()),
					 charset);

			putField(threadDbb, record,
					 DumpField(f_sec_attr_value, VALUE_STRING, b->value.length(), b->value.c_str()),
					 charset);

			buffer->store(record);
		}
	}
}

RecordBuffer* UserManagement::getList(thread_db* tdbb, jrd_rel* relation)
{
	fb_assert(relation);
	fb_assert(relation->rel_id == rel_sec_user_attributes || relation->rel_id == rel_sec_users);

	RecordBuffer* recordBuffer = getData(relation);
	if (recordBuffer)
	{
		return recordBuffer;
	}

	try
	{
		threadDbb = tdbb;
		MemoryPool* const pool = threadDbb->getTransaction()->tra_pool;
		allocBuffer(threadDbb, *pool, rel_sec_users);
		allocBuffer(threadDbb, *pool, rel_sec_user_attributes);

		class FillSnapshot : public Firebird::AutoIface<Auth::IListUsers, FB_AUTH_LIST_USERS_VERSION>
		{
		public:
			explicit FillSnapshot(UserManagement* um)
				: userManagement(um)
			{ }

			// IListUsers implementation
			void FB_CARG list(Auth::IUser* user)
			{
				MasterInterfacePtr()->upgradeInterface(user, FB_AUTH_USER_VERSION, upInfo);
				userManagement->list(user);
			}

		private:
			UserManagement* userManagement;
		};

		FillSnapshot fillSnapshot(this);

		LocalStatus status;
		Auth::StackUserData u;
		u.op = Auth::DIS_OPER;
		int errcode = manager->execute(&status, &u, &fillSnapshot);
		checkSecurityResult(errcode, &status, "Unknown", &u);
	}
	catch (const Exception&)
	{
		clearSnapshot();
		throw;
	}

	return getData(relation);
}
