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

#include "firebird.h"
#ifdef DARWIN
#if defined(i386) || defined(__x86_64__)
#include <architecture/i386/io.h>
#else
#include <io.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "../common/utils_proto.h"
#include "InputDevices.h"

using Firebird::PathName;


InputDevices::indev::indev()
	: indev_fpointer(0), indev_line(0), indev_aux(0), indev_next(0),
	  indev_fn(*getDefaultMemoryPool()), indev_fn_display(*getDefaultMemoryPool())
{
	makeFullFileName();
}

InputDevices::indev::indev(FILE* fp, const char* fn, const char* fn_display)
	: indev_fpointer(fp), indev_line(0), indev_aux(0), indev_next(0),
	  indev_fn(*getDefaultMemoryPool()), indev_fn_display(*getDefaultMemoryPool())
{
	indev_fn = fn;
	indev_fn_display = fn_display;
	makeFullFileName();
}

// Performs the same task that one of the constructors, but called manually.
// File handle and file name are assumed to be valid and the rest of the data
// members aren't copied but reset.
void InputDevices::indev::init(FILE* fp, const char* fn, const char* fn_display)
{
	indev_fpointer = fp;
	indev_line = 0;
	indev_aux = 0;
	indev_fn = fn;
	indev_fn_display = fn_display;
	indev_next = 0;

	makeFullFileName();
}

// Copies only the file handle and file name from one indev to another.
// File handle and file name are assumed to be valid.
void InputDevices::indev::init(const indev& src)
{
	indev_fpointer = src.indev_fpointer;
	indev_line = 0;
	indev_aux = 0;
	indev_fn = src.indev_fn;
	indev_fn_display = src.indev_fn_display;
	indev_next = 0;
}

//InputDevices::indev::~indev()
//{
	// Nothing for now. Let the owner or caller close the file pointer if needed.
//}

// Initializes one indev with another. Useful for method of the owning class
// that needs to copy from the input indev to the top of the stack and vice-versa.
void InputDevices::indev::copy_from(const indev* src)
{
	fb_assert(src);
	indev_fpointer = src->indev_fpointer;
	indev_line = src->indev_line;
	indev_aux = src->indev_aux;
	indev_fn = src->indev_fn;
	indev_fn_display = src->indev_fn_display;
	// indev_next not copied.
}

// Drop a file associated with an indev.
void InputDevices::indev::drop()
{
	fb_assert(indev_fpointer != stdin);
	fb_assert(!indev_fn.isEmpty()); // Some name should exist.
	fclose(indev_fpointer);
	unlink(indev_fn.c_str());
}

// Save the reading position in the parameter.
void InputDevices::indev::getPos(fpos_t* out) const
{
	fb_assert(out);
	fb_assert(indev_fpointer);
	fgetpos(indev_fpointer, out);
}

// Restore a previously stored reading position held in the parameter.
void InputDevices::indev::setPos(const fpos_t* in)
{
	fb_assert(in);
	fb_assert(indev_fpointer);
#ifdef SFIO
// hack to fix bad sfio header
	fsetpos(indev_fpointer, const_cast<fpos_t*>(in));
#else
	fsetpos(indev_fpointer, in);
#endif
}

void InputDevices::indev::makeFullFileName()
{
	if (!indev_fn.isEmpty() && PathUtils::isRelative(indev_fn))
	{
		PathName name = indev_fn;
		PathName path;
		fb_utils::getCwd(path);
		PathUtils::concatPath(indev_fn, path, name);
	}
}


// Clear the chain of indev, closing file handles.
void InputDevices::clear(FILE* fpointer)
{
	while (m_head)
	{
		FILE* const p = m_head->indev_fpointer;
		if (fpointer && p == fpointer)
			break;

		if (p != stdin && p != m_ofp.indev_fpointer)
			fclose(p);
		indev* const flist = m_head->indev_next;
		delete m_head;
		m_head = flist;
		--m_count;
	}
	if (m_ifp.indev_fpointer && // In case we called clear(NULL) manually before.
		m_ifp.indev_fpointer != stdin && m_ifp.indev_fpointer != m_ofp.indev_fpointer)
	{
		fclose(m_ifp.indev_fpointer);
		m_ifp.indev_fpointer = 0;
	}

	fb_assert(m_count == 0 || fpointer != 0);
	if (!fpointer)
	{
		m_head = 0;
		m_count = 0;
	}
}

// Insert an indev in the chain, always in LIFO way.
bool InputDevices::insert(FILE* fp, const char* name, const char* display)
{
	if (!m_head)
	{
		fb_assert(m_count == 0);
		m_head = new indev(fp, name, display);
	}
	else
	{
		fb_assert(m_count > 0);
		indev* p = m_head;
		m_head = new indev(fp, name, display);
		m_head->indev_next = p;
	}
	++m_count;
	return true;
}

// Shortcut for inserting the currently input file in the indev chain.
bool InputDevices::insertIfp()
{
	if (insert(NULL, "", ""))
	{
		m_head->copy_from(&m_ifp);
		return true;
	}
	return false;
}

// Remove the top (last inserted) indev in the chain.
bool InputDevices::remove()
{
	if (m_head)
	{
		fb_assert(m_count > 0);
		indev* p = m_head;
		m_head = m_head->indev_next;
		delete p;
		--m_count;
		return true;
	}
	return false;
}

// Shortcut for moving the top indev in the chain to the current input file.
void InputDevices::removeIntoIfp()
{
	fb_assert(m_head && m_count > 0);
	m_ifp.copy_from(m_head);
	// When we come back from inout(), we continue inside the get_statement loop.
	// If we are inside do_isql() due to Ctrl-C, it doesn't cause any harm.
	m_ifp.indev_line = m_ifp.indev_aux;
	remove();
}

// Shortcut for testing whether current input and output handles are the same.
// This may happen if we are reading from the command history file that's filled
// automatically with every interactive SQL command (isql commands aren't logged).
bool InputDevices::sameInputAndOutput() const
{
	fb_assert(m_ifp.indev_fpointer);
	fb_assert(m_ofp.indev_fpointer);
	return m_ifp.indev_fpointer == m_ofp.indev_fpointer;
}

// Save SQL command (not isql's own commands) to a history file only if we
// are in interactive mode.
void InputDevices::saveCommand(const char* statement, const char* term)
{
	if (m_ifp.indev_fpointer == stdin)
	{
		FILE* f = m_ofp.indev_fpointer;
		if (f)
		{
			fputs(statement, f);
			fputs(term, f);
			// Add newline to make the file more readable.
			fputc('\n', f);
		}
		else
		{
			Command* command = new Command(statement, term);
			commands.add(command);
		}
	}
}

// Are we reading from stdin now or from a file?
bool InputDevices::readingStdin() const
{
	fb_assert(m_ifp.indev_fpointer);
	return m_ifp.indev_fpointer == stdin;
}

// Go to end of file if we are reading from a file (not stdin).
void InputDevices::gotoEof()
{
	fb_assert(m_ifp.indev_fpointer);
	if (m_ifp.indev_fpointer != stdin)
		fseek(m_ifp.indev_fpointer, 0, SEEK_END);
}

InputDevices::Command::Command(const char* statement, const char* term)
	: m_statement(getPool())
{
	m_statement = statement;
	m_statement += term;
}

void InputDevices::Command::toFile(FILE* f)
{
	fputs(m_statement.c_str(), f);
	// Add newline to make the file more readable.
	fputc('\n', f);
}

void InputDevices::commandsToFile(FILE* f)
{
	for (unsigned n = 0; n < commands.getCount(); ++n)
	{
		commands[n]->toFile(f);
		delete commands[n];
	}

	commands.clear();
}
