/*
 *	PROGRAM:		JRD FileSystem Path Handler
 *	MODULE:			path_utils.cpp
 *	DESCRIPTION:	POSIX_specific class for file path management
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
 *  The Original Code was created by John Bellardo
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 John Bellardo <bellardo at cs.ucsd.edu>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include "../jrd/os/path_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/// The POSIX implementation of the path_utils abstraction.

const char PathUtils::dir_sep = '/';
const char* PathUtils::up_dir_link = "..";

class PosixDirItr : public PathUtils::dir_iterator
{
public:
	PosixDirItr(MemoryPool& p, const Firebird::PathName& path)
		: dir_iterator(p, path), dir(0), file(p), done(false)
	{
		init();
	}
	PosixDirItr(const Firebird::PathName& path)
		: dir_iterator(path), dir(0), done(false)
	{
		init();
	}
	~PosixDirItr();
	const PosixDirItr& operator++();
	const Firebird::PathName& operator*() { return file; }
	operator bool() { return !done; }

private:
	DIR *dir;
	Firebird::PathName file;
	bool done;
	void init();
};

void PosixDirItr::init()
{
	dir = opendir(dirPrefix.c_str());
	if (!dir)
		done = true;
	else
		++(*this);
}

PosixDirItr::~PosixDirItr()
{
	if (dir)
		closedir(dir);
	dir = 0;
	done = true;
}

const PosixDirItr& PosixDirItr::operator++()
{
	if (done)
		return *this;
	struct dirent *ent = readdir(dir);
	if (ent == NULL)
	{
		done = true;
	}
	else
	{
		PathUtils::concatPath(file, dirPrefix, ent->d_name);
	}
	return *this;
}

PathUtils::dir_iterator *PathUtils::newDirItr(MemoryPool& p, const Firebird::PathName& path)
{
	return FB_NEW(p) PosixDirItr(p, path);
}

void PathUtils::splitLastComponent(Firebird::PathName& path, Firebird::PathName& file,
		const Firebird::PathName& orgPath)
{
	Firebird::PathName::size_type pos = orgPath.rfind(dir_sep);
	if (pos == Firebird::PathName::npos)
	{
		path = "";
		file = orgPath;
		return;
	}

	path.erase();
	path.append(orgPath, 0, pos);	// skip the directory separator
	file.erase();
	file.append(orgPath, pos + 1, orgPath.length() - pos - 1);
}

void PathUtils::concatPath(Firebird::PathName& result,
		const Firebird::PathName& first,
		const Firebird::PathName& second)
{
	if (second.length() == 0)
	{
		result = first;
		return;
	}
	if (first.length() == 0)
	{
		result = second;
		return;
	}

	if (first[first.length() - 1] != dir_sep &&
		second[0] != dir_sep)
	{
		result = first + dir_sep + second;
		return;
	}
	if (first[first.length() - 1] == dir_sep &&
		second[0] == dir_sep)
	{
		result = first;
		result.append(second, 1, second.length() - 1);
		return;
	}

	result = first + second;
}

// We don't work correctly with MBCS.
void PathUtils::ensureSeparator(Firebird::PathName& in_out)
{
	if (in_out.length() == 0)
		in_out = PathUtils::dir_sep;

	if (in_out[in_out.length() - 1] != PathUtils::dir_sep)
		in_out += PathUtils::dir_sep;
}

bool PathUtils::isRelative(const Firebird::PathName& path)
{
	if (path.length() > 0)
		return path[0] != dir_sep;
	return false;
}

bool PathUtils::isSymLink(const Firebird::PathName& path)
{
	struct stat st, lst;
	if (stat(path.c_str(), &st) != 0)
		return false;
	if (lstat(path.c_str(), &lst) != 0)
		return false;
	return st.st_ino != lst.st_ino;
}

bool PathUtils::canAccess(const Firebird::PathName& path, int mode)
{
	return access(path.c_str(), mode) == 0;
}

