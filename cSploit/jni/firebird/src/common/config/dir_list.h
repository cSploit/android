/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		dir_list.h
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

#ifndef CONFIG_DIR_LIST_H
#define CONFIG_DIR_LIST_H

#include "fb_types.h"
#include "../common/classes/fb_string.h"
#include "../common/config/config_file.h"

namespace Firebird {

//
// This class is used internally by DirectoryList
// to store single path in format of pre-parsed
// elements of that path & perform basic operations
// with this path representation.
// Because of it's internal nature it has only
// internally required subset of possible operators.
//
class ParsedPath : public ObjectsArray<PathName>
{
	typedef ObjectsArray<PathName> inherited;
public:
	explicit ParsedPath(MemoryPool& p) : ObjectsArray<PathName>(p) { }
	ParsedPath(MemoryPool& p, const PathName& path)
		: ObjectsArray<PathName>(p)
	{
		parse(path);
	}
	ParsedPath() : ObjectsArray<PathName>() { }
	explicit ParsedPath(const PathName& path)
		: ObjectsArray<PathName>()
	{
		parse(path);
	}
	// Take new path inside
	void parse(const PathName& path);
	// Convert internal representation to traditional one
	operator PathName() const;
	// Compare with path given by constant
	bool operator==(const char* path) const
	{
		return PathName(*this) == path;
	}
	// Check, whether pPath lies inside directory tree,
	// specified by *this ParsedPath. Also checks against
	// possible symbolic links.
	bool contains(const ParsedPath& pPath) const;
	// Returns path, containing elements from 0 to n-1
	PathName subPath(size_t n) const;
};


class DirectoryList : public ObjectsArray<ParsedPath>
{
private:
	typedef ObjectsArray<ParsedPath> inherited;
	enum ListMode {NotInitialized = -1, None = 0, Restrict = 1, Full = 2, SimpleList = 3};
	ListMode mode;
	// Check, whether Value begins with Key,
	// followed by any character from Next.
	// If Next is empty, Value shoult exactly match Key.
	// If Key found, sets Mode to KeyMode and returns true.
	bool keyword(const ListMode keyMode, PathName& value, PathName key, PathName next);
protected:
	// Clear allocated memory and reinitialize
	void clear()
	{
		((inherited*) this)->clear();
		mode = NotInitialized;
	}
	// Used for various configuration parameters -
	// returns parameter PathName from Config Manager.
	virtual const PathName getConfigString() const = 0;
	// Initialize loads data from Config Manager.
	// With simple mutex add-on may be easily used to
	// load them dynamically. Now called locally
	// when IsPathInList() invoked first time.
	void initialize(bool simple_mode = false);
public:
	explicit DirectoryList(MemoryPool& p)
		: ObjectsArray<ParsedPath>(p), mode(NotInitialized)
	{ }

	DirectoryList()
		: ObjectsArray<ParsedPath>(), mode(NotInitialized)
	{ }

	virtual ~DirectoryList() {clear();}

	// Check, whether Path is inside this DirectoryList
	bool isPathInList(const PathName& path) const;

	// Search for file Name in all directories of DirectoryList.
	// If found, return full path to it in Path.
	// Otherwise Path = Name.
	bool expandFileName(PathName& path, const PathName& name) const;

	// Use first directory in this directory list
	// to build default full name for a file
	bool defaultName(PathName& path, const PathName& name) const;
};

class TempDirectoryList : public DirectoryList {
public:
	explicit TempDirectoryList(MemoryPool& p)
		: DirectoryList(p)
	{
		initialize(true);
	}
	TempDirectoryList()
	{
		initialize(true);
	}

private:
	// implementation of the inherited function
	const PathName getConfigString() const;
};

} //namespace Firebird

#endif //	CONFIG_DIR_LIST_H
