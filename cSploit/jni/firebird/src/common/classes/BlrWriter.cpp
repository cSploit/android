/*
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
 * Adriano dos Santos Fernandes - refactored from others modules.
 */

#include "firebird.h"
#include "consts_pub.h"
#include "dyn_consts.h"
#include "gen/iberror.h"
#include "../common/classes/BlrWriter.h"
#include "../jrd/blr.h"


namespace Firebird {


// Write out a string valued attribute.
void BlrWriter::appendString(UCHAR verb, const char* string, USHORT length)
{
	// TMN: Doesn't this look pretty awkward? If we are given
	// a verb, the length is a ushort, else it's uchar.
	if (verb)
	{
		appendUChar(verb);
		appendUShort(length);
	}
	else
	{
		fb_assert(length <= MAX_UCHAR);
		appendUChar(length);
	}

	if (string)
		appendBytes(reinterpret_cast<const UCHAR*>(string), length);
}

// Input
//   blr_ptr: current position of blr being generated
//   verb: blr byte of which number is an argument
//   number: value to be written to blr
// Function
//   Write out a numeric valued attribute.
void BlrWriter::appendNumber(UCHAR verb, SSHORT number)
{
	if (verb)
		appendUChar(verb);

	appendUShortWithLength(number);
}

void BlrWriter::appendUShortWithLength(USHORT val)
{
	// append an USHORT value, prepended with the USHORT length of an USHORT
	appendUShort(2);
	appendUShort(val);
}

void BlrWriter::appendULongWithLength(ULONG val)
{
	// append an ULONG value, prepended with the USHORT length of an ULONG
	appendUShort(4);
	appendULong(val);
}

void BlrWriter::appendVersion()
{
	appendUChar(isVersion4() ? blr_version4 : blr_version5);
}

// Write out a string of blr as part of a ddl string, as in a view or computed field definition.
void BlrWriter::beginBlr(UCHAR verb)
{
	if (verb)
		appendUChar(verb);

	baseOffset = blrData.getCount();

	// put in a place marker for the size of the blr, since it is unknown
	appendUShort(0);
	appendVersion();
}

// Complete the stuffing of a piece of blr by going back and inserting the length.
void BlrWriter::endBlr()
{
	appendUChar(blr_eoc);

	// go back and stuff in the proper length

	UCHAR* blr_base = &blrData[baseOffset];
	const ULONG length = (blrData.getCount() - baseOffset) - 2;

	if (length > 0xFFFF)
		raiseError(Arg::Gds(isc_too_big_blr) << Arg::Num(length) << Arg::Num(0xFFFF));

	*blr_base++ = (UCHAR) length;
	*blr_base = (UCHAR) (length >> 8);
}

void BlrWriter::raiseError(const Arg::StatusVector& vector)
{
	vector.raise();
}

}	// namespace Jrd
