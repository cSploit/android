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
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/tra.h"
#include "../jrd/msg_encode.h"
#include "../utilities/gsec/gsec.h"
#include "../utilities/gsec/secur_proto.h"

using namespace Jrd;
using namespace Firebird;


UserManagement::UserManagement(jrd_tra* tra)
	: database(0), transaction(0), commands(*tra->tra_pool)
{
	char securityDatabaseName[MAXPATHLEN];
	SecurityDatabase::getPath(securityDatabaseName);
	ISC_STATUS_ARRAY status;
	Attachment* att = tra->tra_attachment;

	ClumpletWriter dpb(ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);
	dpb.insertByte(isc_dpb_gsec_attach, TRUE);
	dpb.insertString(isc_dpb_trusted_auth, att->att_user->usr_user_name);
	if (att->att_user->usr_flags & USR_trole)
	{
		dpb.insertString(isc_dpb_trusted_role, ADMIN_ROLE, strlen(ADMIN_ROLE));
	}
	else if (att->att_user->usr_sql_role_name.hasData() && att->att_user->usr_sql_role_name != NULL_ROLE)
	{
		dpb.insertString(isc_dpb_sql_role_name, att->att_user->usr_sql_role_name);
	}
	else if (att->att_requested_role.hasData())
    {
    	dpb.insertString(isc_dpb_sql_role_name, att->att_requested_role);
	}

	if (isc_attach_database(status, 0, securityDatabaseName, &database,
							dpb.getBufferLength(), reinterpret_cast<const char*>(dpb.getBuffer())))
	{
		status_exception::raise(status);
	}

	if (isc_start_transaction(status, &transaction, 1, &database, 0, NULL))
	{
		status_exception::raise(status);
	}
}

UserManagement::~UserManagement()
{
	for (ULONG i = 0; i < commands.getCount(); ++i)
	{
		delete commands[i];
	}
	commands.clear();

	ISC_STATUS_ARRAY status;
	if (transaction)
	{
		// Rollback transaction in security database ...
		if (isc_rollback_transaction(status, &transaction))
		{
			status_exception::raise(status);
		}
	}

	if (database)
	{
		if (isc_detach_database(status, &database))
		{
			status_exception::raise(status);
		}
	}
}

void UserManagement::commit()
{
	ISC_STATUS_ARRAY status;
	if (transaction)
	{
		// Commit transaction in security database
		if (isc_commit_transaction(status, &transaction))
		{
			status_exception::raise(status);
		}
		transaction = 0;
	}
}

USHORT UserManagement::put(internal_user_data* userData)
{
	const size_t ret = commands.getCount();
	if (ret > MAX_USHORT)
	{
		status_exception::raise(Arg::Gds(isc_random) << "Too many user management DDL per transaction)");
	}
	commands.push(userData);
	return ret;
}

void UserManagement::execute(USHORT id)
{
#if (defined BOOT_BUILD || defined EMBEDDED)
	status_exception::raise(Arg::Gds(isc_wish_list));
#else
	if (!transaction || !commands[id])
	{
		// Already executed
		return;
	}

	if (id >= commands.getCount())
	{
		status_exception::raise(Arg::Gds(isc_random) << "Wrong job id passed to UserManagement::execute()");
	}

	ISC_STATUS_ARRAY status;
	int errcode = (!commands[id]->user_name_entered) ? GsecMsg18 :
		SECURITY_exec_line(status, database, transaction, commands[id], NULL, NULL);

	switch (errcode)
	{
	case 0: // nothing
	    break;
	case GsecMsg22:
		{
			Arg::StatusVector tmp;
			tmp << Arg::Gds(ENCODE_ISC_MSG(errcode, GSEC_MSG_FAC)) << Arg::Str(commands[id]->user_name);
			tmp.append(Arg::StatusVector(&status[0]));
			tmp.raise();
		}

	default:
		{
			Arg::StatusVector tmp;
			tmp << Arg::Gds(ENCODE_ISC_MSG(errcode, GSEC_MSG_FAC));
			tmp.append(Arg::StatusVector(&status[0]));
			tmp.raise();
		}
	}

	delete commands[id];
	commands[id] = NULL;
#endif
}
