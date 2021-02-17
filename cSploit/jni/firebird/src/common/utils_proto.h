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
 *  The Original Code was created by Claudio Valderrama on 25-Dec-2003
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2003 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  Nickolay Samofatov <nickolay@broadviewsoftware.com>
 */


// =====================================
// Utility functions

#ifndef INCLUDE_UTILS_PROTO_H
#define INCLUDE_UTILS_PROTO_H

#include <string.h>
#include "../common/classes/fb_string.h"
#include "../common/classes/array.h"
#include "gen/iberror.h"
#include "firebird/Provider.h"

#ifdef SFIO
#include <stdio.h>
#endif

namespace fb_utils
{
	char* copy_terminate(char* dest, const char* src, size_t bufsize);
	char* exact_name(char* const name);
	inline void exact_name(Firebird::string& str)
	{
		str.rtrim();
	}
	char* exact_name_limit(char* const name, size_t bufsize);
	bool implicit_domain(const char* domain_name);
	bool implicit_integrity(const char* integ_name);
	bool implicit_pk(const char* pk_name);
	int name_length(const TEXT* const name);
	int name_length_limit(const TEXT* const name, size_t bufsize);
	bool readenv(const char* env_name, Firebird::string& env_value);
	bool readenv(const char* env_name, Firebird::PathName& env_value);
	int snprintf(char* buffer, size_t count, const char* format...);
	char* cleanup_passwd(char* arg);
	inline char* get_passwd(char* arg)
	{
		return cleanup_passwd(arg);
	}
	typedef char* arg_string;

	// Warning: Only wrappers:

	// ********************
	// s t r i c m p
	// ********************
	// Abstraction of incompatible routine names
	// for case insensitive comparison.
	inline int stricmp(const char* a, const char* b)
	{
#if defined(HAVE_STRCASECMP)
		return ::strcasecmp(a, b);
#elif defined(HAVE_STRICMP)
		return ::stricmp(a, b);
#else
#error dont know how to compare strings case insensitive on this system
#endif
	}


	// ********************
	// s t r n i c m p
	// ********************
	// Abstraction of incompatible routine names
	// for counted length and case insensitive comparison.
	inline int strnicmp(const char* a, const char* b, size_t count)
	{
#if defined(HAVE_STRNCASECMP)
		return ::strncasecmp(a, b, count);
#elif defined(HAVE_STRNICMP)
		return ::strnicmp(a, b, count);
#else
#error dont know how to compare counted length strings case insensitive on this system
#endif
	}

#ifdef WIN_NT
	bool prefix_kernel_object_name(char* name, size_t bufsize);
	bool isGlobalKernelPrefix();
#endif

	Firebird::PathName get_process_name();
	SLONG genUniqueId();

	void getCwd(Firebird::PathName& pn);

	void inline init_status(ISC_STATUS* status)
	{
		status[0] = isc_arg_gds;
		status[1] = FB_SUCCESS;
		status[2] = isc_arg_end;
	}

	void inline init_status(Firebird::IStatus* status)
	{
		status->init();
	}

	unsigned int copyStatus(ISC_STATUS* const to, const unsigned int space,
							const ISC_STATUS* const from, const unsigned int count) throw();

	unsigned int statusLength(const ISC_STATUS* const status) throw();

	enum FetchPassResult {
		FETCH_PASS_OK,
		FETCH_PASS_FILE_OPEN_ERROR,
		FETCH_PASS_FILE_READ_ERROR,
		FETCH_PASS_FILE_EMPTY
	};
	FetchPassResult fetchPassword(const Firebird::PathName& name, const char*& password);

	// Returns current value of performance counter
	SINT64 query_performance_counter();

	// Returns frequency of performance counter in Hz
	SINT64 query_performance_frequency();

	void get_process_times(SINT64 &userTime, SINT64 &sysTime);

	void exactNumericToStr(SINT64 value, int scale, Firebird::string& target, bool append = false);

	// Returns true if called from firebird build process (appr. environment is set)
	bool bootBuild();

	// Add appropriate file prefix.
	Firebird::PathName getPrefix(unsigned prefType, const char* name);

	// moves DB path information (from limbo transaction) to another buffer
	void getDbPathInfo(unsigned int& itemsLength, const unsigned char*& items,
		unsigned int& bufferLength, unsigned char*& buffer,
		Firebird::Array<unsigned char>& newItemsBuffer, const Firebird::PathName& dbpath);

	// returns true if passed info items work with running svc thread
	bool isRunningCheck(const UCHAR* items, unsigned int length);

	// converts bytes to BASE64 representation
	void base64(Firebird::string& b64, const Firebird::UCharBuffer& bin);

	// generate random string in BASE64 representation
	void random64(Firebird::string& randomValue, size_t length);

	void logAndDie(const char* text);

	// Returns next offset value
	unsigned sqlTypeToDsc(unsigned prevOffset, unsigned sqlType, unsigned sqlLength,
		unsigned* dtype, unsigned* len, unsigned* offset, unsigned* nullOffset);

	// Check does vector contain particular code or not
	bool containsErrorCode(const ISC_STATUS* v, ISC_STATUS code);
} // namespace fb_utils

#endif // INCLUDE_UTILS_PROTO_H
