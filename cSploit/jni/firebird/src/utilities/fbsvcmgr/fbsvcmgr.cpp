/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		fbsvcmgr.cpp
 *	DESCRIPTION:	Command line interface with services manager
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
 *  Copyright (c) 2007 Alex Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  2008 Khorsun Vladyslav
 */

#include "firebird.h"
#include <signal.h>
#ifdef WIN_NT
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../yvalve/gds_proto.h"
#include "../jrd/ibase.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/timestamp.h"
#include "../common/utils_proto.h"
#include "../common/classes/MsgPrint.h"
#include "../jrd/license.h"

using namespace Firebird;

// Here we define main control structure

typedef bool PopulateFunction(char**&, ClumpletWriter&, unsigned int);

struct SvcSwitches
{
	const char* name;
	PopulateFunction* populate;
	const SvcSwitches* options;
	unsigned int tag;
	UCHAR tagInf;
};

// Get message from messages database

namespace
{
	const int SVCMGR_FACILITY = 22;
	using MsgFormat::SafeArg;
}

string getMessage(int n)
{
	char buffer[256];
	static const SafeArg dummy;

	fb_msg_format(0, SVCMGR_FACILITY, n, sizeof(buffer), buffer, dummy);

	return string(buffer);
}

string prepareSwitch(const char* arg)
{
	string s(arg);
	if (s[0] == '-')
	{
		s.erase(0, 1);
	}
	s.lower();

	return s;
}

// add string tag to spb

bool putStringArgument(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	if (! *av)
		return false;

	char* x = *av++;
	string s(tag == isc_spb_password ? fb_utils::get_passwd(x) : x);
	spb.insertString(tag, s);

	return true;
}

// add string tag from file (fetch password)

bool putFileArgument(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	if (! *av)
		return false;

	const char* s = NULL;
	switch (fb_utils::fetchPassword(*av, s))
	{
	case fb_utils::FETCH_PASS_OK:
		break;
	case fb_utils::FETCH_PASS_FILE_OPEN_ERROR:
		(Arg::Gds(isc_fbsvcmgr_fp_open) << *av << Arg::OsError()).raise();
		break;
	case fb_utils::FETCH_PASS_FILE_READ_ERROR:
		(Arg::Gds(isc_fbsvcmgr_fp_read) << *av << Arg::OsError()).raise();
		break;
	case fb_utils::FETCH_PASS_FILE_EMPTY:
		(Arg::Gds(isc_fbsvcmgr_fp_empty) << *av).raise();
		break;
	}

	spb.insertString(tag, s, strlen(s));
	++av;

	return true;
}

bool putFileFromArgument(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	if (! *av)
		return false;

	FILE* const file = fopen(*av, "rb");
	if (!file) {
		(Arg::Gds(isc_fbsvcmgr_fp_open) << *av << Arg::OsError()).raise();
	}

	fseek(file, 0, SEEK_END);
	const long len = ftell(file);
	if (len == 0)
	{
		fclose(file);
		(Arg::Gds(isc_fbsvcmgr_fp_empty) << *av).raise();
	}

	HalfStaticArray<UCHAR, 1024> buff(*getDefaultMemoryPool(), len);
	UCHAR* p = buff.getBuffer(len);

	fseek(file, 0, SEEK_SET);
	if (fread(p, 1, len, file) != size_t(len))
	{
		fclose(file);
		(Arg::Gds(isc_fbsvcmgr_fp_read) << *av << Arg::OsError()).raise();
	}

	fclose(file);
	spb.insertBytes(tag, p, len);
	++av;

	return true;
}

// add some special format tags to spb

bool putSpecTag(char**& av, ClumpletWriter& spb, unsigned int tag,
				const SvcSwitches* sw, ISC_STATUS errorCode)
{
	if (! *av)
		return false;

	const string s(prepareSwitch(*av++));
	for (; sw->name; ++sw)
	{
		if (s == sw->name)
		{
			spb.insertByte(tag, sw->tag);
			return true;
		}
	}

	status_exception::raise(Arg::Gds(errorCode));
	return false;	// compiler warning silencer
}

const SvcSwitches amSwitch[] =
{
	{"prp_am_readonly", 0, 0, isc_spb_prp_am_readonly, 0},
	{"prp_am_readwrite", 0, 0, isc_spb_prp_am_readwrite, 0},
	{0, 0, 0, 0, 0}
};

bool putAccessMode(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	return putSpecTag(av, spb, tag, amSwitch, isc_fbsvcmgr_bad_am);
}

const SvcSwitches wmSwitch[] =
{
	{"prp_wm_async", 0, 0, isc_spb_prp_wm_async, 0},
	{"prp_wm_sync", 0, 0, isc_spb_prp_wm_sync, 0},
	{0, 0, 0, 0, 0}
};

bool putWriteMode(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	return putSpecTag(av, spb, tag, wmSwitch, isc_fbsvcmgr_bad_wm);
}

const SvcSwitches rsSwitch[] =
{
	{"prp_res_use_full", 0, 0, isc_spb_prp_res_use_full, 0},
	{"prp_res", 0, 0, isc_spb_prp_res, 0},
	{0, 0, 0, 0, 0}
};

bool putReserveSpace(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	return putSpecTag(av, spb, tag, rsSwitch, isc_fbsvcmgr_bad_rs);
}

const SvcSwitches shutSwitch[] =
{
	{"prp_sm_normal", 0, 0, isc_spb_prp_sm_normal, 0},
	{"prp_sm_multi", 0, 0, isc_spb_prp_sm_multi, 0},
	{"prp_sm_single", 0, 0, isc_spb_prp_sm_single, 0},
	{"prp_sm_full", 0, 0, isc_spb_prp_sm_full, 0},
	{0, 0, 0, 0, 0}
};

bool putShutdownMode(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	return putSpecTag(av, spb, tag, shutSwitch, isc_fbsvcmgr_bad_sm);
}

// add numeric (int32) tag to spb

bool putNumericArgument(char**& av, ClumpletWriter& spb, unsigned int tag)
{
	if (! *av)
		return false;

	int n = atoi(*av++);
	spb.insertInt(tag, n);

	return true;
}

// add boolean option to spb

bool putOption(char**&, ClumpletWriter& spb, unsigned int tag)
{
	spb.insertInt(isc_spb_options, tag);

	return true;
}

// add argument-less tag to spb

bool putSingleTag(char**&, ClumpletWriter& spb, unsigned int tag)
{
	spb.insertTag(tag);

	return true;
}

// populate spb with tags according to user-defined command line switches
// and programmer-defined set of SvcSwitches array

bool populateSpbFromSwitches(char**& av, ClumpletWriter& spb,
							 const SvcSwitches* sw, ClumpletWriter* infoSpb)
{
	if (! *av)
		return false;

	const string s(prepareSwitch(*av));

	for (; sw->name; ++sw)
	{
		if (s == sw->name)
		{
			av++;
			if (sw->populate(av, spb, sw->tag))
			{
				if (infoSpb && sw->tagInf)
				{
					infoSpb->insertTag(sw->tagInf);
				}
				if (sw->options)
				{
					while (populateSpbFromSwitches(av, spb, sw->options, infoSpb))
						;
					return false;
				}
				return true;
			}
			(Arg::Gds(isc_fbsvcmgr_bad_arg) << av[-1]).raise();
		}
	}

	return false;
}

const SvcSwitches attSwitch[] =
{
	{"user", putStringArgument, 0, isc_spb_user_name, 0},
	{"user_name", putStringArgument, 0, isc_spb_user_name, 0},
	{"password", putStringArgument, 0, isc_spb_password, 0},
	{"fetch_password", putFileArgument, 0, isc_spb_password, 0},
	{"trusted_auth", putSingleTag, 0, isc_spb_trusted_auth, 0},
	{"expected_db", putStringArgument, 0, isc_spb_expected_db, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches infSwitch[] =
{
	{"info_server_version", putSingleTag, 0, isc_info_svc_server_version, 0},
	{"info_implementation", putSingleTag, 0, isc_info_svc_implementation, 0},
	{"info_user_dbpath", putSingleTag, 0, isc_info_svc_user_dbpath, 0},
	{"info_get_env", putSingleTag, 0, isc_info_svc_get_env, 0},
	{"info_get_env_lock", putSingleTag, 0, isc_info_svc_get_env_lock, 0},
	{"info_get_env_msg", putSingleTag, 0, isc_info_svc_get_env_msg, 0},
	{"info_svr_db_info", putSingleTag, 0, isc_info_svc_svr_db_info, 0},
	{"info_version", putSingleTag, 0, isc_info_svc_version, 0},
	{"info_capabilities", putSingleTag, 0, isc_info_svc_capabilities, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches backupOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"verbose", putSingleTag, 0, isc_spb_verbose, 0},
	{"bkp_file", putStringArgument, 0, isc_spb_bkp_file, 0},
	{"bkp_length", putNumericArgument, 0, isc_spb_bkp_length, 0},
	{"bkp_factor", putNumericArgument, 0, isc_spb_bkp_factor, 0},
	{"bkp_ignore_checksums", putOption, 0, isc_spb_bkp_ignore_checksums, 0},
	{"bkp_ignore_limbo", putOption, 0, isc_spb_bkp_ignore_limbo, 0},
	{"bkp_metadata_only", putOption, 0, isc_spb_bkp_metadata_only, 0},
	{"bkp_no_garbage_collect", putOption, 0, isc_spb_bkp_no_garbage_collect, 0},
	{"bkp_old_descriptions", putOption, 0, isc_spb_bkp_old_descriptions, 0},
	{"bkp_non_transportable", putOption, 0, isc_spb_bkp_non_transportable, 0},
	{"bkp_convert", putOption, 0, isc_spb_bkp_convert, 0},
	{"bkp_no_triggers", putOption, 0, isc_spb_bkp_no_triggers, 0},
	{"verbint", putNumericArgument, 0, isc_spb_verbint, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches restoreOptions[] =
{
	{"bkp_file", putStringArgument, 0, isc_spb_bkp_file, 0},
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"res_length", putNumericArgument, 0, isc_spb_res_length, 0},
	{"verbose", putSingleTag, 0, isc_spb_verbose, 0},
	{"res_buffers", putNumericArgument, 0, isc_spb_res_buffers, 0},
	{"res_page_size", putNumericArgument, 0, isc_spb_res_page_size, 0},
	{"res_access_mode", putAccessMode, 0, isc_spb_res_access_mode, 0},
	{"res_deactivate_idx", putOption, 0, isc_spb_res_deactivate_idx, 0},
	{"res_no_shadow", putOption, 0, isc_spb_res_no_shadow, 0},
	{"res_no_validity", putOption, 0, isc_spb_res_no_validity, 0},
	{"res_one_at_a_time", putOption, 0, isc_spb_res_one_at_a_time, 0},
	{"res_replace", putOption, 0, isc_spb_res_replace, 0},
	{"res_create", putOption, 0, isc_spb_res_create, 0},
	{"res_use_all_space", putOption, 0, isc_spb_res_use_all_space, 0},
	{"res_fix_fss_data", putStringArgument, 0, isc_spb_res_fix_fss_data, 0},
	{"res_fix_fss_metadata", putStringArgument, 0, isc_spb_res_fix_fss_metadata, 0},
	{"res_metadata_only", putOption, 0, isc_spb_res_metadata_only, 0},
	{"verbint", putNumericArgument, 0, isc_spb_verbint, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches propertiesOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"prp_page_buffers", putNumericArgument, 0, isc_spb_prp_page_buffers, 0},
	{"prp_sweep_interval", putNumericArgument, 0, isc_spb_prp_sweep_interval, 0},
	{"prp_shutdown_db", putNumericArgument, 0, isc_spb_prp_shutdown_db, 0},
	{"prp_deny_new_transactions", putNumericArgument, 0, isc_spb_prp_deny_new_transactions, 0},
	{"prp_deny_new_attachments", putNumericArgument, 0, isc_spb_prp_deny_new_attachments, 0},
	{"prp_set_sql_dialect", putNumericArgument, 0, isc_spb_prp_set_sql_dialect, 0},
	{"prp_access_mode", putAccessMode, 0, isc_spb_prp_access_mode, 0},
	{"prp_reserve_space", putReserveSpace, 0, isc_spb_prp_reserve_space, 0},
	{"prp_write_mode", putWriteMode, 0, isc_spb_prp_write_mode, 0},
	{"prp_activate", putOption, 0, isc_spb_prp_activate, 0},
	{"prp_db_online", putOption, 0, isc_spb_prp_db_online, 0},
	{"prp_force_shutdown", putNumericArgument, 0, isc_spb_prp_force_shutdown, 0},
	{"prp_attachments_shutdown", putNumericArgument, 0, isc_spb_prp_attachments_shutdown, 0},
	{"prp_transactions_shutdown", putNumericArgument, 0, isc_spb_prp_transactions_shutdown, 0},
	{"prp_shutdown_mode", putShutdownMode, 0, isc_spb_prp_shutdown_mode, 0},
	{"prp_online_mode", putShutdownMode, 0, isc_spb_prp_online_mode, 0},
	{"prp_nolinger", putOption, 0, isc_spb_prp_nolinger, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches repairOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"rpr_commit_trans", putNumericArgument, 0, isc_spb_rpr_commit_trans, 0},
	{"rpr_rollback_trans", putNumericArgument, 0, isc_spb_rpr_rollback_trans, 0},
	{"rpr_recover_two_phase", putNumericArgument, 0, isc_spb_rpr_recover_two_phase, 0},
	{"rpr_check_db", putOption, 0, isc_spb_rpr_check_db, 0},
	{"rpr_ignore_checksum", putOption, 0, isc_spb_rpr_ignore_checksum, 0},
	{"rpr_kill_shadows", putOption, 0, isc_spb_rpr_kill_shadows, 0},
	{"rpr_mend_db", putOption, 0, isc_spb_rpr_mend_db, 0},
	{"rpr_validate_db", putOption, 0, isc_spb_rpr_validate_db, 0},
	{"rpr_full", putOption, 0, isc_spb_rpr_full, 0},
	{"rpr_sweep_db", putOption, 0, isc_spb_rpr_sweep_db, 0},
	{"rpr_list_limbo_trans", putOption, 0, isc_spb_rpr_list_limbo_trans, isc_info_svc_limbo_trans},
	{0, 0, 0, 0, 0}
};

const SvcSwitches statisticsOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"sts_record_versions", putOption, 0, isc_spb_sts_record_versions, 0},
	{"sts_nocreation", putOption, 0, isc_spb_sts_nocreation, 0},
	{"sts_table", putStringArgument, 0, isc_spb_sts_table, 0},
	{"sts_data_pages", putOption, 0, isc_spb_sts_data_pages, 0},
	{"sts_hdr_pages", putOption, 0, isc_spb_sts_hdr_pages, 0},
	{"sts_idx_pages", putOption, 0, isc_spb_sts_idx_pages, 0},
	{"sts_sys_relations", putOption, 0, isc_spb_sts_sys_relations, 0},
	{"sts_encryption", putOption, 0, isc_spb_sts_encryption, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches dispdelOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"sec_username", putStringArgument, 0, isc_spb_sec_username, 0},
	{"sql_role_name", putStringArgument, 0, isc_spb_sql_role_name, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches mappingOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"sql_role_name", putStringArgument, 0, isc_spb_sql_role_name, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches addmodOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"sec_username", putStringArgument, 0, isc_spb_sec_username, 0},
	{"sql_role_name", putStringArgument, 0, isc_spb_sql_role_name, 0},
	{"sec_password", putStringArgument, 0, isc_spb_sec_password, 0},
	{"sec_groupname", putStringArgument, 0, isc_spb_sec_groupname, 0},
	{"sec_firstname", putStringArgument, 0, isc_spb_sec_firstname, 0},
	{"sec_middlename", putStringArgument, 0, isc_spb_sec_middlename, 0},
	{"sec_lastname", putStringArgument, 0, isc_spb_sec_lastname, 0},
	{"sec_userid", putNumericArgument, 0, isc_spb_sec_userid, 0},
	{"sec_groupid", putNumericArgument, 0, isc_spb_sec_groupid, 0},
	{"sec_admin", putNumericArgument, 0, isc_spb_sec_admin, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches nbackOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"nbk_file", putStringArgument, 0, isc_spb_nbk_file, 0},
	{"nbk_level", putNumericArgument, 0, isc_spb_nbk_level, 0},
	{"nbk_no_triggers", putOption, 0, isc_spb_nbk_no_triggers, 0},
	{"nbk_direct", putStringArgument, 0, isc_spb_nbk_direct, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches nrestOptions[] =
{
	{"dbname", putStringArgument, 0, isc_spb_dbname, 0},
	{"nbk_file", putStringArgument, 0, isc_spb_nbk_file, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches traceStartOptions[] =
{
	{"trc_cfg", putFileFromArgument, 0, isc_spb_trc_cfg, 0},
	{"trc_name", putStringArgument, 0, isc_spb_trc_name, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches traceChgStateOptions[] =
{
	{"trc_name", putStringArgument, 0, isc_spb_trc_name, 0},
	{"trc_id", putNumericArgument, 0, isc_spb_trc_id, 0},
	{0, 0, 0, 0, 0}
};

const SvcSwitches actionSwitch[] =
{
	{"action_backup", putSingleTag, backupOptions, isc_action_svc_backup, isc_info_svc_to_eof},
	{"action_restore", putSingleTag, restoreOptions, isc_action_svc_restore, isc_info_svc_line},
	{"action_properties", putSingleTag, propertiesOptions, isc_action_svc_properties, 0},
	{"action_repair", putSingleTag, repairOptions, isc_action_svc_repair, 0},
	{"action_db_stats", putSingleTag, statisticsOptions, isc_action_svc_db_stats, isc_info_svc_line},
	{"action_get_fb_log", putSingleTag, 0, isc_action_svc_get_fb_log, isc_info_svc_to_eof},
	{"action_get_ib_log", putSingleTag, 0, isc_action_svc_get_ib_log, isc_info_svc_to_eof},
	{"action_display_user", putSingleTag, dispdelOptions, isc_action_svc_display_user, isc_info_svc_get_users},
	{"action_display_user_adm", putSingleTag, dispdelOptions, isc_action_svc_display_user_adm, isc_info_svc_get_users},
	{"action_add_user", putSingleTag, addmodOptions, isc_action_svc_add_user, 0},
	{"action_delete_user", putSingleTag, dispdelOptions, isc_action_svc_delete_user, 0},
	{"action_modify_user", putSingleTag, addmodOptions, isc_action_svc_modify_user, 0},
	{"action_nbak", putSingleTag, nbackOptions, isc_action_svc_nbak, isc_info_svc_line},
	{"action_nrest", putSingleTag, nrestOptions, isc_action_svc_nrest, isc_info_svc_line},
	{"action_trace_start", putSingleTag, traceStartOptions, isc_action_svc_trace_start, isc_info_svc_to_eof},
	{"action_trace_suspend", putSingleTag, traceChgStateOptions, isc_action_svc_trace_suspend, isc_info_svc_line},
	{"action_trace_resume", putSingleTag, traceChgStateOptions, isc_action_svc_trace_resume, isc_info_svc_line},
	{"action_trace_stop", putSingleTag, traceChgStateOptions, isc_action_svc_trace_stop, isc_info_svc_line},
	{"action_trace_list", putSingleTag, 0, isc_action_svc_trace_list, isc_info_svc_line},
	{"action_set_mapping", putSingleTag, mappingOptions, isc_action_svc_set_mapping, 0},
	{"action_drop_mapping", putSingleTag, mappingOptions, isc_action_svc_drop_mapping, 0},
	{0, 0, 0, 0, 0}
};

// print information, returned by isc_svc_query() call

bool getLine(string& dest, const char*& p)
{
	unsigned short length = (unsigned short) isc_vax_integer (p, sizeof(unsigned short));
	p += sizeof (unsigned short);
	dest.assign(p, length);
	p += length;
	return length > 0;
}

int getNumeric(const char*& p)
{
	unsigned int num = (unsigned int) isc_vax_integer (p, sizeof(unsigned int));
	p += sizeof (unsigned int);
	return num;
}

bool printLine(const char*& p)
{
	string s;
	bool rc = getLine(s, p);
	if (rc)
		printf ("%s\n", s.c_str());
	return rc;
}

bool printData(const char*& p)
{
	static int binout = -1;
	if (binout == -1)
	{
#ifdef WIN_NT
		binout = fileno(stdout);
		setmode(binout, 0);
#else
		binout = 1;
#endif
	}

	string s;
	bool rc = getLine(s, p);
	if (rc)
	{
		FB_UNUSED(write(binout, s.c_str(), s.length()));
	}
	return rc;
}

void printString(const char*& p, int num)
{
	printf ("%s: ", getMessage(num).c_str());
	if (!printLine(p))
	{
		printf ("<no data>\n");
	}
}

void printMessage(int num)
{
	printf ("%s\n", getMessage(num).c_str());
}

void printMessage(USHORT number, const SafeArg& arg, bool newLine = true)
{
	char buffer[256];
	fb_msg_format(NULL, SVCMGR_FACILITY, number, sizeof(buffer), buffer, arg);
	if (newLine)
		printf("%s\n", buffer);
	else
		printf("%s", buffer);
}

void printNumeric(const char*& p, int num)
{
	printf ("%s: %d\n", getMessage(num).c_str(), getNumeric(p));
}

const char* capArray[] = {
	"WAL_SUPPORT",
	"MULTI_CLIENT_SUPPORT",
	"REMOTE_HOP_SUPPORT",
	"NO_SVR_STATS_SUPPORT",
	"NO_DB_STATS_SUPPORT",
	"LOCAL_ENGINE_SUPPORT",
	"NO_FORCED_WRITE_SUPPORT",
	"NO_SHUTDOWN_SUPPORT",
	"NO_SERVER_SHUTDOWN_SUPPORT",
	"SERVER_CONFIG_SUPPORT",
	"QUOTED_FILENAME_SUPPORT",
	NULL};

void printCapabilities(const char*& p)
{
	printMessage(57);

	int caps = getNumeric(p);
	bool print = false;

	for (unsigned i = 0; capArray[i]; ++i)
	{
		if (caps & (1 << i))
		{
			print = true;
			printf("  %s\n", capArray[i]);
		}
	}

	if (!print)
	{
		printf("  <None>\n");
	}
}

class UserPrint
{
public:
	string login, first, middle, last;
	int gid, uid, admin;

private:
	int hasData;

public:
	UserPrint() : hasData(0)
	{
		clear();
	}

	~UserPrint()
	{
		// print data, accumulated for last user
		newUser();
	}

	void clear()
	{
		login = first = middle = last = "";
		gid = uid = admin = 0;
	}

	void newUser()
	{
		if (hasData == 0)
		{
			hasData = 1;
			return;
		}
		if (hasData == 1)
		{
			printf("%-28.28s %-40.40s %4.4s %4.4s %3.3s\n", "Login",
				"Full name", "uid", "gid", "adm");
			hasData = 2;
		}

		printf("%-28.28s %-40.40s %4d %4d %3.3s\n", login.c_str(),
			(first + " " + middle + " " + last).c_str(), uid, gid, admin ? "yes" : "no");
		clear();
	}
};

bool printInfo(const char* p, size_t pSize, UserPrint& up, ULONG& stdinRq)
{
	bool ret = false;
	bool ignoreTruncation = false;
	stdinRq = 0;
	const char* const end = p + pSize;

	while (p < end && *p != isc_info_end)
	{
		switch (*p++)
		{
		case isc_info_svc_version:
			printNumeric(p, 7);
			break;
		case isc_info_svc_server_version:
			printString(p, 8);
			break;
		case isc_info_svc_implementation:
			printString(p, 9);
			break;
		case isc_info_svc_get_env_msg:
			printString(p, 10);
			break;
		case isc_info_svc_get_env:
			printString(p, 11);
			break;
		case isc_info_svc_get_env_lock:
			printString(p, 12);
			break;
		case isc_info_svc_user_dbpath:
			printString(p, 13);
			break;

		case isc_info_svc_svr_db_info:
			printf ("%s:\n", getMessage(14).c_str());
			while (*p != isc_info_flag_end)
			{
				switch (*p++)
				{
				case isc_spb_dbname:
					printString(p, 15);
					break;
				case isc_spb_num_att:
					printNumeric(p, 16);
					break;
				case isc_spb_num_db:
					printNumeric(p, 17);
					break;
				default:
					status_exception::raise(Arg::Gds(isc_fbsvcmgr_info_err) <<
											Arg::Num(static_cast<unsigned char>(p[-1])));
				}
			}
			p++;
			break;

		case isc_info_svc_limbo_trans:
			while (*p != isc_info_flag_end)
			{
				switch (*p++)
				{
				case isc_spb_tra_host_site:
					printString(p, 36);
					break;
				case isc_spb_tra_state:
					switch (*p++)
					{
					case isc_spb_tra_state_limbo:
			            printMessage(38);
						break;
					case isc_spb_tra_state_commit:
			            printMessage(39);
						break;
					case isc_spb_tra_state_rollback:
			            printMessage(40);
						break;
					case isc_spb_tra_state_unknown:
			            printMessage(41);
						break;
					default:
						status_exception::raise(Arg::Gds(isc_fbsvcmgr_info_err) <<
												Arg::Num(static_cast<unsigned char>(p[-1])));
					}
					break;
				case isc_spb_tra_remote_site:
					printString(p, 42);
					break;
				case isc_spb_tra_db_path:
					printString(p, 43);
					break;
				case isc_spb_tra_advise:
					switch (*p++)
					{
					case isc_spb_tra_advise_commit:
			            printMessage(44);
						break;
					case isc_spb_tra_advise_rollback:
			            printMessage(45);
						break;
					case isc_spb_tra_advise_unknown:
			            printMessage(46);
						break;
					default:
						status_exception::raise(Arg::Gds(isc_fbsvcmgr_info_err) <<
												Arg::Num(static_cast<unsigned char>(p[-1])));
					}
					break;
				case isc_spb_multi_tra_id:
					printNumeric(p, 35);
					break;
				case isc_spb_single_tra_id:
					printNumeric(p, 34);
					break;
				case isc_spb_tra_id:
					printNumeric(p, 37);
					break;
				default:
					status_exception::raise(Arg::Gds(isc_fbsvcmgr_info_err) <<
											Arg::Num(static_cast<unsigned char>(p[-1])));
				}
			}
			p++;
			break;

		case isc_info_svc_get_users:
			p += sizeof(unsigned short);
			break;
		case isc_spb_sec_username:
			up.newUser();
			getLine(up.login, p);
			break;
		case isc_spb_sec_firstname:
			getLine(up.first, p);
			break;
		case isc_spb_sec_middlename:
			getLine(up.middle, p);
			break;
		case isc_spb_sec_lastname:
			getLine(up.last, p);
			break;
		case isc_spb_sec_groupid:
			up.gid = getNumeric(p);
			break;
		case isc_spb_sec_userid:
			up.uid = getNumeric(p);
			break;
		case isc_spb_sec_admin:
			up.admin = getNumeric(p);
			break;

		case isc_info_svc_line:
			ret = printLine(p);
			break;

		case isc_info_svc_to_eof:
			ret = printData(p);
			ignoreTruncation = true;
			break;

		case isc_info_truncated:
			if (!ignoreTruncation)
			{
				printf("\n%s\n", getMessage(18).c_str());
			}
			fflush(stdout);
			ret = true;
			break;

		case isc_info_svc_timeout:
		case isc_info_data_not_ready:
			ret = true;
			break;

		case isc_info_svc_stdin:
			stdinRq = getNumeric(p);
			if (stdinRq > 0)
			{
				ret = true;
			}
			break;

		case isc_info_svc_capabilities:
			printCapabilities(p);
			break;

		default:
			status_exception::raise(Arg::Gds(isc_fbsvcmgr_query_err) <<
									Arg::Num(static_cast<unsigned char>(p[-1])));
		}
	}

	fflush(stdout);
	return ret;
}

// print known switches help

struct TypeText
{
	PopulateFunction* populate;
	const char* text;
} typeText[] = {
	{ putStringArgument, "string value" },
	{ putFileArgument, "file name" },
	{ putFileFromArgument, "file name" },
	{ putAccessMode, "prp_am_readonly | prp_am_readwrite" },
	{ putWriteMode, "prp_wm_async | prp_wm_sync" },
	{ putReserveSpace, "prp_res_use_full | prp_res" },
	{ putShutdownMode, "prp_sm_normal | prp_sm_multi | prp_sm_single | prp_sm_full" },
	{ putNumericArgument, "numeric value" },
	{ putOption, NULL },
	{ putSingleTag, NULL },
	{ NULL, NULL }
};

void printHelp(unsigned int offset, const SvcSwitches* sw)
{
	for (; sw->name; ++sw)
	{
		TypeText* tt = typeText;
		for (; tt->populate; ++tt)
		{
			if (sw->populate == tt->populate)
			{
				for (unsigned int n = 0; n < offset; ++n)
					putchar(' ');

				printf("%s", sw->name);
				if (tt->text)
					printf(" [%s]", tt->text);
				if (sw->options)
					putchar(':');
				putchar('\n');

				if (sw->options)
					printHelp(offset + 4, sw->options);
				break;
			}
		}

		fb_assert(tt->populate);
	}
}

// short usage from firebird.msg

void usage(bool listSwitches)
{
	for (int i = 19; i <= 33; ++i)
	{
		printf("%s\n", getMessage(i).c_str());
	}

	if (! listSwitches)
	{
		printf("%s\n", getMessage(53).c_str());
	}
	else
	{
		printf("\n%s\n", getMessage(54).c_str());
		printHelp(0, attSwitch);

		printf("\n%s\n", getMessage(55).c_str());
		printHelp(0, infSwitch);

		printf("\n%s\n", getMessage(56).c_str());
		printHelp(0, actionSwitch);
	}
}


typedef void (*SignalHandlerPointer)(int);

static SignalHandlerPointer prevCtrlCHandler = NULL;
static bool terminated = false;

static void ctrl_c_handler(int signal)
{
	if (signal == SIGINT)
		terminated = true;

	if (prevCtrlCHandler)
		prevCtrlCHandler(signal);
}


// simple main function

int main(int ac, char** av)
{
	if (ac < 2 || (ac == 2 && strcmp(av[1], "-?") == 0))
	{
		usage(ac == 2);
		return 1;
	}

	if (ac == 2 && (strcmp(av[1], "-z") == 0 || strcmp(av[1], "-Z") == 0))
	{
		printMessage(51, SafeArg() << FB_VERSION);
		return 0;
	}

	prevCtrlCHandler = signal(SIGINT, ctrl_c_handler);

	ISC_STATUS_ARRAY status;

	try {
		const int maxbuf = 16384;
		av++;

		const char* name = *av;
		if (name)
		{
			av++;
		}

		ClumpletWriter spbAtt(ClumpletWriter::spbList, maxbuf);
		while (populateSpbFromSwitches(av, spbAtt, attSwitch, 0))
			;

		ClumpletWriter spbStart(ClumpletWriter::SpbStart, maxbuf);
		ClumpletWriter spbItems(ClumpletWriter::SpbReceiveItems, 256);
		// single action per one utility run, it may populate info items also
		populateSpbFromSwitches(av, spbStart, actionSwitch, &spbItems);

		if (spbStart.getBufferLength() == 0)
		{
			while (populateSpbFromSwitches(av, spbItems, infSwitch, 0))
				;
		}

		// Here we are over with av parse, look - may be unknown switch left
		if (*av)
		{
			if (strcmp(av[0], "-z") == 0 || strcmp(av[0], "-Z") == 0)
			{
				printMessage(51, SafeArg() << FB_VERSION);
				++av;
			}
		}

		if (*av)
		{
			status_exception::raise(Arg::Gds(isc_fbsvcmgr_switch_unknown) << Arg::Str(*av));
		}

		isc_svc_handle svc_handle = 0;
		if (isc_service_attach(status, 0, name, &svc_handle,
					static_cast<USHORT>(spbAtt.getBufferLength()),
					reinterpret_cast<const char*>(spbAtt.getBuffer())))
		{
			isc_print_status(status);
			return 1;
		}

		if (spbStart.getBufferLength() > 0)
		{
			if (isc_service_start(status, &svc_handle, 0,
					static_cast<USHORT>(spbStart.getBufferLength()),
					reinterpret_cast<const char*>(spbStart.getBuffer())))
			{
				isc_print_status(status);
				isc_service_detach(status, &svc_handle);
				return 1;
			}
		}

		if (spbItems.getBufferLength() > 0)
		{
			if (fb_utils::isRunningCheck(spbItems.getBuffer(), spbItems.getBufferLength()))
			{
				// running service may request stdin data
				spbItems.insertTag(isc_info_svc_stdin);
			}

			// use one second timeout to poll service
			char send[16];
			char* p = send;
			*p++ = isc_info_svc_timeout;
			ADD_SPB_LENGTH(p, 4);
			ADD_SPB_NUMERIC(p, 1);
			*p++ = isc_info_end;

			char results[maxbuf];
			UserPrint uPrint;
			ULONG stdinRequest = 0;
			Array<char> stdinBuffer;
			do
			{
				char* sendBlock = send;
				USHORT sendSize = p - send;
				if (stdinRequest)
				{
					--sendSize;
					size_t len = sendSize;
					len += (1 + 2 + stdinRequest);
					if (len > MAX_USHORT - 1)
					{
						len = MAX_USHORT - 1;
						stdinRequest = len - (1 + 2) - sendSize;
					}
					sendBlock = stdinBuffer.getBuffer(len + 1);
					memcpy(sendBlock, send, sendSize);

					static int binIn = -1;
					if (binIn == -1)
					{
#ifdef WIN_NT
						binIn = fileno(stdin);
						setmode(binIn, 0);
#else
						binIn = 0;
#endif
					}

					int n = read(binIn, &sendBlock[sendSize + 1 + 2], stdinRequest);
					if (n < 0)
					{
						perror("stdin");
						break;
					}

					stdinRequest = n;
					sendBlock[sendSize] = isc_info_svc_line;
					sendBlock[sendSize + 1] = stdinRequest;
					sendBlock[sendSize + 2] = stdinRequest >> 8;
					sendBlock [sendSize + 1 + 2 + stdinRequest] = isc_info_end;
					sendSize += (1 + 2 + stdinRequest + 1);

					stdinRequest = 0;
				}

				if (isc_service_query(status, &svc_handle, 0, sendSize, sendBlock,
						static_cast<USHORT>(spbItems.getBufferLength()),
						reinterpret_cast<const char*>(spbItems.getBuffer()),
						sizeof(results), results))
				{
					isc_print_status(status);
					isc_service_detach(status, &svc_handle);
					return 1;
				}
			} while (printInfo(results, sizeof(results), uPrint, stdinRequest) && !terminated);
		}

		if (isc_service_detach(status, &svc_handle))
		{
			isc_print_status(status);
			return 1;
		}
		return 0;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
		isc_print_status(status);
	}
	return 2;
}
