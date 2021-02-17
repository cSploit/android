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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// =====================================
// File functions

#include "firebird.h"

#include "../common/classes/init.h"
#include "../common/gdsassert.h"
#include "../common/os/os_utils.h"
#include "../jrd/constants.h"
#include "../common/os/path_utils.h"
#include "../common/isc_proto.h"
#include "gen/iberror.h"

#include <direct.h>
#include <io.h> // isatty()
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include <aclapi.h>

namespace os_utils
{

// waits for implementation
SLONG get_user_group_id(const TEXT* /*user_group_name*/)
{
	return 0;
}


// waits for implementation
SLONG get_user_id(const TEXT* /*user_name*/)
{
	return -1;
}


// waits for implementation
bool get_user_home(int /*user_id*/, Firebird::PathName& /*homeDir*/)
{
	return false;
}

// allow different users to read\write\delete files in lock directory
// in case of any error just log it and don't stop engine execution
void adjustLockDirectoryAccess(const char* pathname)
{
	PSECURITY_DESCRIPTOR pSecDesc = NULL;
	PSID pSID_Users = NULL;
	PSID pSID_Administrators = NULL;
	PACL pNewACL = NULL;
	try
	{
		// We should pass root directory in format "C:\" into GetVolumeInformation().
		// In case of pathname is not local folder (i.e. \\share\folder) let
		// GetVolumeInformation() return an error.
		Firebird::PathName root(pathname);
		const size_t pos = root.find(':', 0);
		if (pos == 1)
		{
			root.erase(pos + 1, root.length());
			PathUtils::ensureSeparator(root);
		}

		DWORD fsflags;
		if (!GetVolumeInformation(root.c_str(), NULL, 0, NULL, NULL, &fsflags, NULL, 0))
			Firebird::system_error::raise("GetVolumeInformation");

		if (!(fsflags & FS_PERSISTENT_ACLS))
			return;

		// Adjust security for our new folder : allow BUILTIN\Users group to
		// read\write\delete files
		PACL pOldACL = NULL;

		if (GetNamedSecurityInfo((LPSTR) pathname,
				SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
				NULL, NULL, &pOldACL, NULL,
				&pSecDesc) != ERROR_SUCCESS)
		{
			Firebird::system_error::raise("GetNamedSecurityInfo");
		}

		SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
		if (!AllocateAndInitializeSid(&sidAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pSID_Users))
		{
			Firebird::system_error::raise("AllocateAndInitializeSid");
		}

		if (!AllocateAndInitializeSid(&sidAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSID_Administrators))
		{
			Firebird::system_error::raise("AllocateAndInitializeSid");
		}

		EXPLICIT_ACCESS eas[2];
		memset(eas, 0, sizeof(eas));

		eas[0].grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE;
		eas[0].grfAccessMode = GRANT_ACCESS;
		eas[0].grfInheritance = SUB_OBJECTS_ONLY_INHERIT;
		eas[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		eas[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		eas[0].Trustee.ptstrName  = (LPSTR) pSID_Users;

		eas[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE;
		eas[1].grfAccessMode = GRANT_ACCESS;
		eas[1].grfInheritance = SUB_OBJECTS_ONLY_INHERIT;
		eas[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		eas[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		eas[1].Trustee.ptstrName  = (LPSTR) pSID_Administrators;

		if (SetEntriesInAcl(2, eas, pOldACL, &pNewACL) != ERROR_SUCCESS)
			Firebird::system_error::raise("SetEntriesInAcl");

		if (SetNamedSecurityInfo((LPSTR) pathname,
				SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
				NULL, NULL, pNewACL, NULL) != ERROR_SUCCESS)
		{
			Firebird::system_error::raise("SetNamedSecurityInfo");
		}
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::string str;
		str.printf("Error adjusting access rights for folder \"%s\" :", pathname);

		iscLogException(str.c_str(), ex);
	}

	if (pSID_Users) {
		FreeSid(pSID_Users);
	}
	if (pSID_Administrators) {
		FreeSid(pSID_Administrators);
	}
	if (pNewACL) {
		LocalFree(pNewACL);
	}
	if (pSecDesc) {
		LocalFree(pSecDesc);
	}
}


// create directory for lock files and set appropriate access rights
void createLockDirectory(const char* pathname)
{
	static bool errorLogged = false;

	DWORD attr = GetFileAttributes(pathname);
	DWORD errcode = 0;
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		errcode = GetLastError();
		if (errcode == ERROR_FILE_NOT_FOUND)
		{
			if (!CreateDirectory(pathname, NULL)) {
				errcode = GetLastError();
			}
			else
			{
				adjustLockDirectoryAccess(pathname);

				attr = GetFileAttributes(pathname);
				if (attr == INVALID_FILE_ATTRIBUTES) {
					errcode = GetLastError();
				}
			}
		}
	}

	Firebird::string err;
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		err.printf("Can't create directory \"%s\". OS errno is %d", pathname, errcode);
		if (!errorLogged)
		{
			errorLogged = true;
			gds__log(err.c_str());
		}
		Firebird::fatal_exception::raise(err.c_str());
	}

	if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		err.printf("Can't create directory \"%s\". File with same name already exists", pathname);
		if (!errorLogged)
		{
			errorLogged = true;
			gds__log(err.c_str());
		}
		Firebird::fatal_exception::raise(err.c_str());
	}

	if (attr & FILE_ATTRIBUTE_READONLY)
	{
		err.printf("Can't create directory \"%s\". Readonly directory with same name already exists", pathname);
		if (!errorLogged)
		{
			errorLogged = true;
			gds__log(err.c_str());
		}
		Firebird::fatal_exception::raise(err.c_str());
	}
}

// open (or create if missing) and set appropriate access rights
int openCreateSharedFile(const char* pathname, int flags)
{
	int rc = ::open(pathname, flags | O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
	if (rc < 0)
	{
		(Firebird::Arg::Gds(isc_io_error) << "open" << pathname << Firebird::Arg::Gds(isc_io_open_err)
			<< strerror(errno)).raise();
	}
	return rc;
}

// set file's last access and modification time to current time
bool touchFile(const char* pathname)
{
    FILETIME ft;
    SYSTEMTIME st;

	HANDLE hFile = CreateFile(pathname,
		GENERIC_READ | FILE_WRITE_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		ISC_get_security_desc(),
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

    GetSystemTime(&st);
	const bool ret = SystemTimeToFileTime(&st, &ft) && SetFileTime(hFile, NULL, &ft, &ft);
	CloseHandle(hFile);

	return ret;
}

} // namespace os_utils
