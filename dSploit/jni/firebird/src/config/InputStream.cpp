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

// InputStream.cpp: implementation of the InputStream class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include "firebird.h"
#include "../jrd/common.h"
#include "InputStream.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InputStream::InputStream()
{
	init();
	segmentLength = 0;
}

InputStream::InputStream(const char* stuff)
{
	init();
	segment = stuff;
	segmentLength = (int) strlen (segment);
}

InputStream::~InputStream()
{
}

void InputStream::init()
{
	prior = NULL;
	ptr = segment;
	useCount = 1;
	lineNumber = 0;
	segment = NULL;
	segmentOffset = 0;
}

const char* InputStream::getSegment()
{
	if (segmentOffset)
		return NULL;

	segmentOffset = 1;

	return segment;
}

int InputStream::getOffset(const char* pointer)
{
	return (int) (segmentOffset + pointer - segment);
}

const char* InputStream::getEnd()
{
	return segment + segmentLength;
}

void InputStream::close()
{

}


void InputStream::addRef()
{
	++useCount;
}

void InputStream::release()
{
	if (--useCount == 0)
		delete this;
}

const char* InputStream::getFileName() const
{
	return "";
}

InputFile* InputStream::getInputFile()
{
	return NULL;
}
