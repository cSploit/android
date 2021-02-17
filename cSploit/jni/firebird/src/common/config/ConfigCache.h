/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ConfigCache.h
 *	DESCRIPTION:	Base for class, representing cached config file.
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef COMMON_CONFIG_CASHE_H
#define COMMON_CONFIG_CASHE_H

#include "../common/classes/alloc.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/rwlock.h"

class ConfigCache : public Firebird::PermanentStorage
{
public:
	ConfigCache(Firebird::MemoryPool& p, const Firebird::PathName& fName);
	virtual ~ConfigCache();

	void checkLoadConfig();
	void addFile(const Firebird::PathName& fName);
	Firebird::PathName getFileName();

protected:
	virtual void loadConfig() = 0;

private:
	class File : public Firebird::PermanentStorage
	{
	public:
		File(Firebird::MemoryPool& p, const Firebird::PathName& fName);
		~File();

		bool checkLoadConfig(bool set);
		void add(const Firebird::PathName& fName);
		void trim();

	public:
		Firebird::PathName fileName;

	private:
		volatile time_t fileTime;
		File* next;
		time_t getTime();
	};
	File* files;

public:
	Firebird::RWLock rwLock;
};

#endif // COMMON_CONFIG_CASHE_H
