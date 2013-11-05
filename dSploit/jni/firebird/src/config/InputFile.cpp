/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// InputFile.cpp: implementation of the InputFile class.
//
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <stdio.h>
#include <string.h>
#include "firebird.h"
#include "../common/classes/alloc.h"
#include "InputFile.h"
#include "AdminException.h"

#define ISLOWER(c)			(c >= 'a' && c <= 'z')
//#define ISUPPER(c)			(c >= 'A' && c <= 'Z')
//#define ISDIGIT(c)			(c >= '0' && c <= '9')

#ifndef UPPER
#define UPPER(c)			((ISLOWER (c)) ? c - 'a' + 'A' : c)
#endif

static const int BACKUP_FILES = 5;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InputFile::InputFile(const char *name) : fileName(getPool())
{
	changes = NULL;
	file = NULL;

	if (!openInputFile (name))
		throw AdminException ("can't open file \"%s\"", name);
}

InputFile::InputFile() : fileName(getPool())
{
	changes = NULL;
	file = NULL;
}

InputFile::~InputFile()
{
	close();

	for (FileChange *change; change = changes;)
	{
		changes = change->next;
		delete change;
	}
}

void InputFile::close()
{
	if (file)
	{
		fclose ((FILE*) file);
		file = NULL;
	}
}

const char* InputFile::getSegment()
{
	if (!file)
		throw AdminException ("file has been closed");

	if (!fgets (buffer, sizeof (buffer), (FILE*) file))
		return NULL;

	//++lineNumber;
	segmentLength = (int) strlen (buffer);

	return buffer;
}

const char* InputFile::getFileName() const
{
	return fileName.c_str();
}

InputFile* InputFile::getInputFile()
{
	return this;
}

void InputFile::postChange(int line, int skip, const Firebird::string& insertion)
{
	FileChange *change = new FileChange;
	change->lineNumber = line;
	change->linesSkipped = skip;
	change->insertion = insertion;

	for (FileChange **p = &changes;; p = &(*p)->next)
	{
		FileChange *next = *p;
		if (!next || next->lineNumber > change->lineNumber)
		{
			change->next = *p;
			*p = change;
			break;
		}
	}
}

/* Commented out by Alex as unused and problematic.
void InputFile::rewrite()
{
	FILE *input = fopen (fileName.c_str(), "r");

	if (!input)
		throw AdminException ("can't open \"%s\" for input", fileName.c_str());

	Firebird::PathName tempName;
	tempName.printf("%.*s.tmp", MAXPATHLEN - 5, fileName.c_str());
	FILE *output = fopen (tempName.c_str(), "w");

	if (!output)
		throw AdminException ("can't open \"%s\" for output", tempName.c_str());

	char temp [1024];
	int line = 0;

	for (FileChange *change = changes; change; change = change->next)
	{
		for (; line < change->lineNumber; ++line)
		{
			if (fgets (temp, sizeof (temp), input))
				fputs (temp, output);
		}
		//fputs ("#insertion starts here\n", output);
		fputs (change->insertion.c_str(), output);
		//fputs ("#insertion end here\n", output);
		for (int n = 0; n < change->linesSkipped; ++n)
			fgets (temp, sizeof (temp), input);
		line += change->linesSkipped;
	}

	while (fgets (temp, sizeof (temp), input))
		fputs (temp, output);

	fclose (input);
	fclose (output);
	Firebird::PathName filename1, filename2;

	fb_assert(BACKUP_FILES < 10); // assumption to avoid too long filenames

	for (int n = BACKUP_FILES; n >= 0; --n)
	{
		filename1.printf("%.*s.%d", MAXPATHLEN - 3, fileName.c_str(), n);
		if (n)
			filename2.printf("%.*s.%d", MAXPATHLEN - 3, fileName.c_str(), n - 1);
		else
			filename2 = fileName;	// limited to correct length in openInputFile
		if (n == BACKUP_FILES)
			unlink (filename1.c_str());
		rename (filename2.c_str(), filename1.c_str());
	}

	if (rename (tempName.c_str(), fileName.c_str()))
		perror ("rename");
}
*/

bool InputFile::pathEqual(const char *path1, const char *path2)
{
#ifdef _WIN32
	for (; *path1 && *path2; ++path1, ++path2)
	{
		if (*path1 != *path2 &&
		    UPPER (*path1) != UPPER (*path2) &&
			!((*path1 == '/' || *path1 == '\\') &&
			  (*path2 == '/' || *path2 == '\\')))
		{
			return false;
		}
	}
#else
	for (; *path1 && *path2; ++path1, ++path2)
	{
		if (*path1 != *path2)
			return false;
	}
#endif

	return *path1 == 0 && *path2 == 0;
}

bool InputFile::openInputFile(const char* name)
{
	// It's good idea to keep file names limited to meet OS requirements
	if (!name || strlen(name) >= MAXPATHLEN)
		return false;

	if (!(file = fopen (name, "r")))
		return false;

	fileName = name;
	segment = buffer;
	changes = NULL;

	return true;
}
