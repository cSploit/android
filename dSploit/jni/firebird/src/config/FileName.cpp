#include "firebird.h"
#include "FileName.h"

#ifdef WIN_NT
#define IS_SLASH(c)	(c == '/' || c == '\\')
#else
#define IS_SLASH(c)	(c == '/')
#endif

FileName::FileName(const Firebird::PathName& name) :
	pathName(getPool()), directory(getPool()), root(getPool()), extension(getPool())
{
	pathName = name;
	const char* const start = pathName.c_str();
	const char* slash = NULL;
	const char* dot = NULL;
	const char* rootName = start;
	absolute = IS_SLASH (start [0]);

	for (const char* p = start; *p; ++p)
	{
		if (!dot && IS_SLASH (*p))
			slash = p;
		else if (*p == '.')
			dot = p;
	}

	if (slash)
	{
		directory.assign (start, (int) (slash - rootName));
		rootName = slash + 1;
	}

	if (dot)
	{
		extension = dot + 1;
		root.assign (rootName, (int) (dot - rootName));
	}
	else
		root = rootName;
}

FileName::~FileName()
{
}
