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

#include "firebird.h"
#include "../jrd/common.h"
#include "BaseStream.h"
#include "MsgPrint.h"
#include <string.h>
#include "../jrd/gds_proto.h"
#include "../common/utils_proto.h"
#include "../jrd/file_params.h"


namespace MsgFormat
{

// Enough to the current conversions. If SINT128 is decoded as a full number
// instead of two parts, it may be updated to 64 and 63 respectively,
// because 2^128 ~ 3.4e38.
const int DECODE_BUF_SIZE = 32;
const int DECODE_BUF_LEN = 31;

// The maximum numeric base we support, using 0..Z
const int MAX_RADIX = 36;
// We don't mess with octal and the like, otherwise DECODE_BUF_* constants have to be enlarged.
const int MIN_RADIX = 10;
// We won't output strings of more than 64K.
const size_t MAX_STRING = 1 << 16;

// Generic functions.
int decode(uint64_t value, char* const rc, int radix = 10);
int decode(int64_t value, char* const rc, int radix = 10);
int decode(double value, char* rc);
int adjust_prefix(int radix, int rev, bool is_neg, char* const rc);
int MsgPrintHelper(BaseStream& out_stream, const safe_cell& item);



// Decode unsigned integer values in any base between 10 and 36.
int decode(uint64_t value, char* const rc, int radix)
{
	int rev = DECODE_BUF_LEN;
	if (radix < MIN_RADIX || radix > MAX_RADIX)
		radix = MIN_RADIX;

	if (radix == 10) // go faster for this option
	{
		while (true)
		{
			rc[rev--] = static_cast<unsigned char>(value % 10) + '0';
			value /= 10;
			if (!value)
				break;
		}
	}
	else
	{
		while (true)
		{
			int temp = static_cast<int>(value % radix);
			rc[rev--] = static_cast<unsigned char>(temp < 10 ? temp + '0' : temp - 10 + 'A');
			value /= radix;
			if (!value)
				break;
		}
	}

	return adjust_prefix(radix, rev, false, rc);
}


// Decode signed integer values in any base between 10 and 36.
int decode(int64_t value, char* const rc, int radix)
{
	if (value >= 0)
		return decode(static_cast<uint64_t>(value), rc, radix);

	int rev = DECODE_BUF_LEN;
	if (radix < MIN_RADIX || radix > MAX_RADIX)
		radix = MIN_RADIX;

	// The remainder with negative values is not consistent across compilers.
	if (radix == 10) // go faster for this option
	{
		while (true)
		{
			int64_t temp = (value / 10) * 10 - value;
			rc[rev--] = static_cast<unsigned char>(temp) + '0';
			value /= 10;
			if (!value)
				break;
		}
	}
	else
	{
		while (true)
		{
			int64_t temp = (value / radix) * radix - value;
			rc[rev--] = static_cast<unsigned char>(temp < 10 ? temp + '0' : temp - 10 + 'A');
			value /= radix;
			if (!value)
				break;
		}
	}

	return adjust_prefix(radix, rev, true, rc);
}


// Stub that relies on the printf family to write a double using "g"
// for smallest representation in text form.
int decode(double value, char* rc)
{
	return sprintf(rc, "%g", value);
}


// Sets the radix indicator and returns the length of the adjusted string.
int adjust_prefix(int radix, int rev, bool is_neg, char* const rc)
{
	int fwd = 0;

	if (is_neg)
		rc[fwd++] = '-';

	if (radix == 16)
	{
		rc[fwd++] = '0';
		rc[fwd++] = 'x';
	}
	else if (radix > 10)
	{
		rc[fwd++] = '(';
		rc[fwd++] = static_cast<char>(radix / 10) + '0';
		rc[fwd++] = static_cast<char>(radix % 10) + '0';
		rc[fwd++] = ')';
	}

	while (rev < DECODE_BUF_LEN)
		rc[fwd++] = rc[++rev];

	rc[fwd] = 0;
	// fb_assert(fwd < DECODE_BUF_SIZE);

	return fwd;
}


// Prints one specific item in the array of type-safe arguments.
// Assumes item is valid.
int MsgPrintHelper(BaseStream& out_stream, const safe_cell& item)
{
	switch (item.type)
	{
	case safe_cell::at_char:
	case safe_cell::at_uchar: // For now, treat UCHAR same as char.
		return out_stream.write(&item.c_value, 1);
	//case safe_cell::at_int16:
	//case safe_cell::at_int32:
	case safe_cell::at_int64:
		{
			char s[DECODE_BUF_SIZE];
			int n = decode(item.i_value, s);
			return out_stream.write(s, n);
		}
	case safe_cell::at_uint64:
		{
			char s[DECODE_BUF_SIZE];
			int n = decode(static_cast<uint64_t>(item.i_value), s);
			return out_stream.write(s, n);
		}
	case safe_cell::at_int128:
		{
			// Warning: useless display in real life
			char s[DECODE_BUF_SIZE];
			int n = decode(item.i128_value.high, s);
			int n2 = out_stream.write(s, n) + out_stream.write(".", 1);
			n = decode(item.i128_value.low, s);
			return n2 + out_stream.write(s, n);
		}
	case safe_cell::at_double:
		{
			char s[DECODE_BUF_SIZE];
			int n = decode(item.d_value, s);
			return out_stream.write(s, n);
		}
	case safe_cell::at_str:
		{
			const char* s = item.st_value.s_string;
			if (!s)
				s = "(null)";

			size_t n = strlen(s);
			if (n > MAX_STRING)
				n = MAX_STRING;

			return out_stream.write(s, n);
		}
	case safe_cell::at_ptr:
		{
			uint64_t v = reinterpret_cast<uint64_t>(item.p_value);
			char s[DECODE_BUF_SIZE];
			int n = decode(v, s, 16);
			return out_stream.write(s, n);
		}
	default: // safe_cell::at_none and whatever out of range.
		return out_stream.write("(unknown)", 9);
	}
}


// Prints the whole chain of arguments, according to format and in the specified stream.
int MsgPrint(BaseStream& out_stream, const char* format, const SafeArg& arg)
{
	int out_bytes = 0;
	for (const char* iter = format; true; ++iter)
	{
		switch (*iter)
		{
		case 0:
			return out_bytes;

		case '@':
			switch (iter[1])
			{
			case 0:
				out_bytes += out_stream.write("@(EOF)", 6);
				return out_bytes;
			case '@':
				out_bytes += out_stream.write(iter, 1);
				break;
			default:
				{
					const int pos = iter[1] - '0';
					if (pos > 0 && static_cast<size_t>(pos) <= arg.m_count)
						out_bytes += MsgPrintHelper(out_stream, arg.m_arguments[pos - 1]);
					else if (pos >= 0 && pos <= 9)
					{
						// Show the missing or out of range param number.
						out_bytes += MsgPrint(out_stream,
							"<Missing arg #@1 - possibly status vector overflow>",
							SafeArg() << pos);
					}
					else // Something not a number following @, invalid.
						out_bytes += out_stream.write("(error)", 7);
				}
			}
			++iter;
			break;

		case '\\':
			switch (iter[1])
			{
			case 0:
				out_bytes += out_stream.write("\\(EOF)", 6);
				return out_bytes;
			case 'n':
				out_bytes += out_stream.write("\n", 1);
				break;
			case 't':
				out_bytes += out_stream.write("\t", 1);
				break;
			default:
				out_bytes += out_stream.write(iter, 2); // iter[0] and iter[1]
			}
			++iter;
			break;

		default:
			{
				const char* iter2 = iter;
				// Take into account the previous cases: 0, @ and backslash.
				while (iter2[1] && iter2[1] != '@' && iter2[1] != '\\')
					++iter2;

				out_bytes += out_stream.write(iter, iter2 - iter + 1);
				iter = iter2;
			}
			break;
		}
	}

	return 0;
}


// Shortcut version to format a string with arguments on standard output.
int MsgPrint(const char* format, const SafeArg& arg)
{
	StdioStream st(stdout);
	return MsgPrint(st, format, arg);
}


// Shortcut version to format a string without arguments on standard output.
int MsgPrint(const char* format)
{
	static const SafeArg dummy;

	StdioStream st(stdout);
	return MsgPrint(st, format, dummy);
}


// Shortcut version to format a string with arguments on a string output
// of a given size.
int MsgPrint(char* plainstring, unsigned int s_size, const char* format, const SafeArg& arg)
{
	StringStream st(plainstring, s_size);
	return MsgPrint(st, format, arg);
}


// Shortcut version to format a string with arguments on standard error.
int MsgPrintErr(const char* format, const SafeArg& arg)
{
	StdioStream st(stderr, true); // flush
	return MsgPrint(st, format, arg);
}

} // namespace


int fb_msg_format(void*        handle,
				  USHORT       facility,
				  USHORT       number,
				  unsigned int bsize,
				  TEXT*        buffer,
				  const        MsgFormat::SafeArg& arg)
{
/**************************************
 *
 *	f b _ m s g _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Lookup and format message.  Return as much of formatted string
 *	as fits in caller's buffer.
 *
 **************************************/

	using MsgFormat::MsgPrint;

	// The field MESSAGES.TEXT is 118 bytes long.
	int total_msg = 0;
	char msg[120] = "";
	const int n = gds__msg_lookup(handle, facility, number, sizeof(msg), msg, NULL);

	if (n > 0 && unsigned(n) < sizeof(msg))
	{
		// Shameful bridge, gds__msg_format emulation for old format messages.
		if (strchr(msg, '%'))
		{
			const TEXT* rep[5];
			arg.dump(rep, 5);
			total_msg = fb_utils::snprintf(buffer, bsize, msg, rep[0], rep[1], rep[2], rep[3], rep[4]);
		}
		else
			total_msg = MsgPrint(buffer, bsize, msg, arg);
	}
	else
	{
		Firebird::string s;
		s.printf("can't format message %d:%d -- ", facility, number);
		if (n == -1)
			s += "message text not found";
		else if (n == -2)
		{
			s += "message file ";
			s += fb_utils::getPrefix(fb_utils::FB_DIR_MSG, MSG_FILE).ToString();
			s += " not found";
		}
		else
		{
			fb_utils::snprintf(buffer, bsize, "message system code %d", n);
			s += buffer;
		}
		total_msg = s.copyTo(buffer, bsize);
	}

	return (n > 0 ? total_msg : -total_msg);
}
