/*
 **************************************************************************
 *
 *	PROGRAM:		standard descriptors for gbak & gsplit
 *	MODULE:			std_desc.h
 *	DESCRIPTION:	standard descriptors for different OS's
 *
 *
 **************************************************************************
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
 * The Original Code was created by Alexander Peshkoff <peshkoff@mail.ru>.
 * Thanks to Achim Kalwa <achim.kalwa@winkhaus.de>.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef GBAK_STD_DESC_H
#define GBAK_STD_DESC_H

#include "firebird.h"

#ifdef WIN_NT

#include <windows.h>

typedef HANDLE DESC;

static inline DESC GBAK_STDIN_DESC()
{
	return GetStdHandle(STD_INPUT_HANDLE); // standard input file descriptor
}
static inline DESC GBAK_STDOUT_DESC()
{
	return GetStdHandle(STD_OUTPUT_HANDLE);	// standard output file descriptor
}

#else //WIN_NT

typedef int DESC;

static inline DESC GBAK_STDIN_DESC()
{
	return 0;	// standard input file descriptor
}
static inline DESC GBAK_STDOUT_DESC()
{
	return 1;	// standard output file descriptor
}

const int INVALID_HANDLE_VALUE = -1;

#endif //WIN_NT

typedef DESC FILE_DESC;

#endif  //GBAK_STD_DESC_H
