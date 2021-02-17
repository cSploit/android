/*
 *	PROGRAM:		Windows Win32 Client Library Installation Tools
 *	MODULE:			install.cpp
 *	DESCRIPTION:	Functions which help installing components to WinSysDir
 *
 *  The contents of this file are subject to the Initial Developer's
 *  Public License Version 1.0 (the "License"); you may not use this
 *  file except in compliance with the License. You may obtain a copy
 *  of the License here:
 *
 *    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code is (C) 2003 Olivier Mascia.
 *
 *  The Initial Developer of the Original Code is Olivier Mascia.
 *
 *  All Rights Reserved.
 *
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <windows.h>
//#include "../jrd/license.h"
#include "../utilities/install/install_nt.h"
#include "../utilities/install/install_proto.h"

const DWORD GDSVER_MAJOR	= 6;
const DWORD GDSVER_MINOR	= 3;

const char* SHARED_KEY	= "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs";

namespace
{
	USHORT GetVersion(const TEXT* gds32, DWORD& verMS, DWORD& verLS, err_handler_t);
	USHORT PatchVersion(const TEXT* gds32, DWORD verMS, err_handler_t);
	USHORT IncrementSharedCount(const TEXT* gds32, err_handler_t);
	USHORT DecrementSharedCount(const TEXT* gds32, bool sw_force, err_handler_t);
} // namespace

//
//	--- Public Functions ---
//

USHORT CLIENT_install(const TEXT * rootdir, USHORT client, bool sw_force, err_handler_t err_handler)
{
/**************************************
 *
 *	C L I E N T _ i n s t a l l
 *
 **************************************
 *
 * Functional description
 *	Depending on the USHORT client parameter, installs FBCLIENT.DLL or
 *	GDS32.DLL in the Windows System directory.
 *
 *	When installing and -force is NOT used then
 *
 *		test for existence of client library
 *		if earlier version then copy, increment shared library count
 *		if same version then do nothing
 *		if later version then do nothing
 *
 *	If -force is supplied then
 *
 *		test for existence of client library
 *		if the version doesn't match then copy, increment shared library count
 *		else do nothing
 *
 **************************************/

	// Get Windows System Directory
	TEXT sysdir[MAXPATHLEN];
	int len = GetSystemDirectory(sysdir, sizeof(sysdir));
	if (len == 0)
		return (*err_handler) (GetLastError(), "GetSystemDirectory()");

	// Compute newverMS and newverLS. They will contain the full client version
	// number that we intend to install to WinSysDir.
	TEXT fbdll[MAXPATHLEN];
	lstrcpy(fbdll, rootdir);
//	lstrcat(fbdll, "\\bin\\");
	lstrcat(fbdll, "\\");
	lstrcat(fbdll, FBCLIENT_NAME);

	DWORD newverMS = 0, newverLS = 0;
	USHORT status = GetVersion(fbdll, newverMS, newverLS, err_handler);
	if (status != FB_SUCCESS)
		return status;

	if (client == CLIENT_GDS)
	{
		// For the GDS32.DLL installation, the major/minor version is hardcoded
		newverMS = (GDSVER_MAJOR << 16) | GDSVER_MINOR;
	}

	// Check for existence and version of current installed DLL
	TEXT target[MAXPATHLEN];
	lstrcpy(target, sysdir);
	lstrcat(target, "\\");
	lstrcat(target, client == CLIENT_GDS ? GDS32_NAME : FBCLIENT_NAME);

	DWORD targetverMS = 0, targetverLS = 0;
	status = GetVersion(target, targetverMS, targetverLS, err_handler);
	if (client == CLIENT_GDS)
	{
		//Our patching logic is to only change the Major.minor version
		//number from A.B to 6.3. This leaves the release.build number
		//intact. Ie, after patching v1.5.3.3106 it will return v6.3.3.3106
		//This means that when considering whether to install our new patched
		//version our initial comparison of (newverMS =targetverMS) will always
		//be true. So, we are left with comparing targetverLS against newverLS.
		//However, a gds32 installed from 1.5.3 is going to leave us with
		//a point release greater than the point release in v2.0.0 so our
		//comparison will fail and the new library will not be copied. To
		//avoind this we must mask out the point release and just work
		//on the build number when comparing the gds32 version.
		//
		//This solution will be fine as long as we don't reset the build number
		//(unlikely for 2.0.) If we do reset the build number we will need to
		//either increment GDSVER_MINOR or patch an altered point release value.
		targetverLS = (targetverLS & 0x0000FFFF);
	}

	if (status == FB_FAILURE)
		return FB_FAILURE;

	if (status == FB_SUCCESS)
	{
		// The target DLL already exists
		if (! sw_force)
		{
			if (targetverMS > newverMS)
				return FB_INSTALL_NEWER_VERSION_FOUND;
			if (targetverMS == newverMS && targetverLS > newverLS)
				return FB_INSTALL_NEWER_VERSION_FOUND;
		}
		if (targetverMS == newverMS && targetverLS == newverLS)
			return FB_INSTALL_SAME_VERSION_FOUND;
	}

	// Copy FBCLIENT.DLL as _FBCLIENT.DLL (in WinSysDir).
	// Such a step is needed because the MoveFile techniques used later
	// can't span different volumes. So the intermediate target has to be
	// already on the same final destination volume as the final target.
	TEXT workfile[MAXPATHLEN];
	lstrcpy(workfile, sysdir);
	lstrcat(workfile, "\\_");
	lstrcat(workfile, FBCLIENT_NAME);

	if (CopyFile(fbdll, workfile, FALSE) == 0)
	{
		return (*err_handler) (GetLastError(),
			"CopyFile(FBCLIENT.DLL, WinSysDir\\_FBCLIENT.DLL)");
	}

	if (client == CLIENT_GDS)
	{
		// Patch File Version, this is needed only for GDS32.DLL.
		status = PatchVersion(workfile, newverMS, err_handler);
		if (status != FB_SUCCESS)
			return status;
	}

	// Move _FBCLIENT.DLL as the final target
	if (MoveFile(workfile, target) == 0)
	{
		// MoveFile failed, this is expected if target already exists
		ULONG werr = GetLastError();
		if (werr != ERROR_ALREADY_EXISTS)
		{
			DeleteFile(workfile);
			return (*err_handler) (werr, "MoveFile(_FBCLIENT.DLL, 'target')");
		}

		// Failed moving because a destination target file already exists
		// Let's try again by attempting a remove of the destination
		if (DeleteFile(target) != 0)
		{
			// Success deleting the target, MoveFile should now succeed.
			if (MoveFile(workfile, target) != 0)
			{
				// Successfull !
				IncrementSharedCount(target, err_handler);
				return FB_SUCCESS;
			}
		}

		// Deleting the target failed OR moving after target delete failed.
		// Let's try once more using reboot-time update.
		HMODULE kernel32 = LoadLibrary("KERNEL32.DLL");
		if (kernel32 != 0)
		{
			typedef BOOL __stdcall proto_ntmove(LPCSTR, LPCSTR, DWORD);
			proto_ntmove* ntmove = (proto_ntmove*) GetProcAddress(kernel32, "MoveFileExA");
			if (ntmove != 0)
			{
				// We are definitely running on a system supporting the
				// MoveFileEx API. Let's use it for setting up the reboot-copy.

				if ((*ntmove) (target, 0, MOVEFILE_DELAY_UNTIL_REBOOT) == 0)
				{
					ULONG werr = GetLastError();
					FreeLibrary(kernel32);
					DeleteFile(workfile);
					return (*err_handler) (werr, "MoveFileEx(delete 'target')");
				}

				if ((*ntmove) (workfile, target, MOVEFILE_DELAY_UNTIL_REBOOT) == 0)
				{
					ULONG werr = GetLastError();
					FreeLibrary(kernel32);
					DeleteFile(workfile);
					return (*err_handler) (werr, "MoveFileEx(replace 'target')");
				}

				FreeLibrary(kernel32);
				IncrementSharedCount(target, err_handler);
				return FB_INSTALL_COPY_REQUIRES_REBOOT;
			}

			FreeLibrary(kernel32);
		}

		// We are running a system which doesn't support MoveFileEx API.
		// Let's use the WININIT.INI method (Win95/98/Me).
		// For this we need "short file names".

		TEXT ssysdir[MAXPATHLEN];
		if (GetShortPathName(sysdir, ssysdir, sizeof(ssysdir)) == 0)
			return (*err_handler) (GetLastError(), "GetShortPathName()");

		TEXT sworkfile[MAXPATHLEN];
		lstrcpy(sworkfile, ssysdir);
		lstrcat(sworkfile, "\\_");
		lstrcat(sworkfile, FBCLIENT_NAME);

		TEXT starget[MAXPATHLEN];
		lstrcpy(starget, ssysdir);
		lstrcat(starget, "\\");
		lstrcat(starget, client == CLIENT_GDS ? GDS32_NAME : FBCLIENT_NAME);

		if (WritePrivateProfileString("rename", "NUL", starget,
				"WININIT.INI") == 0)
		{
			ULONG werr = GetLastError();
			DeleteFile(workfile);
			return (*err_handler) (werr, "WritePrivateProfileString(delete 'target')");
		}

		if (WritePrivateProfileString("rename", starget, sworkfile,
				"WININIT.INI") == 0)
		{
			ULONG werr = GetLastError();
			DeleteFile(workfile);
			return (*err_handler) (werr, "WritePrivateProfileString(replace 'target')");
		}

		IncrementSharedCount(target, err_handler);
		return FB_INSTALL_COPY_REQUIRES_REBOOT;
	}

	// Straight plain MoveFile succeeded immediately.
	IncrementSharedCount(target, err_handler);
	return FB_SUCCESS;
}

USHORT CLIENT_remove(const TEXT* rootdir, USHORT client, bool sw_force, err_handler_t err_handler)
{
/**************************************
 *
 *	C L I E N T _ r e m o v e
 *
 **************************************
 *
 * Functional description
 *	Depending on the USHORT client parameter, remove FBCLIENT.DLL or GDS32.DLL
 *	from the Windows System directory.
 *
 *	when removing and -force is NOT used
 *
 *		test for existence of client library
 *		if version matches then decrement shared library count
 *			if count=0 then delete library, remove entry from shared library
 *		if version doesn't match then do nothing. It is not ours.
 *
 *	when removing and -force IS used
 *
 *		test for existence of client library
 *		if version matches then delete library, remove entry for shared library
 *		if version doesn't match then do nothing. It is not ours.
 *
 **************************************/

	// Get Windows System Directory
	TEXT sysdir[MAXPATHLEN];
	int len = GetSystemDirectory(sysdir, sizeof(sysdir));
	if (len == 0)
		return (*err_handler) (GetLastError(), "GetSystemDirectory()");

	// Compute ourverMS and ourverLS. They will contain the full client version
	// number that we could have installed to WinSysDir.
	TEXT fbdll[MAXPATHLEN];
	lstrcpy(fbdll, rootdir);
//	lstrcat(fbdll, "\\bin\\");
	lstrcat(fbdll, "\\");
	lstrcat(fbdll, FBCLIENT_NAME);

	DWORD ourverMS = 0, ourverLS = 0;
	USHORT status = GetVersion(fbdll, ourverMS, ourverLS, err_handler);
	if (status != FB_SUCCESS)
		return status;

	if (client == CLIENT_GDS)
	{
		// For the GDS32.DLL, the major/minor version is hardcoded
		ourverMS = (GDSVER_MAJOR << 16) | GDSVER_MINOR;
	}

	// Check for existence and version of current installed client
	TEXT target[MAXPATHLEN];
	lstrcpy(target, sysdir);
	lstrcat(target, "\\");
	lstrcat(target, client == CLIENT_GDS ? GDS32_NAME : FBCLIENT_NAME);

	DWORD targetverMS = 0, targetverLS = 0;
	status = GetVersion(target, targetverMS, targetverLS, err_handler);
	if (status != FB_SUCCESS)
		return status;	// FB_FAILURE or FB_INSTALL_FILE_NOT_FOUND

	if (targetverMS != ourverMS || targetverLS != ourverLS)
		return FB_INSTALL_CANT_REMOVE_ALIEN_VERSION;

	status = DecrementSharedCount(target, sw_force, err_handler);
	if (status == FB_INSTALL_SHARED_COUNT_ZERO)
	{
		if (! DeleteFile(target))
		{
			// Could not delete the file, restore shared count
			IncrementSharedCount(target, err_handler);
			return FB_INSTALL_FILE_PROBABLY_IN_USE;
		}
	}

	return FB_SUCCESS;
}

USHORT CLIENT_query(USHORT client, ULONG& verMS, ULONG& verLS, ULONG& sharedCount,
	err_handler_t err_handler)
{
/**************************************
 *
 *	C L I E N T _ q u e r y
 *
 **************************************
 *
 * Functional description
 *	Depending on the USHORT client parameter, returns version info and
 *	shared dll count of the currently installed GDS32.DLL or FBCLIENT.DLL.
 *	Eventually return FB_INSTALL_FILE_NOT_FOUND.
 *
 **************************************/

	// Get Windows System Directory
	TEXT sysdir[MAXPATHLEN];
	int len = GetSystemDirectory(sysdir, sizeof(sysdir));
	if (len == 0)
		return (*err_handler) (GetLastError(), "GetSystemDirectory()");

	TEXT target[MAXPATHLEN];
	lstrcpy(target, sysdir);
	lstrcat(target, "\\");
	lstrcat(target, client == CLIENT_GDS ? GDS32_NAME : FBCLIENT_NAME);

	verMS = verLS = sharedCount = 0;
	USHORT status = GetVersion(target, verMS, verLS, err_handler);
	if (status != FB_SUCCESS)
		return status;	// FB_FAILURE or FB_INSTALL_FILE_NOT_FOUND

	HKEY hkey;
	LONG keystatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SHARED_KEY, 0,
		KEY_READ, &hkey);
	if (keystatus != ERROR_SUCCESS)
		return (*err_handler) (keystatus, "RegOpenKeyEx");

	DWORD type, size = sizeof(sharedCount);
	sharedCount = 0;
	RegQueryValueEx(hkey, target, NULL, &type, reinterpret_cast<BYTE*>(&sharedCount), &size);
	RegCloseKey(hkey);

	return FB_SUCCESS;
}

//
//	--- Local Functions ---
//

namespace {

USHORT GetVersion(const TEXT* filename, DWORD& verMS, DWORD& verLS, err_handler_t err_handler)
{
/**************************************
 *
 *	G e t V e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Get the file version of a dll whose full path and name is given.
 *	Returns a status value to report a failure, a non-existing file or success.
 *	'version' is only updated in case of success.
 *
 **************************************/

	HANDLE hfile = CreateFile(filename, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE)
		return FB_INSTALL_FILE_NOT_FOUND;

	// We'll keep hfile opened until we have read version so that the file
	// can't be deleted between check for existence and version read.

	DWORD dwUnused;
	DWORD rsize = GetFileVersionInfoSize(const_cast<TEXT*>(filename), &dwUnused);
	if (rsize == 0)
	{
		ULONG werr = GetLastError();
		CloseHandle(hfile);
		return (*err_handler) (werr, "GetFileVersionInfoSize()");
	}

	BYTE* hver = new BYTE[rsize];
	if (! GetFileVersionInfo(const_cast<TEXT*>(filename), 0, rsize, hver))
	{
		ULONG werr = GetLastError();
		delete[] hver;
		CloseHandle(hfile);
		return (*err_handler) (werr, "GetFileVersionInfo()");
	}
	CloseHandle(hfile);	// Not needed anymore

	VS_FIXEDFILEINFO* ffi;
	UINT uiUnused;
	if (! VerQueryValue(hver, "\\", (void**)&ffi, &uiUnused))
	{
		ULONG werr = GetLastError();
		delete[] hver;
		return (*err_handler) (werr, "VerQueryValue()");
	}

	verMS = ffi->dwFileVersionMS;
	verLS = ffi->dwFileVersionLS;

	delete[] hver;
	return FB_SUCCESS;
}

USHORT PatchVersion(const TEXT* filename, DWORD verMS, err_handler_t err_handler)
{
/**************************************
 *
 *	P a t c h V e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Patches the FileVersion and ProductVersion of a DLL whose full
 *	path is given as "filename" parameter. The function only updates the major
 *	and minor version numbers, leaving the sub-minor and build intact.
 *	We typically use this trick to build a GDS32.DLL whose version is 6.3
 *	from our FBCLIENT.DLL whose version is 1.5.
 *
 *	The politically correct way (speaking of Win32 API) of changing the
 *	version info, should involve using GetFileVersionInfo() and VerQueryValue()
 *	to read the existing version resource, and using BeginUpdateResource(),
 *	UpdateResource() and EndUpdateResource() to actually update the dll file.
 *	Unfortunately those last 3 APIs are not implemented on Win95/98/Me.
 *
 *	Therefore this function proceeds by straight hacking of the dll file.
 *	This is not intellectually satisfactory, but does work perfectly and
 *	fits the bill.
 *
 **************************************/

	HANDLE hfile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
		0 /* FILE_SHARE_NONE */, 0, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (hfile == INVALID_HANDLE_VALUE)
		return (*err_handler) (GetLastError(), "CreateFile()");

	DWORD fsize = GetFileSize(hfile, 0);

	HANDLE hmap = CreateFileMapping(hfile, 0,
		PAGE_READWRITE | SEC_COMMIT, 0, 0, 0);
	if (hmap == 0)
	{
		ULONG werr = GetLastError();
		CloseHandle(hfile);
		return (*err_handler) (werr, "CreateFileMapping()");
	}

	BYTE* mem = static_cast<BYTE*>(MapViewOfFile(hmap,
		FILE_MAP_WRITE, 0, 0, 0));
	if (mem == 0)
	{
		ULONG werr = GetLastError();
		CloseHandle(hmap);
		CloseHandle(hfile);
		return (*err_handler) (werr, "MapViewOfFile()");
	}

	// This is a "magic value" that will allow locating the version info.
	// Windows itself does something equivalent internally.
	const BYTE lookup[] =
	{
		'V', 0, 'S', 0, '_', 0,
		'V', 0, 'E', 0, 'R', 0, 'S', 0, 'I', 0, 'O', 0, 'N', 0, '_', 0,
		'I', 0, 'N', 0, 'F', 0, 'O', 0, 0, 0, 0, 0, 0xbd, 0x04, 0xef, 0xfe
	};
	BYTE* p = mem;				// First byte of mapped file
	BYTE* end = mem + fsize;	// Last byte + 1 of mapped file

	int i = 0;
	while (p < end)
	{
		if (*p++ == lookup[i])
		{
			if (++i == sizeof(lookup)) break;
		}
		else i = 0;
	}

	if (p >= end)
	{
		UnmapViewOfFile(mem);
		CloseHandle(hmap);
		CloseHandle(hfile);
		return (*err_handler) (0, "Could not locate the version info resource.");
	}

	// The VS_FIXEDFILEINFO structure itself is located 4 bytes before the end
	// of the above "lookup" value (said differently : the first 4 bytes of
	// FIXEDFILEINFO are the last 4 bytes of the "lookup" value).
	VS_FIXEDFILEINFO* ffi = (VS_FIXEDFILEINFO*)(p - 4);
	ffi->dwFileVersionMS = verMS;
	ffi->dwProductVersionMS = verMS;

	/*
	printf("Signature: %8.8x\n", ffi->dwSignature);
	printf("FileVersionMS : %8.8x\n", ffi->dwFileVersionMS);
	printf("FileVersionLS : %8.8x\n", ffi->dwFileVersionLS);
	printf("ProductVersionMS : %8.8x\n", ffi->dwProductVersionMS);
	printf("ProductVersionLS : %8.8x\n", ffi->dwProductVersionLS);
	*/

	/*
	// The start of the full VS_VERSIONINFO pseudo structure is located 6 bytes
	// before the above "lookup" value. This "vi" (versioninfo) pointer points
	// to the same block of bytes that would have been returned by the
	// GetFileVersionInfo API.
	BYTE* vi = p - sizeof(lookup) - 6;

	// The first WORD of this pseudo structure is its byte length
	WORD viLength = *(WORD*)vi;

	// Let's patch the FileVersion strings (unicode strings).
	// This patch assumes the original version string has a major version and
	// a minor version made of a single digit, like 1.5.x.y. It would need
	// to be updated if we want to use versions like 1.51.x.y.
	const BYTE flookup[] =
	{
		'F', 0, 'i', 0, 'l', 0, 'e', 0,
		'V', 0, 'e', 0, 'r', 0, 's', 0, 'i', 0, 'o', 0, 'n', 0, 0, 0
	};
	p = vi;
	end = vi + viLength;
	i = 0;
	while (p < end)
	{
		if (*p++ == flookup[i])
		{
			if (++i == sizeof(flookup))
			{
				// The unicode string is aligned on the next 32 bit value
				// (See PSDK Version Info "String" pseudo structure.)
				p = p + ((unsigned int)p % 4);
				//printf("FileVersion : %ls\n", p);
				while (! isdigit(*p)) ++p;	// Get to first digit
				*p = (verMS >> 16) + '0';	// Patch major digit
				while (*p != '.') ++p;		// Get to dot
				while (! isdigit(*p)) ++p;	// Get to first digit after dot
				*p = (verMS & 0xffff) + '0';// Patch minor digit
				i = 0; // To resume scanning from here (p)
			}
		}
		else i = 0;
	}
	*/

	/*
	// Let's patch the ProductVersion strings.
	// This patch assumes the original version string has a major version and
	// a minor version made of a single digit, like 1.5.x.y. It would need
	// to be updated if we want to use versions like 1.51.x.y.
	const BYTE plookup[] =
	{
		'P', 0, 'r', 0, 'o', 0, 'd', 0, 'u', 0, 'c', 0, 't', 0,
		'V', 0, 'e', 0, 'r', 0, 's', 0, 'i', 0, 'o', 0, 'n', 0, 0, 0
	};
	p = vi;
	end = vi + viLength;
	i = 0;
	while (p < end)
	{
		if (*p++ == plookup[i])
		{
			if (++i == sizeof(plookup))
			{
				// The unicode string is aligned on the next 32 bit value
				// (See PSDK Version Info "String" pseudo structure.)
				p = p + ((unsigned int)p % 4);
				//printf("ProductVersion : %ls\n", p);
				while (! isdigit(*p)) ++p;	// Get to first digit
				*p = (verMS >> 16) + '0';	// Patch major digit
				while (*p != '.') ++p;		// Get to dot
				while (! isdigit(*p)) ++p;	// Get to first digit after dot
				*p = (verMS & 0xffff) + '0';// Patch minor digit
				i = 0; // To resume scanning from here (p)
			}
		}
		else i = 0;
	}
	*/

	UnmapViewOfFile(mem);
	CloseHandle(hmap);
	CloseHandle(hfile);

	return FB_SUCCESS;
}

USHORT IncrementSharedCount(const TEXT* filename, err_handler_t err_handler)
{
/**************************************
 *
 *	I n c r e m e n t S h a r e d C o u n t
 *
 **************************************
 *
 * Functional description
 *	Increment the registry Shared Count of a module whose full path & file
 *	name is given in parameter 'filename'. Typically used to increment the
 *	shared count of the GDS32.DLL after its installation to WinSysDir. But
 *	can be used for just about any other shared component.
 *	User has to have enough rights to write to the HKLM key tree.
 *
 **************************************/

	HKEY hkey;
	LONG status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SHARED_KEY,
		0, "", REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 0, &hkey, 0);
	if (status != ERROR_SUCCESS)
		return (*err_handler) (status, "RegCreateKeyEx");

	DWORD count = 0;
	DWORD type, size = sizeof(count);
	RegQueryValueEx(hkey, filename, NULL, &type,
		reinterpret_cast<BYTE*>(&count), &size);

	++count;

	status = RegSetValueEx(hkey, filename, 0, REG_DWORD,
		reinterpret_cast<BYTE*>(&count), sizeof(DWORD));
	if (status != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		return (*err_handler) (status, "RegSetValueEx");
	}
	RegCloseKey(hkey);

	return FB_SUCCESS;
}

USHORT DecrementSharedCount(const TEXT* filename, bool sw_force, err_handler_t err_handler)
{
/**************************************
 *
 *	D e c r e m e n t S h a r e d C o u n t
 *
 **************************************
 *
 * Functional description
 *	Decrements the registry Shared Count of a module whose full path & file
 *	name is given in parameter 'filename'. Typically used to decrement the
 *	shared count of the GDS32.DLL prior to its eventual removal from WinSysDir.
 *	Can be used for just about any other shared component.
 *	User has to have enough rights to write to the HKLM key tree.
 *	If the shared count reaches zero (or was already zero), the shared count
 *	value is removed and FB_INSTALL_SHARED_COUNT_ZERO is returned.
 *	If the sw_force is set, the shared count value is removed and
 *	FB_INSTALL_SHARED_COUNT_ZERO returned too).
 *	FB_SUCCESS is returned when the count was decremented without reaching
 *	zero. FB_FAILURE is returned only in cases of registry access failures.
 *
 **************************************/

	HKEY hkey;
	DWORD disp;
	LONG status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SHARED_KEY,
		0, "", REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hkey, &disp);
	if (status != ERROR_SUCCESS)
		return (*err_handler) (status, "RegCreateKeyEx");

	LONG count = 0;
	if (! sw_force)
	{
		DWORD type;
		DWORD size = sizeof(count);
		RegQueryValueEx(hkey, filename, NULL, &type,
			reinterpret_cast<BYTE*>(&count), &size);

		--count;
	}

	if (count > 0)
	{
		status = RegSetValueEx(hkey, filename, 0, REG_DWORD,
			reinterpret_cast<BYTE*>(&count), sizeof(DWORD));
		if (status != ERROR_SUCCESS)
		{
			RegCloseKey(hkey);
			return (*err_handler) (status, "RegSetValueEx");
		}
		RegCloseKey(hkey);
		return FB_SUCCESS;
	}

	status = RegDeleteValue(hkey, filename);
	if (status != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		return (*err_handler) (status, "RegDeleteValue");
	}

	RegCloseKey(hkey);
	return FB_INSTALL_SHARED_COUNT_ZERO;
}

}	// namespace { }

//
// EOF
//
