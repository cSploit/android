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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/gdsassert.h"
#include "../jrd/jrd.h"
#include "../jrd/req.h"

#include "../jrd/RecordBuffer.h"

const char* const SCRATCH = "fb_recbuf_";

using namespace Jrd;

RecordBuffer::RecordBuffer(MemoryPool& pool, const Format* format)
	: length(format->fmt_length), count(0), filled(false)
{
	space = FB_NEW(pool) TempSpace(pool, SCRATCH);

	record = FB_NEW_RPT(pool, length) Record(pool);
	record->rec_format = format;
	record->rec_length = length;
}

RecordBuffer::~RecordBuffer()
{
	delete record;
	delete space;
}

size_t RecordBuffer::getCount() const
{
	return count;
}

Record* RecordBuffer::getTempRecord() const
{
	return record;
}

const Format* RecordBuffer::getFormat() const
{
	return record->rec_format;
}

offset_t RecordBuffer::store(const Record* new_record)
{
	fb_assert(new_record->rec_length == length);

	fb_assert(!filled);

	space->write(count * length, new_record->rec_data, length);

	return count++;
}

bool RecordBuffer::fetch(offset_t position, Record* to_record)
{
	fb_assert(to_record->rec_length == length);

	filled = true;

	if (position >= count)
	{
		return false;
	}

	space->read(position * length, to_record->rec_data, length);

	return true;
}
