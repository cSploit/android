/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			IntlParametersBlock.cpp
 *	DESCRIPTION:	Convert strings in parameters block to/from UTF8.
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
 *
 *
 */

#include "firebird.h"
#include "../common/IntlParametersBlock.h"

#include "consts_pub.h"
#include "../common/isc_f_proto.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/UtilSvc.h"

using namespace Firebird;

namespace
{

void strToUtf8(string& s)
{
	ISC_systemToUtf8(s);
}

void strFromUtf8(string& s)
{
	ISC_unescape(s);
	ISC_utf8ToSystem(s);
}

void processCommandLine(IntlParametersBlock::ProcessString* processString, string& par)
{
	bool flagIn = false;
	string current, result;

	for (const char* s = par.begin(); s < par.end(); ++s)
	{
		if (s[0] == SVC_TRMNTR)
		{
			if (!flagIn)
			{
				flagIn = true;
			}
			else if (s[1] == SVC_TRMNTR)
			{
				current += SVC_TRMNTR;
				++s;
			}
			else
			{
				flagIn = false;
				processString(current);
				if (result.hasData())
				{
					result += ' ';
				}
				result += current;
				current = "";
			}
		}
		else if (s[0] != ' ' || flagIn || current.hasData())
		{
			current += s[0];
		}
	}

	if (current.hasData())
	{
		processString(current);
		if (result.hasData())
		{
			result += ' ';
		}
		result += current;
	}

	par = result;
}

}

namespace Firebird
{

void IntlParametersBlock::toUtf8(ClumpletWriter& pb, UCHAR utf8Tag)
{
	if (utf8Tag)
	{
		pb.insertTag(utf8Tag);
	}

	processParametersBlock(strToUtf8, pb);
}

void IntlParametersBlock::fromUtf8(ClumpletWriter& pb, UCHAR utf8Tag)
{
	if (utf8Tag)
	{
		pb.deleteWithTag(utf8Tag);
	}

	processParametersBlock(strFromUtf8, pb);
}


void IntlParametersBlock::processParametersBlock(ProcessString* processString, ClumpletWriter& pb)
{
	for (pb.rewind(); !pb.isEof(); pb.moveNext())
	{
		UCHAR tag = pb.getClumpTag();
		string s;

		switch (checkTag(tag))
		{
		case TAG_SKIP:
			break;

		case TAG_STRING:
			pb.getString(s);
			processString(s);
			pb.deleteClumplet();
			pb.insertString(tag, s);
			break;

		case TAG_COMMAND_LINE:
			pb.getString(s);
			processCommandLine(processString, s);
			pb.deleteClumplet();
			pb.insertString(tag, s);
			break;
		}
	}
}


IntlParametersBlock::TagType IntlDpb::checkTag(UCHAR tag)
{
	switch (tag)
	{
	case isc_dpb_user_name:
	case isc_dpb_password:
	case isc_dpb_sql_role_name:
	case isc_dpb_trusted_auth:
	case isc_dpb_trusted_role:
	case isc_dpb_working_directory:
	case isc_dpb_set_db_charset:
	case isc_dpb_process_name:
	case isc_dpb_host_name:
	case isc_dpb_os_user:
		return TAG_STRING;
	}

	return TAG_SKIP;
}


IntlParametersBlock::TagType IntlSpb::checkTag(UCHAR tag)
{
	switch (tag)
	{
	case isc_spb_user_name:
	case isc_spb_password:
	case isc_spb_sql_role_name:
	case isc_spb_trusted_auth:
	case isc_spb_trusted_role:
	case isc_spb_process_name:
	case isc_spb_expected_db:
		return TAG_STRING;

	case isc_spb_command_line:
		return TAG_COMMAND_LINE;
	}

	return TAG_SKIP;
}


IntlParametersBlock::TagType IntlSpbStart::checkTag(UCHAR tag)
{
	switch (mode)
	{
	case 0:
		switch (tag)
		{
		case isc_spb_dbname:
			return TAG_STRING;

		case isc_action_svc_backup:
		case isc_action_svc_restore:
		case isc_action_svc_repair:
		case isc_action_svc_add_user:
		case isc_action_svc_delete_user:
		case isc_action_svc_modify_user:
		case isc_action_svc_display_user:
		case isc_action_svc_display_user_adm:
		case isc_action_svc_nbak:
		case isc_action_svc_nrest:
		case isc_action_svc_trace_start:
		case isc_action_svc_db_stats:
			mode = tag;
			break;
		}
		break;

	case isc_action_svc_backup:
	case isc_action_svc_restore:
		switch (tag)
		{
		case isc_spb_bkp_file:
			return TAG_STRING;
		}
		break;

	case isc_action_svc_repair:
		switch (tag)
		{
		case isc_spb_tra_db_path:
			return TAG_STRING;
		}
		break;

	case isc_action_svc_add_user:
	case isc_action_svc_delete_user:
	case isc_action_svc_modify_user:
	case isc_action_svc_display_user:
	case isc_action_svc_display_user_adm:
		switch (tag)
		{
		case isc_spb_sec_username:
		case isc_spb_sec_password:
		case isc_spb_sec_groupname:
		case isc_spb_sec_firstname:
		case isc_spb_sec_middlename:
		case isc_spb_sec_lastname:
			return TAG_STRING;
		}
		break;

	case isc_action_svc_nbak:
	case isc_action_svc_nrest:
		switch (tag)
		{
		case isc_spb_nbk_file:
			return TAG_STRING;
		}
		break;

	case isc_action_svc_trace_start:
		switch (tag)
		{
		case isc_spb_trc_name:
		case isc_spb_trc_cfg:
			return TAG_STRING;
		}
		break;

	case isc_action_svc_db_stats:
		switch (tag)
		{
		case isc_spb_command_line:
			return TAG_COMMAND_LINE;
		}
		break;
	}

	return TAG_SKIP;
}

}
