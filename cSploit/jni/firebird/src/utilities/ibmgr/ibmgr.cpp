/*
 *
 *	PROGRAM:		Firebird server manager
 *	MODULE:			ibmgr.cpp
 *	DESCRIPTION:	Main routine and parser
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include "../jrd/ibase.h"
#include "../utilities/ibmgr/ibmgr.h"
#include "../utilities/ibmgr/ibmgrswi.h"
#include "../jrd/license.h"
#include "../utilities/ibmgr/srvrmgr_proto.h"
#include "../common/utils_proto.h"


const int MAXARGS		= 20;		// max number of args allowed on command line
const USHORT MAXSTUFF	= 1000;		// longest interactive command line


// Codes returned by get_switches()
// FB_SUCCESS is defined in common.h

const SSHORT ERR_SYNTAX	= -1;
const SSHORT ERR_OTHER	= -2;


// Codes returned by parse_cmd_line()
// FB_SUCCESS is defined in common.h

const SSHORT ACT_NONE	= -1;
const SSHORT ACT_QUIT	= 1;
const SSHORT ACT_PROMPT	= 2;

static void copy_str_upper(TEXT*, const TEXT*);
static bool get_line(int*, SCHAR**, TEXT*);
static SSHORT get_switches(int argc, TEXT** argv, const Switches::in_sw_tab_t* in_sw_table,
						   ibmgr_data_t* ibmgr_data, bool* quitflag, bool zapPasswd);
static SSHORT parse_cmd_line(int, TEXT**, bool);
static void print_config();
static void print_help();


static ibmgr_data_t ibmgr_data;


int CLIB_ROUTINE main( int argc, char **argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	If there is no command line, prompt for one, read it
 *	and make an artificial argc/argv.   Otherwise, pass
 *	the specified argc/argv to IBMGR_exec_line (see below).
 *
 **************************************/

	fprintf(stderr, "*** fbmgr is deprecated, will be removed soon ***\n");
	gds__log("*** fbmgr is deprecated, will be removed soon ***");

	// Let's see if we have something in environment variables
	Firebird::string user, password;
	fb_utils::readenv(ISC_USER, user);
	fb_utils::readenv(ISC_PASSWORD, password);

	Firebird::string host;
	// MMM - do not allow to change host now
	//fb_utils::readenv("ISC_HOST", host);

	TEXT msg[MSG_LEN];

	// Let's get a real user name. This info is used by
	// START server command. Because server is not running
	// we can not check the password, thus we require a
	// real user to be root or FIREBIRD_USER_NAME or
	// INTERBASE_USER_NAME or INTERBASE_USER_SHORT

	const struct passwd* pw = getpwuid(getuid());
	if (pw == NULL)
	{
		perror("getpwuid");
		SRVRMGR_msg_get(MSG_GETPWFAIL, msg);
		fprintf(OUTFILE, "%s\n", msg);
		exit(FINI_ERROR);
	}

	strcpy(ibmgr_data.real_user, pw->pw_name);

	if (!strcmp(pw->pw_name, "root") ||
		!strcmp(pw->pw_name, FIREBIRD_USER_NAME) ||
		!strcmp(pw->pw_name, INTERBASE_USER_NAME) ||
		!strcmp(pw->pw_name, INTERBASE_USER_SHORT))
	{
		strcpy(ibmgr_data.user, SYSDBA_USER_NAME);
	}
	else
		copy_str_upper(ibmgr_data.user, pw->pw_name);


	if (user.length())
		copy_str_upper(ibmgr_data.user, user.c_str());

	if (password.length())
		strcpy(ibmgr_data.password, password.c_str());
	else
		ibmgr_data.password[0] = '\0';

	if (host.length())
		strcpy(ibmgr_data.host, host.c_str());
	else
		strcpy(ibmgr_data.host, "localhost");


	// Shutdown is not in progress and we are not attached to service yet.
	// But obviously we will need attachment.

	ibmgr_data.shutdown = false;
	ibmgr_data.attached = 0;
	ibmgr_data.reattach |= (REA_HOST | REA_USER | REA_PASSWORD);

	// No pidfile by default
	ibmgr_data.pidfile[0] = 0;



	// Special case a solitary -z switch.
	// Print the version and then drop into prompt mode.

	if (argc == 2 && *argv[1] == '-' && (argv[1][1] == 'Z' || argv[1][1] == 'z'))
	{
		parse_cmd_line(argc, argv, false);
		argc--;
	}

	SSHORT ret;
	if (argc > 1)
	{
		ret = parse_cmd_line(argc, argv, true);
		if (ret == FB_SUCCESS)
		{
			ret = SRVRMGR_exec_line(&ibmgr_data);
			if (ret)
			{
				SRVRMGR_msg_get(ret, msg);
				fprintf(OUTFILE, "%s\n", msg);
			}
			// We also need to check the shutdown flag here (if operation was
			// -shut -[noat|notr]) and, depending on what we want to do, either
			// wait here on some sort of a shutdown event, or go to the prompt mode
			SRVRMGR_cleanup(&ibmgr_data);
			exit(FINI_OK);
		}
		else if (ret != ACT_PROMPT)
			exit(FINI_OK);
	}

	int local_argc;
	SCHAR* local_argv[MAXARGS];
	TEXT stuff[MAXSTUFF];

	for (;;)
	{
		if (get_line(&local_argc, local_argv, stuff))
			break;
		if (local_argc > 1)
		{
			ret = parse_cmd_line(local_argc, local_argv, false);
			if (ret == ACT_QUIT)
				break;
			if (ret == FB_SUCCESS)
			{
				ret = SRVRMGR_exec_line(&ibmgr_data);
				if (ret)
				{
					SRVRMGR_msg_get(ret, msg);
					fprintf(OUTFILE, "%s\n", msg);
				}
			}
		}
	}
	SRVRMGR_cleanup(&ibmgr_data);
	exit(FINI_OK);
}


static bool get_line( int *argc, SCHAR** argv, TEXT* stuff)
{
/**************************************
 *
 *	g e t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Read the current line and put its pieces into an argc/argv
 *	structure.   Reads a max of MAXARGS - 1 pieces (argv [0] is
 *	unused), and a max of MAXSTUFF characters, at which point
 *
 **************************************/
	TEXT msg[MSG_LEN];

	SRVRMGR_msg_get(MSG_PROMPT, msg);
	printf("%s ", msg);		// "FBMGR> "

	//if (sw_service_gsec)
	//	putc ('\001', stdout);

	fflush(stdout);
	*argc = 1;
	TEXT* cursor = stuff;
	USHORT count = MAXSTUFF - 1;
	bool first = true;

	// for each input character, if it's white space (or any non-printable,
	// non-newline for that matter), ignore it; if it's a newline, we're
	// done; otherwise, put it in the current argument

	while (*argc < MAXARGS && count > 0)
	{
		TEXT c = getc(stdin);
		if (c > ' ' && c <= '~')
		{
			// note that the first argument gets a '-' appended to the front to fool
			// the switch checker into thinking it came from the command line

			for (argv[(*argc)++] = cursor; count > 0; count--)
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


static SSHORT get_switches(int argc,
						   TEXT** argv,
						   const Switches::in_sw_tab_t* in_sw_table,
						   ibmgr_data_t* ibmgr_data, bool * quitflag, bool zapPasswd)
{
/**************************************
 *
 *	g e t _ s w i t c h e s
 *
 **************************************
 *
 * Functional description
 *	Parse the input line arguments.
 *	Returns: FB_SUCCESS  if success
 *		 ERR_SYNTAX  in case of syntax error.
 *		 ERR_OTHER  non-syntax error.
 *
 **************************************/
	TEXT msg[MSG_LEN];
	bool sw_version = false;
	SSHORT err_msg_no;

	// Look at each argument. It's either a switch or a parameter.
	// parameters must always follow a switch, but not all switches
	// need parameters.

	*quitflag = false;
	USHORT last_sw = IN_SW_IBMGR_0;
	for (--argc; argc > 0; argc--)
	{
		TEXT* string = *++argv;
		if (*string == '?')
			ibmgr_data->operation = OP_HELP;
		else if (*string != '-')
		{
			// this is not a switch, so it must be a parameter for
			// the previous switch, if any
			switch (last_sw)
			{
			case IN_SW_IBMGR_POOL:
				if (strlen(string) >= MAXPATHLEN)
				{
					SRVRMGR_msg_get(MSG_FLNMTOOLONG, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				strcpy(ibmgr_data->print_file, string);
				break;

			case IN_SW_IBMGR_HOST:

				// If the name of the host is the same we are
				// currently attached to, do nothing

				// MMM - it has to be more sophisticated check
				// against compete host name including domain
				// and/or network address, but... May be later

				if (strcmp(ibmgr_data->host, string))
				{
					strcpy(ibmgr_data->host, string);
					ibmgr_data->reattach |= REA_HOST;
				}
				break;

			case IN_SW_IBMGR_PASSWORD:

				// If the password is the same, do nothing
				if (zapPasswd)
				{
					string = fb_utils::get_passwd(string);
				}
				if (strcmp(ibmgr_data->password, string))
				{
					strcpy(ibmgr_data->password, string);
					ibmgr_data->reattach |= REA_PASSWORD;
				}
				break;

			case IN_SW_IBMGR_USER:

				// MMM - we need to compare new entered user
				// with one we already have and do not set the
				// reattach flag if they are the same

				copy_str_upper(ibmgr_data->user, string);
				if (strlen(ibmgr_data->user) > 127)
				{
					SRVRMGR_msg_get(MSG_INVUSER, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				ibmgr_data->reattach |= REA_USER;
				break;

			case IN_SW_IBMGR_PIDFILE:
				{
					Firebird::PathName pf(string);
					pf.copyTo(ibmgr_data->pidfile, sizeof(ibmgr_data->pidfile));
				}
				break;

			case IN_SW_IBMGR_0:
				SRVRMGR_msg_get(MSG_INVPAR, msg);
				fprintf(OUTFILE, "%s\n", msg);
				return ERR_SYNTAX;

			// The following switches do not take any parameters
			case IN_SW_IBMGR_START:
			case IN_SW_IBMGR_ONCE:
			case IN_SW_IBMGR_FOREVER:
			case IN_SW_IBMGR_SIGNORE:
			case IN_SW_IBMGR_SHUT:
			case IN_SW_IBMGR_NOW:
			case IN_SW_IBMGR_NOAT:
			case IN_SW_IBMGR_NOTR:
			case IN_SW_IBMGR_IGNORE:
			case IN_SW_IBMGR_SET:
			case IN_SW_IBMGR_SHOW:
			case IN_SW_IBMGR_QUIT:
			case IN_SW_IBMGR_HELP:
			case IN_SW_IBMGR_Z:
				SRVRMGR_msg_get(MSG_SWNOPAR, msg);
				fprintf(OUTFILE, "%s\n", msg);
				return ERR_SYNTAX;

			default:
#ifdef DEV_BUILD
				fprintf(OUTFILE, "ASSERT: file %s line %"LINEFORMAT": last_sw = %d\n",
						   __FILE__, __LINE__, last_sw);
#endif
				;
			}
			last_sw = IN_SW_IBMGR_0;
		}
		else
		{
			// iterate through the switch table, looking for matches
			USHORT in_sw = IN_SW_IBMGR_0;
			const TEXT* q;
			for (const Switches::in_sw_tab_t* in_sw_tab = in_sw_table; q = in_sw_tab->in_sw_name; in_sw_tab++)
			{
				const TEXT* p = string + 1;

				// handle orphaned hyphen case
				if (!*p--)
					break;

				// compare switch to switch name in table
				int l = 0;
				while (*p)
				{
					if (!*++p)
					{
						if (l >= in_sw_tab->in_sw_min_length)
							in_sw = in_sw_tab->in_sw;
						else
							in_sw = IN_SW_IBMGR_AMBIG;
					}
					if (UPPER(*p) != *q++)
						break;
					l++;
				}

				// end of input means we got a match.  stop looking
				if (!*p)
					break;
			}

			// If the previous switch requires parameters and we are
			// here, this is an error
			switch (last_sw)
			{
			case IN_SW_IBMGR_PASSWORD:
			case IN_SW_IBMGR_USER:
			case IN_SW_IBMGR_HOST:
			case IN_SW_IBMGR_POOL:
				SRVRMGR_msg_get(MSG_REQPAR, msg);
				fprintf(OUTFILE, "%s\n", msg);
				return ERR_SYNTAX;
			}

			switch (in_sw)
			{
			case IN_SW_IBMGR_START:
			case IN_SW_IBMGR_SHUT:
			case IN_SW_IBMGR_SET:
			case IN_SW_IBMGR_SHOW:
			case IN_SW_IBMGR_QUIT:
			case IN_SW_IBMGR_HELP:
			case IN_SW_IBMGR_PRINT:

				// Only one operation can be specified on command line
				if (ibmgr_data->operation)
				{
					SRVRMGR_msg_get(MSG_OPSPEC, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				switch (in_sw)
				{
				case IN_SW_IBMGR_START:
					ibmgr_data->operation = OP_START;
					break;

				case IN_SW_IBMGR_SHUT:
					ibmgr_data->operation = OP_SHUT;
					break;

				case IN_SW_IBMGR_SET:
					ibmgr_data->operation = OP_SET;
					break;

				case IN_SW_IBMGR_SHOW:
					ibmgr_data->operation = OP_SHOW;
					break;

				case IN_SW_IBMGR_QUIT:
					ibmgr_data->operation = OP_QUIT;
					*quitflag = true;
					break;

				case IN_SW_IBMGR_HELP:
					ibmgr_data->operation = OP_HELP;
					break;

				case IN_SW_IBMGR_PRINT:
					ibmgr_data->operation = OP_PRINT;
					break;
				}
				break;

			case IN_SW_IBMGR_PASSWORD:
			case IN_SW_IBMGR_USER:
			case IN_SW_IBMGR_HOST:

				// The above switches are separate case: they can be
				// used as operation or parameter switches. If an
				// operation has already been specified, we assume it
				// is a switch else we set an implied operation type
				// OP_SET.
				// We also can not change any the above parameters if
				// a server shutdown is in progress.

				if (ibmgr_data->shutdown)
				{
					SRVRMGR_msg_get(MSG_SHUTDOWN, msg);
					fprintf(OUTFILE, "%s\n", msg);
					SRVRMGR_msg_get(MSG_CANTCHANGE, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_OTHER;
				}
				if (!ibmgr_data->operation)
					ibmgr_data->operation = OP_SET;

				// Check if any of these switches has been already
				// specified. Note that change in any of these
				// parameters will require new attachment to service
				err_msg_no = 0;
				switch (in_sw)
				{
				case IN_SW_IBMGR_HOST:
					if (ibmgr_data->par_entered & ENT_HOST)
					{
						err_msg_no = MSG_INVSWSW;
						break;
					}
					ibmgr_data->par_entered |= ENT_HOST;
					break;

				case IN_SW_IBMGR_PASSWORD:
					if (ibmgr_data->par_entered & ENT_PASSWORD)
					{
						err_msg_no = MSG_INVSWSW;
						break;
					}
					ibmgr_data->par_entered |= ENT_PASSWORD;
					break;

				case IN_SW_IBMGR_USER:
					if (ibmgr_data->par_entered & ENT_USER)
					{
						err_msg_no = MSG_INVSWSW;
						break;
					}
					ibmgr_data->par_entered |= ENT_USER;
					break;
				}
				if (err_msg_no)
				{
					SRVRMGR_msg_get(err_msg_no, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				break;

			case IN_SW_IBMGR_ONCE:
			case IN_SW_IBMGR_FOREVER:
			case IN_SW_IBMGR_SIGNORE:
			case IN_SW_IBMGR_NOW:
			case IN_SW_IBMGR_NOAT:
			case IN_SW_IBMGR_NOTR:
			case IN_SW_IBMGR_IGNORE:
			case IN_SW_IBMGR_POOL:

				// These switches are operation modifiers or
				// suboperations. Each of them makes sense only
				// in a contex of certain operation. So, let's
				// make sure it is a right contex.
				// But first let's see if any operation has
				// been specified

				if (!ibmgr_data->operation)
				{
					SRVRMGR_msg_get(MSG_NOOPSPEC, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}

				// if a suboperation has already been specified
				// get the hell out of here.
				if (ibmgr_data->suboperation)
				{
					SRVRMGR_msg_get(MSG_INVSWSW, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				err_msg_no = 0;
				switch (in_sw)
				{
				case IN_SW_IBMGR_ONCE:
				case IN_SW_IBMGR_FOREVER:
				case IN_SW_IBMGR_SIGNORE:
					if (ibmgr_data->operation != OP_START)
					{
						err_msg_no = MSG_INVSWOP;
						break;
					}

					switch (in_sw)
					{
					case IN_SW_IBMGR_ONCE:
						ibmgr_data->suboperation = SOP_START_ONCE;
						break;
					case IN_SW_IBMGR_SIGNORE:
						ibmgr_data->suboperation = SOP_START_SIGNORE;
						break;
					default:
						ibmgr_data->suboperation = SOP_START_FOREVER;
						break;
					}
					break;

				case IN_SW_IBMGR_NOW:
				case IN_SW_IBMGR_NOAT:
				case IN_SW_IBMGR_NOTR:
				case IN_SW_IBMGR_IGNORE:
					if (ibmgr_data->operation != OP_SHUT)
					{
						err_msg_no = MSG_INVSWOP;
						break;
					}

					switch (in_sw)
					{
					case IN_SW_IBMGR_NOW:
						ibmgr_data->suboperation = SOP_SHUT_NOW;
						break;
					case IN_SW_IBMGR_NOAT:
						ibmgr_data->suboperation = SOP_SHUT_NOAT;
						break;
					case IN_SW_IBMGR_NOTR:
						ibmgr_data->suboperation = SOP_SHUT_NOTR;
						break;
					default:
						ibmgr_data->suboperation = SOP_SHUT_IGN;
						break;
					}
					break;

				case IN_SW_IBMGR_POOL:
					if (ibmgr_data->operation != OP_PRINT)
					{
						err_msg_no = MSG_INVSWOP;
						break;
					}
					ibmgr_data->suboperation = SOP_PRINT_POOL;
					break;
				}
				if (err_msg_no)
				{
					SRVRMGR_msg_get(err_msg_no, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				break;

			case IN_SW_IBMGR_Z:

				// This is also a separate case - it could be
				// operation switch or just switch. Also,
				// does not matter how many times version switch
				// was specified, we output version only once

				if (!ibmgr_data->operation)
					ibmgr_data->operation = OP_VERSION;
				if (!sw_version)
				{
					SRVRMGR_msg_get(MSG_VERSION, msg);
					fprintf(OUTFILE, "%s %s\n", msg, FB_VERSION);
				}
				break;

			case IN_SW_IBMGR_0:
				SRVRMGR_msg_get(MSG_INVSW, msg);
				fprintf(OUTFILE, "%s\n", msg);
				return ERR_SYNTAX;

			case IN_SW_IBMGR_AMBIG:
				SRVRMGR_msg_get(MSG_AMBSW, msg);
				fprintf(OUTFILE, "%s\n", msg);
				return ERR_SYNTAX;

			case IN_SW_IBMGR_PIDFILE:
			{
				if (ibmgr_data->pidfile[0])
				{
					SRVRMGR_msg_get(MSG_INVSWSW, msg);
					fprintf(OUTFILE, "%s\n", msg);
					return ERR_SYNTAX;
				}
				break;
			}

			default:
#ifdef DEV_BUILD
				fprintf(OUTFILE, "ASSERT: file %s line %"LINEFORMAT": in_sw = %d\n",
						   __FILE__, __LINE__, in_sw);
#endif
				;
			}
			last_sw = in_sw;
		}

		if (*quitflag)
			break;
	}

	// If the previous switch requires parameters and we are here, this is an error

	switch (last_sw)
	{
	case IN_SW_IBMGR_PASSWORD:
	case IN_SW_IBMGR_USER:
	case IN_SW_IBMGR_HOST:
	case IN_SW_IBMGR_PRINT:
	case IN_SW_IBMGR_POOL:
		SRVRMGR_msg_get(MSG_REQPAR, msg);
		fprintf(OUTFILE, "%s\n", msg);
		return ERR_SYNTAX;
	}

#ifdef DEV_BUILD
	if (!ibmgr_data->operation)
		fprintf(OUTFILE, "ASSERT: file %s line %"LINEFORMAT": no operation has been specified\n",
				   __FILE__, __LINE__);
#endif

	return FB_SUCCESS;
}


static void print_config()
{
/**************************************
 *
 *	p r i n t _ c o n f i g
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	fprintf(OUTFILE, "\nHost:\t%s\nUser:\t%s\n\n", ibmgr_data.host, ibmgr_data.user);
#ifdef DEBUG
	fprintf(OUTFILE, "\nReal user:\t%s\nPassword:\t%s\n\n",
			   ibmgr_data.real_user, ibmgr_data.password);
	fprintf(OUTFILE, "Attached:\t%s\nNew attach:\t%s %s %s\nShutdown:\t%s\n\n",
			   ibmgr_data.attached ? "yes" : "no",
			   ibmgr_data.reattach & REA_HOST ? "HOST" : "no",
			   ibmgr_data.reattach & REA_PASSWORD ? "PASSWORD" : "no",
			   ibmgr_data.reattach & REA_USER ? "USER" : "no",
			   ibmgr_data.shutdown ? "yes" : "no");
#endif
}


static void print_help()
{
/**************************************
 *
 *	p r i n t _ h e l p
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	TEXT msg[MSG_LEN];
	TEXT msg2[MSG_LEN];

	fprintf(OUTFILE, "\n\n");
	fprintf(OUTFILE, "Usage:		fbmgr -command [-option [parameter]]\n\n");
	fprintf(OUTFILE, "or		fbmgr<RETURN>\n");
	fprintf(OUTFILE, "		FBMGR> command [-option [parameter]]\n\n");
	fprintf(OUTFILE, "		shut  [-now]		shutdown server\n");
	fprintf(OUTFILE, "		show			show host and user\n");
	fprintf(OUTFILE, "		user <user_name>	set user name\n");
	fprintf(OUTFILE, "		password <password>	set DBA password\n");
	fprintf(OUTFILE, "		pidfile <filename>	file to save fbserver's PID\n");
	fprintf(OUTFILE, "		help			prints help text\n");
	fprintf(OUTFILE, "		quit			quit prompt mode\n\n");
	fprintf(OUTFILE, "Command switches 'user' and 'password' can also be used\n");
	fprintf(OUTFILE, "as an option switches for commands like start or shut.\n");
	fprintf(OUTFILE, "For example, to shutdown server you can: \n\n");
	fprintf(OUTFILE, "fbmgr -shut -password <password>\n\n");
	fprintf(OUTFILE, "or\n\n");
	fprintf(OUTFILE, "fbmgr<RETURN>\n");
	fprintf(OUTFILE, "FBMGR> shut -password <password>\n\n");
	fprintf(OUTFILE, "or\n\n");
	fprintf(OUTFILE, "fbmgr<RETURN>\n");
	fprintf(OUTFILE, "FBMGR> password <password>\n");
	fprintf(OUTFILE, "FBMGR> shut\n\n\n");
}


static SSHORT parse_cmd_line( int argc, TEXT** argv, bool zapPasswd)
{
/**************************************
 *
 *	p a r s e _ c m d _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Parses the command line. After it was parsed
 *	executes commands that do require server access
 *	(help, show, etc.) and checks some conditions
 *	for other command.
 *
 *	returns:
 *
 *	    FB_SUCCESS   on normal completion,
 *	    ACT_QUIT  if user chooses to quit
 *	    ACT_PROMT used by main() to switch into
 *	              the prompt mode
 *	    ACT_NONE  on error or
 *	              if no further actions necessary
 *	              (like help, version, etc.)
 *
 **************************************/
	TEXT msg[MSG_LEN];
	bool quitflag = false;

	ibmgr_data.operation = OP_NONE;
	ibmgr_data.suboperation = SOP_NONE;
	ibmgr_data.par_entered = 0;

	SSHORT ret = get_switches(argc, argv, ibmgr_in_sw_table, &ibmgr_data, &quitflag, zapPasswd);
	if (ret != FB_SUCCESS)
	{
		if (ret == ERR_SYNTAX)
		{
			SRVRMGR_msg_get(MSG_SYNTAX, msg);
			fprintf(OUTFILE, "%s\n", msg);
		}
		return ACT_NONE;
	}
	switch (ibmgr_data.operation)
	{
	case OP_SHUT:
		if (strcmp(ibmgr_data.user, SYSDBA_USER_NAME))
		{
			SRVRMGR_msg_get(MSG_NOPERM, msg);
			fprintf(OUTFILE, "%s\n", msg);
			ret = ACT_NONE;
			break;
		}
		// The only allowed suboperation when shutdown
		// has been started is to IGNORE it.
		if (ibmgr_data.shutdown && ibmgr_data.suboperation != SOP_SHUT_IGN)
		{
			SRVRMGR_msg_get(MSG_SHUTDOWN, msg);
			fprintf(OUTFILE, "%s\n", msg);
			SRVRMGR_msg_get(MSG_CANTSHUT, msg);
			fprintf(OUTFILE, "%s\n", msg);
			ret = ACT_NONE;
		}
		break;

	case OP_START:
		if ((strcmp(ibmgr_data.real_user, "root") &&
				strcmp(ibmgr_data.real_user, FIREBIRD_USER_NAME) &&
				strcmp(ibmgr_data.real_user, INTERBASE_USER_NAME) &&
				strcmp(ibmgr_data.real_user, INTERBASE_USER_SHORT)) ||
			strcmp(ibmgr_data.user, SYSDBA_USER_NAME))
		{
			SRVRMGR_msg_get(MSG_NOPERM, msg);
			fprintf(OUTFILE, "%s\n", msg);
			ret = ACT_NONE;
			break;
		}
		break;

	case OP_HELP:
		print_help();
		ret = ACT_NONE;
		break;

	case OP_SHOW:
		print_config();
		ret = ACT_NONE;
		break;

	case OP_VERSION:
		ret = ACT_NONE;
		break;

	case OP_SET:
		ret = ACT_PROMPT;
		break;
	}

	if (quitflag)
		if (ibmgr_data.shutdown)
		{
			SRVRMGR_msg_get(MSG_SHUTDOWN, msg);
			fprintf(OUTFILE, "%s\n", msg);
			SRVRMGR_msg_get(MSG_CANTQUIT, msg);
			fprintf(OUTFILE, "%s\n", msg);
			ret = ACT_NONE;
		}
		else
			ret = ACT_QUIT;

	return ret;
}

static void copy_str_upper( TEXT* str1, const TEXT* str2)
{
/**************************************
 *
 *	c o p y _ s t r _ u p p e r
 *
 **************************************
 *
 * Functional description
 *	Copies string str2 to address str1
 *	converting str2 to upper case
 *
 **************************************/

	for (; *str2; str1++, str2++)
		*str1 = UPPER(*str2);

	*str1 = '\0';
}

