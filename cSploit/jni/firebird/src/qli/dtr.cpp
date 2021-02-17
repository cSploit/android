/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		dtr.cpp
 *	DESCRIPTION:	Top level driving module
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
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"

#define QLI_MAIN

#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "../qli/dtr.h"
#include "../qli/parse.h"
#include "../qli/compile.h"
#include "../yvalve/perf.h"
#include "../jrd/license.h"
#include "../jrd/ibase.h"
#include "../qli/exe.h"
#include "../qli/all_proto.h"
#include "../qli/compi_proto.h"
#include "../qli/err_proto.h"
#include "../qli/exe_proto.h"
#include "../qli/expan_proto.h"
#include "../qli/gener_proto.h"
#include "../qli/help_proto.h"
#include "../qli/lex_proto.h"
#include "../qli/meta_proto.h"
#include "../qli/parse_proto.h"
#include "../yvalve/gds_proto.h"
#include "../yvalve/perf_proto.h"
#include "fb_exception.h"
#include "../common/utils_proto.h"
#include "../jrd/align.h"
#include "../common/classes/Switches.h"
#include "../qli/qliswi.h"

using MsgFormat::SafeArg;


const char* const STARTUP_FILE = "HOME";	// Assume it's Unix


extern TEXT* QLI_prompt;

static int async_quit(const int, const int, void*);
static void enable_signals();
static bool process_statement(bool);
static void CLIB_ROUTINE signal_arith_excp(USHORT, USHORT, USHORT);
static void usage(const Switches& switches);
static bool yes_no(USHORT, const TEXT*);

// the old name new_handler comflicts with std::new_handler for the "new" operator
typedef void (*new_signal_handler)(int);


struct answer_t
{
	TEXT answer[30];
	bool value;
};

static bool yes_no_loaded = false;
static answer_t answer_table[] =
{
	{ "NO", false },					// NO
	{ "YES", true },					// YES
	{ "", false }
};


int CLIB_ROUTINE main(int argc, char** argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Top level routine.
 *
 **************************************/

	// Look at options, if any

	Firebird::PathName startup_file = STARTUP_FILE;

#ifdef UNIX
	// If a Unix system, get home directory from environment
	SCHAR home_directory[MAXPATHLEN];
	if (!fb_utils::readenv("HOME", startup_file))
		startup_file = ".qli_startup";
	else
		startup_file.append("/.qli_startup");
#endif

	const TEXT* application_file = NULL;
	ALLQ_init();
	LEX_init();
	bool version_flag = false;
	bool banner_flag = true;
	sw_buffers = 0;
	strcpy(QLI_prompt_string, "QLI> ");
	strcpy(QLI_cont_string, "CON> ");
	// Let's define the default number of columns on a machine by machine basis
	QLI_columns = 80;
#ifdef TRUSTED_AUTH
	QLI_trusted = false;
#endif
	QLI_nodb_triggers = false;
	QLI_lines = 60;
	QLI_name_columns = 0;
	QLI_prompt = QLI_prompt_string;
	QLI_matching_language = 0;
	QLI_default_user[0] = 0;
	QLI_default_password[0] = 0;
	QLI_charset[0] = 0;
	bool help_flag = false;

#ifdef DEV_BUILD
	QLI_hex_output = false;
#endif

	SLONG debug_value; // aparently unneeded, see usage below.

	const Switches switches(qli_in_sw_table, FB_NELEM(qli_in_sw_table), false, true);

	const TEXT* const* const arg_end = argv + argc;
	argv++;
	while (argv < arg_end)
	{
		const TEXT* const p = *argv++;
		if (*p != '-')
		{
			banner_flag = false;
			LEX_pop_line();
			LEX_push_string(p);
			continue;
		}
		if (!p[1])
			continue;

		const Switches::in_sw_tab_t* option = switches.findSwitch(p);
		const int in_sw = option ? option->in_sw : IN_SW_QLI_0;

		switch (in_sw)
		{
		case IN_SW_QLI_APP_SCRIPT:
			if (argv >= arg_end)
			{
				ERRQ_msg_put(23);	// Msg23 Please retry, supplying an application script file name
				exit(FINI_ERROR);
			}

			application_file = *argv++;
			break;

		case IN_SW_QLI_BUFFERS:
			if (argv < arg_end && **argv != '-')
				sw_buffers = atoi(*argv++);
			break;

		case IN_SW_QLI_FETCH_PASSWORD:
			{
				if (argv >= arg_end || **argv == '-')
					break;
				const char* pwd = NULL;
				if (fb_utils::fetchPassword(*argv++, pwd) != fb_utils::FETCH_PASS_OK)
					break;
				fb_utils::copy_terminate(QLI_default_password, pwd, sizeof(QLI_default_password));
			}
			break;

		case IN_SW_QLI_INITIAL_SCRIPT:
			if (argv >= arg_end || **argv == '-')
				startup_file = "";
			else
				startup_file = *argv++;
			break;

#ifdef TRUSTED_AUTH
		case IN_SW_QLI_TRUSTED_AUTH:
			QLI_trusted = true;
			break;
#endif

		case IN_SW_QLI_NOBANNER:
			banner_flag = false;
			break;

		case IN_SW_QLI_NODBTRIGGERS:
			QLI_nodb_triggers = true;
			break;

		case IN_SW_QLI_PASSWORD:
			if (argv >= arg_end || **argv == '-')
				break;
			fb_utils::copy_terminate(QLI_default_password, fb_utils::get_passwd(*argv++),
				sizeof(QLI_default_password));
			break;

		case IN_SW_QLI_TRACE:
			sw_trace = true;
			break;

		case IN_SW_QLI_USER:
			if (argv >= arg_end || **argv == '-')
				break;
			fb_utils::copy_terminate(QLI_default_user, *argv++, sizeof(QLI_default_user));
			break;

		case IN_SW_QLI_VERIFY:
			sw_verify = true;
			break;

		case IN_SW_QLI_X:
			debug_value = 1;
			isc_set_debug(debug_value);
			break;

		// This switch's name is arbitrary; since it is an internal
		// mechanism it can be changed at will
		case IN_SW_QLI_Y:
			QLI_trace = true;
			break;

		case IN_SW_QLI_Z:
			version_flag = true;
			break;

		case IN_SW_QLI_HELP:
			help_flag = true;
			break;

		default:
			ERRQ_msg_put(529, SafeArg() << p);
			// Msg469 qli: ignoring unknown switch %c
			break;
		}
	}

	enable_signals();

	if (help_flag)
	{
		usage(switches);
		HELP_fini();
		MET_shutdown();
		LEX_fini();
		ALLQ_fini();
		return FINI_OK;
	}

	if (banner_flag)
		ERRQ_msg_put(24);	// Msg24 Welcome to QLI Query Language Interpreter

	if (version_flag)
		ERRQ_msg_put(25, SafeArg() << FB_VERSION);	// Msg25 qli version %s

	if (application_file)
		LEX_push_file(application_file, true);

	if (startup_file.length())
		LEX_push_file(startup_file.c_str(), false);

#if defined(_MSC_VER) && _MSC_VER >= 1400
	_set_output_format(_TWO_DIGIT_EXPONENT);
#endif

	for (bool got_started = false; !got_started;)
	{
		got_started = true;
		try {
			PAR_token();
		}
		catch (const Firebird::Exception&)
		{
			// try again
			got_started = false;
			ERRQ_pending();
		}
	}
	QLI_error = NULL;

	// Loop until end of file or forced exit

	bool flush_flag = false;
	while (QLI_line)
	{
		qli_plb* temp = QLI_default_pool = ALLQ_pool();
		flush_flag = process_statement(flush_flag);
		ERRQ_pending();
		ALLQ_rlpool(temp);
	}

	HELP_fini();
	MET_shutdown();
	LEX_fini();
	ALLQ_fini();
#ifdef DEBUG_GDS_ALLOC
	// Report any memory leaks noticed.
	// We don't particularly care about QLI specific memory leaks, so all
	// QLI allocations have been marked as "don't report".  However, much
	// of the test-base uses QLI so having a report when QLI finishes
	// could find leaks within the engine.

	gds_alloc_report(0, __FILE__, __LINE__);
#endif
	return (FINI_OK);
}


#ifdef DEV_BUILD
void QLI_validate_desc(const dsc* d)
{
    fb_assert(d->dsc_dtype > dtype_unknown);
    fb_assert(d->dsc_dtype < DTYPE_TYPE_MAX);
    ULONG addr = (ULONG) (U_IPTR) (d->dsc_address);	// safely ignore higher bits even if present
    USHORT ta = type_alignments[d->dsc_dtype];
    if (ta > 1)
		fb_assert((addr & (ta - 1)) == 0);
}
#endif


static int async_quit(const int reason, const int, void*)
{
/**************************************
 *
 *	a s y n c _ q u i t
 *
 **************************************
 *
 * Functional description
 *	Stop whatever we happened to be doing.
 *
 **************************************/
	if (reason == fb_shutrsn_signal)
	{
		EXEC_abort();
		return FB_FAILURE;
	}
	return FB_SUCCESS;
}


static void enable_signals()
{
/**************************************
 *
 *	e n a b l e _ s i g n a l s
 *
 **************************************
 *
 * Functional description
 *	Enable signals.
 *
 **************************************/

#ifdef SIGQUIT
	signal(SIGQUIT, SIG_IGN);
#endif
	fb_shutdown_callback(0, async_quit, fb_shut_confirmation, 0);
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
	signal(SIGFPE, (new_signal_handler) signal_arith_excp);
}


static bool process_statement(bool flush_flag)
{
/**************************************
 *
 *	p r o c e s s _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Parse, compile, and execute a single statement.  If an input flush
 *	is required, return true (or status), otherwise return false.
 *
 **************************************/

	// Clear database active flags in preparation for a new statement

	QLI_abort = false;

	for (qli_dbb* dbb = QLI_databases; dbb; dbb = dbb->dbb_next)
		dbb->dbb_flags &= ~DBB_active;

	// If the last statement wrote out anything to the terminal, skip a line

	if (QLI_skip_line)
	{
		printf("\n");
		QLI_skip_line = false;
	}

	// Enable signal handling for the next statement.  Each signal will
	// be caught at least once, then reset to allow the user to really
	// kill the process

	enable_signals();

	// Enable error unwinding and enable the unwinding environment

	try {

		// Set up the appropriate prompt and get the first significant token.  If
		// we don't get one, we're at end of file

		QLI_prompt = QLI_prompt_string;

		// This needs to be done after setting QLI_prompt to prevent
		// and infinite loop in LEX/next_line.
		// If there was a prior syntax error, flush the token stream

		if (flush_flag)
			LEX_flush();

		while (QLI_token->tok_keyword == KW_SEMI)
			LEX_token();

		PAR_real();

		if (!QLI_line)
			return false;

		EXEC_poll_abort();

		// Mark the current token as starting the statement.  This is allows
		// the EDIT command to find the last statement

		LEX_mark_statement();

		// Change the prompt string to the continuation prompt, and parse
		// the next statement

		QLI_prompt = QLI_cont_string;

		qli_syntax* syntax_tree = PARQ_parse();
		if (!syntax_tree)
			return false;

		EXEC_poll_abort();

		// If the statement was EXIT, force end of file on command input

		if (syntax_tree->syn_type == nod_exit)
		{
			QLI_line = NULL;
			return false;
		}

		// If the statement was quit, ask the user if he want to rollback

		if (syntax_tree->syn_type == nod_quit)
		{
			QLI_line = NULL;
			for (qli_dbb* dbb = QLI_databases; dbb; dbb = dbb->dbb_next)
			{
				if ((dbb->dbb_transaction) && (dbb->dbb_flags & DBB_updates))
				{
					// Msg460 Do you want to rollback updates for <dbb>?
					if (yes_no(460, dbb->dbb_symbol->sym_string))
						MET_transaction(nod_rollback, dbb);
					else
						MET_transaction(nod_commit, dbb);
				}
			}
			return false;
		}

		// Expand the statement.  It will return NULL if the statement was
		// a command.  An error will be unwound

		qli_nod* expanded_tree = EXP_expand(syntax_tree);
		if (!expanded_tree)
			return false;

		// Compile the statement

		qli_nod* execution_tree = CMPQ_compile(expanded_tree);
		if (!execution_tree)
			return false;

		// Generate any BLR needed to support the request

		if (!GEN_generate(execution_tree))
			return false;

		if (QLI_statistics)
		{
			for (qli_dbb* dbb = QLI_databases; dbb; dbb = dbb->dbb_next)
			{
				if (dbb->dbb_flags & DBB_active)
				{
					if (!dbb->dbb_statistics)
					{
						dbb->dbb_statistics = (int*) gds__alloc((SLONG) sizeof(PERF64));
#ifdef DEBUG_GDS_ALLOC
						// We don't care about QLI specific memory leaks for V4.0
						gds_alloc_flag_unfreed((void *) dbb->dbb_statistics);	// QLI: don't care
#endif
					}
					perf64_get_info(&dbb->dbb_handle, (perf64*) dbb->dbb_statistics);
				}
			}
		}

		// Execute the request, for better or worse

		EXEC_top(execution_tree);

		if (QLI_statistics)
		{
			PERF64 statistics;
			TEXT buffer[512], report[256];
			for (qli_dbb* dbb = QLI_databases; dbb; dbb = dbb->dbb_next)
			{
				report[0] = 0;
				if (dbb->dbb_flags & DBB_active)
				{
					ERRQ_msg_get(505, report, sizeof(report));
					// Msg505 "    reads = !r writes = !w fetches = !f marks = !m\n"
					size_t used_len = strlen(report);
					ERRQ_msg_get(506, report + used_len, sizeof(report) - used_len);
					// Msg506 "    elapsed = !e cpu = !u system = !s mem = !x, buffers = !b"
					perf64_get_info(&dbb->dbb_handle, &statistics);
					perf64_format((perf64*) dbb->dbb_statistics, &statistics, report, buffer, 0);
					ERRQ_msg_put(26, SafeArg() << dbb->dbb_filename << buffer);
					// Msg26 Statistics for database %s %s
					QLI_skip_line = true;
				}
			}
		}

		// Release resources associated with the request

		GEN_release();

		return false;

	}	// try
	catch (const Firebird::Exception&)
	{
		GEN_release();
		return true;
	}
}


static void CLIB_ROUTINE signal_arith_excp(USHORT /*sig*/, USHORT code, USHORT /*scp*/)
{
/**************************************
 *
 *	s i g n a l _ a r i t h _ e x c p
 *
 **************************************
 *
 * Functional description
 *	Catch arithmetic exception.
 *
 **************************************/
	USHORT msg_number;

#if defined(FPE_INOVF_TRAP) || defined(FPE_INTDIV_TRAP) || \
	defined(FPE_FLTOVF_TRAP) || defined(FPE_FLTDIV_TRAP) || \
	defined(FPE_FLTUND_TRAP) || defined(FPE_FLTOVF_FAULT) || \
	defined(FPE_FLTUND_FAULT)

	switch (code)
	{
#ifdef FPE_INOVF_TRAP
	case FPE_INTOVF_TRAP:
		msg_number = 14;		// Msg14 integer overflow
		break;
#endif

#ifdef FPE_INTDIV_TRAP
	case FPE_INTDIV_TRAP:
		msg_number = 15;		// Msg15 integer division by zero
		break;
#endif

#ifdef FPE_FLTOVF_TRAP
	case FPE_FLTOVF_TRAP:
		msg_number = 16;		// Msg16 floating overflow trap
		break;
#endif

#ifdef FPE_FLTDIV_TRAP
	case FPE_FLTDIV_TRAP:
		msg_number = 17;		// Msg17 floating division by zero
		break;
#endif

#ifdef FPE_FLTUND_TRAP
	case FPE_FLTUND_TRAP:
		msg_number = 18;		// Msg18 floating underflow trap
		break;
#endif

#ifdef FPE_FLTOVF_FAULT
	case FPE_FLTOVF_FAULT:
		msg_number = 19;		// Msg19 floating overflow fault
		break;
#endif

#ifdef FPE_FLTUND_FAULT
	case FPE_FLTUND_FAULT:
		msg_number = 20;		// Msg20 floating underflow fault
		break;
#endif

	default:
		msg_number = 21;		// Msg21 arithmetic exception
	}
#else
	msg_number = 21;
#endif

	signal(SIGFPE, (new_signal_handler) signal_arith_excp);

	IBERROR(msg_number);
}


static void usage(const Switches& switches)
{
/**************************************
 *
 *	u s a g e
 *
 **************************************
 *
 * Functional description
 *	Print help about command-line arguments.
 *
 **************************************/
	ERRQ_msg_put(513);
	ERRQ_msg_put(514);
	ERRQ_msg_put(515);
	for (const Switches::in_sw_tab_t* p = switches.getTable(); p->in_sw; ++p)
	{
		if (p->in_sw_msg)
			ERRQ_msg_put(p->in_sw_msg);
	}
	ERRQ_msg_put(527);
	ERRQ_msg_put(528);
}

static bool yes_no(USHORT number, const TEXT* arg1)
{
/**************************************
 *
 *	y e s _ n o
 *
 **************************************
 *
 * Functional description
 *	Put out a prompt that expects a yes/no
 *	answer, and keep trying until we get an
 *	acceptable answer (e.g. y, Yes, N, etc.)
 *
 **************************************/
	TEXT prompt[256];

	ERRQ_msg_format(number, sizeof(prompt), prompt, SafeArg() << arg1);

	if (!yes_no_loaded)
	{
		yes_no_loaded = true;
		// Msg498 NO
		if (!ERRQ_msg_get(498, answer_table[0].answer, sizeof(answer_table[0].answer)))
			strcpy(answer_table[0].answer, "NO");	// default if msg_get fails

		// Msg497 YES
		if (!ERRQ_msg_get(497, answer_table[1].answer, sizeof(answer_table[1].answer)))
			strcpy(answer_table[1].answer, "YES");
	}

	TEXT buffer[256];
	while (true)
	{
		buffer[0] = 0;
		if (!LEX_get_line(prompt, buffer, sizeof(buffer)))
			return true;
		for (const answer_t* response = answer_table; *response->answer != '\0'; response++)
		{
			const TEXT* p = buffer;
			while (*p == ' ')
				p++;
			if (*p == EOF)
				return true;
			for (const TEXT* q = response->answer; *p && UPPER(*p) == *q++; p++)
				;
			if (!*p || *p == '\n')
				return response->value;
		}
	}
}
