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
 *  The Original Code was created by Claudio Valderrama on 5-Oct-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


#ifndef FB_INPUT_DEVICES_H
#define FB_INPUT_DEVICES_H

#include "../common/classes/fb_string.h"
#include "../common/os/path_utils.h"
#include "../common/classes/array.h"

#include <stdio.h>

// This is basically a stack of input files caused by the INPUT command,
// because it can be invoked interactively and from a file. In turn, each file
// (SQL script) can contain another INPUT command and so on.
// Each "indev" represents an input device, but they are used by extension to
// represent one output file: the command history file, created automatically by isql.
// Do not confuse this "Ofp" with the user defined redirection of isql output to a file.
// The logic could be simpler but changing Borland code is tricky here.

class InputDevices
{
public:
	class indev
	{
	public:
		indev();
		indev(FILE* fp, const char* fn, const char* fn_display);
		void init(FILE* fp, const char* fn, const char* fn_display);
		void init(const indev& src);
		//~indev();
		void copy_from(const indev* src);
		Firebird::PathName fileName(bool display) const;
		void close();
		void drop();
		void getPos(fpos_t* out) const;
		void setPos(const fpos_t* in);
		FILE* indev_fpointer;
		int indev_line;
		int indev_aux;
		indev* indev_next;

	private:
		void makeFullFileName();

		Firebird::PathName indev_fn, indev_fn_display;

		void operator=(const void*); // prevent surprises.
	};

	InputDevices();
	explicit InputDevices(Firebird::MemoryPool&);
	~InputDevices();
	void clear(FILE* fpointer = 0);
	//const indev* getHead();
	indev& Ifp();
	indev& Ofp();
	bool insert(FILE* fp, const char* name, const char* display);
	bool remove();
	bool insertIfp();
	void removeIntoIfp();
	bool sameInputAndOutput() const;
	size_t count() const;
	void saveCommand(const char* statement, const char* term);
	bool readingStdin() const;
	void gotoEof();
	void commandsToFile(FILE* fpointer);

private:
	size_t m_count;
	indev* m_head;
	indev m_ifp;
	indev m_ofp;

	class Command : public Firebird::GlobalStorage
	{
	public:
		Command(const char* statement, const char* term);
		void toFile(FILE* fpointer);
	private:
		Firebird::string m_statement;
	};

	Firebird::HalfStaticArray<Command*, 32> commands;
};


inline Firebird::PathName InputDevices::indev::fileName(bool display) const
{
	return display ? indev_fn_display : indev_fn;
}

inline void InputDevices::indev::close()
{
	fclose(indev_fpointer);
	// Do not make the pointer NULL, as some comparisons happen after this.
}


inline InputDevices::InputDevices()
	: m_count(0), m_head(0), m_ifp(0, "", ""), m_ofp(0, "", ""), commands(*getDefaultMemoryPool())
{
}

inline InputDevices::InputDevices(Firebird::MemoryPool& p)
	: m_count(0), m_head(0), m_ifp(0, "", ""), m_ofp(0, "", ""), commands(p)
{
}

inline InputDevices::~InputDevices()
{
	clear();
}

//inline const InputDevices::indev* InputDevices::getHead()
//{
//	return m_head;
//}

inline InputDevices::indev& InputDevices::Ifp()
{
	return m_ifp;
}

inline InputDevices::indev& InputDevices::Ofp()
{
	return m_ofp;
}

inline size_t InputDevices::count() const
{
	return m_count;
}

#endif // FB_INPUT_DEVICES_H

