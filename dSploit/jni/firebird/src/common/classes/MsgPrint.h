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
 *  The Original Code was created by Claudio Valderrama on 3-Mar-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// Localized messages type-safe printing facility.

#ifndef FB_MSGPRINT_H
#define FB_MSGPRINT_H

#include "SafeArg.h"


namespace MsgFormat
{
class BaseStream;

// Here we have routines that print a message that contains placeholders for
// the parameters in a SafeArg object. The placeholders are @n where n can go
// from 1 to 7. Since the schema is positional, the same parameter can be
// referenced as many times as needed inside a format message.
// The routines rely on the parameters provided to SafeArg for knowing the type,
// therefore remember to pass arguments by value/reference not by pointer (except
// strings and UCHAR strings, of course).
// The generic usage is:
// 1.- Create an object of a class derived from the abstract BaseStream (look at
// BaseStream.h) and pass to its constructor the needed argument.
// 2.- Create an object of class SafeArg.
// 3.- Push the parameters into the SafeArg object.
// 4.- Pass the stream object, the format string and the SafeArg object to the
// full MsgPrint routine.
// In practice, most places in the code don't need such functionality and steps
// may be omitted by constructing unnamed objects on the fly.
// Example of full routine using on the fly objects:
// char s[100];
// MsgPrint(StringStream(s, sizeof(s)), "Value is @1\n", SafeArg() << 468);
// But there's a simplified version that does this for you already:
// MsgPrint(s, sizeof(s), "Value is @1\n", SafeArg() << 468);
// Now "s" can be used elsewhere or can be printed to the screen using another
// overloaded function in the MsgPrint family: MsgPrint(s);
// With s being too small, we get:
// char s[15];
// MsgPrint(s, sizeof(s), "Excess @1\n", 3.1415926);
// Now s is "Excess 3.14..." and the \0 used the last available position.
// Another typicaly usage may be:
// SafeArg sa(intarr, FB_NELEM(intarr));  <- array of five integers
// MsgPrint("@1 @5 @2 @4 @3\n", sa); <- prints intarr[0], [4], [1], [3] and [2].
// Remember the positions used by MsgPrint start at one, not zero.
// Now we clean the structure and start fresh:
// MsgPrint("New argument is @1\n", sa.clear() << 3.55);
// It's possible to address more data types as targets to be filled by MsgPrint
// by creating a derivative of BaseStream and creating a shortcut MsgPrint like D
// that handles the creation of the new BaseStream object internally.
//
// The routine fb_msg_format is the safe type replacement of gds__msg_format and
// takes almost the same parameters as the old function with the difference that
// the old function needs five fixed parameters whereas the new uses the MsgFormat
// class to describe parameters and the format string that's retrieved from the
// msg.fdb database uses @n placeholders insteaf of printf formatting arguments.
// In case the function detects % in the format, it calls snprintf.


// A. The basic routine.
int MsgPrint(BaseStream& out_stream, const char* format, const SafeArg& arg);

// B. Printf replacement. Output to stdout.
int MsgPrint(const char* format, const SafeArg& arg);

// C. Print without arguments to stdout.
int MsgPrint(const char* format);

// D. Print to a string, without buffer overrun.
int MsgPrint(char* plainstring, unsigned int s_size, const char* format, const SafeArg& arg);

// E. Prints a formatted string into stderr and flushed the buffer.
int MsgPrintErr(const char* format, const SafeArg& arg);
} // namespace

// Type safe replacement of the old gds__msg_format.
int fb_msg_format(void*        handle,
				  USHORT       facility,
				  USHORT       number,
				  unsigned int bsize,
				  TEXT*        buffer,
				  const        MsgFormat::SafeArg& arg);

#endif // FB_MSGPRINT_H
