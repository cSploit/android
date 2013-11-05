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

#include "firebird.h"
#include "../jrd/common.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#else
#define __need_size_t
#include <stddef.h>
#undef __need_size_t
#endif
#include <stdarg.h>
#include <stdio.h>

#include "../jrd/gdsassert.h"
#include "../common/utils_proto.h"
#include "../common/classes/locks.h"
#include "../common/classes/init.h"
#include "../jrd/constants.h"
#include "../jrd/os/path_utils.h"

#ifdef WIN_NT
#include <direct.h>
#include <io.h> // isatty()
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

namespace fb_utils
{

bool implicit_name(const char* name, const char* prefix, int prefix_len);


char* copy_terminate(char* dest, const char* src, size_t bufsize)
{
/**************************************
 *
 * c o p y _ t e r m i n a t e
 *
 **************************************
 *
 * Functional description
 *	Do the same as strncpy but ensure the null terminator is written.
 *
 **************************************/
	if (!bufsize) // Was it a joke?
		return dest;

	--bufsize;
	strncpy(dest, src, bufsize);
	dest[bufsize] = 0;
	return dest;
}


char* exact_name(char* const str)
{
/**************************************
 *
 *	e x a c t _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Trim off trailing spaces from a metadata name.
 *	eg: insert a null after the last non-blank character.
 *
 *	SQL delimited identifier may have blank as part of the name
 *
 *	Parameters:  str - the string to terminate
 *	Returns:     str
 *
 **************************************/
	char* p = str;
	while (*p)
	    ++p;
	// Now, let's go back
	--p;
	while (p >= str && *p == '\x20') // blank character, ASCII(32)
		--p;
	*(p + 1) = '\0';
	return str;
}


char* exact_name_limit(char* const str, size_t bufsize)
{
/**************************************
 *
 *	e x a c t _ n a m e _ l i m i t
 *
 **************************************
 *
 * Functional description
 *	Trim off trailing spaces from a metadata name.
 *	eg: insert a null after the last non-blank character.
 *	It has a maximum length to ensure working between bounds.
 *
 *	SQL delimited identifier may have blank as part of the name
 *
 *	Parameters:  str - the string to terminate
 *               bufsize - the size of the variable containing the string.
 *	Returns:     str
 *
 **************************************/
	const char* const end = str + bufsize - 1;
	char* p = str;
	while (*p && p < end)
	    ++p;
	// Now, let's go back
	--p;
	while (p >= str && *p == '\x20') // blank character, ASCII(32)
		--p;
	*(p + 1) = '\0';
	return str;
}


// *****************************
// i m p l i c i t _ d o m a i n
// *****************************
// Determines if a domain or index is of the form RDB$<n[...n]>[<spaces>]
// This may be true for implicit domains and for unique and non-unique indices except PKs.
bool implicit_domain(const char* domain_name)
{
	return implicit_name(domain_name, IMPLICIT_DOMAIN_PREFIX, IMPLICIT_DOMAIN_PREFIX_LEN);
}


// ***********************************
// i m p l i c i t _ i n t e g r i t y
// ***********************************
// Determines if a table integrity constraint domain is of the form INTEG_<n[...n]>[<spaces>]
bool implicit_integrity(const char* integ_name)
{
	return implicit_name(integ_name, IMPLICIT_INTEGRITY_PREFIX, IMPLICIT_INTEGRITY_PREFIX_LEN);
}


// ***********************************
// i m p l i c i t _ p k
// ***********************************
// Determines if an index is of the form RDB$PRIMARY<n[...n]>[<spaces>]
bool implicit_pk(const char* pk_name)
{
	return implicit_name(pk_name, IMPLICIT_PK_PREFIX, IMPLICIT_PK_PREFIX_LEN);
}


// ***********************************
// i m p l i c i t _ n a m e
// ***********************************
// Determines if a name is of the form prefix<n[...n]>[<spaces>]
// where prefix has a fixed known length.
bool implicit_name(const char* name, const char* prefix, int prefix_len)
{
	if (strncmp(name, prefix, prefix_len) != 0)
		return false;

	int i = prefix_len;
	while (name[i] >= '0' && name[i] <= '9')
		++i;

	if (i == prefix_len) // 'prefix' alone isn't valid
		return false;

	while (name[i] == ' ')
		++i;

	return !name[i]; // we reached null term
}


int name_length(const TEXT* const name)
{
/**************************************
 *
 *	n a m e _ l e n g t h
 *
 **************************************
 *
 * Functional description
 *	Compute effective length of system relation name and others.
 *	SQL delimited identifier may contain blanks. Trailing blanks are ignored.
 *  Assumes input is null terminated.
 *
 **************************************/
	const TEXT* q = name - 1;
	for (const TEXT* p = name; *p; p++)
	{
		if (*p != ' ') {
			q = p;
		}
	}

	return (q + 1) - name;
}


//***************
// r e a d e n v
//***************
// Goes to read directly the environment variables from the operating system on Windows
// and provides a stub for UNIX.
bool readenv(const char* env_name, Firebird::string& env_value)
{
#ifdef WIN_NT
	const DWORD rc = GetEnvironmentVariable(env_name, NULL, 0);
	if (rc)
	{
		env_value.reserve(rc - 1);
		DWORD rc2 = GetEnvironmentVariable(env_name, env_value.begin(), rc);
		if (rc2 < rc && rc2 != 0)
		{
			env_value.recalculate_length();
			return true;
		}
	}
#else
	const char* p = getenv(env_name);
	if (p)
		return env_value.assign(p).length() != 0;
#endif
	// Not found, clear the output var.
	env_value.begin()[0] = 0;
	env_value.recalculate_length();
	return false;
}


bool readenv(const char* env_name, Firebird::PathName& env_value)
{
	Firebird::string result;
	bool rc = readenv(env_name, result);
	env_value.assign(result.c_str(), result.length());
	return rc;
}


// ***************
// s n p r i n t f
// ***************
// Provide a single place to deal with vsnprintf and error detection.
int snprintf(char* buffer, size_t count, const char* format...)
{
	va_list args;
	va_start(args, format);
	const int rc = VSNPRINTF(buffer, count, format, args);
	buffer[count - 1] = 0;
	va_end(args);
#if defined(DEV_BUILD) && !defined(HAVE_VSNPRINTF)
	// We don't have the safe functions, then check if we overflowed the buffer.
	// I would prefer to make this functionality available in prod build, too.
	// If the docs are right, the null terminator is not counted => rc < count.
#if defined(fb_assert_continue)
	fb_assert_continue(rc >= 0 && rc < count);
#else
	fb_assert(rc >= 0 && rc < count);
#endif
#endif
	return rc;
}

// *******************
// c l e a n u p _ p a s s w d
// *******************
// Copy password to newly allocated place and replace existing one in argv with spaces.
// Allocated space is released upon exit from utility.
// This is planned leak of a few bytes of memory in utilities.
// This function is deprecated. Use UtilSvc::hidePasswd(ArgvType&, int) whenever possible.
// However, there are several usages through fb_utils::get_passwd(char* arg);
char* cleanup_passwd(char* arg)
{
	if (! arg)
	{
		return arg;
	}

	const int lpass = strlen(arg);
	char* savePass = (char*) gds__alloc(lpass + 1);
	if (! savePass)
	{
		// No clear idea, how will it work if there is no memory
		// for password, but let others think. As a minimum avoid AV.
		return arg;
	}
	memcpy(savePass, arg, lpass + 1);
	memset(arg, ' ', lpass);
	return savePass;
}


#ifdef WIN_NT

static bool validateProductSuite (LPCSTR lpszSuiteToValidate);

// hvlad: begins from Windows 2000 we can safely add 'Global\' prefix for
// names of all kernel objects we use. For Win9x we must not add this prefix.
// Win NT will accept such names only if Terminal Server is installed.
// Check OS version carefully and add prefix if we can add it

bool prefix_kernel_object_name(char* name, size_t bufsize)
{
	static bool bGlobalPrefix = false;
	static bool bInitDone = false;

	if (!bInitDone)
	{
		bGlobalPrefix = isGlobalKernelPrefix();
		bInitDone = true;
	}

	// Backwards compatibility feature with Firebird 2.0.3 and earlier.
	// If the name already contains some prefix (specified by the user, as was
	// recommended in firebird.conf) additional prefix is not added
	if (bGlobalPrefix && !strchr(name, '\\'))
	{
		const char* prefix = "Global\\";
		const size_t len_prefix = strlen(prefix);
		const size_t len_name = strlen(name) + 1;

		// if name and prefix can't fit in name's buffer than we must
		// not overwrite end of name because it contains object type
		const int move_prefix = (len_name + len_prefix > bufsize) ?
			(bufsize - len_name) : len_prefix;

		memmove(name + move_prefix, name, len_name);
		memcpy(name, prefix, move_prefix);
		// CVC: Unfortunately, things like Glob instead of Global\\ do not achieve the objective
		// of telling the NT kernel the object is global and hence I consider them failures.
		//return move_prefix > 0; // Soft version of the check
		return move_prefix == len_prefix; // Strict version of the check.
	}
	return true;
}


// Simply handle guardian.
class DynLibHandle
{
public:
	explicit DynLibHandle(HMODULE mod)
		: m_handle(mod)
	{}
	~DynLibHandle()
	{
		if (m_handle)
			FreeLibrary(m_handle);
	}
	operator HMODULE() const
	{
		return m_handle;
	}
	/* The previous conversion is invoked with !object so this is enough.
	bool operator!() const
	{
		return !m_handle;
	}
	*/
private:
	HMODULE m_handle;
};


// hvlad: two functions below got from
// http://msdn2.microsoft.com/en-us/library/aa380797.aspx
// and slightly adapted for our coding style

// -------------------------------------------------------------
//   Note that the validateProductSuite and isTerminalServices
//   functions use ANSI versions of the functions to maintain
//   compatibility with Windows Me/98/95.
//   -------------------------------------------------------------

bool isGlobalKernelPrefix()
{
	// The strategy of this function is as follows: use Global\ kernel namespace
	// for engine objects if we can. This can be prevented by either lack of OS support
	// for the feature (Win9X) or lack of privileges (Vista, Windows 2000/XP restricted accounts)

	const DWORD dwVersion = GetVersion();

	// Is Windows NT running?
	if (!(dwVersion & 0x80000000))
	{
		if (LOBYTE(LOWORD(dwVersion)) <= 4) // This is Windows NT 4.0 or earlier.
			return validateProductSuite("Terminal Server");

		// Is it Windows 2000 or greater? It is possible to use Global\ prefix on any
		// version of Windows from Windows 2000 and up
		// Check if we have enough privileges to create global handles.
		// If not fall back to creating local ones.
		// The API for that is the NT thing, so we have to get addresses of the
		// functions dynamically to avoid troubles on Windows 9X platforms

		DynLibHandle hmodAdvApi(LoadLibrary("advapi32.dll"));

		if (!hmodAdvApi)
		{
			gds__log("LoadLibrary failed for advapi32.dll. Error code: %lu", GetLastError());
			return false;
		}

		typedef BOOL (WINAPI *PFnOpenProcessToken) (HANDLE, DWORD, PHANDLE);
		typedef BOOL (WINAPI *PFnLookupPrivilegeValue) (LPCSTR, LPCSTR, PLUID);
		typedef BOOL (WINAPI *PFnPrivilegeCheck) (HANDLE, PPRIVILEGE_SET, LPBOOL);

		PFnOpenProcessToken pfnOpenProcessToken =
			(PFnOpenProcessToken) GetProcAddress(hmodAdvApi, "OpenProcessToken");
		PFnLookupPrivilegeValue pfnLookupPrivilegeValue =
			(PFnLookupPrivilegeValue) GetProcAddress(hmodAdvApi, "LookupPrivilegeValueA");
		PFnPrivilegeCheck pfnPrivilegeCheck =
			(PFnPrivilegeCheck) GetProcAddress(hmodAdvApi, "PrivilegeCheck");

		if (!pfnOpenProcessToken || !pfnLookupPrivilegeValue || !pfnPrivilegeCheck)
		{
			// Should never happen, really
			gds__log("Cannot access privilege management API");
			return false;
		}

		HANDLE hProcess = GetCurrentProcess();
		HANDLE hToken;
		if (pfnOpenProcessToken(hProcess, TOKEN_QUERY, &hToken) == 0)
		{
			gds__log("OpenProcessToken failed. Error code: %lu", GetLastError());
			return false;
		}

		PRIVILEGE_SET ps;
		memset(&ps, 0, sizeof(ps));
		ps.Control = PRIVILEGE_SET_ALL_NECESSARY;
		ps.PrivilegeCount = 1;
		if (pfnLookupPrivilegeValue(NULL, TEXT("SeCreateGlobalPrivilege"), &ps.Privilege[0].Luid) == 0)
		{
			// Failure here means we're running on old version of Windows 2000 or XP
			// which always allow creating global handles
			CloseHandle(hToken);
			return true;
		}

		BOOL checkResult;
		if (pfnPrivilegeCheck(hToken, &ps, &checkResult) == 0)
		{
			gds__log("PrivilegeCheck failed. Error code: %lu", GetLastError());
			CloseHandle(hToken);
			return false;
		}

		CloseHandle(hToken);

		return checkResult;
	}

	return false;
}


// This is a very basic registry querying class. Not much validation, but avoids
// leaving the registry open by mistake.

class NTRegQuery
{
public:
	NTRegQuery();
	~NTRegQuery();
	bool openForRead(const char* key);
	bool readValueSize(const char* value);
	// Assumes previous call to readValueSize.
	bool readValueData(LPSTR data);
	void close();
	DWORD getDataType() const;
	DWORD getDataSize() const;
private:
	HKEY m_hKey;
	DWORD m_dwType;
	DWORD m_dwSize;
	const char* m_value;
};

inline NTRegQuery::NTRegQuery()
	: m_hKey(NULL), m_dwType(0), m_dwSize(0)
{
}

inline NTRegQuery::~NTRegQuery()
{
	close();
}

bool NTRegQuery::openForRead(const char* key)
{
	return RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_QUERY_VALUE, &m_hKey) == ERROR_SUCCESS;
}

bool NTRegQuery::readValueSize(const char* value)
{
	m_value = value;
	return RegQueryValueExA(m_hKey, value, NULL, &m_dwType, NULL, &m_dwSize) == ERROR_SUCCESS;
}

bool NTRegQuery::readValueData(LPSTR data)
{
	return RegQueryValueExA(m_hKey, m_value, NULL, &m_dwType, (LPBYTE) data, &m_dwSize) == ERROR_SUCCESS;
}

void NTRegQuery::close()
{
	if (m_hKey)
		RegCloseKey(m_hKey);

	m_hKey = NULL;
}

inline DWORD NTRegQuery::getDataType() const
{
	return m_dwType;
}

inline DWORD NTRegQuery::getDataSize() const
{
	return m_dwSize;
}


// This class represents the local allocation of dynamic memory in Windows.

class NTLocalString
{
public:
	explicit NTLocalString(DWORD dwSize);
	LPCSTR c_str() const;
	LPSTR getString();
	bool allocated() const;
	~NTLocalString();
private:
	LPSTR m_string;
};

NTLocalString::NTLocalString(DWORD dwSize)
{
	m_string = (LPSTR) LocalAlloc(LPTR, dwSize);
}

NTLocalString::~NTLocalString()
{
	if (m_string)
		LocalFree(m_string);
}

inline LPCSTR NTLocalString::c_str() const
{
	return m_string;
}

inline LPSTR NTLocalString::getString()
{
	return m_string;
}

inline bool NTLocalString::allocated() const
{
	return m_string != 0;
}


////////////////////////////////////////////////////////////
// validateProductSuite function
//
// Terminal Services detection code for systems running
// Windows NT 4.0 and earlier.
//
////////////////////////////////////////////////////////////

bool validateProductSuite (LPCSTR lpszSuiteToValidate)
{
	NTRegQuery query;

	// Open the ProductOptions key.
	if (!query.openForRead("System\\CurrentControlSet\\Control\\ProductOptions"))
		return false;

	// Determine required size of ProductSuite buffer.
	// If we get size == 1 it means multi string data with only a terminator.
	if (!query.readValueSize("ProductSuite") || query.getDataSize() < 2)
		return false;

	// Allocate buffer.
	NTLocalString lpszProductSuites(query.getDataSize());
	if (!lpszProductSuites.allocated())
		return false;

	// Retrieve array of product suite strings.
	if (!query.readValueData(lpszProductSuites.getString()) || query.getDataType() != REG_MULTI_SZ)
		return false;

	query.close();  // explicit but redundant.

	// Search for suite name in array of strings.
	bool fValidated = false;
	LPCSTR lpszSuite = lpszProductSuites.c_str();
	LPCSTR end = lpszSuite + query.getDataSize(); // paranoid check
	while (*lpszSuite && lpszSuite < end)
	{
		if (lstrcmpA(lpszSuite, lpszSuiteToValidate) == 0)
		{
			fValidated = true;
			break;
		}
		lpszSuite += (lstrlenA(lpszSuite) + 1);
	}

	return fValidated;
}

#endif // WIN_NT

// *******************************
// g e t _ p r o c e s s _ n a m e
// *******************************
// Return the name of the current process

Firebird::PathName get_process_name()
{
	char buffer[MAXPATHLEN];

#if defined(WIN_NT)
	const int len = GetModuleFileName(NULL, buffer, sizeof(buffer));
#elif defined(HAVE__PROC_SELF_EXE)
    const int len = readlink("/proc/self/exe", buffer, sizeof(buffer));
#else
	const int len = 0;
#endif

	if (len <= 0)
		buffer[0] = 0;
	else if (size_t(len) < sizeof(buffer))
		buffer[len] = 0;
	else
		buffer[len - 1] = 0;

	return buffer;
}

SLONG genUniqueId()
{
	static Firebird::AtomicCounter cnt;
	return ++cnt;
}

void getCwd(Firebird::PathName& pn)
{
	char* buffer = pn.getBuffer(MAXPATHLEN);
#if defined(WIN_NT)
	_getcwd(buffer, MAXPATHLEN);
#elif defined(HAVE_GETCWD)
	getcwd(buffer, MAXPATHLEN);
#else
	getwd(buffer);
#endif
	pn.recalculate_length();
}

namespace {
	class InputFile
	{
	public:
		explicit InputFile(const Firebird::PathName& name)
		  : flagEcho(false)
		{
			if (name == "stdin") {
				f = stdin;
			}
			else {
				f = fopen(name.c_str(), "rt");
			}
			if (f && isatty(fileno(f)))
			{
				fprintf(stderr, "Enter password: ");
				fflush(stderr);
#ifdef HAVE_TERMIOS_H
				flagEcho = tcgetattr(fileno(f), &oldState) == 0;
				if (flagEcho)
				{
					flagEcho = oldState.c_lflag & ECHO;
				}
				if (flagEcho)
				{
					struct termios newState(oldState);
					newState.c_lflag &= ~ECHO;
					tcsetattr(fileno(f), TCSANOW, &newState);
				}
#elif defined(WIN_NT)
				HANDLE handle = (HANDLE) _get_osfhandle(fileno(f));
				DWORD dwMode;
				flagEcho = GetConsoleMode(handle, &dwMode) && (dwMode & ENABLE_ECHO_INPUT);
				if (flagEcho)
					SetConsoleMode(handle, dwMode & ~ENABLE_ECHO_INPUT);
#endif
			}
		}
		~InputFile()
		{
			if (flagEcho)
			{
				fprintf(stderr, "\n");
				fflush(stderr);
#ifdef HAVE_TERMIOS_H
				tcsetattr(fileno(f), TCSANOW, &oldState);
#elif defined(WIN_NT)
				HANDLE handle = (HANDLE) _get_osfhandle(fileno(f));
				DWORD dwMode;
				if (GetConsoleMode(handle, &dwMode))
					SetConsoleMode(handle, dwMode | ENABLE_ECHO_INPUT);
#endif
			}
			if (f && f != stdin) {
				fclose(f);
			}
		}

		FILE* getStdioFile() { return f; }
		bool operator!() { return !f; }

	private:
		FILE* f;
#ifdef HAVE_TERMIOS_H
		struct termios oldState;
#endif
		bool flagEcho;
	};
} // namespace

// fetch password from file
FetchPassResult fetchPassword(const Firebird::PathName& name, const char*& password)
{
	InputFile file(name);
	if (!file)
	{
		return FETCH_PASS_FILE_OPEN_ERROR;
	}

	Firebird::string pwd;
	if (! pwd.LoadFromFile(file.getStdioFile()))
	{
		return ferror(file.getStdioFile()) ? FETCH_PASS_FILE_READ_ERROR : FETCH_PASS_FILE_EMPTY;
	}

	// this is planned leak of a few bytes of memory in utilities
	char* pass = FB_NEW(*getDefaultMemoryPool()) char[pwd.length() + 1];
	pwd.copyTo(pass, pwd.length() + 1);
	password = pass;
	return FETCH_PASS_OK;
}



const SINT64 BILLION = 1000000000;
static SINT64 saved_frequency = 0;

// Returns current value of performance counter
SINT64 query_performance_counter()
{
#if defined(WIN_NT)

	// Use Windows performance counters
	LARGE_INTEGER counter;
	if (QueryPerformanceCounter(&counter) == 0)
		return 0;

	return counter.QuadPart;
#elif defined(HAVE_CLOCK_GETTIME)

	// Use high-resultion clock
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) != 0)
		return 0;

	return static_cast<SINT64>(tp.tv_sec) * BILLION + tp.tv_nsec;
#else

	// This is not safe because of possible wrapping and very imprecise
	return clock();
#endif
}


// Returns frequency of performance counter in Hz
SINT64 query_performance_frequency()
{
#if defined(WIN_NT)
	if (saved_frequency)
		return saved_frequency;

	LARGE_INTEGER frequency;
	if (QueryPerformanceFrequency(&frequency) == 0)
		return 1;

	saved_frequency = frequency.QuadPart;
	return frequency.QuadPart;
#elif defined(HAVE_CLOCK_GETTIME)

	return BILLION;
#else

	// This is not safe because of possible wrapping and very imprecise
	return CLOCKS_PER_SEC;
#endif
}

void exactNumericToStr(SINT64 value, int scale, Firebird::string& target, bool append)
{
	if (value == 0)
	{
		if (append)
			target.append("0", 1);
		else
			target.assign("0", 1);
		return;
	}

	const int MAX_SCALE = 25;
	const int MAX_BUFFER = 50;

	if (scale < -MAX_SCALE || scale > MAX_SCALE)
	{
		fb_assert(false);
		return; // throw exception here?
	}

	const bool neg = value < 0;
	const bool dot = scale < 0; // Need the decimal separator or not?
	char buffer[MAX_BUFFER];
	int iter = MAX_BUFFER;

	buffer[--iter] = '\0';

	if (scale > 0)
	{
		while (scale-- > 0)
			buffer[--iter] = '0';
	}

	bool dot_used = false;
	FB_UINT64 uval = neg ? FB_UINT64(-(value + 1)) + 1 : value; // avoid problems with MIN_SINT64

	while (uval != 0)
	{
		buffer[--iter] = static_cast<char>(uval % 10) + '0';
		uval /= 10;

		if (dot && !++scale)
		{
			buffer[--iter] = '.';
			dot_used = true;
		}
	}

	if (dot)
	{
		// if scale > 0 we have N.M
		// if scale == 0 we have .M and we need 0.M
		// if scale < 0 we have pending zeroes and need 0.{0+}M
		if (!dot_used)
		{
			while (scale++ < 0)
				buffer[--iter] = '0';

			buffer[--iter] = '.';
			buffer[--iter] = '0';
		}
		else if (!scale)
			buffer[--iter] = '0';
	}

	if (neg)
		buffer[--iter] = '-';

	const size_t len = MAX_BUFFER - iter - 1;

	if (append)
		target.append(buffer + iter, len);
	else
		target.assign(buffer + iter, len);
}


// Build full file name in specified directory
Firebird::PathName getPrefix(FB_DIR prefType, const char* name)
{
	Firebird::PathName s;
	char tmp[MAXPATHLEN];

#ifndef BOOT_BUILD
	const char* configDir[] = {
		FB_BINDIR, FB_SBINDIR, FB_CONFDIR, FB_LIBDIR, FB_INCDIR, FB_DOCDIR, FB_UDFDIR, FB_SAMPLEDIR,
		FB_SAMPLEDBDIR, FB_HELPDIR, FB_INTLDIR, FB_MISCDIR, FB_SECDBDIR, FB_MSGDIR, FB_LOGDIR,
		FB_GUARDDIR, FB_PLUGDIR
	};

	fb_assert(FB_NELEM(configDir) == FB_DIR_LAST);
	fb_assert(prefType < FB_DIR_LAST);

	if (prefType != FB_DIR_CONF && prefType != FB_DIR_MSG && configDir[prefType][0])
	{
		// Value is set explicitly and is not environment overridable
		PathUtils::concatPath(s, configDir[prefType], name);
		return s;
	}
#endif

	switch(prefType)
	{
		case FB_DIR_BIN:
		case FB_DIR_SBIN:
			s = "bin";
			break;

		case FB_DIR_CONF:
		case FB_DIR_LOG:
		case FB_DIR_GUARD:
		case FB_DIR_SECDB:
			s = "";
			break;

		case FB_DIR_LIB:
			s = "lib";
			break;

		case FB_DIR_PLUGINS:
			s = "plugins";
			break;

		case FB_DIR_INC:
			s = "include";
			break;

		case FB_DIR_DOC:
			s = "doc";
			break;

		case FB_DIR_UDF:
			s = "UDF";
			break;

		case FB_DIR_SAMPLE:
			s = "examples";
			break;

		case FB_DIR_SAMPLEDB:
			s = "examples/empbuild";
			break;

		case FB_DIR_HELP:
			s = "help";
			break;

		case FB_DIR_INTL:
			s = "intl";
			break;

		case FB_DIR_MISC:
			s = "misc";
			break;

		case FB_DIR_MSG:
			gds__prefix_msg(tmp, name);
			return tmp;

		default:
			fb_assert(false);
			break;
	}

	if (s.hasData() && name[0])
	{
		s += '/';
	}
	s += name;
	gds__prefix(tmp, s.c_str());
	return tmp;
}

} // namespace fb_utils
