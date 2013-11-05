//____________________________________________________________
//
//		PROGRAM:	Alice (All Else) Utility
//		MODULE:		alice.cpp
//		DESCRIPTION:	Neo-Debe (does everything but eat)
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//
//
//____________________________________________________________
//
// 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
//                         conditionals, as the engine now fully supports
//                         readonly databases.
//
// 2002.10.29 Sean Leyne - Removed support for obsolete "Netware" port
//
// 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
//
//

#include "firebird.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "../jrd/ibase.h"
#include "../jrd/common.h"
#include "../jrd/license.h"
#include "../alice/alice.h"
#include "../alice/aliceswi.h"
#include "../alice/exe_proto.h"
#include "../jrd/msg_encode.h"
#include "../jrd/gds_proto.h"
#include "../jrd/svc.h"
#include "../alice/alice_proto.h"
#include "../common/utils_proto.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

using MsgFormat::SafeArg;


static const USHORT val_err_table[] =
{
	0,
	55,				// msg 55: \n\tNumber of record level errors\t: %ld
	56,				// msg 56: \tNumber of Blob page errors\t: %ld
	57,				// msg 57: \tNumber of data page errors\t: %ld
	58,				// msg 58: \tNumber of index page errors\t: %ld
	59,				// msg 59: \tNumber of pointer page errors\t: %ld
	60,				// msg 60: \tNumber of transaction page errors\t: %ld
	61				// msg 61: \tNumber of database page errors\t: %ld
};


const int ALICE_MSG_FAC = 3;

void ALICE_exit(int code, AliceGlobals* tdgbl)
{
	tdgbl->exit_code = code;
    Firebird::LongJump::raise();
}

static void alice_output(bool error, const SCHAR*, ...) ATTRIBUTE_FORMAT(2,3);



//____________________________________________________________
//
//	Entry point for GFIX in case of service manager.
//

THREAD_ENTRY_DECLARE ALICE_main(THREAD_ENTRY_PARAM arg)
{
	Firebird::UtilSvc* uSvc = (Firebird::UtilSvc*) arg;
	int exit_code = FINI_OK;

	try {
		exit_code = alice(uSvc);
	}
	catch (const Firebird::Exception& e)
	{
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);
		uSvc->setServiceStatus(status);
		uSvc->started();
		exit_code = FB_FAILURE;
	}

	uSvc->finish();
	return (THREAD_ENTRY_RETURN)(IPTR) exit_code;
}

//____________________________________________________________
//
//		Routine called by command line utility, and server manager
//		Parse switches and do work
//

int alice(Firebird::UtilSvc* uSvc)
{
	AliceGlobals gblInstance(uSvc);
	AliceGlobals* tdgbl = &gblInstance;
	AliceGlobals::putSpecific(tdgbl);
	int exit_code = FINI_ERROR;

	try {

//  Perform some special handling when run as a Firebird service.  The
//  first switch can be "-svc" (lower case!) or it can be "-svc_re" followed
//  by 3 file descriptors to use in re-directing stdin, stdout, and stderr.

	tdgbl->ALICE_data.ua_user = NULL;
	tdgbl->ALICE_data.ua_password = NULL;
#ifdef TRUSTED_AUTH
	tdgbl->ALICE_data.ua_trusted = false;
#endif
	tdgbl->ALICE_data.ua_tr_user = NULL;
	tdgbl->ALICE_data.ua_tr_role = false;

//  Start by parsing switches

	bool error = false, help = false;
	SINT64 flags = 0;
	tdgbl->ALICE_data.ua_shutdown_delay = 0;
	const TEXT* database = NULL;
	TEXT	string[512];

	const char** argv = uSvc->argv.begin();
	int argc = uSvc->argv.getCount();
	++argv;

	// tested outside the loop
	const in_sw_tab_t* table = alice_in_sw_table;

	while (--argc > 0)
	{
		if ((*argv)[0] != '-')
		{
			if (database)
			{
				ALICE_error(1, SafeArg() << database);
				// msg 1: "data base file name (%s) already given",
			}
			database = *argv++;

			continue;
		}

		ALICE_down_case(*argv++, string, sizeof(string));
		if (!string[1]) {
			continue;
		}
		if (strcmp(string, "-?") == 0)
		{
			error = help = true;
			break;
		}

		for (table = alice_in_sw_table; true; ++table)
		{
			const TEXT* p = (TEXT*) table->in_sw_name;
			if (!p)
			{
				ALICE_print(2, SafeArg() << (*--argv));	// msg 2: invalid switch %s
				error = true;
				break;
			}

			TEXT* q = &string[1];
			while (*q && *p++ == *q)
			{
				q++;
			}
			if (!*q && (q - &string[1] >= table->in_sw_min_length))
			{
				break;
			}
		}
		if (error) {
			break;
		}
		if (*table->in_sw_name == 'x') {
			tdgbl->ALICE_data.ua_debug++;
		}
        if (table->in_sw_value & sw_trusted_svc)
        {
			uSvc->checkService();
			if (--argc <= 0) {
				ALICE_error(13);	// msg 13: user name required
			}
			tdgbl->ALICE_data.ua_tr_user = *argv++;
			continue;
		}
        if (table->in_sw_value & sw_trusted_role)
        {
			uSvc->checkService();
			tdgbl->ALICE_data.ua_tr_role = true;
			continue;
		}
#ifdef TRUSTED_AUTH
        if (table->in_sw_value & sw_trusted_auth)
        {
			tdgbl->ALICE_data.ua_trusted = true;
			continue;
		}
#endif
		if (table->in_sw_value == sw_z) {
			ALICE_print(3, SafeArg() << GDS_VERSION);	// msg 3: gfix version %s
		}
		if ((table->in_sw_incompatibilities & flags) ||
			(table->in_sw_requires && !(table->in_sw_requires & flags)))
		{
			ALICE_print(4);	// msg 4: incompatible switch combination
			error = true;
			break;
		}
		flags |= table->in_sw_value;

		if ((table->in_sw_value & (sw_shut | sw_online)) && (argc > 1))
		{
			ALICE_down_case(*argv, string, sizeof(string));
			bool found = true;
			if (strcmp(string, "normal") == 0)
				tdgbl->ALICE_data.ua_shutdown_mode = SHUT_NORMAL;
			else if (strcmp(string, "multi") == 0)
				tdgbl->ALICE_data.ua_shutdown_mode = SHUT_MULTI;
			else if (strcmp(string, "single") == 0)
				tdgbl->ALICE_data.ua_shutdown_mode = SHUT_SINGLE;
			else if (strcmp(string, "full") == 0)
				tdgbl->ALICE_data.ua_shutdown_mode = SHUT_FULL;
			else
				found = false;
			// Consume argument only if we identified mode
			// Let's hope that database with names of modes above are unusual
			if (found)
			{
				argv++;
				argc--;
			}
		}

#ifdef DEV_BUILD
		/*
		if (table->in_sw_value & sw_begin_log)
		{
			if (--argc <= 0) {
				ALICE_error(5);	// msg 5: replay log pathname required
			}
			fb_utils::copy_terminate(tdgbl->ALICE_data.ua_log_file, *argv++, sizeof(tdgbl->ALICE_data.ua_log_file));
		}
		*/
#endif

		if (table->in_sw_value & sw_buffers)
		{
			if (--argc <= 0) {
				ALICE_error(6);	// msg 6: number of page buffers for cache required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if ((!(tdgbl->ALICE_data.ua_page_buffers = atoi(string))) && (strcmp(string, "0")))
			{
				ALICE_error(7);	// msg 7: numeric value required
			}
			if (tdgbl->ALICE_data.ua_page_buffers < 0) {
				ALICE_error(114);	// msg 114: positive or zero numeric value required
			}
		}

		if (table->in_sw_value & sw_housekeeping)
		{
			if (--argc <= 0) {
				ALICE_error(9);	// msg 9: number of transactions per sweep required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if ((!(tdgbl->ALICE_data.ua_sweep_interval = atoi(string))) && (strcmp(string, "0")))
			{
				ALICE_error(7);	// msg 7: numeric value required
			}
			if (tdgbl->ALICE_data.ua_sweep_interval < 0) {
				ALICE_error(114);	// msg 114: positive or zero numeric value required
			}
		}

		if (table->in_sw_value & sw_set_db_dialect)
		{
			if (--argc <= 0) {
				ALICE_error(113);	// msg 113: dialect number required
			}

			ALICE_down_case(*argv++, string, sizeof(string));

			if ((!(tdgbl->ALICE_data.ua_db_SQL_dialect = atoi(string))) && (strcmp(string, "0")))
			{
				ALICE_error(7);	// msg 7: numeric value required
			}

			// JMB: Removed because tdgbl->ALICE_data.ua_db_SQL_dialect is
			//		an unsigned number.  Therefore this check is useless.
			// if (tdgbl->ALICE_data.ua_db_SQL_dialect < 0)
			// {
			//	ALICE_error(114);	// msg 114: positive or zero numeric value required
			// }
		}

		if (table->in_sw_value & (sw_commit | sw_rollback | sw_two_phase))
		{
			if (--argc <= 0) {
				ALICE_error(10);	// msg 10: transaction number or "all" required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if (!(tdgbl->ALICE_data.ua_transaction = atoi(string)))
			{
				if (strcmp(string, "all")) {
					ALICE_error(10);	// msg 10: transaction number or "all" required
				}
				else {
					flags |= sw_list;
				}
			}
		}

		if (table->in_sw_value & sw_write)
		{
			if (--argc <= 0) {
				ALICE_error(11);	// msg 11: "sync" or "async" required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if (!strcmp(string, ALICE_SW_SYNC)) {
				tdgbl->ALICE_data.ua_force = true;
			}
			else if (!strcmp(string, ALICE_SW_ASYNC)) {
				tdgbl->ALICE_data.ua_force = false;
			}
			else {
				ALICE_error(11);	// msg 11: "sync" or "async" required
			}
		}

		if (table->in_sw_value & sw_no_reserve)
		{
			if (--argc <= 0) {
				ALICE_error(12);	// msg 12: "full" or "reserve" required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if (!strcmp(string, "full")) {
				tdgbl->ALICE_data.ua_no_reserve = true;
			}
			else if (!strcmp(string, "reserve")) {
				tdgbl->ALICE_data.ua_no_reserve = false;
			}
			else {
				ALICE_error(12);	// msg 12: "full" or "reserve" required
			}
		}

		if (table->in_sw_value & sw_user)
		{
			if (--argc <= 0) {
				ALICE_error(13);	// msg 13: user name required
			}
			tdgbl->ALICE_data.ua_user = *argv++;
		}

		if (table->in_sw_value & sw_password)
		{
			if (--argc <= 0) {
				ALICE_error(14);	// msg 14: password required
			}
			uSvc->hidePasswd(uSvc->argv, argv - uSvc->argv.begin());
			tdgbl->ALICE_data.ua_password = *argv++;
		}

		if (table->in_sw_value & sw_fetch_password)
		{
			if (--argc <= 0) {
				ALICE_error(14);	// msg 14: password required
			}
			switch (fb_utils::fetchPassword(*argv, tdgbl->ALICE_data.ua_password))
			{
			case fb_utils::FETCH_PASS_OK:
				break;
			case fb_utils::FETCH_PASS_FILE_OPEN_ERROR:
				ALICE_error(116, MsgFormat::SafeArg() << *argv << errno);
				// error @2 opening password file @1
				break;
			case fb_utils::FETCH_PASS_FILE_READ_ERROR:
				ALICE_error(117, MsgFormat::SafeArg() << *argv << errno);
				// error @2 reading password file @1
				break;
			case fb_utils::FETCH_PASS_FILE_EMPTY:
				ALICE_error(118, MsgFormat::SafeArg() << *argv);
				// password file @1 is empty
				break;
			}
			++argv;
		}

		if (table->in_sw_value & sw_disable)
		{
			if (--argc <= 0) {
				ALICE_error(15);	// msg 15: subsystem name
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if (strcmp(string, "wal")) {
				ALICE_error(16);	// msg 16: "wal" required
			}
		}

		if (table->in_sw_value & (sw_attach | sw_force | sw_tran | sw_cache))
		{
			if (--argc <= 0) {
				ALICE_error(17);	// msg 17: number of seconds required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if ((!(tdgbl->ALICE_data.ua_shutdown_delay = atoi(string))) && (strcmp(string, "0")))
			{
				ALICE_error(7);	// msg 7: numeric value required
			}
			if (tdgbl->ALICE_data.ua_shutdown_delay < 0 ||
				tdgbl->ALICE_data.ua_shutdown_delay > 32767)
			{
				ALICE_error(18);	// msg 18: numeric value between 0 and 32767 inclusive required
			}
		}

		if (table->in_sw_value & sw_mode)
		{
			if (--argc <= 0) {
				ALICE_error(110);	// msg 110: "read_only" or "read_write" required
			}
			ALICE_down_case(*argv++, string, sizeof(string));
			if (!strcmp(string, ALICE_SW_MODE_RO)) {
				tdgbl->ALICE_data.ua_read_only = true;
			}
			else if (!strcmp(string, ALICE_SW_MODE_RW)) {
				tdgbl->ALICE_data.ua_read_only = false;
			}
			else {
				ALICE_error(110);	// msg 110: "read_only" or "read_write" required
			}
		}

	}

//  put this here since to put it above overly complicates the parsing
//  can't use tbl_requires since it only looks backwards on command line
	if ((flags & sw_shut) && !(flags & ((sw_attach | sw_force | sw_tran | sw_cache))))
	{
		ALICE_error(19);	// msg 19: must specify type of shutdown
	}

//  catch the case where -z is only command line option
//  flags is unset since sw_z == 0
	if (!flags && !error && table->in_sw_value == sw_z) {
		ALICE_exit(FINI_OK, tdgbl);
	}

	if (!flags || !(flags & ~(sw_user | sw_password | sw_fetch_password |
								sw_trusted_auth | sw_trusted_svc | sw_trusted_role)))
	{
		if (!help && !uSvc->isService())
		{
			ALICE_print(20);	// msg 20: please retry, specifying an option
		}
		error = true;
	}

	if (error)
	{
		if (uSvc->isService())
		{
			uSvc->setServiceStatus(ALICE_MSG_FAC, 20, MsgFormat::SafeArg());
		}
		else
		{
			if (help)
				ALICE_print(120); // usage: gfix [options] <database>
			ALICE_print(21);	// msg 21: plausible options are:
			for (table = alice_in_sw_table; table->in_sw_name; table++)
			{
				if (table->in_sw_msg)
					ALICE_print(table->in_sw_msg);
			}
			ALICE_print(22);	// msg 22: \n    qualifiers show the major option in parenthesis
		}
		ALICE_exit(FINI_ERROR, tdgbl);
	}

	if (!database) {
		ALICE_error(23);	// msg 23: please retry, giving a database name
	}

	//  generate the database parameter block for the attach,
	//  based on the various flags

	USHORT ret;

	if (flags & (sw_list | sw_commit | sw_rollback | sw_two_phase))
	{
		ret = EXE_two_phase(database, flags);
	}
	else
	{
		ret = EXE_action(database, flags);

		const SLONG* ua_val_errors = tdgbl->ALICE_data.ua_val_errors;

		if (!ua_val_errors[VAL_INVALID_DB_VERSION])
		{
			bool any_error = false;

			for (int i = 0; i < MAX_VAL_ERRORS; ++i)
			{
				if (ua_val_errors[i])
				{
					any_error = true;
					break;
				}
			}

			if (any_error)
			{
				ALICE_print(24);	// msg 24: Summary of validation errors\n

				for (int i = 0; i < MAX_VAL_ERRORS; ++i)
				{
					if (ua_val_errors[i]) {
						ALICE_print(val_err_table[i], SafeArg() << ua_val_errors[i]);
					}
				}
			}
		}
	}

	if (ret == FINI_ERROR)
	{
		ALICE_print_status(true, tdgbl->status);
		ALICE_exit(FINI_ERROR, tdgbl);
	}

	ALICE_exit(FINI_OK, tdgbl);

	}	// try

	catch (const Firebird::LongJump&)
	{
		// All "calls" to ALICE_exit(), normal and error exits, wind up here
		exit_code = tdgbl->exit_code;
	}

	catch (const Firebird::Exception& e)
	{
		// Non-alice exception was caught
		e.stuff_exception(tdgbl->status_vector);
		ALICE_print_status(true, tdgbl->status_vector);
		exit_code = FINI_ERROR;
	}

	AliceGlobals::restoreSpecific();

#if defined(DEBUG_GDS_ALLOC)
	if (!uSvc->isService())
	{
		gds_alloc_report(0, __FILE__, __LINE__);
	}
#endif

	if ((exit_code != FINI_OK) && uSvc->isService())
	{
		uSvc->initStatus();
		uSvc->setServiceStatus(tdgbl->status);
	}
	tdgbl->uSvc->started();

	return exit_code;
}


//____________________________________________________________
//
//		Copy a string, down casing as we go.
//

void ALICE_down_case(const TEXT* in, TEXT* out, const size_t buf_size)
{
	const TEXT* const end = out + buf_size - 1;
	for (TEXT c = *in++; c && out < end; c = *in++) {
		*out++ = (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
	}
	*out = 0;
}


//____________________________________________________________
//
//		Display a formatted error message
//

void ALICE_print(USHORT	number, const SafeArg& arg)
{
	TEXT buffer[256];

	fb_msg_format(0, ALICE_MSG_FAC, number, sizeof(buffer), buffer, arg);
	alice_output(false, "%s\n", buffer);
}


//____________________________________________________________
//
//		Print error message. Use fb_interpret
//		to allow redirecting output.
//

void ALICE_print_status(bool error, const ISC_STATUS* status_vector)
{
	if (status_vector && status_vector[1])
	{
		const ISC_STATUS* vector = status_vector;
		AliceGlobals* tdgbl = AliceGlobals::getSpecific();
		tdgbl->uSvc->setServiceStatus(status_vector);

		if (error && tdgbl->uSvc->isService())
		{
			return;
		}

		SCHAR s[1024];
		if (fb_interpret(s, sizeof(s), &vector))
		{
			alice_output(error, "%s\n", s);

			// Continuation of error
			s[0] = '-';
			while (fb_interpret(s + 1, sizeof(s) - 1, &vector)) {
				alice_output(error, "%s\n", s);
			}
		}
	}
}


//____________________________________________________________
//
//		Format and print an error message, then punt.
//

void ALICE_error(USHORT	number, const SafeArg& arg)
{
	AliceGlobals* tdgbl = AliceGlobals::getSpecific();
	TEXT buffer[256];

	tdgbl->uSvc->setServiceStatus(ALICE_MSG_FAC, number, arg);
	if (!tdgbl->uSvc->isService())
	{
		fb_msg_format(0, ALICE_MSG_FAC, number, sizeof(buffer), buffer, arg);
		alice_output(true, "%s\n", buffer);
	}

	ALICE_exit(FINI_ERROR, tdgbl);
}


//____________________________________________________________
//
//		Platform independent output routine.
//

static void alice_output(bool error, const SCHAR* format, ...)
{

	AliceGlobals* tdgbl = AliceGlobals::getSpecific();

	va_list arglist;
	va_start(arglist, format);
	Firebird::string buf;
	buf.vprintf(format, arglist);
	va_end(arglist);

	if (error)
		tdgbl->uSvc->outputError(buf.c_str());
	else
		tdgbl->uSvc->outputVerbose(buf.c_str());
}

