/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		err.cpp
 *	DESCRIPTION:	Error handlers
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
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "gen/iberror.h"
#include "../qli/dtr.h"
#include "../qli/parse.h"
#include "../qli/err_proto.h"
#include "../qli/help_proto.h"
#include "../qli/lex_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/classes/MsgPrint.h"

using MsgFormat::SafeArg;


static TEXT ERRQ_message[256];

void ERRQ_bugcheck( USHORT number)
{
/**************************************
 *
 *	E R R Q _ b u g c h e c k
 *
 **************************************
 *
 * Functional description
 *	Somebody has screwed up.  Bugcheck.
 *
 **************************************/
	TEXT s[256];

	ERRQ_msg_format(number, sizeof(s), s);
	ERRQ_error(9, SafeArg() << s);	// Msg9 INTERNAL: %s
}


void ERRQ_database_error( qli_dbb* dbb, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	E R R Q _ d a t a b a s e _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Print message from database error and unwind.
 *
 **************************************/

	if (dbb)
	{
		ERRQ_msg_put(10, dbb->dbb_filename);	// Msg10 ** QLI error from database %s **
		gds__print_status(status_vector);
	}
	else
	{
		ERRQ_msg_put(11);	// Msg11 ** QLI error from database **
		gds__print_status(status_vector);
	}

	QLI_skip_line = true;

	// if we've really got the database open and get an I/O error,
	// close up neatly.  If we get an I/O error trying to open the
	// database, somebody else will clean up

	if (dbb && dbb->dbb_handle && status_vector[1] == isc_io_error)
		ERRQ_msg_put(458, dbb->dbb_filename);	// Msg458 ** connection to database %s lost **

	Firebird::LongJump::raise();
}


void ERRQ_error(USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	E R R Q _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An error has occurred.  Put out an error message and
 *	unwind.  If this was called before the unwind path
 *	was established, don't unwind just print error and exit.
 *
 **************************************/

	ERRQ_pending();
	ERRQ_error_format(number, arg);

	Firebird::LongJump::raise();
	/*
	else
	{
		ERRQ_pending();
		ERRQ_exit(FINI_ERROR);
	}
	*/
}

void ERRQ_error(USHORT number, const char* str)
{
/**************************************
 *
 *	E R R Q _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An error has occurred.  Put out an error message and
 *	unwind.  If this was called before the unwind path
 *	was established, don't unwind just print error and exit.
 *
 **************************************/

	ERRQ_error(number, SafeArg() << str);
}


void ERRQ_error_format(USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	E R R Q _ e r r o r _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file and format it
 *      in the standard qli error format, put it where
 *      ERRQ_pending expects to find it.
 **************************************/
	TEXT s[256];

	fb_msg_format(0, QLI_MSG_FAC, number, sizeof(s), s, arg);
	fb_msg_format(0, QLI_MSG_FAC, 12, sizeof(ERRQ_message), ERRQ_message, SafeArg() << s);
	// Msg12 ** QLI error: %s **
	QLI_error = ERRQ_message;
	QLI_skip_line = true;
}


void ERRQ_exit( int status)
{
/**************************************
 *
 *	E R R Q _ e x i t
 *
 **************************************
 *
 * Functional description
 *	Exit after shutting down a bit.
 *
 **************************************/

	HELP_fini();
	LEX_fini();
	exit(status);
}


void ERRQ_msg_format(USHORT number, USHORT length, TEXT* output_string, const SafeArg& arg)
{
/**************************************
 *
 *	E R R Q _ m s g _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file and format it
 *
 **************************************/

	fb_msg_format(0, QLI_MSG_FAC, number, length, output_string, arg);
}


void ERRQ_msg_partial(USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	E R R Q _ m s g _ p a r t i a l
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it
 *
 **************************************/

	fb_msg_format(0, QLI_MSG_FAC, number, sizeof(ERRQ_message), ERRQ_message, arg);
	printf("%s", ERRQ_message);
}


void ERRQ_msg_put(USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	E R R Q _ m s g _ p u t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it
 * It's same outcome as ERRQ_msg_partial but with a newline at the end.
 *
 **************************************/

	fb_msg_format(0, QLI_MSG_FAC, number, sizeof(ERRQ_message), ERRQ_message, arg);
	printf("%s\n", ERRQ_message);
}


void ERRQ_msg_put(USHORT number, const char* str)
{
/**************************************
 *
 *	E R R Q _ m s g _ p u t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file, format it, and print it
 * It's same outcome as ERRQ_msg_partial but with a newline at the end.
 *
 **************************************/

	fb_msg_format(0, QLI_MSG_FAC, number, sizeof(ERRQ_message), ERRQ_message, SafeArg() << str);
	printf("%s\n", ERRQ_message);
}


int ERRQ_msg_get( USHORT number, TEXT* output_msg, size_t s_size)
{
/**************************************
 *
 *	E R R Q _ m s g _ g e t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file
 *
 **************************************/
	static const SafeArg arg;

	int l = fb_msg_format(0, QLI_MSG_FAC, number, s_size, output_msg, arg);
	return (l >= 0);
}


void ERRQ_pending()
{
/**************************************
 *
 *	E R R Q _ p e n d i n g
 *
 **************************************
 *
 * Functional description
 *	Print out an error message if one is pending.
 *
 **************************************/

	if (QLI_error)
	{
		printf("%s\n", QLI_error);
		QLI_error = NULL;
	}
}


void ERRQ_print_error(USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	E R R Q _ p r i n t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An error has occurred.  Put out an error message and
 *	unwind.
 *
 **************************************/

	ERRQ_error(number, arg);
}


void ERRQ_print_error(USHORT number, const char* str)
{
/**************************************
 *
 *	E R R Q _ p r i n t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An error has occurred.  Put out an error message and
 *	unwind.
 *
 **************************************/

	ERRQ_error(number, SafeArg() << str);
}


void ERRQ_syntax( USHORT number)
{
/**************************************
 *
 *	E R R Q _ s y n t a x
 *
 **************************************
 *
 * Functional description
 *	Syntax error has occurred.  Give some hint of what went
 *	wrong.
 *
 **************************************/
	TEXT s[256];

	ERRQ_msg_format(number, sizeof(s), s);
	ERRQ_error(13, SafeArg() << s << QLI_token->tok_string);
	// Msg13 expected %s, encountered %s
}
