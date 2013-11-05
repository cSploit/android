#ifndef _FILENAME_H_
#define _FILENAME_H_

#include "../common/classes/fb_string.h"

class FileName : public Firebird::GlobalStorage
{
public:
	explicit FileName(const Firebird::PathName& name);
	~FileName();

	Firebird::PathName	pathName;
	Firebird::PathName	directory;
	Firebird::PathName	root;
	Firebird::PathName	extension;
	bool        isAbsolute() const { return absolute; }
private:
	bool		absolute;
};

#endif

