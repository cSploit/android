/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Claudio Valderrama C. for IBPhoenix.
 *  The development of the Original Code was sponsored by Craig Leonardi.
 *
 *  Copyright (c) 2001 IBPhoenix
 *  All Rights Reserved.
 *
 * 2002.01.07 Claudio Valderrama: change the impolite way truncate and round work,
 *	make null handling more consistent and add dpower(x,y).
 *	Beware the SQL declaration for those functions has changed.
 * 2002.01.20 Claudio Valderrama: addMonth should work with negative values, too.
 * 2003.10.26: Made some values const and other minor changes.
 * 2004.09.29 Claudio Valderrama: fix numeric overflow in addHour reported by
 *	"jssahdra" <joga.singh at inventum.cc>. Since all add<time> functions are
 *	wrappers around addTenthMSec, only two lines needed to be fixed.
 */


// fbudf.cpp : Defines the entry point for the DLL application.
//

/*
CVC: The MS way of doing things puts the includes in stdafx. I expect
that you can continue this trend without problems on other compilers
or platforms. Since I conditioned the Windows-related includes, it should
be easy to add needed headers to stdafx.h after a makefile is built.
*/


#include "stdafx.h"

#ifndef FBUDF_EXPORTS
#define FBUDF_EXPORTS
#endif

#include "fbudf.h"
#include "../common/classes/timestamp.h"

#if defined(HAVE_GETTIMEOFDAY) && (!(defined (HAVE_LOCALTIME_R) && defined (HAVE_GMTIME_R)))
#define NEED_TIME_MUTEX
#include "../common/classes/locks.h"
#endif


//Original code for this library was written by Claudio Valderrama
// on July 2001 for IBPhoenix.

// Define this symbol if you want truncate and round to be symmetric wr to zero.
//#define SYMMETRIC_MATH

#if defined (_WIN32)
/*
BOOL APIENTRY DllMain( HANDLE ,//hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID //lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
*/
#endif


// To do: go from C++ native types to abstract FB types.


#ifdef DEV_BUILD
// This function shouldn't be defined in production.
FBUDF_API paramdsc* testreflect(paramdsc* rc)
{
	rc->dsc_address = 0;
	rc->dsc_flags = 1 | 4; //DSC_null | DSC_nullable;
	return rc;
}
#endif


namespace internal
{
	typedef ISC_USHORT fb_len;

	const long seconds_in_day = 86400;
	const long tenthmsec_in_day = seconds_in_day * ISC_TIME_SECONDS_PRECISION;
	const int varchar_indicator_size = sizeof(ISC_USHORT);
	const int max_varchar_size = 65535 - varchar_indicator_size; // in theory


	// This definition comes from jrd\val.h and is used in helper
	// functions {get/set}_varchar_len defined below.
	struct vvary
	{
		fb_len		vary_length;
		ISC_UCHAR	vary_string[max_varchar_size];
	};

	/*
	inline fb_len get_varchar_len(const char* vchar)
	{
		return reinterpret_cast<const vvary*>(vchar)->vary_length;
	}
	*/

	inline fb_len get_varchar_len(const ISC_UCHAR* vchar)
	{
		return reinterpret_cast<const vvary*>(vchar)->vary_length;
	}

	inline void set_varchar_len(char* vchar, const fb_len len)
	{
		reinterpret_cast<vvary*>(vchar)->vary_length = len;
	}

	inline void set_varchar_len(ISC_UCHAR* vchar, const fb_len len)
	{
		reinterpret_cast<vvary*>(vchar)->vary_length = len;
	}

	bool isnull(const paramdsc* v)
	{
		return !v || !v->dsc_address || (v->dsc_flags & DSC_null);
	}

	paramdsc* setnull(paramdsc* v)
	{
		if (v)
			v->dsc_flags |= DSC_null;
		return v;
	}

	int get_int_type(const paramdsc* v, ISC_INT64& rc)
	{
		int s = -1;
		switch (v->dsc_dtype)
		{
		case dtype_short:
			rc = *reinterpret_cast<ISC_SHORT*>(v->dsc_address);
			s = sizeof(ISC_SHORT);
			break;
		case dtype_long:
			rc = *reinterpret_cast<ISC_LONG*>(v->dsc_address);
			s = sizeof(ISC_LONG);
			break;
		case dtype_int64:
			rc = *reinterpret_cast<ISC_INT64*>(v->dsc_address);
			s = sizeof(ISC_INT64);
			break;
		default:
			break;
		}
		return s;
	}

	void set_int_type(paramdsc* v, const ISC_INT64 iv)
	{
		switch (v->dsc_dtype)
		{
		case dtype_short:
			*reinterpret_cast<ISC_SHORT*>(v->dsc_address) = static_cast<ISC_SHORT>(iv);
			break;
		case dtype_long:
			*reinterpret_cast<ISC_LONG*>(v->dsc_address) = static_cast<ISC_LONG>(iv);
			break;
		case dtype_int64:
			*reinterpret_cast<ISC_INT64*>(v->dsc_address) = iv;
			break;
		}
	}

	int get_double_type(const paramdsc* v, double& rc)
	{
		int s = -1;
		switch (v->dsc_dtype)
		{
		case dtype_real:
			rc = static_cast<double>(*reinterpret_cast<float*>(v->dsc_address));
			s = sizeof(float);
			break;
		case dtype_double:
			rc = *reinterpret_cast<double*>(v->dsc_address);
			s = sizeof(double);
			break;
		default:
			break;
		}
		return s;
	}

	void set_double_type(paramdsc* v, const double iv)
	{
		switch (v->dsc_dtype)
		{
		case dtype_real:
			*reinterpret_cast<float*>(v->dsc_address) = static_cast<float>(iv);
			break;
		case dtype_double:
			*reinterpret_cast<double*>(v->dsc_address) = iv;
			break;
		}
	}

	int get_any_string_type(const paramdsc* v, ISC_UCHAR*& text)
	{
		int len = v->dsc_length;
		switch (v->dsc_dtype)
		{
		case dtype_text:
			text = v->dsc_address;
			break;
		case dtype_cstring:
			text = v->dsc_address;
			if (len && text)
			{
				const ISC_UCHAR* p = text; //strlen(v->dsc_address);
				while (*p)
					++p; // couldn't use strlen!
				if (p - text < len)
					len = p - text;
			}
			break;
		case dtype_varying:
			len -= varchar_indicator_size;
			text = reinterpret_cast<vvary*>(v->dsc_address)->vary_string;
			{
				const int x = get_varchar_len(v->dsc_address);
				if (x < len)
					len = x;
			}
			break;
		default:
			len = -1;
			break;
		}
		return len;
	}

	void set_any_string_type(paramdsc* v, const int len0, ISC_UCHAR* text = 0)
	{
		fb_len len = static_cast<fb_len>(len0);
		switch (v->dsc_dtype)
		{
		case dtype_text:
			v->dsc_length = len;
			if (text)
				memcpy(v->dsc_address, text, len);
			else
				memset(v->dsc_address, ' ', len);
			break;
		case dtype_cstring:
			v->dsc_length = len;
			if (text)
				memcpy(v->dsc_address, text, len);
			else
				v->dsc_length = len = 0;
			v->dsc_address[len] = 0;
			break;
		case dtype_varying:
			if (!text)
				len = 0;
			else if (len > max_varchar_size)
				len = max_varchar_size;
			v->dsc_length = len + static_cast<fb_len>(varchar_indicator_size);
			set_varchar_len(v->dsc_address, len);
			if (text)
				memcpy(v->dsc_address + varchar_indicator_size, text, len);
			break;
		}
	}

	int get_scaled_double(const paramdsc* v, double& rc)
	{
		ISC_INT64 iv;
		int rct = get_int_type(v, iv);
		if (rct < 0)
			rct = get_double_type(v, rc);
		else
		{
			rc = static_cast<double>(iv);
			int scale = v->dsc_scale;
			for (; scale < 0; ++scale)
				rc /= 10;
			for (; scale > 0; --scale)
				rc *= 10;
		}
		return rct;
	}
} // namespace internal


// BEGIN DEPRECATED FUNCTIONS.
FBUDF_API paramdsc* idNvl(paramdsc* v, paramdsc* v2)
{
	if (!internal::isnull(v))
		return v;
	return v2;
}

FBUDF_API void sNvl(const paramdsc* v, const paramdsc* v2, paramdsc* rc)
{
	if (!internal::isnull(v))
	{
		ISC_UCHAR* sv = 0;
		const int len = internal::get_any_string_type(v, sv);
		if (len < 0)
			internal::setnull(rc);
		else
			internal::set_any_string_type(rc, len, sv);
		return;
	}
	if (!internal::isnull(v2))
	{
		ISC_UCHAR* sv2 = 0;
		const int len = internal::get_any_string_type(v2, sv2);
		if (len < 0)
			internal::setnull(rc);
		else
			internal::set_any_string_type(rc, len, sv2);
		return;
	}
	internal::setnull(rc);
}


FBUDF_API paramdsc* iNullIf(paramdsc* v, paramdsc* v2)
{
	if (internal::isnull(v) || internal::isnull(v2))
		return 0;
	ISC_INT64 iv, iv2;
	const int rc = internal::get_int_type(v, iv);
	const int rc2 = internal::get_int_type(v2, iv2);
	if (rc < 0 || rc2 < 0)
		return v;
	if (iv == iv2 && v->dsc_scale == v2->dsc_scale)
		return 0;
	return v;
}

FBUDF_API paramdsc* dNullIf(paramdsc* v, paramdsc* v2)
{
	if (internal::isnull(v) || internal::isnull(v2))
		return 0;
	double iv, iv2;
	const int rc = internal::get_double_type(v, iv);
	const int rc2 = internal::get_double_type(v2, iv2);
	if (rc < 0 || rc2 < 0)
		return v;
	if (iv == iv2) // && v->dsc_scale == v2->dsc_scale) double w/o scale
		return 0;
	return v;
}

FBUDF_API void sNullIf(const paramdsc* v, const paramdsc* v2, paramdsc* rc)
{
	if (internal::isnull(v) || internal::isnull(v2))
	{
		internal::setnull(rc);
		return;
	}
	ISC_UCHAR* sv;
	const int len = internal::get_any_string_type(v, sv);
	ISC_UCHAR* sv2;
	const int len2 = internal::get_any_string_type(v2, sv2);
	if (len < 0 || len2 < 0) // good luck with the result, we can't do more.
		return;
	if (len == len2 && (!len || !memcmp(sv, sv2, len)) &&
		(v->dsc_sub_type == v2->dsc_sub_type || // ttype
		!v->dsc_sub_type || !v2->dsc_sub_type)) // tyype
	{
		internal::setnull(rc);
		return;
	}
	internal::set_any_string_type(rc, len, sv);
	return;
}
// END DEPRECATED FUNCTIONS.

namespace internal
{
	void decode_timestamp(const GDS_TIMESTAMP* date, tm* times_arg)
	{
		Firebird::TimeStamp::decode_timestamp(*date, times_arg);
	}

	void encode_timestamp(const tm* times_arg, GDS_TIMESTAMP* date)
	{
		*date = Firebird::TimeStamp::encode_timestamp(times_arg);
	}

	enum day_format {day_short, day_long};
	const fb_len day_len[] = {4, 14};
	const char* day_fmtstr[] = {"%a", "%A"};

	void get_DOW(const ISC_TIMESTAMP* v, paramvary* rc, const day_format df)
	{
		tm times;
		decode_timestamp(v, &times);
		const int dow = times.tm_wday;
		if (dow >= 0 && dow <= 6)
		{
			fb_len name_len = day_len[df];
			const char* name_fmt = day_fmtstr[df];
			// There should be a better way to do this than to alter the thread's locale.
			if (!strcmp(setlocale(LC_TIME, NULL), "C"))
				setlocale(LC_ALL, "");
			char* const target = reinterpret_cast<char*>(rc->vary_string);
			name_len = strftime(target, name_len, name_fmt, &times);
			if (name_len)
			{
				// There's no clarity in the docs whether '\0' is counted or not; be safe.
				if (!target[name_len - 1])
					--name_len;
				rc->vary_length = name_len;
				return;
			}
		}
		rc->vary_length = 5;
		memcpy(rc->vary_string, "ERROR", 5);
	}
} // namespace internal

FBUDF_API void DOW(const ISC_TIMESTAMP* v, paramvary* rc)
{
	internal::get_DOW(v, rc, internal::day_long);
}

FBUDF_API void SDOW(const ISC_TIMESTAMP* v, paramvary* rc)
{
	internal::get_DOW(v, rc, internal::day_short);
}

FBUDF_API void right(const paramdsc* v, const ISC_SHORT& rl, paramdsc* rc)
{
	if (internal::isnull(v))
	{
		internal::setnull(rc);
		return;
	}
	ISC_UCHAR* text = 0;
	int len = internal::get_any_string_type(v, text);
	const int diff = len - rl;
	if (rl < len)
		len = rl;
	if (len < 0)
	{
		internal::setnull(rc);
		return;
	}
	if (diff > 0)
		text += diff;
	internal::set_any_string_type(rc, len, text);
	return;
}

FBUDF_API ISC_TIMESTAMP* addDay(ISC_TIMESTAMP* v, const ISC_LONG& ndays)
{
	v->timestamp_date += ndays;
	return v;
}

FBUDF_API void addDay2(const ISC_TIMESTAMP* v0, const ISC_LONG& ndays, ISC_TIMESTAMP* v)
{
	*v = *v0;
	v->timestamp_date += ndays;
}

FBUDF_API ISC_TIMESTAMP* addWeek(ISC_TIMESTAMP* v, const ISC_LONG& nweeks)
{
	v->timestamp_date += nweeks * 7;
	return v;
}

FBUDF_API ISC_TIMESTAMP* addMonth(ISC_TIMESTAMP* v, const ISC_LONG& nmonths)
{
	tm times;
	internal::decode_timestamp(v, &times);
	const int y = nmonths / 12, m = nmonths % 12;
	times.tm_year += y;
	if ((times.tm_mon += m) > 11)
	{
		++times.tm_year;
		times.tm_mon -= 12;
	}
	else if (times.tm_mon < 0)
	{
		--times.tm_year;
		times.tm_mon += 12;
	}
	const int ly = times.tm_year + 1900;
	const bool leap = ly % 4 == 0 && ly % 100 != 0 || ly % 400 == 0;
	const int md[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (times.tm_mday > md[times.tm_mon])
		times.tm_mday = md[times.tm_mon];
	internal::encode_timestamp(&times, v);
	return v;
}

FBUDF_API ISC_TIMESTAMP* addYear(ISC_TIMESTAMP* v, const ISC_LONG& nyears)
{
	tm times;
	internal::decode_timestamp(v, &times);
	times.tm_year += nyears;
	internal::encode_timestamp(&times, v);
	return v;
}

namespace internal
{
	ISC_TIMESTAMP* addTenthMSec(ISC_TIMESTAMP* v, SINT64 tenthmilliseconds, int multiplier)
	{
		const SINT64 full = tenthmilliseconds * multiplier;
		const long days = full / tenthmsec_in_day, secs = full % tenthmsec_in_day;
		v->timestamp_date += days;
		 // Time portion is unsigned, so we avoid unsigned rolling over negative values
		// that only produce a new unsigned number with the wrong result.
		if (secs < 0 && static_cast<ISC_ULONG>(-secs) > v->timestamp_time)
		{
			--v->timestamp_date;
			v->timestamp_time += tenthmsec_in_day + secs;
		}
		else if ((v->timestamp_time += secs) >= static_cast<ISC_ULONG>(tenthmsec_in_day))
		{
			++v->timestamp_date;
			v->timestamp_time -= tenthmsec_in_day;
		}
		return v;
	}
} // namespace internal

FBUDF_API ISC_TIMESTAMP* addMilliSecond(ISC_TIMESTAMP* v, const ISC_LONG& nmseconds)
{
	return internal::addTenthMSec(v, nmseconds, ISC_TIME_SECONDS_PRECISION / 1000);
}

FBUDF_API ISC_TIMESTAMP* addSecond(ISC_TIMESTAMP* v, const ISC_LONG& nseconds)
{
	return internal::addTenthMSec(v, nseconds, ISC_TIME_SECONDS_PRECISION);
}

FBUDF_API ISC_TIMESTAMP* addMinute(ISC_TIMESTAMP* v, const ISC_LONG& nminutes)
{
	return internal::addTenthMSec(v, nminutes, 60 * ISC_TIME_SECONDS_PRECISION);
}

FBUDF_API ISC_TIMESTAMP* addHour(ISC_TIMESTAMP* v, const ISC_LONG& nhours)
{
	return internal::addTenthMSec(v, nhours, 3600 * ISC_TIME_SECONDS_PRECISION);
}

#ifdef NEED_TIME_MUTEX
Firebird::Mutex timeMutex;
#endif

FBUDF_API void getExactTimestamp(ISC_TIMESTAMP* rc)
{
#if defined(HAVE_GETTIMEOFDAY)
	timeval tv;
	GETTIMEOFDAY(&tv);
	const time_t seconds = tv.tv_sec;

	tm timex;
#if defined(HAVE_LOCALTIME_R)
	tm* times = localtime_r(&seconds, &timex);
#else
	timeMutex.enter();
	tm* times = localtime(&seconds);
	if (times)
	{
		// Copy to local variable before we exit the mutex.
		timex = *times;
		times = &timex;
	}
	timeMutex.leave();
#endif // localtime_r

	if (times)
	{
		internal::encode_timestamp(times, rc);
		rc->timestamp_time += tv.tv_usec / 100;
	}
	else
		rc->timestamp_date = rc->timestamp_time = 0;

#else // gettimeofday
	_timeb timebuffer;
	_ftime(&timebuffer);
	// localtime uses thread local storage in NT, no need to lock threads.
	// Of course, this facility is only available in multithreaded builds.
	tm* times = localtime(&timebuffer.time);
	if (times)
	{
		internal::encode_timestamp(times, rc);
		rc->timestamp_time += timebuffer.millitm * 10;
	}
	else
		rc->timestamp_date = rc->timestamp_time = 0;
#endif
	return;
}

FBUDF_API void getExactTimestampUTC(ISC_TIMESTAMP* rc)
{
#if defined(HAVE_GETTIMEOFDAY)
	timeval tv;
	GETTIMEOFDAY(&tv);
	const time_t seconds = tv.tv_sec;

	tm timex;
#if defined(HAVE_GMTIME_R)
	tm* times = gmtime_r(&seconds, &timex);
#else
	timeMutex.enter();
	tm* times = gmtime(&seconds);
	if (times)
	{
		// Copy to local variable before we exit the mutex.
		timex = *times;
		times = &timex;
	}
	timeMutex.leave();
#endif // gmtime_r

	if (times)
	{
		internal::encode_timestamp(times, rc);
		rc->timestamp_time += tv.tv_usec / 100;
	}
	else
		rc->timestamp_date = rc->timestamp_time = 0;

#else // gettimeofday
	_timeb timebuffer;
	_ftime(&timebuffer);
	// gmtime uses thread local storage in NT, no need to lock threads.
	// Of course, this facility is only available in multithreaded builds.
	tm* times = gmtime(&timebuffer.time);
	if (times)
	{
		internal::encode_timestamp(times, rc);
		rc->timestamp_time += timebuffer.millitm * 10;
	}
	else
		rc->timestamp_date = rc->timestamp_time = 0;
#endif
	return;
}

FBUDF_API ISC_LONG isLeapYear(const ISC_TIMESTAMP* v)
{
	tm times;
	internal::decode_timestamp(v, &times);
	const int ly = times.tm_year + 1900;
	return ly % 4 == 0 && ly % 100 != 0 || ly % 400 == 0;
}

FBUDF_API void fbtruncate(const paramdsc* v, paramdsc* rc)
{
	if (internal::isnull(v))
	{
		internal::setnull(rc);
		return;
	}
	ISC_INT64 iv;
	const int rct = internal::get_int_type(v, iv);
	if (rct < 0 || v->dsc_scale > 0)
	{
		internal::setnull(rc);
		return;
	}
	if (!v->dsc_scale /*|| !v->dsc_sub_type*/) //second test won't work with ODS9
	{
		internal::set_int_type(rc, iv);
		rc->dsc_scale = 0;
		return;
	}

	// truncate(0.9)  =>  0
	// truncate(-0.9) => -1
	// truncate(-0.9) =>  0 ### SYMMETRIC_MATH defined.
	int scale = v->dsc_scale;
#if defined(SYMMETRIC_MATH)
	while (scale++ < 0)
		iv /= 10;
#else
	const bool isNeg = iv < 0;
	bool gt = false;
	while (scale++ < 0)
	{
		if (iv % 10)
			gt = true;
		iv /= 10;
	}
	if (gt)
	{
		if (isNeg)
			--iv;
	}
#endif
	internal::set_int_type(rc, iv);
	rc->dsc_scale = 0;
	return;
}

FBUDF_API void fbround(const paramdsc* v, paramdsc* rc)
{
	if (internal::isnull(v))
	{
		internal::setnull(rc);
		return;
	}
	ISC_INT64 iv;
	const int rct = internal::get_int_type(v, iv);
	if (rct < 0 || v->dsc_scale > 0)
	{
		internal::setnull(rc);
		return;
	}
	if (!v->dsc_scale /*|| !v->dsc_sub_type*/) //second test won't work with ODS9
	{
		internal::set_int_type(rc, iv);
		rc->dsc_scale = 0;
		return;
	}

	// round(0.3)  => 0 ### round(0.5)  =>  1
	// round(-0.3) => 0 ### round(-0.5) =>  0
	// round(-0.3) => 0 ### round(-0.5) => -1 ### SYMMETRIC_MATH defined.
	const bool isNeg = iv < 0;
	int scale = v->dsc_scale;
	bool gt = false, check_more = false;
	while (scale++ < 0)
	{
		if (!scale)
		{
			int dig;
			if (iv == MIN_SINT64)
				dig = -(iv + 10) % 10;
			else
				dig = static_cast<int>(iv >= 0 ? iv % 10 : -iv % 10);

#if defined(SYMMETRIC_MATH)
			if (dig >= 5)
				gt = true;
#else
			if (!isNeg)
			{
				if (dig >= 5)
					gt = true;
			}
 			else
 			{
 				if (dig > 5 || dig == 5 && check_more)
	 				gt = true;
 			}
#endif
		}
		else if (isNeg && !check_more)
		{
			if (iv % 10 != 0)
				check_more = true;
		}
		iv /= 10;
	}
	if (gt)
	{
		if (isNeg)
			--iv;
		else
			++iv;
	}
	internal::set_int_type(rc, iv);
	rc->dsc_scale = 0;
	return;
}

FBUDF_API void power(const paramdsc* v, const paramdsc* v2, paramdsc* rc)
{
	if (internal::isnull(v) || internal::isnull(v2))
	{
		internal::setnull(rc);
		return;
	}
	double d, d2;
	const int rct = internal::get_scaled_double(v, d);
	const int rct2 = internal::get_scaled_double(v2, d2);

	// If we cause a div by zero, SS shuts down in response.
	// The doc I read says 0^0 will produce 1, so it's not tested below.
	if (rct < 0 || rct2 < 0 || !d && d2 < 0)
	{
		internal::setnull(rc);
		return;
	}

	internal::set_double_type(rc, pow(d, d2));
	rc->dsc_scale = 0;
	return;
}

FBUDF_API void string2blob(const paramdsc* v, blobcallback* outblob)
{
	if (internal::isnull(v))
	{
	    outblob->blob_handle = 0; // hint for the engine, null blob.
		return;
	}
	ISC_UCHAR* text = 0;
	const int len = internal::get_any_string_type(v, text);
	if (len < 0 && outblob)
		outblob->blob_handle = 0; // hint for the engine, null blob.
	if (!outblob || !outblob->blob_handle)
		return;
	outblob->blob_put_segment(outblob->blob_handle, text, len);
	return;
}

