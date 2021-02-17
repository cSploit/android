
#include "firebird.h"
#include "../common/os/path_utils.h"
#include <io.h> 		// _access
#include <direct.h>		// _mkdir

/// The Win32 implementation of the path_utils abstraction.

const char PathUtils::dir_sep = '\\';
const char* PathUtils::up_dir_link = "..";
const char PathUtils::dir_list_sep = ';';

class Win32DirItr : public PathUtils::dir_iterator
{
public:
	Win32DirItr(MemoryPool& p, const Firebird::PathName& path)
		: dir_iterator(p, path), dir(0), file(getPool()), done(false)
	{
		Win32DirInit();
	}
	Win32DirItr(const Firebird::PathName& path)
		: dir_iterator(path), dir(0), file(getPool()), done(false)
	{
		Win32DirInit();
	}
	~Win32DirItr();
	const PathUtils::dir_iterator& operator++();
	const Firebird::PathName& operator*() { return file; }
	operator bool() { return !done; }

private:
	HANDLE dir;
	WIN32_FIND_DATA fd;
	Firebird::PathName file;
	bool done;

	void Win32DirInit();
};

void Win32DirItr::Win32DirInit()
{
	Firebird::PathName dirPrefix2 = dirPrefix;

	if (dirPrefix.length() && dirPrefix[dirPrefix.length() - 1] != PathUtils::dir_sep)
		dirPrefix2 = dirPrefix2 + PathUtils::dir_sep;
	dirPrefix2 += "*.*";

	dir = FindFirstFile(dirPrefix2.c_str(), &fd);
	if (dir == INVALID_HANDLE_VALUE) {
		dir = 0;
		done = true;
	}
}

Win32DirItr::~Win32DirItr()
{
	if (dir)
		FindClose(dir);

	dir = 0;
	done = true;
}

const PathUtils::dir_iterator& Win32DirItr::operator++()
{
	if (done)
		return *this;

	if (!FindNextFile(dir, &fd))
		done = true;
	else
		PathUtils::concatPath(file, dirPrefix, fd.cFileName);

	return *this;
}

PathUtils::dir_iterator *PathUtils::newDirItr(MemoryPool& p, const Firebird::PathName& path)
{
	return FB_NEW(p) Win32DirItr(p, path);
}

void PathUtils::splitLastComponent(Firebird::PathName& path, Firebird::PathName& file,
		const Firebird::PathName& orgPath)
{
	Firebird::PathName::size_type pos = orgPath.rfind(PathUtils::dir_sep);
	if (pos == Firebird::PathName::npos)
	{
		pos = orgPath.rfind('/');	// temp hack to make it work with paths,
									// not expanded by ISC_expand_filename
		if (pos == Firebird::PathName::npos)
		{
			path = "";
			file = orgPath;
			return;
		}
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

	if (first[first.length() - 1] != PathUtils::dir_sep &&
		second[0] != PathUtils::dir_sep)
	{
		result = first + PathUtils::dir_sep + second;
		return;
	}
	if (first[first.length() - 1] == PathUtils::dir_sep &&
		second[0] == PathUtils::dir_sep)
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

static bool hasDriveLetter(const Firebird::PathName& path)
{
	return path.length() > 2 && path[1] == ':' &&
		(('A' <= path[0] && path[0] <= 'Z') ||
		 ('a' <= path[0] && path[0] <= 'z'));
}

bool PathUtils::isRelative(const Firebird::PathName& path)
{
	if (path.length() > 0)
	{
		const char ds = hasDriveLetter(path) ? path[2] : path[0];
		return ds != PathUtils::dir_sep && ds != '/';
	}
	return true;
}

void PathUtils::splitPrefix(Firebird::PathName& path, Firebird::PathName& prefix)
{
	prefix.erase();
	if (hasDriveLetter(path))
	{
		prefix = path.substr(0, 2);
		path.erase(0, 2);
	}
	if (path.hasData() && (path[0] == PathUtils::dir_sep || path[0] == '/'))
	{
		prefix += path[0];
		path.erase(0, 1);
	}
}

// This function can be made to return something util if we consider junctions (since w2k)
// and NTFS symbolic links (since WinVista).
bool PathUtils::isSymLink(const Firebird::PathName&)
{
	return false;
}

bool PathUtils::canAccess(const Firebird::PathName& path, int mode)
{
	return _access(path.c_str(), mode) == 0;
}

void PathUtils::setDirIterator(char* path)
{
	for (; *path; ++path)
	{
		if (*path == '/')
			*path = '\\';
	}
}

int PathUtils::makeDir(const Firebird::PathName& path)
{
	return _mkdir(path.c_str()) ? errno : 0;
}
