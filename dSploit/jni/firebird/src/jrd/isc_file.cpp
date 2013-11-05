/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_file.cpp
 *	DESCRIPTION:	General purpose but non-user routines.
 *
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
 *
 * 2001.06.14: Claudio Valderrama: Possible buffer overrun in
 *	expand_share_name(TEXT*) has been closed. Parameter is return value, too.
 *	This function and its caller in this same file don't report error conditions.
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old M88K and NCR3000 port
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 * 2002.10.29 Sean Leyne - Removed support for obsolete IPX/SPX Protocol
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2002.10.30 Sean Leyne - Code Cleanup, removed obsolete "SUN3_3" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../jrd/common.h"
#include "gen/iberror.h"
#include "../jrd/jrd.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/jrd_proto.h"
#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../common/classes/init.h"
#include "../common/utils_proto.h"
#include "../jrd/os/os_utils.h"

#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#include "../common/config/config.h"

const char INET_FLAG = ':';

/* Unix/NFS specific stuff */
#ifndef NO_NFS

#if defined(HAVE_MNTENT_H)
#include <mntent.h>	/* get setmntent/endmntent */
#elif defined(HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>	/* get MNTTAB/_PATH_MNTTAB */
#elif defined(AIX)
#error ancient versions of AIX that do not provide "<mntent.h>" are not
#error supported. AIX 5.1+ provides this header.
#endif

#if   defined(_PATH_MOUNTED)
const char* const MTAB	= _PATH_MOUNTED;
#elif defined(HPUX)
const char* const MTAB	= "/etc/mnttab";
#elif defined(SOLARIS)
const char* const MTAB	= "/etc/mnttab";
#elif defined(FREEBSD)
const char* const MTAB	= "/etc/fstab";
#else
const char* const MTAB	= "/etc/mtab";
#endif

#ifdef HAVE_SETMNTENT
#define MTAB_OPEN(path, type)	setmntent(path, "r")
#define MTAB_CLOSE(stream)	endmntent(stream)
#else
#define MTAB_OPEN(path, type)	fopen(path, type)
#define MTAB_CLOSE(stream)	fclose(stream)
#endif

#endif //NO_NFS

#if defined(HPUX) && (!defined HP11)
#include <cluster.h>
#endif

#ifndef MAXHOSTLEN
#define MAXHOSTLEN	64
#endif

using namespace Firebird;

namespace {
	typedef Firebird::PathName tstring;
	typedef tstring::size_type size;
	typedef tstring::iterator iter;
	const size npos = tstring::npos;

#ifndef NO_NFS
	class osMtab
	{
	public:
		FILE* mtab;

		osMtab()
			: mtab(MTAB_OPEN(MTAB, "r"))
		{ }

		~osMtab()
		{
			if (mtab)
				MTAB_CLOSE(mtab);
		}

		bool ok() const { return mtab; }
	};

	class Mnt
	{
	private:
#ifdef DARWIN
		struct statfs* mnt_info;
		int mnt_cnt;
		int mnt_i;
#else
		osMtab mtab;
#endif // DARWIN
	public:
/*		Mnt() : AutoMemory(), mtab(), node(getPool()),
				mount(getPool()), path(getPool()) { } */
#ifdef DARWIN
		Mnt();
		bool ok() const { return this->mnt_cnt > 0; }
#else
		bool ok() const { return mtab.ok(); }
#endif /*DARWIN*/
		bool get();
		tstring node,  /* remote server name */
			mount, /* local mount point */
			path;  /* path on remote server */
	};
#endif //NO_NFS
} // anonymous namespace

#if (!defined NO_NFS || defined FREEBSD || defined NETBSD)
static void expand_filename2(tstring&, bool);
#endif


#if defined(WIN_NT)
static void translate_slashes(tstring&);
static void expand_share_name(tstring&);
static void share_name_from_resource(tstring&, LPNETRESOURCE);
static void share_name_from_unc(tstring&, LPREMOTE_NAME_INFO);
static bool get_full_path(const tstring&, tstring&);
#endif

#if defined(HPUX) && (!defined HP11)
static bool get_server(tstring&, tstring&);
#endif


#ifndef NO_NFS
bool ISC_analyze_nfs(tstring& expanded_filename, tstring& node_name)
{
/**************************************
 *
 *	I S C _ a n a l y z e _ n f s
 *
 **************************************
 *
 * Functional description
 *	Check a file name for an NFS mount point.  If so,
 *	decompose into node name and remote file name.
 *
 **************************************/

	// If we are ignoring NFS remote mounts then do not bother checking here
	// and pretend it's only local. MOD 16-Nov-2002

	if (Config::getRemoteFileOpenAbility()) {
		return false;
	}

	tstring max_node, max_path;
	size_t len = 0;

	// Search mount points
	Mnt mount;
	if (!mount.ok())
	{
		return false;
	}
	while (mount.get())
	{
		// first, expand any symbolic links in the mount point
		ISC_expand_filename(mount.mount, false);

		// if the whole mount point is not contained in the expanded_filename
		// or the mount point is not a valid pathname in the expanded_filename,
		// skip it
		if (expanded_filename.length() <= mount.mount.length() ||
			expanded_filename.compare(0, mount.mount.length(), mount.mount) != 0 ||
			expanded_filename[mount.mount.length()] != '/')
		{
			if (mount.mount == "/" && mount.path.hasData())
			{
				// root mount point = diskless client case
				mount.path += '/';
			}
			else
			{
				continue;
			}
		}

		// the longest mount point contained in the expanded_filename wins
		if (mount.mount.length() >= len)
		{
			len = mount.mount.length();
			if (mount.node.hasData())
			{
				max_node = mount.node;
				max_path = mount.path;
			}
			else
			{
				max_node = "";
				max_path = "";
			}
		}
	}

/* If the longest mount point was a local one, max_path is empty.
   Return false, leaving node_name empty and expanded_filename as is.

   If the longest mount point is from a remote node, max_path
   contains the root of the file's path as it is known on the
   remote node.  Return true, loading node_name with the remote
   node name and expanded_filename with the remote file name. */

	bool flag = !max_path.isEmpty();
	if (flag)
	{
		expanded_filename.replace(0, len, max_path);
		node_name = max_node;
	}
#if defined(HPUX) && (!defined HP11)
	else
	{
		flag = get_server(expanded_filename, node_name);
	}
#endif

	return flag;
}
#endif


#if defined(WIN_NT)
bool ISC_analyze_pclan(tstring& expanded_name, tstring& node_name)
{
/**************************************
 *
 *	I S C _ a n a l y z e _ p c l a n
 *
 **************************************
 *
 * Functional description
 *	Analyze a filename for a named pipe node name on the front.
 *	If one is found, extract the node name, compute the residual
 *	file name, and return true.  Otherwise return false.
 *
 **************************************/
	node_name.erase();
	if (expanded_name.length() < 2 ||
		(expanded_name[0] != '\\' && expanded_name[0] != '/') ||
		(expanded_name[1] != '\\' && expanded_name[1] != '/'))
	{
		return false;
	}

	const size p = expanded_name.find_first_of("\\/", 2);
	if (p == npos)
		return false;

	if (Config::getRemoteFileOpenAbility())
	{
		if (expanded_name.find(':', p + 1) == npos)
			return false;
	}

	node_name = "\\\\";
	node_name += expanded_name.substr(2, p - 2);

/* If this is a loopback, substitute "." for the host name.  Otherwise,
   the CreateFile on the pipe will fail. */
	TEXT localhost[MAXHOSTLEN];
	ISC_get_host(localhost, sizeof(localhost));
	if (node_name.substr(2, npos) == localhost)
	{
		node_name.replace(2, npos, ".");
	}

	expanded_name.erase(0, p + 1);
	return true;
}
#endif	// WIN_NT


bool ISC_analyze_tcp(tstring& file_name, tstring& node_name)
{
/**************************************
 *
 *	I S C _ a n a l y z e _ t c p		( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Analyze a filename for a TCP node name on the front.  If
 *	one is found, extract the node name, compute the residual
 *	file name, and return true.  Otherwise return false.
 *
 **************************************/

	// Avoid trivial case

	if (!file_name.hasData())
		return false;

/* Scan file name looking for separator character */

	node_name.erase();
	const size p = file_name.find(INET_FLAG);
	if (p == npos || p == 0 || p == file_name.length() - 1)
		return false;

	node_name = file_name.substr(0, p);

#ifdef WIN_NT
/* For Windows NT, insure that a single character node name does
   not conflict with an existing drive letter. */

	if (p == 1)
	{
		const ULONG dtype = GetDriveType((node_name + ":\\").c_str());
		// Is it removable, fixed, cdrom or ramdisk?
		if (dtype > DRIVE_NO_ROOT_DIR && (dtype != DRIVE_REMOTE || Config::getRemoteFileOpenAbility()))
		{
			// CVC: If we didn't match, clean our garbage or we produce side effects
			// in the caller.
			node_name.erase();
			return false;
		}
	}
#endif

	file_name.erase(0, p + 1);
	return true;
}


bool ISC_check_if_remote(const tstring& file_name, bool implicit_flag)
{
/**************************************
 *
 *	I S C _ c h e c k _ i f _ r e m o t e
 *
 **************************************
 *
 * Functional description
 *	Check to see if a path name resolves to a
 *	remote file.  If implicit_flag is true, then
 *	analyze the path to see if it resolves to a
 *	file on a remote machine.  Otherwise, simply
 *	check for an explicit node name.
 *
 **************************************/
	tstring temp_name = file_name;
	tstring host_name;
	return ISC_extract_host(temp_name, host_name, implicit_flag) != ISC_PROTOCOL_LOCAL;
}


iscProtocol ISC_extract_host(Firebird::PathName& file_name,
							 Firebird::PathName& host_name,
							 bool implicit_flag)
{
/**************************************
 *
 *	I S C _ e x t r a c t _ h o s t
 *
 **************************************
 *
 * Functional description
 *	Check to see if a file name resolves to a
 *	remote file.  If implicit_flag is true, then
 *	analyze the path to see if it resolves to a
 *	file on a remote machine.  Otherwise, simply
 *	check for an explicit node name.
 *  If file is found to be remote, extract
 *  the node name and compute the residual file name.
 *  Return protocol type.
 *
 **************************************/

/* Always check for an explicit TCP node name */

	if (ISC_analyze_tcp(file_name, host_name))
	{
		return ISC_PROTOCOL_TCPIP;
	}
#ifndef NO_NFS
	if (implicit_flag)
	{
		/* Check for a file on an NFS mounted device */

		if (ISC_analyze_nfs(file_name, host_name))
		{
			return ISC_PROTOCOL_TCPIP;
		}
	}
#endif

#if defined(WIN_NT)
/* Check for an explicit named pipe node name */

	if (ISC_analyze_pclan(file_name, host_name))
	{
		return ISC_PROTOCOL_WLAN;
	}

	if (implicit_flag)
	{
		/* Check for a file on a shared drive.  First try to expand
		   the path.  Then check the expanded path for a TCP or
		   named pipe. */

		ISC_expand_share(file_name);
		if (ISC_analyze_tcp(file_name, host_name))
		{
			return ISC_PROTOCOL_TCPIP;
		}
		if (ISC_analyze_pclan(file_name, host_name))
		{
			return ISC_PROTOCOL_WLAN;
		}

	}
#endif	// WIN_NT

	return ISC_PROTOCOL_LOCAL;
}


#if (!defined NO_NFS || defined FREEBSD || defined NETBSD)
bool ISC_expand_filename(tstring& buff, bool expand_mounts)
{
/**************************************
 *
 *	I S C _ e x p a n d _ f i l e n a m e		( N F S )
 *
 **************************************
 *
 * Functional description
 *	Expand a filename by following links.  As soon as a TCP node name
 *	shows up, stop translating.
 *
 **************************************/

	expand_filename2(buff, expand_mounts);
	return true;
}
#endif


#ifdef WIN_NT

static void translate_slashes(tstring& Path)
{
	const char sep = '\\';
	const char bad_sep = '/';
	for (char *p = Path.begin(), *q = Path.end(); p < q; p++)
	{
		if (*p == bad_sep) {
			*p = sep;
		}
	}
}


static bool isDriveLetter(const tstring::char_type letter)
{
	return (letter >= 'A' && letter <= 'Z') || (letter >= 'a' && letter <= 'z');
}


// Code of this function is a slightly changed version of this routine
// from Jim Barry (jim.barry@bigfoot.com) published at
// http://www.geocities.com/SiliconValley/2060/articles/longpaths.html

static bool ShortToLongPathName(tstring& Path)
{
	// Special characters.
	const char sep = '\\';
	const char colon = ':';

	// Copy the short path into the work buffer and convert forward
	// slashes to backslashes.
	translate_slashes(Path);

	// We need a couple of markers for stepping through the path.
	size left = 0;
	size right = 0;
	bool found_root = false;

	// Parse the first bit of the path.
	// Probably has to change to use GetDriveType.
	if (Path.length() >= 2 && isDriveLetter(Path[0]) && colon == Path[1]) // Drive letter?
	{
		if (Path.length() == 2) // 'bare' drive letter
		{
			right = npos; // skip main block
		}
		else if (sep == Path[2]) // drive letter + backslash
		{
			// FindFirstFile doesn't like "X:\"
			if (Path.length() == 3)
			{
				right = npos; // skip main block
			}
			else
			{
				left = right = 3;
				found_root = true;
			}
		}
		else
		{
			return false; // parsing failure
		}
	}
	else if (Path.length() >= 1 && sep == Path[0])
	{
		if (Path.length() == 1) // 'bare' backslash
		{
			right = npos;  // skip main block
		}
		else
		{
			if (sep == Path[1]) // is it UNC?
			{
				// Find end of machine name
				right = Path.find_first_of(sep, 2);
				if (npos == right)
				{
					return false;
				}

				// Find end of share name
				right = Path.find_first_of(sep, right + 1);
				if (npos == right)
				{
					return false;
				}
			}
			found_root = true;
			++right;
		}
	}
	// else FindFirstFile will handle relative paths

	bool error = false;

	if (npos != right)
	{
		// We don't allow wilcards as they will be processed by FindFirstFile
		// and we would get the first matching file. Incidentally, we are disablimg
		// escape sequences to produce long names beyond MAXPATHLEN with ??
		if (Path.find_first_of("*") != npos || Path.find_first_of("?") != npos)
		{
		    right = npos;
			error = true;
		}
		else
		{
			// We'll assume there's a file at the end. If the user typed a dir,
			// we'll go one dir above.
			const size last = Path.find_last_of(sep);
			if (npos != last)
			{
				Path[last] = 0;
				const DWORD rc = GetFileAttributes(Path.c_str());
				// Assuming the user included a file name (that's what we want),
				// the path one level above should exist and should be a directory.
				if (rc == 0xFFFFFFFF || !(rc & FILE_ATTRIBUTE_DIRECTORY))
				{
					right = npos;
					error = true;
				}

				Path[last] = sep;
			}
		}
	}

	// The data block for FindFirstFile.
	WIN32_FIND_DATA fd;

	// Main parse block - step through path.
	HANDLE hf = INVALID_HANDLE_VALUE;
	const size leftmost = right;

	while (npos != right)
	{
		left = right; // catch up

		// Find next separator.
		const size right2 = Path.find_first_of(sep, right);

		// Temporarily replace the separator with a null character so that
		// the path so far can be passed to FindFirstFile.
		if (npos != right2)
		{
			Path[right2] = 0;
		}

		// Prevent the directory traversal attack and other anomalies like
		// duplicate directory names.
		// Take advantage of the previous statement (truncation) to compare directly
		// with the special directory names, avoiding the overhead of substr().
		// Please note that we are more thorough than GetFullPathName but we yield
		// here different results because that API function interprets "." and ".."
		// but we skip them here.
		tstring::const_pointer special_dir = &Path.at(right);
		if (!strcmp(special_dir, ".") || (!found_root || right < 2) && !strcmp(special_dir, ".."))
		{
			Path.erase(right, (npos == right2) ? npos : right2 - right + 1);
			if (right >= Path.length())
				right = npos;

			continue;
		}

		if (found_root && !strcmp(special_dir, ".."))
		{
			// right being zero handled above
			const size prev = Path.find_last_of(sep, right - 2);
			if (prev >= leftmost && prev < right) // prev != npos implicit
				right = prev + 1;

			Path.erase(right, (npos == right2) ? npos : right2 - right + 1);

			if (right >= Path.length())
				right = npos;

			continue;
		}

		right = right2;

		// Call FindFirstFile on the path.
		hf = FindFirstFile(Path.c_str(), &fd);

		// Put back the separator.
		if (npos != right)
		{
			Path[right] = sep;
		}

		// See what FindFirstFile makes of the path so far.
		if (hf == INVALID_HANDLE_VALUE)
		{
			error = (npos != right);
			break;
		}
		FindClose(hf);

		// The file was found - replace the short name with the long.
		const size old_len = (npos == right) ? Path.length() - left : right - left;
		const size new_len = strlen(fd.cFileName);
		Path.replace(left, old_len, fd.cFileName, new_len);

		// More to do?
		if (right != npos)
		{
			// Yes - move past separator .
			right = left + new_len + 1;

			// Did we overshoot the end? (i.e. path ends with a separator).
			if (right >= Path.length())
			{
				right = npos;
			}
		}
	}

	// We failed to find this file.
	if (hf == INVALID_HANDLE_VALUE && error)
	{
		return false;
	}

	return true;
}

bool ISC_expand_filename(tstring& file_name, bool expand_mounts)
{
/**************************************
 *
 *	I S C _ e x p a n d _ f i l e n a m e		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Fully expand a file name.  If the file doesn't exist, do something
 *	intelligent.
 *
 **************************************/
	// check for empty filename to avoid multiple checks later
	if (file_name.isEmpty())
	{
		return false;
	}

	bool fully_qualified_path = false;
	tstring temp = file_name;

	expand_share_name(temp);

	// If there is an explicit node name of the form \\DOPEY or //DOPEY
	// assume named pipes.  Translate forward slashes to back slashes
	// and return with no further processing.

	if ((file_name.length() >= 2) &&
		((file_name[0] == '\\' && file_name[1] == '\\') ||
			(file_name[0] == '/' && file_name[1] == '/')))
	{
		file_name = temp;

		// Translate forward slashes to back slashes
		translate_slashes(file_name);
		return true;
	}

	tstring device;
	const size colon_pos = temp.find(INET_FLAG);
	if (colon_pos != npos)
	{
		file_name = temp;
		if (colon_pos != 1)
		{
			return true;
		}
		device = temp.substr(0, 1) + ":\\";
		const USHORT dtype = GetDriveType(device.c_str());
		if (dtype <= DRIVE_NO_ROOT_DIR)
		{
			return true;
		}

		// This happen if remote interface of our server
		// rejected WNet connection or we were called with:
		// localhost:R:\Path\To\Database, where R - remote disk
		if (dtype == DRIVE_REMOTE && expand_mounts)
		{
			ISC_expand_share(file_name);
			translate_slashes(file_name);
			return true;
		}

		if ((temp.length() >= 3) && (temp[2] == '/' || temp[2] == '\\'))
		{
			fully_qualified_path = true;
		}
	}

	// Translate forward slashes to back slashes

	translate_slashes(temp);

	// If there is an explicit node name of the form \\DOPEY don't do any
	// additional translations -- everything will need to be applied at
	// the other end.

	if ((temp.length() >= 2) && (temp[0] == '\\' && temp[1] == '\\'))
	{
		file_name = temp;
		return true;
	}
	if (temp[0] == '\\' || temp[0] == '/')
	{
		fully_qualified_path = true;
	}

	// Expand the file name

#ifdef SUPERSERVER
	if (!fully_qualified_path)
	{
		fb_utils::getCwd(file_name);
		if (device.hasData() && device[0] == file_name[0])
		{
			// case where temp is of the form "c:foo.fdb" and
			// expanded_name is "c:\x\y".
			file_name += '\\';
			file_name.append (temp, 2, npos);
		}
		else if (device.empty())
		{
			// case where temp is of the form "foo.fdb" and
			// expanded_name is "c:\x\y".
			file_name += '\\';
			file_name += temp;
		}
		else
		{
			// case where temp is of the form "d:foo.fdb" and
			// expanded_name is "c:\x\y".
			// Discard expanded_name and use temp as it is.
			// In this case use the temp but we need to ensure that we expand to
			// temp from "d:foo.fdb" to "d:\foo.fdb"
			if (!get_full_path(temp, file_name))
			{
				file_name = temp;
			}
		}
	}
	else
#endif
	{
		// Here we get "." and ".." translated by the API.
		if (!get_full_path(temp, file_name))
		{
			file_name = temp;
		}
	}

	// convert then name to its longer version ie. convert longfi~1.fdb
	// to longfilename.fdb
	bool rc = ShortToLongPathName(file_name);

	// Filenames are case insensitive on NT.  If filenames are
	// typed in mixed cases, strcmp () used in various places
	// results in incorrect behavior.
	file_name.upper();
	return rc;
}
#endif


#if defined(WIN_NT)
void ISC_expand_share(tstring& file_name)
{
/**************************************
 *
 *	I S C _ e x p a n d _ s h a r e
 *
 **************************************
 *
 * Functional description
 *	Expand a file name by chasing shared disk
 *	information.
 *
 **************************************/
// see NT reference for WNetEnumResource for the following constants
	DWORD nument = 0xffffffff, bufSize = 16384;

// Look for a drive letter and make sure that it corresponds to a remote disk
	const size p = file_name.find(':');
	if (p != 1)
	{
		return;
	}

	// If RemoteFileOpenAbility = 1 doesn't expand share
	if (Config::getRemoteFileOpenAbility()) {
		return;
	}

	tstring device(file_name.substr(0, 1));
	const USHORT dtype = GetDriveType((device + ":\\").c_str());
	if (dtype != DRIVE_REMOTE)
	{
		return;
	}

	HANDLE handle;
	if (WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK, 0, NULL, &handle) != NO_ERROR)
	{
		return;
	}
	LPNETRESOURCE resources = (LPNETRESOURCE) gds__alloc((SLONG) bufSize);
/* FREE: in this routine */
	if (!resources)				/* NOMEM: don't expand the filename */
	{
		return;
	}

	DWORD ret = WNetEnumResource(handle, &nument, resources, &bufSize);
	if (ret == ERROR_MORE_DATA)
	{
		gds__free(resources);
		resources = (LPNETRESOURCE) gds__alloc((SLONG) bufSize);
		/* FREE: in this routine */
		if (!resources)			/* NOMEM: don't expand the filename */
		{
			return;
		}
		ret = WNetEnumResource(handle, &nument, resources, &bufSize);
	}

	LPNETRESOURCE res = resources;
	DWORD i = 0;
	while (i < nument && (!res->lpLocalName || (device[0] != *(res->lpLocalName))))
	{
		i++;
		res++;
	}
	if (i != nument)			/* i.e. we found the drive in the resources list */
	{
		share_name_from_resource(file_name, res);
	}

	WNetCloseEnum(handle);

/* Win95 doesn't seem to return shared drives, so the following
   has been added... */

	if (i == nument)
	{
		device += ':';
		LPREMOTE_NAME_INFO res2 = (LPREMOTE_NAME_INFO) resources;
		ret = WNetGetUniversalName(device.c_str(), REMOTE_NAME_INFO_LEVEL, res2, &bufSize);
		if (ret == ERROR_MORE_DATA)
		{
			gds__free(resources);
			resources = (LPNETRESOURCE) gds__alloc((SLONG) bufSize);
			if (!resources)		/* NOMEM: don't expand the filename */
			{
				return;
			}
			res2 = (LPREMOTE_NAME_INFO) resources;
			ret = WNetGetUniversalName(device.c_str(), REMOTE_NAME_INFO_LEVEL, res2, &bufSize);
		}
		if (ret == NO_ERROR)
		{
			share_name_from_unc(file_name, res2);
		}
	}


	if (resources)
	{
		gds__free(resources);
	}
}
#endif	// WIN_NT

#ifdef NOT_USED_OR_REPLACED
// There's no signature for this function in any header file.
#ifdef SUPERSERVER
int ISC_strip_extension(TEXT* file_name)
{
/**************************************
 *
 *      I S C _ s t r i p _ e x t e n s i o n 	( S U P E R S E R V E R )
 *
 **************************************
 *
 * Functional description
 *	Get rid of the file name extension part
 *	(after the dot '.')
 *
 **************************************/

/* Set p to point to the starting part of the actual file name
   (sans directory name) */

	TEXT* p = strrchr(file_name, '/');
	TEXT* q = strrchr(file_name, '\\');

	if (p || q)
	{
		/* Get the maximum of the two */

		if (q > p)
			p = q;
	}
	else
		p = file_name;

/* Now search for the first dot in the actual file name */

	q = strchr(p, '.');
	if (q)
		*q = '\0';				/* Truncate the extension including the dot */

	return strlen(file_name);
}
#endif
#endif


#if (!defined NO_NFS || defined FREEBSD || defined NETBSD)
static void expand_filename2(tstring& buff, bool expand_mounts)
{
/**************************************
 *
 *	e x p a n d _ f i l e n a m e 2		( N F S )
 *
 **************************************
 *
 * Functional description
 *	Expand a filename by following links.  As soon as a TCP node name
 *	shows up, stop translating.
 *
 **************************************/

	// If the filename contains a TCP node name, don't even try to expand it
	if (buff.find(INET_FLAG) != npos)
	{
		return;
	}

	const tstring src = buff;
	const char* from = src.c_str();
	buff = "";

	// Handle references to default directories (tilde refs)
	if (*from == '~')
	{
		++from;
		tstring q;
		while (*from && *from != '/')
			q += *from++;
		if (os_utils::get_user_home(q.hasData() ? os_utils::get_user_id(q.c_str()) : geteuid(),
									buff))
		{
			expand_filename2(buff, expand_mounts);
		}
	}

	// If the file is local, expand partial pathnames with default directory
	if (*from && *from != '/')
	{
		fb_utils::getCwd(buff);
		buff += '/';
	}

	// Process file name segment by segment looking for symbolic links.
	while (*from)
	{

		// skip dual // (will collapse /// to / as well)
		if (*from == '/' && from[1] == '/')
		{
			++from;
			continue;
		}

		// Copy the leading slash, if any
		if (*from == '/')
		{
			if (buff.hasData() && (buff.end()[-1] == '/'))
			{
				++from;
			}
			else
			{
				buff += *from++;
			}
			continue;
		}

		// Handle self references
		if (*from == '.' && (from[1] == '.' || from[1] == '/'))
		{
			if (*++from == '.')
			{
				++from;
				if (buff.length() > 2)
				{
					const size slash = buff.rfind('/', buff.length() - 2);
					buff = slash != npos ? buff.substr(0, slash + 1) : "/";
				}
			}
			continue;
		}

		// Copy the rest of the segment name
		const int segment = buff.length();
		while (*from && *from != '/')
		{
			buff += *from++;
		}

		// If the file is local, check for a symbol link
		TEXT temp[MAXPATHLEN];
		const int n = readlink(buff.c_str(), temp, sizeof(temp));
		if (n < 0)
		{
			continue;
		}

		// We've got a link.  If it contains a node name or it starts
		// with a slash, it replaces the initial segment so far.
		const tstring link(temp, n);
		if (link.find(INET_FLAG) != npos)
		{
			buff = link;
			return;
		}
		if (link[0] == '/')
		{
			buff = link;
		}
		else
		{
			buff.replace(segment, buff.length() - segment, link);
		}

		// Whole link needs translating -- recurse
		expand_filename2(buff, expand_mounts);
	}

#ifndef NO_NFS
	// If needed, call ISC_analyze_nfs to handle NFS mount points.
	if (expand_mounts)
	{
		tstring nfsServer;
		if (ISC_analyze_nfs(buff, nfsServer))
		{
			buff.insert(0, ":");
			buff.insert(0, nfsServer);
		}
	}
#endif //NO_NFS
}
#endif


#ifdef WIN_NT
static void expand_share_name(tstring& share_name)
{
/**************************************
 *
 *	e x p a n d _ s h a r e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Look for a Windows NT share name at the
 *	beginning of a string having the form:
 *
 *	    \!share name!\file name
 *
 *	If such a share name is found, expand it
 *	and then append the file name to the
 *	expanded path.
 *
 **************************************/

	TEXT workspace[MAXPATHLEN];

	const TEXT* p = share_name.c_str();
	if (*p++ != '\\' || *p++ != '!') {
		return;
	}

	strncpy(workspace, p, sizeof(workspace));
	workspace[sizeof(workspace) - 1] = 0;
	// We test for *q, too, to avoid buffer overrun.
	TEXT* q;
	for (q = workspace; *q && *p && *p != '!'; p++, q++);
	// empty body loop
	*q = '\0';
	if (*p++ != '!' || *p++ != '\\') {
		return;
	}

	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares",
					 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
	{
		return;
	}

	BYTE data_buf[MAXPATHLEN];
	DWORD d_size = MAXPATHLEN;
	DWORD type_code;
	LPBYTE data = data_buf;

	DWORD ret = RegQueryValueEx(hkey, workspace, NULL, &type_code, data, &d_size);
	if (ret == ERROR_MORE_DATA)
	{
		d_size++;
		data = (LPBYTE) gds__alloc((SLONG) d_size);
		// FREE: unknown
		if (!data)
		{			// NOMEM:
			RegCloseKey(hkey);
			return;				// Error not really handled
		}
		ret = RegQueryValueEx(hkey, workspace, NULL, &type_code, data, &d_size);
	}

	if (ret == ERROR_SUCCESS)
	{
		for (const TEXT* s = reinterpret_cast<const TEXT*>(data); s && *s;
			s = (type_code == REG_MULTI_SZ) ? s + strlen(s) + 1 : NULL)
		{
			if (!strnicmp(s, "path", 4))
			{
				// CVC: Paranoid protection against buffer overrun.
				// MAXPATHLEN minus NULL terminator, the possible backslash and p==db_name.
				// Otherwise, it's possible to create long share plus long db_name => crash.
				size_t idx = strlen(s + 5);
				if (idx + 1 + (s[4 + idx] == '\\' ? 1 : 0) + strlen(p) >= MAXPATHLEN)
					break;

				strcpy(workspace, s + 5);	// step past the "Path=" part
				// idx = strlen (workspace); Done previously.
				if (workspace[idx - 1] != '\\')
					workspace[idx++] = '\\';
				strcpy(workspace + idx, p);
				share_name = workspace;
				break;
			}
		}
	}

	if (data != data_buf) {
		gds__free(data);
	}

	RegCloseKey(hkey);
}


// Expand the full file name for incomplete or relative paths in Windows.
// Notice the API doesn't guarantee that the resulting path and filename are valid.
// In this regard, our custom ShortToLongPathName() is more thorough, although
// it produces different results, because it skips "." and ".." in the path.
static bool get_full_path(const tstring& part, tstring& full)
{
	TEXT buf[MAXPATHLEN];
	TEXT *p;
	const int l = GetFullPathName(part.c_str(), MAXPATHLEN, buf, &p);
	if (l && l < MAXPATHLEN)
	{
		full = buf;
		return true;
	}

	return false;
}
#endif


namespace {

#ifndef NO_NFS
#if defined(HAVE_GETMNTENT) && !defined(SOLARIS)
#define GET_MOUNTS
#if defined(GETMNTENT_TAKES_TWO_ARGUMENTS) /* SYSV stylish */
bool Mnt::get()
{
/**************************************
 *
 *	g e t _ m o u n t s	( S Y S T E M _ V )
 *				( E P S O N )
 *				( M 8 8 K )
 *				( U N I X W A R E )
 *
 **************************************
 *
 * Functional description
 *	Get ALL mount points.
 *
 **************************************/
	struct mnttab *mptr, mnttab;

/* Start by finding a mount point. */

	TEXT* p = buffer;

	mptr = &mnttab;
	if (getmntent(file, mptr) == 0)
	{
		/* Include non-NFS (local) mounts - some may be longer than
		   NFS mount points */

		mount->mnt_node = p;
		const TEXT* q = mptr->mnt_special;
		while (*q && *q != ':')
			*p++ = *q++;
		*p++ = 0;
		if (*q != ':')
			mount->mnt_node = NULL;
		if (*q)
			q++;
		mount->mnt_path = p;
	    while ((*p++ = *q++) != 0); // empty loop's body.
		mount->mnt_mount = mptr->mnt_mountp;
		return true;
	}

	return false;
}

#else // !GETMNTENT_TAKES_TWO_ARGUMENTS

bool Mnt::get()
{
/**************************************
 *
 *	g e t _ m o u n t s	( M N T E N T )
 *
 **************************************
 *
 * Functional description
 *	Get ALL mount points.
 *
 **************************************/
	// Start by finding a mount point. */
	fb_assert(mtab.mtab);
	struct mntent* mptr = getmntent(mtab.mtab);
	if (!mptr)
	{
		return false;
	}

	// Include non-NFS (local) mounts - some may be longer than
	// NFS mount points, therefore ignore mnt_type

	const char* iflag = strchr(mptr->mnt_fsname, ':');

	if (iflag)
	{
		node = tstring(mptr->mnt_fsname, iflag - mptr->mnt_fsname);
		path = tstring(++iflag);
	}
	else
	{
		node.erase();
		path.erase();
	}
	mount = mptr->mnt_dir;
	return true;
}
#endif // GETMNTENT_TAKES_TWO_ARGUMENTS
#endif // HAVE_GETMNTENT && !SOLARIS


#ifdef SOLARIS
#define GET_MOUNTS
bool Mnt::get()
{
/**************************************
 *
 *	g e t _ m o u n t s	( SOLARIS)
 *
 **************************************
 *
 * Functional description
 *	Get ALL mount points.
 *
 **************************************/

 /* This code is tested on Solaris 2.6 IA */
 TEXT device[128], mount_point[128], type[16], opts[256], ftime[128];

	const int n = fscanf(mtab.mtab, "%s %s %s %s %s ", device, mount_point, type, opts, ftime);
	const char* start = device;

	if (n<5)
	return false;

	const char* iflag = strchr(device, ':');
	if (iflag)
	{
		node = tstring( start , size_t(iflag - start) );
		path = tstring( ++iflag );
	}
	else
	{
		node.erase();
		path.erase();
	}
	mount = mount_point;
	return true;


}
#endif //Solaris

#ifdef DARWIN
#define GET_MOUNTS
Mnt::Mnt() : mnt_i(0)
{
	this->mnt_info = NULL;
	this->mnt_cnt = getmntinfo(&this->mnt_info, MNT_NOWAIT);
}

bool Mnt::get()
{
	if (this->mnt_i >= this->mnt_cnt) {
		return false;
	}

	const char* start = this->mnt_info[this->mnt_i].f_mntfromname;
	const char* iflag = strchr(this->mnt_info[this->mnt_i].f_mntfromname, ':');

	if (iflag)
	{
		node = tstring(start, size_t(iflag - start));
		path = tstring(++iflag);
	}
	else
	{
		node.erase();
		path.erase();
	}
	mount = this->mnt_info[this->mnt_i].f_mntonname;
	this->mnt_i++;
	return true;
}
#endif // DARWIN
#ifndef GET_MOUNTS
bool Mnt::get()
{
/**************************************
 *
 *	g e t _ m o u n t s	( g e n e r i c - U N I X )
 *
 **************************************
 *
 * Functional description
 *	Get ALL mount points.
 *
 **************************************/

/* Solaris uses this because:
	Since we had to substitute an alternative for the stdio supplied
	with Solaris, we cannot use the getmntent() library call which
	wants a Solaris stdio FILE* as an argument, so we parse the text-
	type /etc/mnttab file ourselves.     - from FB1

	This will still apply with SFIO on FB2.  nmcc Dec2002
*/

	TEXT device[128], mount_point[128], type[16], rw[128], foo1[16];

/* Start by finding a mount point. */

	TEXT* p = buffer;

	for (;;)
	{
		const int n = fscanf(file, "%s %s %s %s %s %s", device, mount_point, type, rw, foo1, foo1);
#ifdef SOLARIS
		if (n != 5)
#else
		if (n < 0)
#endif
			break;

		/* Include non-NFS (local) mounts - some may be longer than
		   NFS mount points */

		/****
		if (strcmp (type, "nfs"))
			continue;
		****/

		mount->mnt_node = p;
		const TEXT* q = device;
		while (*q && *q != ':')
			*p++ = *q++;
		*p++ = 0;
		if (*q != ':')
			mount->mnt_node = NULL;
		if (*q)
			q++;
		mount->mnt_path = p;
		while (*p++ = *q++); // empty loop's body
		mount->mnt_mount = p;
		q = mount_point;
		while (*p++ = *q++); // empty loop's body
		return true;
	}

	return false;
}
#endif // GET_MOUNTS

#if defined(HPUX) && (!defined HP11)
static bool get_server(tstring&, tstring& node_name)
{
/**************************************
 *
 *	g e t _ s e r v e r		( H P - U X )
 *
 **************************************
 *
 * Functional description
 *	If we're running on a cnode, the file system belongs
 *	to the server node - load node_name with the server
 *	name and return true.
 *
 **************************************/
	TEXT hostname[64];
	const struct cct_entry* cnode = getccnam(ISC_get_host(hostname, sizeof(hostname)));
	if (!cnode || cnode->cnode_type == 'r')
	{
		return false;
	}

	setccent();
	while (cnode->cnode_type != 'r')
	{
		cnode = getccent();
	}

	node_name = cnode->cnode_name;
	return true;
}
#endif // HPUX
#endif // NO_NFS
} // anonymous namespace


#ifdef WIN_NT
static void share_name_from_resource(tstring& file_name, LPNETRESOURCE resource)
{
/**************************************
 *
 *	s h a r e _ n a m e _ f r o m _ r e s o u r c e
 *
 **************************************
 *
 * Functional description
 *	if the shared drive is Windows or Novell prosess the
 *	name appropriately, otherwise just return the remote name
 *	expects filename to be of the form DRIVE_LETTER:\PATH
 *	returns new filename in expanded_name; shouldn't touch filename
 *
 **************************************/
	tstring expanded_name = resource->lpRemoteName;

	const TEXT* mwn = "Microsoft Windows Network";
	if (!strnicmp(resource->lpProvider, mwn, strlen(mwn)))
	{
		/* If the shared drive is via Windows
		   package it up so that resolution of the share name can
		   occur on the remote machine. The name
		   that will be transmitted to the remote machine will
		   have the form \\REMOTE_NODE\!SHARE_POINT!\FILENAME */

		size p = expanded_name.find('\\', 2);
		expanded_name.insert(++p, 1, '!');
		expanded_name += '!';
		file_name.replace(0, 2, expanded_name);
	}
	else
	{
		// we're guessing that it might be an NFS shared drive

		iter q = expanded_name.end() - 1;
		if (*q == '\\' || *q == '/')	/* chop off any trailing \ or / */
		{
			expanded_name.erase(q);
		}
		file_name.replace(0, 2, expanded_name);

		/* If the expanded filename doesn't begin with a node name of the form
		\\NODE and it contains a ':', then it's probably an NFS mounted drive.
		Therefore we must convert any back slashes to forward slashes. */

		if ((file_name[0] != '\\' || file_name[1] != '\\') && (file_name.find(INET_FLAG) != npos))
		{
			for (q = file_name.begin(); q < file_name.end(); ++q)
			{
				if (*q == '\\')
				{
					*q = '/';
				}
			}
		}
	}
}


static void share_name_from_unc(tstring& file_name, LPREMOTE_NAME_INFO unc_remote)
{
/**************************************
 *
 *      s h a r e _ n a m e _ f r o m _ u n c
 *
 **************************************
 *
 * Functional description
 *      Extract the share name from a REMOTE_NAME_INFO struct
 *      returned by WNetGetUniversalName.   It uses only the
 *      lpConnectionName element of the structure.   It converts
 *      "\\node\sharepoint" to "\\node\!sharepoint!" and appends
 *      the rest of file_name after the drive into expanded_name.
 *
 **************************************/
	tstring expanded_name = unc_remote->lpConnectionName;

	/* bracket the share name with "!" characters */
	size p = expanded_name.find('\\', 2);
	expanded_name.insert(++p, 1, '!');
	p = expanded_name.find('\\', p + 1);
	if (p != npos)
	{
		expanded_name.erase(p, npos);
	}
	expanded_name += '!';

	/* add rest of file name */
	file_name.replace(0, 2, expanded_name);
}
#endif /* WIN_NT */


// Converts a string from the system charset to UTF-8.
void ISC_systemToUtf8(Firebird::AbstractString& str)
{
	if (str.isEmpty())
		return;

#ifdef WIN_NT
	WCHAR utf16Buffer[MAX_PATH];
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(),
		utf16Buffer, sizeof(utf16Buffer) / sizeof(WCHAR));

	if (len == 0)
		status_exception::raise(Arg::Gds(isc_bad_conn_str) << Arg::Gds(isc_transliteration_failed));

	char utf8Buffer[MAX_PATH * 4];
	len = WideCharToMultiByte(CP_UTF8, 0, utf16Buffer, len, utf8Buffer, sizeof(utf8Buffer),
		NULL, NULL);

	if (len == 0)
		status_exception::raise(Arg::Gds(isc_bad_conn_str) << Arg::Gds(isc_transliteration_failed));

	memcpy(str.getBuffer(len), utf8Buffer, len);
#endif
}


// Converts a string from UTF-8 to the system charset.
void ISC_utf8ToSystem(Firebird::AbstractString& str)
{
	if (str.isEmpty())
		return;

#ifdef WIN_NT
	WCHAR utf16Buffer[MAX_PATH];
	int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(),
		utf16Buffer, sizeof(utf16Buffer) / sizeof(WCHAR));

	if (len == 0)
		status_exception::raise(Arg::Gds(isc_bad_conn_str) << Arg::Gds(isc_transliteration_failed));

	char ansiBuffer[MAX_PATH * 4];
	BOOL defaultCharUsed;
	len = WideCharToMultiByte(CP_ACP, 0, utf16Buffer, len, ansiBuffer, sizeof(ansiBuffer),
		NULL, &defaultCharUsed);

	if (len == 0 || defaultCharUsed)
		status_exception::raise(Arg::Gds(isc_bad_conn_str) << Arg::Gds(isc_transliteration_failed));

	memcpy(str.getBuffer(len), ansiBuffer, len);
#endif
}


// Escape Unicode characters from a string
void ISC_escape(AbstractString& str)
{
#if 0	// CORE-2929
	size_t pos = 0;
	while ((pos = str.find_first_of("#", pos)) != npos)
	{
		str.insert(pos, "#");
		pos += 2;
	}
#endif
}


// Adapted from macro in ICU headers.
static inline void FB_U8_APPEND_UNSAFE(char* s, int& i, const int c)
{
	if ((unsigned int) c <= 0x7f) {
		s[i++] = (unsigned char) c;
	}
	else
	{
		if ((unsigned int) c <= 0x7ff) {
			s[i++] = (unsigned char) ((c >> 6) | 0xc0);
		}
		else
		{
			if ((unsigned int) c <= 0xffff) {
				s[i++] = (unsigned char) ((c >> 12) | 0xe0);
			}
			else
			{
				s[i++] = (unsigned char) ((c >> 18) | 0xf0);
				s[i++] = (unsigned char) (((c >> 12) & 0x3f) | 0x80);
			}
			s[i++] = (unsigned char) (((c >> 6) & 0x3f) | 0x80);
		}
		s[i++] = (unsigned char) ((c & 0x3f) | 0x80);
	}
}


// Unescape Unicode characters from a string
void ISC_unescape(AbstractString& str)
{
#if 0	// CORE-2929
	size_t pos = 0;
	while ((pos = str.find_first_of("#", pos)) != npos)
	{
		const char* p = str.c_str() + pos;

		if (pos + 5 <= str.length() &&
			((p[1] >= '0' && p[1] <= '9') || (toupper(p[1]) >= 'A' && toupper(p[1]) <= 'F')) &&
			((p[2] >= '0' && p[2] <= '9') || (toupper(p[2]) >= 'A' && toupper(p[2]) <= 'F')) &&
			((p[3] >= '0' && p[3] <= '9') || (toupper(p[3]) >= 'A' && toupper(p[3]) <= 'F')) &&
			((p[4] >= '0' && p[4] <= '9') || (toupper(p[4]) >= 'A' && toupper(p[4]) <= 'F')))
		{
			char sCode[5];
			memcpy(sCode, p + 1, 4);
			sCode[4] = '\0';
			const int code = strtol(sCode, NULL, 16);

			char unicode[4];
			int len = 0;
			FB_U8_APPEND_UNSAFE(unicode, len, code);

			str.replace(pos, 5, string(unicode, len));
			pos += len;
		}
		else if (pos + 2 <= str.length() && p[1] == '#')
			str.erase(pos++, 1);
		else
			status_exception::raise(Arg::Gds(isc_bad_conn_str) << Arg::Gds(isc_escape_invalid));
	}
#endif
}
