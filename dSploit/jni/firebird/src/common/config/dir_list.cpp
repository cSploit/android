/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		dir_list.cpp
 *	DESCRIPTION:	Directory listing config file operation
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
 * Created by: Alex Peshkov <AlexPeshkov@users.sourceforge.net>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../jrd/os/path_utils.h"
#include "../jrd/gds_proto.h"
#include "../jrd/TempSpace.h"

namespace Firebird {

void ParsedPath::parse(const PathName& path)
{
	clear();

	if (path.length() == 1) {
		add(path);
		return;
	}

	PathName oldpath = path;
	do {
		PathName newpath, elem;
		PathUtils::splitLastComponent(newpath, elem, oldpath);
		oldpath = newpath;
		insert(0, elem);
	} while (oldpath.length() > 0);
}

PathName ParsedPath::subPath(size_t n) const
{
	PathName rc = (*this)[0];
	if (PathUtils::isRelative(rc + PathUtils::dir_sep))
		rc = PathUtils::dir_sep + rc;
	for (size_t i = 1; i < n; i++)
	{
		PathName newpath;
		PathUtils::concatPath(newpath, rc, (*this)[i]);
		rc = newpath;
	}
	return rc;
}

ParsedPath::operator PathName() const
{
	if (!getCount())
		return "";
	return subPath(getCount());
}

bool ParsedPath::contains(const ParsedPath& pPath) const
{
	size_t nFullElem = getCount();
	if (nFullElem > 1 && (*this)[nFullElem - 1].length() == 0)
		nFullElem--;

	if (pPath.getCount() < nFullElem) {
		return false;
	}

	for (size_t i = 0; i < nFullElem; i++)
	{
		if (pPath[i] != (*this)[i]) {
			return false;
		}
	}

	for (size_t i = nFullElem + 1; i <= pPath.getCount(); i++)
	{
		const PathName x = pPath.subPath(i);
		if (PathUtils::isSymLink(x)) {
			return false;
		}
	}
	return true;
}

bool DirectoryList::keyword(const ListMode keyMode, PathName& value, PathName key, PathName next)
{
	if (value.length() < key.length()) {
		return false;
	}
	PathName keyValue = value.substr(0, key.length());
	if (keyValue != key) {
		return false;
	}
	if (next.length() > 0)
	{
		if (value.length() == key.length()) {
			return false;
		}
		keyValue = value.substr(key.length());
		if (next.find(keyValue[0]) == PathName::npos) {
			return false;
		}
		PathName::size_type startPos = keyValue.find_first_not_of(next);
		if (startPos == PathName::npos) {
			return false;
		}
		value = keyValue.substr(startPos);
	}
	else
	{
		if (value.length() > key.length()) {
			return false;
		}
		value.erase();
	}
	mode = keyMode;
	return true;
}

void DirectoryList::initialize(bool simple_mode)
{
	if (mode != NotInitialized)
		return;

	clear();

	PathName val = getConfigString();

	if (simple_mode) {
		mode = SimpleList;
	}
	else
	{
		if (keyword(None, val, "None", "") || keyword(Full, val, "Full", "")) {
			return;
		}
		if (! keyword(Restrict, val, "Restrict", " \t"))
		{
			gds__log("DirectoryList: unknown parameter '%s', defaulting to None", val.c_str());
			mode = None;
			return;
		}
	}

	size_t last = 0;
	PathName root = Config::getRootDirectory();
	size_t i;
	for (i = 0; i < val.length(); i++)
	{
		if (val[i] == ';')
		{
			PathName dir = "";
			if (i > last)
			{
				dir = val.substr(last, i - last);
				dir.trim();
			}
			if (PathUtils::isRelative(dir))
			{
				PathName newdir;
				PathUtils::concatPath(newdir, root, dir);
				dir = newdir;
			}
			add(ParsedPath(dir));
			last = i + 1;
		}
	}
	PathName dir = "";
	if (i > last)
	{
		dir = val.substr(last, i - last);
		dir.trim();
	}
	if (PathUtils::isRelative(dir))
	{
		PathName newdir;
		PathUtils::concatPath(newdir, root, dir);
		dir = newdir;
	}
	add(ParsedPath(dir));
}

bool DirectoryList::isPathInList(const PathName& path) const
{
#ifdef BOOT_BUILD
	return true;
#else  //BOOT_BUILD
	fb_assert(mode != NotInitialized);

	// Handle special cases
	switch (mode)
	{
	case None:
		return false;
	case Full:
		return true;
	}

	// Disable any up-dir(..) references - in case our path_utils
	// and OS handle paths in slightly different ways,
	// this is "wonderful" potential hole for hacks
	// Example of IIS attack attempt:
	// "GET /scripts/..%252f../winnt/system32/cmd.exe?/c+dir HTTP/1.0"
	//								(live from apache access.log :)
	if (path.find(PathUtils::up_dir_link) != PathName::npos)
		return false;

	PathName varpath(path);
	if (PathUtils::isRelative(path)) {
		PathUtils::concatPath(varpath, PathName(Config::getRootDirectory()), path);
	}

	ParsedPath pPath(varpath);
	bool rc = false;
	for (size_t i = 0; i < getCount(); i++)
	{
		if ((*this)[i].contains(pPath))
		{
			rc = true;
			break;
		}
	}
	return rc;
#endif //BOOT_BUILD
}

bool DirectoryList::expandFileName(PathName& path, const PathName& name) const
{
	fb_assert(mode != NotInitialized);
	for (size_t i = 0; i < getCount(); i++)
	{
		PathUtils::concatPath(path, (*this)[i], name);
		if (PathUtils::canAccess(path, 4)) {
			return true;
		}
	}
	path = name;
	return false;
}

bool DirectoryList::defaultName(PathName& path, const PathName& name) const
{
	fb_assert(mode != NotInitialized);
	if (! getCount())
	{
		return false;
	}
	PathUtils::concatPath(path, (*this)[0], name);
	return true;
}

const PathName TempDirectoryList::getConfigString() const
{
	const char* value = Config::getTempDirectories();
	if (!value)
	{
		// Temporary directory configuration has not been defined.
		// Let's make default configuration.
		return TempFile::getTempPath();
	}
	return PathName(value);
}

} //namespace Firebird
