/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		burp.cpp
 *	DESCRIPTION:	Command line interpreter for backup/restore
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" defines
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include <stdarg.h>
#include "../jrd/ibsetjmp.h"
#include "../jrd/msg_encode.h"
#include "../jrd/ods.h"			// to get MAX_PAGE_SIZE
#include "../jrd/svc.h"
#include "../jrd/constants.h"
#include "../burp/burp.h"
#include "../burp/burpswi.h"
#include "../burp/std_desc.h"
#include "../jrd/license.h"

#include "../common/classes/timestamp.h"
#include "../burp/burp_proto.h"
#include "../burp/backu_proto.h"
#include "../burp/mvol_proto.h"
#include "../burp/resto_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/gdsassert.h"
#include "../common/classes/ClumpletWriter.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#include "../common/utils_proto.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifndef O_CREAT
#include <sys/types.h>
#include <sys/file.h>
#endif

using MsgFormat::SafeArg;

const char* fopen_write_type = "w";
const char* fopen_read_type	 = "r";

const int open_mask = 0666;
const char switch_char = '-';


const char* const output_suppress	= "SUPPRESS";
const int burp_msg_fac				= 12;

enum gbak_action
{
	QUIT	=	0,
	BACKUP	=	1 ,
	RESTORE	=	2,
	FDESC	=	3
};

static void close_out_transaction(gbak_action, isc_tr_handle*);
//static void enable_signals();
//static void excp_handler();
static SLONG get_number(const SCHAR*);
static ULONG get_size(const SCHAR*, burp_fil*);
static gbak_action open_files(const TEXT *, const TEXT**, bool, USHORT,
							  const Firebird::ClumpletWriter&);
static int api_gbak(Firebird::UtilSvc*, in_sw_tab_t* const in_sw_tab);
static void burp_output(bool err, const SCHAR*, ...) ATTRIBUTE_FORMAT(2,3);
static void burp_usage(const in_sw_tab_t* const in_sw_tab);
static in_sw_tab_t* findSwitch(in_sw_tab_t* const in_sw_tab, Firebird::string, bool);

// fil.fil_length is ULONG
const ULONG KBYTE	= 1024;
const ULONG MBYTE	= KBYTE * KBYTE;
const ULONG GBYTE	= MBYTE * KBYTE;


THREAD_ENTRY_DECLARE BURP_main(THREAD_ENTRY_PARAM arg)
{
/**************************************
 *
 *	B U R P _ m a i n
 *
 **************************************
 *
 * Functional description
 *	Entrypoint for GBAK via services manager.
 *
 **************************************/
	Firebird::UtilSvc* uSvc = (Firebird::UtilSvc*) arg;
	int exit_code = FINI_OK;

	try {
		exit_code = gbak(uSvc);
	}
	catch (const Firebird::Exception& e)
	{
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);
		uSvc->initStatus();
		uSvc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}

	uSvc->finish();
	return (THREAD_ENTRY_RETURN)(IPTR) exit_code;
}


static int api_gbak(Firebird::UtilSvc* uSvc, in_sw_tab_t* const in_sw_tab)
{
/**********************************************
 *
 *	a p i _ g b a k
 *
 **********************************************
 *
 * Functional description
 *	Run gbak using services APIs
 *
 **********************************************/
    Firebird::string usr, pswd, service;
	bool flag_restore = false;
	bool flag_verbose = false;
#ifdef TRUSTED_AUTH
	bool flag_trusted = false;
#endif

	Firebird::UtilSvc::ArgvType& argv = uSvc->argv;
	const int argc = uSvc->argv.getCount();

	for (int itr = 1; itr < argc; ++itr)
	{
		const in_sw_tab_t* inSw = findSwitch(in_sw_tab, argv[itr], false);
		if (! inSw)
		{
			continue;
		}

		switch (inSw->in_sw)
		{
		case IN_SW_BURP_C:				// create database
		case IN_SW_BURP_R:				// replace database
		case IN_SW_BURP_RECREATE:		// recreate database
			flag_restore = true;
			break;
		case IN_SW_BURP_USER:			// default user name
		case IN_SW_BURP_PASS:			// default password
		case IN_SW_BURP_SE:				// service name
			if (itr >= argc - 1)
			{
				// user name parameter missing
				BURP_error(inSw->in_sw == IN_SW_BURP_USER ? 188 :
								// password parameter missing
						   		inSw->in_sw == IN_SW_BURP_PASS ? 189 :
									// service name parameter missing
									273, true);
			}
			else
			{
				argv[itr++] = 0;
				switch (inSw->in_sw)
				{
				case IN_SW_BURP_USER:			// default user name
					usr = argv[itr];
					break;
				case IN_SW_BURP_PASS:			// default password
					pswd = argv[itr];
					uSvc->hidePasswd(argv, itr);
					break;
				case IN_SW_BURP_SE:				// service name
					service = argv[itr];
					break;
				}
				argv[itr] = 0;
			}
			break;
		case IN_SW_BURP_V:				// verify actions
			flag_verbose = true;
			break;
#ifdef TRUSTED_AUTH
		case IN_SW_BURP_TRUSTED_USER:	// use trusted auth
			flag_trusted = true;
			argv[itr] = 0;
			break;
#endif
		}
	}

	if (usr.isEmpty())
	{
#ifdef TRUSTED_AUTH
		if (!flag_trusted)
#endif
		{
			fb_utils::readenv(ISC_USER, usr);
		}
	}

	if (pswd.isEmpty())
	{
#ifdef TRUSTED_AUTH
		if (!flag_trusted)
#endif
		{
			fb_utils::readenv(ISC_PASSWORD, pswd);
		}
	}

	ISC_STATUS_ARRAY status;
	FB_API_HANDLE svc_handle = 0;

	try
	{
		Firebird::ClumpletWriter spb(Firebird::ClumpletWriter::SpbAttach, 4096, isc_spb_current_version);

		// isc_spb_user_name
		// isc_spb_password
		// isc_spb_trusted_auth
		// isc_spb_options

		if (usr.hasData())
		{
			spb.insertString(isc_spb_user_name, usr);
		}
		if (pswd.hasData())
		{
			spb.insertString(isc_spb_password, pswd);
		}
#ifdef TRUSTED_AUTH
		if (flag_trusted)
		{
			spb.insertTag(isc_spb_trusted_auth);
		}
#endif

		// Fill command line options
		Firebird::string options;
		for (int itr = 1; itr < argc; ++itr)
		{
			if (!argv[itr])
			{
				continue;
			}
			Firebird::UtilSvc::addStringWithSvcTrmntr(argv[itr], options);
		}
		options.rtrim();

		spb.insertString(isc_spb_command_line, options);

		if (isc_service_attach(status, 0, service.c_str(), &svc_handle,
							   spb.getBufferLength(), reinterpret_cast<const char*>(spb.getBuffer())))
		{
			BURP_print_status(true, status);
			BURP_print(true, 83);
			// msg 83 Exiting before completion due to errors
			return FINI_ERROR;
		}

		char thd[2];
		// 'isc_action_svc_restore/isc_action_svc_backup'
		// 'isc_spb_verbose'

		char *thd_ptr = thd;
		if (flag_restore)
			*thd_ptr++ = isc_action_svc_restore;
		else
			*thd_ptr++ = isc_action_svc_backup;

		if (flag_verbose)
			*thd_ptr++ = isc_spb_verbose;

		const USHORT thdlen = thd_ptr - thd;

		if (isc_service_start(status, &svc_handle, NULL, thdlen, thd))
		{
			BURP_print_status(true, status);
			isc_service_detach(status, &svc_handle);
			BURP_print(true, 83);	// msg 83 Exiting before completion due to errors
			return FINI_ERROR;
		}

		const char sendbuf[] = { isc_info_svc_line };
		char respbuf[1024];
		const char* sl;
		do {
			if (isc_service_query(status, &svc_handle, NULL, 0, NULL,
								  sizeof(sendbuf), sendbuf,
								  sizeof(respbuf), respbuf))
			{
				BURP_print_status(true, status);
				isc_service_detach(status, &svc_handle);
				BURP_print(true, 83);	// msg 83 Exiting before completion due to errors
				return FINI_ERROR;
			}

			char* p = respbuf;
			sl = p;

			if (*p++ == isc_info_svc_line)
			{
				const ISC_USHORT len = (ISC_USHORT) isc_vax_integer(p, sizeof(ISC_USHORT));
				p += sizeof(ISC_USHORT);
				if (!len)
				{
					if (*p == isc_info_data_not_ready)
						continue;

					if (*p == isc_info_end)
						break;
				}

				fb_assert(p + len < respbuf + sizeof(respbuf));
				p[len] = '\0';
				burp_output(false, "%s\n", p);
			}
		} while (*sl == isc_info_svc_line);

		isc_service_detach(status, &svc_handle);
		return FINI_OK;
	}
	catch (const Firebird::Exception& e)
	{
		e.stuff_exception(status);
		BURP_print_status(true, status);
		if (svc_handle)
		{
			isc_service_detach(status, &svc_handle);
		}
		BURP_print(true, 83);	// msg 83 Exiting before completion due to errors
		return FINI_ERROR;
	}
}


static bool switchMatch(const Firebird::string& sw, const char* target)
{
/**************************************
 *
 *	s w i t c h M a t c h
 *
 **************************************
 *
 * Functional description
 *	Returns true if switch matches target
 *
 **************************************/
	size_t n = strlen(target);
	if (n < sw.length())
	{
		return false;
	}
	n = sw.length();
	return memcmp(sw.c_str(), target, n) == 0;
}


static in_sw_tab_t* findSwitch(in_sw_tab_t* const table, Firebird::string sw, bool throwErrors)
{
/**************************************
 *
 *	f i n d S w i t c h
 *
 **************************************
 *
 * Functional description
 *	Returns pointer to in_sw_tab entry for current switch
 *	If not a switch, returns 0.
 *	If no match, throws if throwErrors or returns 0.
 *
 **************************************/
	if (sw.isEmpty())
	{
		return 0;
	}
	if (sw[0] != switch_char)
	{
		return 0;
	}
	sw.erase(0, 1);
	sw.upper();

	for (in_sw_tab_t* in_sw_tab = table; in_sw_tab->in_sw_name; in_sw_tab++)
	{
		if (sw.length() < in_sw_tab->in_sw_min_length)
		{
			continue;
		}
		if (switchMatch(sw, in_sw_tab->in_sw_name))
		{
			return in_sw_tab;
		}
	}

	if (throwErrors)
	{
		BURP_print(true, 137, sw.c_str());
		// msg 137  unknown switch %s
		burp_usage(table);
		BURP_error(1, true);
		// msg 1: found unknown switch
	}

	return 0;
}


int gbak(Firebird::UtilSvc* uSvc)
{
/**************************************
 *
 *	g b a k
 *
 **************************************
 *
 * Functional description
 *	Routine called by command line utility and services API.
 *
 **************************************/
	gbak_action action = QUIT;
	int exit_code = FINI_ERROR;

	BurpGlobals sgbl(uSvc);
	BurpGlobals *tdgbl = &sgbl;
	BurpGlobals::putSpecific(tdgbl);

	tdgbl->burp_throw = true;
	tdgbl->file_desc = INVALID_HANDLE_VALUE;

	Firebird::UtilSvc::ArgvType& argv = uSvc->argv;
	const int argc = uSvc->argv.getCount();

	// Copy the static const table to a local array for processing.
	in_sw_tab_t burp_in_sw_table[FB_NELEM(reference_burp_in_sw_table)];

	for (size_t maxt = FB_NELEM(reference_burp_in_sw_table), iter = 0; iter < maxt; ++iter)
	{
		burp_in_sw_table[iter] = reference_burp_in_sw_table[iter];
		burp_in_sw_table[iter].in_sw_state = false;
	}

	// test for "-service" switch
	for (int itr = 1; itr < argc; ++itr)
	{
		const in_sw_tab_t* inSw = findSwitch(burp_in_sw_table, argv[itr], false);

		if (inSw && inSw->in_sw == IN_SW_BURP_SE)
		{
			return api_gbak(uSvc, burp_in_sw_table);
		}
	}

	try {

	in_sw_tab_t* in_sw_tab;

	uSvc->started();

	if (argc <= 1) {
		burp_usage(burp_in_sw_table);
		BURP_exit_local(FINI_ERROR, tdgbl);
	}

	if (argc == 2 && strcmp(argv[1], "-?") == 0)
	{
		burp_usage(burp_in_sw_table);
		BURP_exit_local(FINI_OK, tdgbl);
	}

	USHORT sw_replace = FALSE;

	tdgbl->gbl_sw_compress = true;
	tdgbl->gbl_sw_convert_ext_tables = false;
	tdgbl->gbl_sw_transportable = true;
	tdgbl->gbl_sw_ignore_limbo = false;
	tdgbl->gbl_sw_blk_factor = 0;
	tdgbl->gbl_sw_no_reserve = false;
	tdgbl->gbl_sw_old_descriptions = false;
	tdgbl->gbl_sw_mode = false;
	tdgbl->gbl_sw_skip_count = 0;
	tdgbl->action = NULL;

	burp_fil* file = NULL;
	burp_fil* file_list = NULL;
	tdgbl->io_buffer_size = GBAK_IO_BUFFER_SIZE;

	// Avoid overwriting literal strings.
	char none[] = "-*NONE*";

	for (int itr = 1; itr < argc; ++itr)
	{
		Firebird::string str = argv[itr];
		if (str.isEmpty())
		{
			continue;
		}
		if (str[str.length() - 1] == ',')
		{
			str.erase(str.length() - 1, 1);
		}

		if (str[0] != switch_char)
		{
			if (!file || file->fil_length || !get_size(argv[itr], file))
			{
				// Miserable thing must be a filename
				// (dummy in a length for the backup file

				file = FB_NEW(*getDefaultMemoryPool()) burp_fil(*getDefaultMemoryPool());
				file->fil_name = str.ToPathName();
				file->fil_length = file_list ? 0 : MAX_LENGTH;
				file->fil_next = file_list;
				file_list = file;
			}
			continue;
		}

		if (str.length() == 1)
		{
			str = none;
		}

		in_sw_tab = findSwitch(burp_in_sw_table, str, true);
		fb_assert(in_sw_tab);
		in_sw_tab->in_sw_state = true;

		if (in_sw_tab->in_sw == IN_SW_BURP_RECREATE)
		{
			int real_sw = IN_SW_BURP_C;
			if ((itr < argc - 1) && (*argv[itr + 1] != switch_char))
			{
				// find optional BURP_SW_OVERWRITE parameter
				Firebird::string next(argv[itr + 1]);
				next.upper();
				if (strstr(BURP_SW_OVERWRITE, next.c_str()) == BURP_SW_OVERWRITE)
				{
					real_sw = IN_SW_BURP_R;
					itr++;
				}
			}

			// replace IN_SW_BURP_RECREATE by IN_SW_BURP_R or IN_SW_BURP_C
			in_sw_tab->in_sw_state = false;

			in_sw_tab_t* real_sw_tab = burp_in_sw_table;
			while (real_sw_tab->in_sw != real_sw)
				++real_sw_tab;

			real_sw_tab->in_sw_state = true;
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_S)
		{
			if (++itr >= argc)
			{
				BURP_error(200, true);
				// msg 200: missing parameter for the number of bytes to be skipped
			}
			tdgbl->gbl_sw_skip_count = get_number(argv[itr]);
			if (!tdgbl->gbl_sw_skip_count)
			{
				BURP_error(201, true, argv[itr]);
				// msg 201: expected number of bytes to be skipped, encountered "%s"
			}
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_P)
		{
			if (++itr >= argc)
			{
				BURP_error(2, true);
				// msg 2 page size parameter missing
			}
			tdgbl->gbl_sw_page_size = (USHORT) get_number(argv[itr]);
			if (!tdgbl->gbl_sw_page_size)
			{
				BURP_error(12, true, argv[itr]);
				// msg 12 expected page size, encountered "%s"
			}
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_BU)
		{
			if (++itr >= argc)
			{
				BURP_error(258, true);
				// msg 258 page buffers parameter missing
			}
			tdgbl->gbl_sw_page_buffers = get_number(argv[itr]);
			if (!tdgbl->gbl_sw_page_buffers)
			{
				BURP_error(259, true, argv[itr]);
				// msg 259 expected page buffers, encountered "%s"
			}
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_MODE)
		{
			if (++itr >= argc)
			{
				BURP_error(279, true);
				// msg 279: "read_only" or "read_write" required
			}
			str = argv[itr];
			str.lower();
			if (str == BURP_SW_MODE_RO)
				tdgbl->gbl_sw_mode_val = true;
			else if (str == BURP_SW_MODE_RW)
				tdgbl->gbl_sw_mode_val = false;
			else
			{
				BURP_error(279, true);
				// msg 279: "read_only" or "read_write" required
			}
			tdgbl->gbl_sw_mode = true;
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_PASS)
		{
			if (++itr >= argc)
			{
				BURP_error(189, true);
				// password parameter missing
			}
			uSvc->hidePasswd(argv, itr);
			if (tdgbl->gbl_sw_password)
			{
				BURP_error(307, true);
				// too many passwords provided
			}
			tdgbl->gbl_sw_password = argv[itr];
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_FETCHPASS)
		{
			if (++itr >= argc)
			{
				BURP_error(189, true);
				// password parameter missing
			}
			if (tdgbl->gbl_sw_password)
			{
				BURP_error(307, true);
				// too many passwords provided
			}
			switch (fb_utils::fetchPassword(argv[itr], tdgbl->gbl_sw_password))
			{
			case fb_utils::FETCH_PASS_OK:
				break;
			case fb_utils::FETCH_PASS_FILE_OPEN_ERROR:
				BURP_error(308, true, MsgFormat::SafeArg() << argv[itr] << errno);
				// error @2 opening password file @1
				break;
			case fb_utils::FETCH_PASS_FILE_READ_ERROR:
				BURP_error(309, true, MsgFormat::SafeArg() << argv[itr] << errno);
				// error @2 reading password file @1
				break;
			case fb_utils::FETCH_PASS_FILE_EMPTY:
				BURP_error(310, true, MsgFormat::SafeArg() << argv[itr]);
				// password file @1 is empty
				break;
			}
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_USER)
		{
			if (++itr >= argc)
			{
				BURP_error(188, true);
				// user name parameter missing
			}
			tdgbl->gbl_sw_user = argv[itr];
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_TRUSTED_SVC)
		{
			uSvc->checkService();
			if (++itr >= argc)
			{
				BURP_error(188, true);
				// trusted user name parameter missing
			}
			tdgbl->gbl_sw_tr_user = argv[itr];
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_ROLE)
		{
			if (++itr >= argc)
			{
				BURP_error(253, true);
				// SQL role parameter missing
			}
			tdgbl->gbl_sw_sql_role = argv[itr];
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_FA)
		{
			if (++itr >= argc)
			{
				BURP_error(182, true);
				// msg 182 blocking factor parameter missing
			}
			tdgbl->gbl_sw_blk_factor = get_number(argv[itr]);
			if (!tdgbl->gbl_sw_blk_factor)
			{
				BURP_error(183, true, argv[itr]);
				// msg 183 expected blocking factor, encountered "%s"
			}
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_FIX_FSS_DATA)
		{
			if (++itr >= argc)
			{
				BURP_error(304, true);
				// Character set parameter missing
			}
			tdgbl->gbl_sw_fix_fss_data = argv[itr];
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_FIX_FSS_METADATA)
		{
			if (++itr >= argc)
			{
				BURP_error(304, true);
				// Character set parameter missing
			}
			tdgbl->gbl_sw_fix_fss_metadata = argv[itr];
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_SE)
		{
			if (++itr >= argc)
			{
				BURP_error(273, true);
				// msg 273: service name parameter missing
			}
			// skip a service specification
			in_sw_tab->in_sw_state = false;
		}
		else if (in_sw_tab->in_sw == IN_SW_BURP_Y)
		{
			// want to do output redirect handling now instead of waiting
			const TEXT* redirect = NULL;
			if (++itr < argc)
			{
				redirect = argv[itr];
				if (*redirect == switch_char)
				{
					redirect = NULL;
				}
			}
			if (!redirect)
			{
				BURP_error(4, true);
				// msg 4 redirect location for output is not specified
			}

			Firebird::string up(redirect);
			up.upper();
			tdgbl->sw_redirect = (up == output_suppress) ? NOOUTPUT : REDIRECT;

			if (tdgbl->sw_redirect == REDIRECT)		// not NOREDIRECT, and not NOOUTPUT
			{
				// Make sure the status file doesn't already exist
				FILE* tmp_outfile = fopen(redirect, fopen_read_type);
				if (tmp_outfile) {
					BURP_print(true, 66, redirect);
					// msg 66 can't open status and error output file %s
					fclose(tmp_outfile);
					BURP_exit_local(FINI_ERROR, tdgbl);
				}
				if (! (tdgbl->output_file = fopen(redirect, fopen_write_type)))
				{
					BURP_print(true, 66, redirect);
					// msg 66 can't open status and error output file %s
					BURP_exit_local(FINI_ERROR, tdgbl);
				}
			}
		}					//else if (in_sw_tab->in_sw == IN_SW_BURP_Y)
	}						// for

	// reverse the linked list of file blocks

	tdgbl->gbl_sw_files = NULL;

	burp_fil* next_file = NULL;
	for (file = file_list; file; file = next_file) {
		next_file = file->fil_next;
		file->fil_next = tdgbl->gbl_sw_files;
		tdgbl->gbl_sw_files = file;
	}

	// pop off the obviously boring ones, plus do some checking

	const TEXT* file1 = NULL;
	const TEXT* file2 = NULL;
	for (file = tdgbl->gbl_sw_files; file; file = file->fil_next)
	{
		if (!file1)
			file1 = file->fil_name.c_str();
		else if (!file2)
			file2 = file->fil_name.c_str();
		for (file_list = file->fil_next; file_list;
			 file_list = file_list->fil_next)
		{
			if (file->fil_name == file_list->fil_name)
			{
				BURP_error(9, true);
				// msg 9 mutiple sources or destinations specified
			}
		}

	}

	// Initialize 'dpb'
	Firebird::ClumpletWriter dpb(Firebird::ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);

	dpb.insertString(isc_dpb_gbak_attach, GDS_VERSION, strlen(GDS_VERSION));
	dpb.insertByte(isc_dpb_gsec_attach, 1);		// make it possible to have local security backups
	uSvc->getAddressPath(dpb);

	for (in_sw_tab = burp_in_sw_table; in_sw_tab->in_sw_name; in_sw_tab++)
	{
		if (!in_sw_tab->in_sw_state)
			continue;

		switch (in_sw_tab->in_sw)
		{
		case IN_SW_BURP_B:
			if (sw_replace)
				BURP_error(5, true);
				// msg 5 conflicting switches for backup/restore
			sw_replace = IN_SW_BURP_B;
			break;

		case IN_SW_BURP_C:
			if (sw_replace == IN_SW_BURP_B)
				BURP_error(5, true);
				// msg 5 conflicting switches for backup/restore
			if (sw_replace != IN_SW_BURP_R)
				sw_replace = IN_SW_BURP_C;
			break;

		case IN_SW_BURP_CO:
			tdgbl->gbl_sw_convert_ext_tables = true;
			break;

		case IN_SW_BURP_E:
			tdgbl->gbl_sw_compress = false;
			break;

		case IN_SW_BURP_G:
			dpb.insertTag(isc_dpb_no_garbage_collect);
			break;

		case IN_SW_BURP_I:
			tdgbl->gbl_sw_deactivate_indexes = true;
			break;

		case IN_SW_BURP_IG:
			dpb.insertByte(isc_dpb_damaged, 1);
			break;

		case IN_SW_BURP_K:
			tdgbl->gbl_sw_kill = true;
			break;

		case IN_SW_BURP_L:
			tdgbl->gbl_sw_ignore_limbo = true;
			break;

		case IN_SW_BURP_M:
			tdgbl->gbl_sw_meta = true;
			break;

		case IN_SW_BURP_MODE:
			tdgbl->gbl_sw_mode = true;
			break;

		case IN_SW_BURP_N:
			tdgbl->gbl_sw_novalidity = true;
			break;

		case IN_SW_BURP_NOD:
			dpb.insertByte(isc_dpb_no_db_triggers, 1);
			break;

		case IN_SW_BURP_NT:	// Backup non-transportable format
			tdgbl->gbl_sw_transportable = false;
			break;

		case IN_SW_BURP_O:
			tdgbl->gbl_sw_incremental = true;
			break;

		case IN_SW_BURP_OL:
			tdgbl->gbl_sw_old_descriptions = true;
			break;

		case IN_SW_BURP_PASS:
		case IN_SW_BURP_FETCHPASS:
			dpb.insertString(tdgbl->uSvc->isService() ? isc_dpb_password_enc : isc_dpb_password,
							 tdgbl->gbl_sw_password, strlen(tdgbl->gbl_sw_password));
			break;

		case IN_SW_BURP_R:
			if (sw_replace == IN_SW_BURP_B)
				BURP_error(5, true);
				// msg 5 conflicting switches for backup/restore
			sw_replace = IN_SW_BURP_R;
			break;

		case IN_SW_BURP_T:
			tdgbl->gbl_sw_transportable = true;
			break;

		/*
		case IN_SW_BURP_U:
			BURP_error(7, true);
			// msg 7 protection isn't there yet
			break;
		*/

		case IN_SW_BURP_US:
			tdgbl->gbl_sw_no_reserve = true;
			break;

		case IN_SW_BURP_ROLE:
			dpb.insertString(isc_dpb_sql_role_name,
							 tdgbl->gbl_sw_sql_role, strlen(tdgbl->gbl_sw_sql_role));
			break;

		case IN_SW_BURP_USER:
			dpb.insertString(isc_dpb_user_name, tdgbl->gbl_sw_user, strlen(tdgbl->gbl_sw_user));
			break;

		case IN_SW_BURP_TRUSTED_SVC:
			uSvc->checkService();
			dpb.deleteWithTag(isc_dpb_trusted_auth);
			dpb.insertString(isc_dpb_trusted_auth, tdgbl->gbl_sw_tr_user, strlen(tdgbl->gbl_sw_tr_user));
			break;

		case IN_SW_BURP_TRUSTED_ROLE:
			uSvc->checkService();
			dpb.deleteWithTag(isc_dpb_trusted_role);
			dpb.insertString(isc_dpb_trusted_role, ADMIN_ROLE, strlen(ADMIN_ROLE));
			break;
#ifdef TRUSTED_AUTH
		case IN_SW_BURP_TRUSTED_USER:
			if (!dpb.find(isc_dpb_trusted_auth))
			{
				dpb.insertTag(isc_dpb_trusted_auth);
			}
			break;
#endif
		case IN_SW_BURP_V:
			tdgbl->gbl_sw_verbose = true;
			break;

		case IN_SW_BURP_Z:
			BURP_print(false, 91, GDS_VERSION);
			// msg 91 gbak version %s
			tdgbl->gbl_sw_version = true;
			break;

		default:
			break;
		}
	}

	if (!sw_replace)
		sw_replace = IN_SW_BURP_B;

	if (sw_replace == IN_SW_BURP_B)
	{
		for (burp_fil* f = tdgbl->gbl_sw_files; f; f = f->fil_next)
		{
			if (f->fil_name == "stdout")
			{
				// the very first thing to do not to corrupt backup file...
				tdgbl->uSvc->setDataMode(true);
			}
		}
	}

	if (tdgbl->gbl_sw_page_size)
	{
		if (sw_replace == IN_SW_BURP_B)
			BURP_error(8, true); // msg 8 page size is allowed only on restore or create
		int temp = tdgbl->gbl_sw_page_size;
		for (int curr_pg_size = MIN_NEW_PAGE_SIZE; curr_pg_size <= MAX_PAGE_SIZE; curr_pg_size <<= 1)
		{
			if (temp <= curr_pg_size)
			{
				temp = curr_pg_size;
				break;
			}
		}
		if (temp > MAX_PAGE_SIZE)
		{
			BURP_error(3, true, SafeArg() << tdgbl->gbl_sw_page_size);
			// msg 3 Page size specified (%ld) greater than limit (MAX_PAGE_SIZE bytes)
		}
		if (temp != tdgbl->gbl_sw_page_size) {
			BURP_print(false, 103, SafeArg() << tdgbl->gbl_sw_page_size << temp);
			// msg 103 page size specified (%ld bytes) rounded up to %ld bytes
			tdgbl->gbl_sw_page_size = temp;
		}
	}

	if (tdgbl->gbl_sw_page_buffers) {
		if (sw_replace == IN_SW_BURP_B)
			BURP_error(260, true);
			// msg 260 page buffers is allowed only on restore or create
	}

	if (!tdgbl->gbl_sw_blk_factor || sw_replace != IN_SW_BURP_B)
		tdgbl->gbl_sw_blk_factor = 1;

	if (!file2)
		BURP_error(10, true);
		// msg 10 requires both input and output filenames

	if (!strcmp(file1, file2))
		BURP_error(11, true);
		// msg 11 input and output have the same name.  Disallowed.

	{ // scope
		// The string result produced by ctime contains exactly 26 characters and
		// gbl_backup_start_time is TEXT[30], but let's make sure we don't overflow
		// due to any change.
		const time_t clock = time(NULL);
		fb_utils::copy_terminate(tdgbl->gbl_backup_start_time, ctime(&clock),
			sizeof(tdgbl->gbl_backup_start_time));
		TEXT* nlp = tdgbl->gbl_backup_start_time + strlen(tdgbl->gbl_backup_start_time) - 1;
		if (*nlp == '\n')
			*nlp = 0;
	} // scope

	tdgbl->action = (burp_act*) BURP_alloc_zero(ACT_LEN);
	tdgbl->action->act_total = 0;
	tdgbl->action->act_file = NULL;
	tdgbl->action->act_action = ACT_unknown;

	action = open_files(file1, &file2, tdgbl->gbl_sw_verbose, sw_replace, dpb);

	MVOL_init(tdgbl->io_buffer_size);

	int result;

	tdgbl->uSvc->started();
	switch (action)
	{
	case RESTORE:
		tdgbl->gbl_sw_overwrite = (sw_replace == IN_SW_BURP_R);
		result = RESTORE_restore(file1, file2);
		break;

	case BACKUP:
		result = BACKUP_backup(file1, file2);
		break;

	case QUIT:
		BURP_abort();
		return 0;

	default:
		// result undefined
		fb_assert(false);
		return 0;
	}
	if (result != FINI_OK && result != FINI_DB_NOT_ONLINE)
		BURP_abort();

	BURP_exit_local(result, tdgbl);
	}	// try

	catch (const Firebird::LongJump&)
	{
		// All calls to exit_local(), normal and error exits, wind up here
		tdgbl->burp_throw = false;
		if (tdgbl->action && tdgbl->action->act_action == ACT_backup_fini)
		{
			tdgbl->exit_code = 0;
		}
		exit_code = tdgbl->exit_code;
	}

	catch (const Firebird::Exception& e)
	{
		// Non-burp exception was caught
		tdgbl->burp_throw = false;
		e.stuff_exception(tdgbl->status_vector);
		BURP_print_status(true, tdgbl->status_vector);
		BURP_print(true, 83);	// msg 83 Exiting before completion due to errors
		exit_code = FINI_ERROR;
	}

	// Close the gbak file handles if they still open
	for (burp_fil* file = tdgbl->gbl_sw_backup_files; file; file = file->fil_next)
	{
		if (file->fil_fd != GBAK_STDIN_DESC() && file->fil_fd != GBAK_STDOUT_DESC())
		{
			if (file->fil_fd != INVALID_HANDLE_VALUE)
			{
				close_platf(file->fil_fd);
			}
			if (exit_code != FINI_OK &&
				(tdgbl->action->act_action == ACT_backup_split || tdgbl->action->act_action == ACT_backup))
			{
				unlink_platf(file->fil_name.c_str());
			}
		}
	}

	// Detach from database to release system resources
	if (tdgbl->db_handle != 0) {
		close_out_transaction(action, &tdgbl->tr_handle);
		close_out_transaction(action, &tdgbl->global_trans);
		if (isc_detach_database(tdgbl->status_vector, &tdgbl->db_handle))
		{
			BURP_print_status(true, tdgbl->status_vector);
		}
	}

	// Close the status output file
	if (tdgbl->sw_redirect == REDIRECT && tdgbl->output_file != NULL) {
		fclose(tdgbl->output_file);
		tdgbl->output_file = NULL;
	}

	// Free all unfreed memory used by GBAK itself
	while (tdgbl->head_of_mem_list != NULL) {
		UCHAR* mem = tdgbl->head_of_mem_list;
		tdgbl->head_of_mem_list = *((UCHAR **) tdgbl->head_of_mem_list);
		gds__free(mem);
	}

	BurpGlobals::restoreSpecific();

#if defined(DEBUG_GDS_ALLOC)
	if (!uSvc->isService())
	{
		gds_alloc_report(0, __FILE__, __LINE__);
	}
#endif

	return exit_code;
}



void BURP_abort()
{
/**************************************
 *
 *	B U R P _ a b o r t
 *
 **************************************
 *
 * Functional description
 *	Abandon a failed operation.
 *
 **************************************/
	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	BURP_print(true, tdgbl->action && tdgbl->action->act_action == ACT_backup_fini ? 351 : 83);
	// msg 351 Error closing database, but backup file is OK
	// msg 83 Exiting before completion due to errors

	tdgbl->uSvc->started();

	BURP_exit_local(FINI_ERROR, tdgbl);
}

void BURP_error(USHORT errcode, bool abort, const SafeArg& arg)
{
/**************************************
 *
 *      B U R P _ e r r o r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	tdgbl->uSvc->setServiceStatus(burp_msg_fac, errcode, arg);
	tdgbl->uSvc->started();

	if (!tdgbl->uSvc->isService())
	{
		BURP_msg_partial(true, 256);	// msg 256: gbak: ERROR:
		BURP_msg_put(true, errcode, arg);
	}

	if (abort)
	{
		BURP_abort();
	}
}


void BURP_error(USHORT errcode, bool abort, const char* str)
{
/**************************************
 *
 *	 B U R P _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Format and print an error message, then punt.
 *
 **************************************/

	BURP_error(errcode, abort, SafeArg() << str);
}


void BURP_error_redirect(const ISC_STATUS* status_vector, USHORT errcode, const SafeArg& arg)
{
/**************************************
 *
 *	B U R P _ e r r o r _ r e d i r e c t
 *
 **************************************
 *
 * Functional description
 *	Issue error message. Output messages then abort.
 *
 **************************************/

	BURP_print_status(true, status_vector);
	BURP_error(errcode, true, arg);
}


// Raises an exception when the old SEH system would jump to another place.
void BURP_exit_local(int code, BurpGlobals* tdgbl)
{
	tdgbl->exit_code = code;
	if (tdgbl->burp_throw)
		throw Firebird::LongJump();
}


void BURP_msg_partial(bool err, USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	B U R P _ m s g _ p a r t i a l
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file,
 *	format it, and print it without a newline.
 *
 **************************************/
	TEXT buffer[256];

	fb_msg_format(NULL, burp_msg_fac, number, sizeof(buffer), buffer, arg);
	burp_output(err, "%s", buffer);
}


void BURP_msg_put(bool err, USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	B U R P _ m s g _ p u t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it.
 *
 **************************************/
	TEXT buffer[256];

	fb_msg_format(NULL, burp_msg_fac, number, sizeof(buffer), buffer, arg);
	burp_output(err, "%s\n", buffer);
}


void BURP_msg_get(USHORT number, TEXT* output_msg, const SafeArg& arg)
{
/**************************************
 *
 *	B U R P _ m s g _ g e t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it and copy it to the buffer
 *
 **************************************/
	TEXT buffer[BURP_MSG_GET_SIZE];

	fb_msg_format(NULL, burp_msg_fac, number, sizeof(buffer), buffer, arg);
	strcpy(output_msg, buffer);
}


void BURP_output_version(void* arg1, const TEXT* arg2)
{
/**************************************
 *
 *	B U R P _ o u t p u t _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Callback routine for access method
 *	printing (specifically show version);
 *	will accept.
 *
 **************************************/

	burp_output(false, static_cast<const char*>(arg1), arg2);
}


void BURP_print(bool err, USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	B U R P _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Display a formatted error message
 *	in a way that VMS or civilized systems
 *	will accept.
 *
 **************************************/

	BURP_msg_partial(err, 169);	// msg 169: gbak:
	BURP_msg_put(err, number, arg);
}


void BURP_print(bool err, USHORT number, const char* str)
{
/**************************************
 *
 *	B U R P _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Display a formatted error message
 *	in a way that VMS or civilized systems
 *	will accept.
 *
 **************************************/

	static const SafeArg dummy;
	BURP_msg_partial(err, 169, dummy);	// msg 169: gbak:
	BURP_msg_put(err, number, SafeArg() << str);
}


void BURP_print_status(bool err, const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	B U R P _ p r i n t _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Print error message. Use fb_interpret
 *	to allow redirecting output.
 *
 **************************************/
	if (status_vector) {
		const ISC_STATUS* vector = status_vector;

		if (err) {
			BurpGlobals* tdgbl = BurpGlobals::getSpecific();
			tdgbl->uSvc->setServiceStatus(vector);
			tdgbl->uSvc->started();
			if (tdgbl->uSvc->isService())
			{
				return;
			}
		}

        SCHAR s[1024];
		if (fb_interpret(s, sizeof(s), &vector)) {
			BURP_msg_partial(err, 256); // msg 256: gbak: ERROR:
			burp_output(err, "%s\n", s);
			while (fb_interpret(s, sizeof(s), &vector)) {
				BURP_msg_partial(err, 256); // msg 256: gbak: ERROR:
				burp_output(err, "    %s\n", s);
			}
		}
	}
}


void BURP_print_warning(const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	B U R P _ p r i n t _ w a r n i n g
 *
 **************************************
 *
 * Functional description
 *	Print warning message. Use fb_interpret
 *	to allow redirecting output.
 *
 **************************************/
	if (status_vector)
	{
		// skip the error, assert that one does not exist
		fb_assert(status_vector[0] == isc_arg_gds);
		fb_assert(status_vector[1] == 0);
		// print the warning message
		const ISC_STATUS* vector = &status_vector[2];
		SCHAR s[1024];
		if (fb_interpret(s, sizeof(s), &vector)) {
			BURP_msg_partial(true, 255); // msg 255: gbak: WARNING:
			burp_output(true, "%s\n", s);
			while (fb_interpret(s, sizeof(s), &vector)) {
				BURP_msg_partial(true, 255); // msg 255: gbak: WARNING:
				burp_output(true, "    %s\n", s);
			}
		}
	}
}


void BURP_verbose(USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	B U R P _ v e r b o s e
 *
 **************************************
 *
 * Functional description
 *	Calls BURP_print for displaying a formatted error message
 *	but only for verbose output.  If not verbose then calls
 *	user defined yielding function.
 *
 **************************************/
	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	if (tdgbl->gbl_sw_verbose)
		BURP_print(false, number, arg);
	else
		burp_output(false, "%s", "");
}


void BURP_verbose(USHORT number, const char* str)
{
/**************************************
 *
 *	B U R P _ v e r b o s e
 *
 **************************************
 *
 * Functional description
 *	Calls BURP_print for displaying a formatted error message
 *	but only for verbose output.  If not verbose then calls
 *	user defined yieding function.
 *
 **************************************/
	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	if (tdgbl->gbl_sw_verbose)
		BURP_print(false, number, str);
	else
		burp_output(false, "%s", "");
}


static void close_out_transaction(gbak_action action, isc_tr_handle* handle)
{
/**************************************
 *
 *	c l o s e _ o u t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Make a transaction go away.  This is
 *	important as we need to detach from the
 *	database so all engine allocated memory is
 *	returned to the system.
 *
 **************************************/
	if (*handle != 0)
	{
		ISC_STATUS_ARRAY status_vector;
		if (action == RESTORE)
		{
			// Even if the restore failed, commit the transaction so that
			// a partial database is at least recovered.
			isc_commit_transaction(status_vector, handle);
			if (status_vector[1]) {
				// If we can't commit - have to roll it back, as
				// we need to close all outstanding transactions before
				// we can detach from the database.
				isc_rollback_transaction(status_vector, handle);
				if (status_vector[1])
					BURP_print_status(false, status_vector);
			}
		}
		else
		{
			// A backup shouldn't touch any data - we ensure that
			// by never writing data during a backup, but let's double
			// ensure it by doing a rollback
			if (isc_rollback_transaction(status_vector, handle))
				BURP_print_status(false, status_vector);
		}
	}
}


static SLONG get_number( const SCHAR* string)
{
/**************************************
 *
 *	g e t _ n u m b e r
 *
 **************************************
 *
 * Functional description
 *	Convert a string to binary, complaining bitterly if
 *	the string is bum.
 *  CVC: where does it complain? It does return zero, nothing else.
 **************************************/
	SCHAR c;
	SLONG value = 0;

	for (const SCHAR* p = string; c = *p++;) {
		if (c < '0' || c > '9')
			return 0;
		value *= 10;
		value += c - '0';
	}

	return value;
}


static gbak_action open_files(const TEXT* file1,
							  const TEXT** file2,
							  bool sw_verbose,
							  USHORT sw_replace,
							  const Firebird::ClumpletWriter& dpb)
{
/**************************************
 *
 *	o p e n _ f i l e s
 *
 **************************************
 *
 * Functional description
 *	from the input file names and
 *	positions, guess what the users
 *	intention was.  Return the type
 *	of the first file, plus open file
 *	and db handle.
 *
 **************************************/
	BurpGlobals* tdgbl = BurpGlobals::getSpecific();
	ISC_STATUS_ARRAY temp_status;
	ISC_STATUS* status_vector = temp_status;


	// try to attach the database using the first file_name

	if (sw_replace != IN_SW_BURP_C && sw_replace != IN_SW_BURP_R)
	{
		if (!isc_attach_database(status_vector,
								  (SSHORT) 0, file1,
								  &tdgbl->db_handle,
								  dpb.getBufferLength(),
								  reinterpret_cast<const char*>(dpb.getBuffer())))
		{
			if (sw_replace != IN_SW_BURP_B) {
				// msg 13 REPLACE specified, but the first file %s is a database
				BURP_error(13, true, file1);
				if (isc_detach_database(status_vector, &tdgbl->db_handle)) {
					BURP_print_status(true, status_vector);
				}
				return QUIT;
			}
			if (tdgbl->gbl_sw_version) {
				// msg 139 Version(s) for database "%s"
				BURP_print(false, 139, file1);
				isc_version(&tdgbl->db_handle, BURP_output_version, (void*) "\t%s\n");
			}
			if (sw_verbose)
				BURP_print(false, 166, file1);
				// msg 166: readied database %s for backup
		}
		else if (sw_replace == IN_SW_BURP_B ||
			(status_vector[1] != isc_io_error && status_vector[1] != isc_bad_db_format))
		{
			BURP_print_status(true, status_vector);
			return QUIT;
		}
	}

	burp_fil* fil = 0;
	if (sw_replace == IN_SW_BURP_B)
	{

		// Now it is safe to skip a db file
		tdgbl->gbl_sw_backup_files = tdgbl->gbl_sw_files->fil_next;
		tdgbl->gbl_sw_files = tdgbl->gbl_sw_files->fil_next;
		fb_assert(tdgbl->gbl_sw_files->fil_name == *file2);

		gbak_action flag = BACKUP;
		tdgbl->action->act_action = ACT_backup;
		for (fil = tdgbl->gbl_sw_files; fil; fil = fil->fil_next)
		{
			// adjust the file size first
			switch (fil->fil_size_code)
			{
			case size_n:
				break;
			case size_k:
				fil->fil_length *= KBYTE;
				break;
			case size_m:
				fil->fil_length *= MBYTE;
				break;
			case size_g:
				fil->fil_length *= GBYTE;
				break;
			case size_e:
				BURP_error(262, true, fil->fil_name.c_str());
				// msg 262 size specification either missing or incorrect for file %s
				break;
			default:
				fb_assert(FALSE);
				break;
			}

			if ((fil->fil_seq = ++tdgbl->action->act_total) >= 2)
			{
				tdgbl->action->act_action = ACT_backup_split;
			}
			if (sw_verbose)
			{
				BURP_print(false, 75, fil->fil_name.c_str());	// msg 75  creating file %s
			}
			if (fil->fil_name == "stdout")
			{
				if (tdgbl->action->act_total >= 2 || fil->fil_next || sw_verbose)
				{
					BURP_error(266, true);
					// msg 266 standard output is not supported when using split operation or in verbose mode
					flag = QUIT;
					break;
				}

				// We ignore SIGPIPE so that we can report an IO error when we
				// try to write to the broken pipe.
#ifndef WIN_NT
				signal(SIGPIPE, SIG_IGN);
#endif
				tdgbl->uSvc->setDataMode(true);
				fil->fil_fd = GBAK_STDOUT_DESC();
				tdgbl->stdIoMode = true;
				break;
			}
			else
			{

#ifdef WIN_NT
				if ((fil->fil_fd = MVOL_open(fil->fil_name.c_str(), MODE_WRITE, CREATE_ALWAYS)) ==
					INVALID_HANDLE_VALUE)
#else
				if ((fil->fil_fd = open(fil->fil_name.c_str(), MODE_WRITE, open_mask)) == -1)
#endif // WIN_NT

				{

					BURP_error(65, false, fil->fil_name.c_str());
					// msg 65 can't open backup file %s
					flag = QUIT;
					break;
				}
			}

			if (fil->fil_length == 0)
			{
				if (fil->fil_next)
				{
					BURP_error(262, true, fil->fil_name.c_str());
					// msg 262 size specification either missing or incorrect for file %s
					flag = QUIT;
					break;
				}
				else
				{
					fil->fil_length = MAX_LENGTH;
					// Write as much as possible to the last file
				}
			}
			if (fil->fil_length < MIN_SPLIT_SIZE)
			{
				BURP_error(271, true, SafeArg() << fil->fil_length << MIN_SPLIT_SIZE);
				// msg file size given (%d) is less than minimum allowed (%d)
				flag = QUIT;
				break;
			}
		}

		if (flag == BACKUP)
		{
			tdgbl->action->act_file = tdgbl->gbl_sw_files;
			tdgbl->file_desc = tdgbl->gbl_sw_files->fil_fd;
		}
		else
		{
			if (isc_detach_database(status_vector, &tdgbl->db_handle))
			{
				BURP_print_status(false, status_vector);
			}
		}

		return flag;
	}


	// If we got to here, then we're really not backing up a database,
	// so open a backup file.

	// There are four possible cases such as:
	//
	//   1. restore single backup file to single db file
	//   2. restore single backup file to multiple db files
	//   3. restore multiple backup files (join operation) to single db file
	//   4. restore multiple backup files (join operation) to multiple db files
	//
	// Just looking at the command line, we can't say for sure whether it is a
	// specification of the last file to be join or it is a specification of the
	// primary db file (case 4), for example:
	//
	//     gbak -c gbk1 gbk2 gbk3 db1 200 db2 500 db3 -v
	//                            ^^^
	//     db1 could be either the last file to be join or primary db file
	//
	// Since 'gbk' and 'gsplit' formats are different (gsplit file has its own
	// header record) hence we can use it as follows:
	//
	//     - open first file
	//     - read & check a header record
	//
	// If a header is identified as a 'gsplit' one then we know exactly how
	// many files need to be join and in which order. We keep opening a file by
	// file till we reach the last one to be join. During this step we check
	// that the files are accessible and are in proper order. It gives us
	// possibility to let silly customer know about an error as soon as possible.
	// Besides we have to find out which file is going to be a db file.
	//
	// If header is not identified as a 'gsplit' record then we assume that
	// we got a single backup file.

	fil = tdgbl->gbl_sw_files;
	tdgbl->gbl_sw_backup_files = tdgbl->gbl_sw_files;

	tdgbl->action->act_action = ACT_restore;
	if (fil->fil_name == "stdin")
	{
		fil->fil_fd = GBAK_STDIN_DESC();
		tdgbl->file_desc = fil->fil_fd;
		tdgbl->stdIoMode = true;
		tdgbl->gbl_sw_files = fil->fil_next;
	}
	else
	{
		tdgbl->stdIoMode = false;

		// open first file
#ifdef WIN_NT
		if ((fil->fil_fd = MVOL_open(fil->fil_name.c_str(), MODE_READ, OPEN_EXISTING)) ==
			INVALID_HANDLE_VALUE)
#else
		if ((fil->fil_fd = open(fil->fil_name.c_str(), MODE_READ)) == INVALID_HANDLE_VALUE)
#endif
		{
			BURP_error(65, true, fil->fil_name.c_str());
			// msg 65 can't open backup file %s
			return QUIT;
		}

		if (sw_verbose)
		{
			BURP_print(false, 100, fil->fil_name.c_str());
			// msg 100 opened file %s
		}

		// read and check a header record
		tdgbl->action->act_file = fil;
		int seq = 1;
		if (MVOL_split_hdr_read())
		{
			tdgbl->action->act_action = ACT_restore_join;
			// number of files to be join
			const int total = tdgbl->action->act_total;
			if (fil->fil_seq != seq || seq > total)
			{
				BURP_error(263, true, fil->fil_name.c_str());
				// msg 263 file %s out of sequence
				return QUIT;
			}

			for (++seq, fil = fil->fil_next; seq <= total; fil = fil->fil_next, seq++)
			{
				if (!fil)
				{
					BURP_error(264, true);
					// msg 264 can't join -- one of the files missing
					return QUIT;
				}
				if (fil->fil_name == "stdin")
				{
					BURP_error(265, true);
					// msg 265 standard input is not supported when using join operation
					return QUIT;
				}
				tdgbl->action->act_file = fil;
#ifdef WIN_NT
				if ((fil->fil_fd = MVOL_open(fil->fil_name.c_str(), MODE_READ, OPEN_EXISTING)) ==
					INVALID_HANDLE_VALUE)
#else
				if ((fil->fil_fd = open(fil->fil_name.c_str(), MODE_READ)) == INVALID_HANDLE_VALUE)
#endif
				{
					BURP_error(65, false, fil->fil_name.c_str());
					// msg 65 can't open backup file %s
					return QUIT;
				}

				if (sw_verbose)
				{
					BURP_print(false, 100, fil->fil_name.c_str());
					// msg 100 opened file %s
				}
				if (MVOL_split_hdr_read())
				{
					if ((total != tdgbl->action->act_total) || (seq != fil->fil_seq) || (seq > total))
					{
						BURP_error(263, true, fil->fil_name.c_str());
						// msg 263 file %s out of sequence
						return QUIT;
					}
				}
				else
				{
					BURP_error(267, true, fil->fil_name.c_str());
					// msg 267 backup file %s might be corrupt
					return QUIT;
				}
			}
			tdgbl->action->act_file = tdgbl->gbl_sw_files;
			tdgbl->file_desc = tdgbl->action->act_file->fil_fd;
			if ((tdgbl->gbl_sw_files = fil) == NULL) {
				BURP_error(268, true);
				// msg 268 database file specification missing
				return QUIT;
			}
		}
		else
		{
			// Move pointer to the begining of the file. At this point we
			// assume -- this is a single backup file because we were
			// not able to read a split header record.
#ifdef WIN_NT
			if (strnicmp(fil->fil_name.c_str(), "\\\\.\\tape", 8))
				SetFilePointer(fil->fil_fd, 0, NULL, FILE_BEGIN);
			else
				SetTapePosition(fil->fil_fd, TAPE_REWIND, 0, 0, 0, FALSE);
#else
			lseek(fil->fil_fd, 0, SEEK_SET);
#endif
			tdgbl->file_desc = fil->fil_fd;
			tdgbl->gbl_sw_files = fil->fil_next;
		}
	}


	// If we got here, we've opened a backup file, and we're
	// thinking about creating or replacing a database.

	*file2 = tdgbl->gbl_sw_files->fil_name.c_str();
	if (tdgbl->gbl_sw_files->fil_size_code != size_n)
		BURP_error(262, true, *file2);
		// msg 262 size specification either missing or incorrect for file %s

	if ((sw_replace == IN_SW_BURP_C || sw_replace == IN_SW_BURP_R) &&
		!isc_attach_database(status_vector,
							 (SSHORT) 0, *file2,
							 &tdgbl->db_handle,
							 dpb.getBufferLength(),
							 reinterpret_cast<const char*>(dpb.getBuffer())))
	{
		if (sw_replace == IN_SW_BURP_C) {
			if (isc_detach_database(status_vector, &tdgbl->db_handle)) {
				BURP_print_status(true, status_vector);
			}
			BURP_error(14, true, *file2);
			// msg 14 database %s already exists.  To replace it, use the -R switch
		}
		else {
			isc_drop_database(status_vector, &tdgbl->db_handle);
			if (tdgbl->db_handle) {
				Firebird::makePermanentVector(status_vector);
				ISC_STATUS_ARRAY status_vector2;
				if (isc_detach_database(status_vector2, &tdgbl->db_handle)) {
					BURP_print_status(false, status_vector2);
				}

				// Complain only if the drop database entrypoint is available.
				// If it isn't, the database will simply be overwritten.

				if (status_vector[1] != isc_unavailable)
					BURP_error(233, true, *file2);
				// msg 233 Cannot drop database %s, might be in use
			}
		}
	}
	if (sw_replace == IN_SW_BURP_R && status_vector[1] == isc_adm_task_denied) {
		// if we got an error from attach database and we have replace switch set
		// then look for error from attach returned due to not owner, if we are
		// not owner then return the error status back up

		BURP_error(274, true);
		// msg # 274 : Cannot restore over current database, must be sysdba
		// or owner of the existing database.
	}

	// check the file size specification
	for (fil = tdgbl->gbl_sw_files; fil; fil = fil->fil_next)
	{
		if (fil->fil_size_code != size_n)
		{
			BURP_error(262, true, fil->fil_name.c_str());
			// msg 262 size specification either missing or incorrect for file %s
		}
	}

	return RESTORE;
}


static void burp_output(bool err, const SCHAR* format, ...)
{
/**************************************
 *
 *	b u r p _ o u t p u t
 *
 **************************************
 *
 * Functional description
 *	Platform independent output routine.
 *
 **************************************/
	va_list arglist;

	BurpGlobals* tdgbl = BurpGlobals::getSpecific();

	if (tdgbl->sw_redirect == NOOUTPUT || format[0] == '\0')
	{
		if (err)
			tdgbl->uSvc->outputError("");
		else
			tdgbl->uSvc->outputVerbose("");
	}
	else if (tdgbl->sw_redirect == REDIRECT && tdgbl->output_file != NULL)
	{
		va_start(arglist, format);
		vfprintf(tdgbl->output_file, format, arglist);
		va_end(arglist);
		if (err)
			tdgbl->uSvc->outputError("");
		else
			tdgbl->uSvc->outputVerbose("");
	}
	else
	{
		va_start(arglist, format);
		Firebird::string buf;
		buf.vprintf(format, arglist);
		va_end(arglist);
		if (err)
			tdgbl->uSvc->outputError(buf.c_str());
		else
			tdgbl->uSvc->outputVerbose(buf.c_str());
		fflush(err ? stderr : stdout);
	}
}


static void burp_usage(const in_sw_tab_t* const in_sw_tab)
{
/**********************************************
 *
 *      b u r p _ u s a g e
 *
 **********************************************
 *
 * Functional description
 *	print usage information
 *
 **********************************************/
	const SafeArg sa(SafeArg() << switch_char);
	const SafeArg dummy;

	BURP_print(true, 317); // usage
	for (int i = 318; i < 323; ++i)
		BURP_msg_put(true, i, dummy); // usage

	BURP_print(true, 95); // msg 95  legal switches are
	for (const in_sw_tab_t* p = in_sw_tab; p->in_sw; ++p)
	{
		if (p->in_sw_msg && p->in_sw_optype == boMain)
			BURP_msg_put(true, p->in_sw_msg, sa);
	}

	BURP_print(true, 323); // backup options are
	for (const in_sw_tab_t* p = in_sw_tab; p->in_sw; ++p)
	{
		if (p->in_sw_msg && p->in_sw_optype == boBackup)
			BURP_msg_put(true, p->in_sw_msg, sa);
	}

	BURP_print(true, 324); // restore options are
	for (const in_sw_tab_t* p = in_sw_tab; p->in_sw; ++p)
	{
		if (p->in_sw_msg && p->in_sw_optype == boRestore)
			BURP_msg_put(true, p->in_sw_msg, sa);
	}

	BURP_print(true, 325); // general options are
	for (const in_sw_tab_t* p = in_sw_tab; p->in_sw; ++p)
	{
		if (p->in_sw_msg && p->in_sw_optype == boGeneral)
			BURP_msg_put(true, p->in_sw_msg, sa);
	}

	BURP_print(true, 132); // msg 132 switches can be abbreviated to the unparenthesized characters
}


static ULONG get_size(const SCHAR* string, burp_fil* file)
{
/**********************************************
 *
 *      g e t _ s i z e
 *
 **********************************************
 *
 * Functional description
 *	Get size specification for either splitting or
 *	restoring to multiple files
 *
 **********************************************/
	UCHAR c;
	ULONG size = 0;
	bool digit = false;

	file->fil_size_code = size_n;
	for (const SCHAR *num = string; c = *num++;)
	{
		if (isdigit(c)) {
			size = size * 10 + (c - '0');
			digit = true;
		}
		else
		{
			if (isalpha(c))
			{
				if (!digit) {
					file->fil_size_code = size_e;
					size = 0;
					break;
				}

				switch (UPPER(c))
				{
				case 'K':
					file->fil_size_code = size_k;
					break;
				case 'M':
					file->fil_size_code = size_m;
					break;
				case 'G':
					file->fil_size_code = size_g;
					break;
				default:
					file->fil_size_code = size_e;
					size = 0;
					break;
				}
				if (*num) {
					file->fil_size_code = size_e;
					size = 0;
				}
				break;
			}
		}
	}

	file->fil_length = size;
	return size;
}


#ifndef WIN_NT
void close_platf(DESC file)
{
/**********************************************
 *
 *      c l o s e _ p l a t f
 *
 **********************************************
 *
 * Functional description
 *	Truncate and close file - posix version
 *
 **********************************************/
	if (sizeof(off_t) > 4)		// 64 bit or b4-bit IO in 32 bit OS
	{
#ifndef	O_ACCMODE
// Suppose compatibility with KR where 0, 1, 2 were documented for RO, WO and RW in open() still exists
#define O_ACCMODE 3
#endif
		off_t fileSize = lseek(file, 0, SEEK_CUR);
		if (fileSize != (off_t)(-1))
		{
			ftruncate(file, fileSize);
		}
	}
	close(file);
}
#endif // WIN_NT
