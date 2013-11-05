/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		ClumpletReader.cpp
 *	DESCRIPTION:	Secure handling of clumplet buffers
 *
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"

#include "../common/classes/ClumpletReader.h"
#include "fb_exception.h"

#include "../jrd/ibase.h"

#ifdef DEBUG_CLUMPLETS
#include "../jrd/gds_proto.h"
#include <ctype.h>

namespace Firebird {

class ClumpletDump : public ClumpletReader
{
public:
	ClumpletDump(Kind k, const UCHAR* buffer, size_t buffLen)
		: ClumpletReader(k, buffer, buffLen)
	{ }
	static string hexString(const UCHAR* b, size_t len)
	{
		string t1, t2;
		for (; len > 0; --len, ++b)
		{
			if (isprint(*b))
				t2 += *b;
			else
			{
				t1.printf("<%02x>", *b);
				t2 += t1;
			}
		}
		return t2;
	}
protected:
	virtual void usage_mistake(const char* what) const
	{
		fatal_exception::raiseFmt("Internal error when using clumplet API: %s", what);
	}
	virtual void invalid_structure(const char* what) const
	{
		fatal_exception::raiseFmt("Invalid clumplet buffer structure: %s", what);
	}
};

void ClumpletReader::dump() const
{
	static int dmp = 0;
	gds__log("*** DUMP ***");
	if (dmp)
	{
		// Avoid infinite recursion during dump
		gds__log("recursion");
		return;
	}
	dmp++;

	try {
		ClumpletDump d(kind, getBuffer(), getBufferLength());
		int t = (kind == SpbStart || kind == UnTagged || kind == WideUnTagged) ? -1 : d.getBufferTag();
		gds__log("Tag=%d Offset=%d Length=%d Eof=%d\n", t, getCurOffset(), getBufferLength(), isEof());
		for (d.rewind(); !(d.isEof()); d.moveNext())
		{
			gds__log("Clump %d at offset %d: %s", d.getClumpTag(), d.getCurOffset(),
				ClumpletDump::hexString(d.getBytes(), d.getClumpLength()).c_str());
		}
	}
	catch (const fatal_exception& x)
	{
		gds__log("Fatal exception during clumplet dump: %s", x.what());
		size_t l = getBufferLength() - getCurOffset();
		const UCHAR *p = getBuffer() + getCurOffset();
		gds__log("Plain dump starting with offset %d: %s", getCurOffset(),
			ClumpletDump::hexString(p, l).c_str());
	}
	dmp--;
}

}
#endif //DEBUG_CLUMPLETS

namespace Firebird {

ClumpletReader::ClumpletReader(Kind k, const UCHAR* buffer, size_t buffLen) :
	kind(k), static_buffer(buffer), static_buffer_end(buffer + buffLen)
{
	rewind();	// this will set cur_offset and spbState
}

ClumpletReader::ClumpletReader(MemoryPool& pool, Kind k, const UCHAR* buffer, size_t buffLen) :
	AutoStorage(pool), kind(k), static_buffer(buffer), static_buffer_end(buffer + buffLen)
{
	rewind();	// this will set cur_offset and spbState
}

const UCHAR* ClumpletReader::getBuffer() const
{
	return static_buffer;
}

const UCHAR* ClumpletReader::getBufferEnd() const
{
	return static_buffer_end;
}

void ClumpletReader::usage_mistake(const char* what) const
{
#ifdef DEBUG_CLUMPLETS
	dump();
#endif
	fatal_exception::raiseFmt("Internal error when using clumplet API: %s", what);
}

void ClumpletReader::invalid_structure(const char* what) const
{
#ifdef DEBUG_CLUMPLETS
	dump();
#endif
	fatal_exception::raiseFmt("Invalid clumplet buffer structure: %s", what);
}

UCHAR ClumpletReader::getBufferTag() const
{
	const UCHAR* const buffer_end = getBufferEnd();
	const UCHAR* buffer_start = getBuffer();

	switch (kind)
	{
	case Tpb:
	case Tagged:
	case WideTagged:
		if (buffer_end - buffer_start == 0)
		{
			invalid_structure("empty buffer");
			return 0;
		}
		return buffer_start[0];
	case SpbStart:
	case UnTagged:
	case WideUnTagged:
	case SpbItems:
		usage_mistake("buffer is not tagged");
		return 0;
	case SpbAttach:
		if (buffer_end - buffer_start == 0)
		{
			invalid_structure("empty buffer");
			return 0;
		}
		switch (buffer_start[0])
		{
		case isc_spb_version1:
			// This is old SPB format, it's almost like DPB -
			// buffer's tag is the first byte.
			return buffer_start[0];
		case isc_spb_version:
			// Buffer's tag is the second byte
			if (buffer_end - buffer_start == 1)
			{
				invalid_structure("buffer too short (1 byte)");
				return 0;
			}
			return buffer_start[1];
		default:
			invalid_structure("spb in service attach should begin with isc_spb_version1 or isc_spb_version");
			return 0;
		}
	default:
		fb_assert(false);
		return 0;
	}
}

ClumpletReader::ClumpletType ClumpletReader::getClumpletType(UCHAR tag) const
{
	switch (kind)
	{
	case Tagged:
	case UnTagged:
	case SpbAttach:
		return TraditionalDpb;
	case WideTagged:
	case WideUnTagged:
		return Wide;
	case Tpb:
		switch (tag)
		{
        case isc_tpb_lock_write:
        case isc_tpb_lock_read:
		case isc_tpb_lock_timeout:
			return TraditionalDpb;
		}
		return SingleTpb;
	case SpbItems:
		return SingleTpb;
	case SpbStart:
		switch (spbState)
		{
		case 0:
			return SingleTpb;
		case isc_action_svc_backup:
		case isc_action_svc_restore:
			switch (tag)
			{
			case isc_spb_bkp_file:
			case isc_spb_dbname:
			case isc_spb_res_fix_fss_data:
			case isc_spb_res_fix_fss_metadata:
				return StringSpb;
			case isc_spb_bkp_factor:
			case isc_spb_bkp_length:
			case isc_spb_res_length:
			case isc_spb_res_buffers:
			case isc_spb_res_page_size:
			case isc_spb_options:
				return IntSpb;
			case isc_spb_verbose:
				return SingleTpb;
			case isc_spb_res_access_mode:
				return ByteSpb;
			}
			invalid_structure("unknown parameter for backup/restore");
			break;
		case isc_action_svc_repair:
			switch (tag)
			{
			case isc_spb_dbname:
				return StringSpb;
			case isc_spb_options:
			case isc_spb_rpr_commit_trans:
			case isc_spb_rpr_rollback_trans:
			case isc_spb_rpr_recover_two_phase:
				return IntSpb;
			}
			invalid_structure("unknown parameter for repair");
			break;
		case isc_action_svc_add_user:
		case isc_action_svc_delete_user:
		case isc_action_svc_modify_user:
		case isc_action_svc_display_user:
		case isc_action_svc_display_user_adm:
		case isc_action_svc_set_mapping:
		case isc_action_svc_drop_mapping:
			switch (tag)
			{
			case isc_spb_dbname:
			case isc_spb_sql_role_name:
			case isc_spb_sec_username:
			case isc_spb_sec_password:
			case isc_spb_sec_groupname:
			case isc_spb_sec_firstname:
			case isc_spb_sec_middlename:
			case isc_spb_sec_lastname:
				return StringSpb;
			case isc_spb_sec_userid:
			case isc_spb_sec_groupid:
			case isc_spb_sec_admin:
				return IntSpb;
			}
			invalid_structure("unknown parameter for security database operation");
			break;
		case isc_action_svc_properties:
			switch (tag)
			{
			case isc_spb_dbname:
				return StringSpb;
			case isc_spb_prp_page_buffers:
			case isc_spb_prp_sweep_interval:
			case isc_spb_prp_shutdown_db:
			case isc_spb_prp_deny_new_attachments:
			case isc_spb_prp_deny_new_transactions:
			case isc_spb_prp_set_sql_dialect:
			case isc_spb_options:
			case isc_spb_prp_force_shutdown:
			case isc_spb_prp_attachments_shutdown:
			case isc_spb_prp_transactions_shutdown:
				return IntSpb;
			case isc_spb_prp_reserve_space:
			case isc_spb_prp_write_mode:
			case isc_spb_prp_access_mode:
			case isc_spb_prp_shutdown_mode:
			case isc_spb_prp_online_mode:
				return ByteSpb;
			}
			invalid_structure("unknown parameter for setting database properties");
			break;
//		case isc_action_svc_add_license:
//		case isc_action_svc_remove_license:
		case isc_action_svc_db_stats:
			switch (tag)
			{
			case isc_spb_dbname:
			case isc_spb_command_line:
				return StringSpb;
			case isc_spb_options:
				return IntSpb;
			}
			invalid_structure("unknown parameter for getting statistics");
			break;
		case isc_action_svc_get_ib_log:
			invalid_structure("unknown parameter for getting log");
			break;
		case isc_action_svc_nbak:
		case isc_action_svc_nrest:
			switch (tag)
			{
			case isc_spb_nbk_file:
			case isc_spb_nbk_direct:
			case isc_spb_dbname:
				return StringSpb;
			case isc_spb_nbk_level:
			case isc_spb_options:
				return IntSpb;
			}
			invalid_structure("unknown parameter for nbackup");
			break;
		case isc_action_svc_trace_start:
		case isc_action_svc_trace_stop:
		case isc_action_svc_trace_suspend:
		case isc_action_svc_trace_resume:
			switch(tag)
			{
			case isc_spb_trc_cfg:
			case isc_spb_trc_name:
				return StringSpb;
			case isc_spb_trc_id:
				return IntSpb;
			}
			break;
		}
		invalid_structure("wrong spb state");
		break;
	}
	invalid_structure("unknown reason");
	return SingleTpb;
}

void ClumpletReader::adjustSpbState()
{
	switch (kind)
	{
	case SpbStart:
		if (spbState == 0) {	// Just started with service start block
			spbState = getClumpTag();
		}
		break;
	default:
		break;
	}
}

size_t ClumpletReader::getClumpletSize(bool wTag, bool wLength, bool wData) const
{
	const UCHAR* clumplet = getBuffer() + cur_offset;
	const UCHAR* const buffer_end = getBufferEnd();

	// Check for EOF
	if (clumplet >= buffer_end)
	{
		usage_mistake("read past EOF");
		return 0;
	}

	size_t rc = wTag ? 1 : 0;
	size_t lengthSize = 0;
	size_t dataSize = 0;

	switch (getClumpletType(clumplet[0]))
	{

	// This form allows clumplets of virtually any size
	case Wide:
		// Check did we receive length component for clumplet
		if (buffer_end - clumplet < 5)
		{
			invalid_structure("buffer end before end of clumplet - no length component");
			return rc;
		}
		lengthSize = 4;
		dataSize = clumplet[4];
		dataSize <<= 8;
		dataSize += clumplet[3];
		dataSize <<= 8;
		dataSize += clumplet[2];
		dataSize <<= 8;
		dataSize += clumplet[1];
		break;

	// This is the most widely used form
	case TraditionalDpb:
		// Check did we receive length component for clumplet
		if (buffer_end - clumplet < 2)
		{
			invalid_structure("buffer end before end of clumplet - no length component");
			return rc;
		}
		lengthSize = 1;
		dataSize = clumplet[1];
		break;

	// Almost all TPB parameters are single bytes
	case SingleTpb:
		break;

	// Used in SPB for long strings
	case StringSpb:
		// Check did we receive length component for clumplet
		if (buffer_end - clumplet < 3)
		{
			invalid_structure("buffer end before end of clumplet - no length component");
			return rc;
		}
		lengthSize = 2;
		dataSize = clumplet[2];
		dataSize <<= 8;
		dataSize += clumplet[1];
		break;

	// Used in SPB for 4-byte integers
	case IntSpb:
		dataSize = 4;
		break;

	// Used in SPB for single byte
	case ByteSpb:
		dataSize = 1;
		break;
	}

	const size_t total = 1 + lengthSize + dataSize;
	if (clumplet + total > buffer_end)
	{
		invalid_structure("buffer end before end of clumplet - clumplet too long");
		size_t delta = total - (buffer_end - clumplet);
		if (delta > dataSize)
			dataSize = 0;
		else
			dataSize -= delta;
	}

	if (wLength) {
		rc += lengthSize;
	}
	if (wData) {
		rc += dataSize;
	}
	return rc;
}

void ClumpletReader::moveNext()
{
	if (isEof())
		return;		// no need to raise useless exceptions
	size_t cs = getClumpletSize(true, true, true);
	adjustSpbState();
	cur_offset += cs;
}

void ClumpletReader::rewind()
{
	if (! getBuffer())
	{
		cur_offset = 0;
		spbState = 0;
		return;
	}
	switch (kind)
	{
	case UnTagged:
	case WideUnTagged:
	case SpbStart:
	case SpbItems:
		cur_offset = 0;
		break;
	default:
		if (kind == SpbAttach && getBufferLength() > 0 && getBuffer()[0] != isc_spb_version1)
			cur_offset = 2;
		else
			cur_offset = 1;
	}
	spbState = 0;
}

bool ClumpletReader::find(UCHAR tag)
{
	const size_t co = getCurOffset();
	for (rewind(); !isEof(); moveNext())
	{
		if (tag == getClumpTag())
		{
			return true;
		}
	}
	setCurOffset(co);
	return false;
}

// Methods which work with currently selected clumplet
UCHAR ClumpletReader::getClumpTag() const
{
	const UCHAR* clumplet = getBuffer() + cur_offset;
	const UCHAR* const buffer_end = getBufferEnd();

	// Check for EOF
	if (clumplet >= buffer_end)
	{
		usage_mistake("read past EOF");
		return 0;
	}

	return clumplet[0];
}

size_t ClumpletReader::getClumpLength() const
{
	return getClumpletSize(false, false, true);
}

const UCHAR* ClumpletReader::getBytes() const
{
	return getBuffer() + cur_offset + getClumpletSize(true, true, false);
}

SINT64 ClumpletReader::fromVaxInteger(const UCHAR* ptr, size_t length)
{
	// We can't handle numbers bigger than int64. Some cases use length == 0.
	fb_assert(ptr && length >= 0 && length < 9);
	// This code is taken from gds__vax_integer
	SINT64 value = 0;
	int shift = 0;
	while (length > 0)
	{
		--length;
		value += ((SINT64) *ptr++) << shift;
		shift += 8;
	}

	return value;
}

SLONG ClumpletReader::getInt() const
{
	const size_t length = getClumpLength();

	if (length > 4)
	{
		invalid_structure("length of integer exceeds 4 bytes");
		return 0;
	}

	return fromVaxInteger(getBytes(), length);
}

double ClumpletReader::getDouble() const
{

	if (getClumpLength() != sizeof(double))
	{
		invalid_structure("length of double must be equal 8 bytes");
		return 0;
	}

	// based on XDR code
	union {
		double temp_double;
		SLONG temp_long[2];
	} temp;

	fb_assert(sizeof(double) == sizeof(temp));

	const UCHAR* ptr = getBytes();
	temp.temp_long[FB_LONG_DOUBLE_FIRST] = fromVaxInteger(ptr, sizeof(SLONG));
	temp.temp_long[FB_LONG_DOUBLE_SECOND] = fromVaxInteger(ptr + sizeof(SLONG), sizeof(SLONG));

	return temp.temp_double;
}

ISC_TIMESTAMP ClumpletReader::getTimeStamp() const
{
	ISC_TIMESTAMP value;

	if (getClumpLength() != sizeof(ISC_TIMESTAMP))
	{
		invalid_structure("length of ISC_TIMESTAMP must be equal 8 bytes");
		value.timestamp_date = 0;
		value.timestamp_time = 0;
		return value;
	}

	const UCHAR* ptr = getBytes();
	value.timestamp_date = fromVaxInteger(ptr, sizeof(SLONG));
	value.timestamp_time = fromVaxInteger(ptr + sizeof(SLONG), sizeof(SLONG));
	return value;
}

SINT64 ClumpletReader::getBigInt() const
{
	const size_t length = getClumpLength();

	if (length > 8)
	{
		invalid_structure("length of BigInt exceeds 8 bytes");
		return 0;
	}

	return fromVaxInteger(getBytes(), length);
}

string& ClumpletReader::getString(string& str) const
{
	const UCHAR* ptr = getBytes();
	const size_t length = getClumpLength();
	str.assign(reinterpret_cast<const char*>(ptr), length);
	str.recalculate_length();
	if (str.length() + 1 < length)
	{
		invalid_structure("string length doesn't match with clumplet");
	}
	return str;
}

PathName& ClumpletReader::getPath(PathName& str) const
{
	const UCHAR* ptr = getBytes();
	const size_t length = getClumpLength();
	str.assign(reinterpret_cast<const char*>(ptr), length);
	str.recalculate_length();
	if (str.length() + 1 < length)
	{
		invalid_structure("path length doesn't match with clumplet");
	}
	return str;
}

bool ClumpletReader::getBoolean() const
{
	const UCHAR* ptr = getBytes();
	const size_t length = getClumpLength();
	if (length > 1)
	{
		invalid_structure("length of boolean exceeds 1 byte");
		return false;
	}
	return length && ptr[0];
}

} // namespace

