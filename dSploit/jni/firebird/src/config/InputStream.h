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

// InputStream.h: interface for the InputStream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INPUTSTREAM_H__63114A0E_18FF_41F4_85A4_CCB8711487C7__INCLUDED_)
#define AFX_INPUTSTREAM_H__63114A0E_18FF_41F4_85A4_CCB8711487C7__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes/alloc.h"

START_NAMESPACE

class InputFile;

class InputStream : public Firebird::GlobalStorage
{
public:
	explicit InputStream (const char* stuff);
	InputStream();
	virtual ~InputStream();

	virtual InputFile* getInputFile();
	virtual const char* getFileName() const;
	void init();
	void release();
	virtual void addRef();

	virtual void close();
	virtual const char* getEnd();
	virtual int getOffset (const char* ptr);
	virtual const char* getSegment();

	int				lineNumber;
	const char*		segment;
	const char*		ptr;
	InputStream*	prior;
protected:
	int				segmentLength; // used by InputFile
private:
	int				segmentOffset;
	int				useCount;
};

END_NAMESPACE

#endif // !defined(AFX_INPUTSTREAM_H__63114A0E_18FF_41F4_85A4_CCB8711487C7__INCLUDED_)
