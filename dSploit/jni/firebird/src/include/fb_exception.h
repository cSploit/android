/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			fb_exception.h
 *	DESCRIPTION:	Firebird's exception classes
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
 *  The Original Code was created by Mike Nordell
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2001 Mike Nordell <tamlin at algonet.se>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */


#ifndef FB_EXCEPTION_H
#define FB_EXCEPTION_H

#include <stddef.h>
#include <string.h>
#include <new>
#include "fb_types.h"
#include "../common/StatusArg.h"
#include "../common/thd.h"
#include "../jrd/common.h"

namespace Firebird {

class MemoryPool;

class Exception
{
protected:
	Exception() throw() { }
public:
	virtual ~Exception() throw();
	virtual ISC_STATUS stuff_exception(ISC_STATUS* const status_vector) const throw() = 0;
	virtual const char* what() const throw() = 0;
};

// Used as jmpbuf to unwind when needed
class LongJump : public Exception
{
public:
	virtual ISC_STATUS stuff_exception(ISC_STATUS* const status_vector) const throw();
	virtual const char* what() const throw();
	static void raise();
	LongJump() throw() : Exception() { }
};

// Used in MemoryPool
class BadAlloc : public std::bad_alloc, public Exception
{
public:
	virtual ISC_STATUS stuff_exception(ISC_STATUS* const status_vector) const throw();
	virtual const char* what() const throw();
	static void raise();
	BadAlloc() throw() : std::bad_alloc(), Exception() { }
};

// Main exception class in firebird
class status_exception : public Exception
{
public:
	status_exception(const ISC_STATUS *status_vector) throw();
	virtual ~status_exception() throw();

	virtual ISC_STATUS stuff_exception(ISC_STATUS* const status_vector) const throw();
	virtual const char* what() const throw();

	const ISC_STATUS* value() const throw() { return m_status_vector; }

	static void raise(const ISC_STATUS *status_vector);
	static void raise(const Arg::StatusVector& statusVector);

protected:
	// Create exception with undefined status vector, this constructor allows
	// derived classes create empty exception ...
	status_exception() throw();
	// .. and adjust it later using somehow created status vector.
	void set_status(const ISC_STATUS *new_vector) throw();

private:
	ISC_STATUS_ARRAY m_status_vector;
};

// Parameter syscall later in both system_error & system_call_failed
// must be literal string! This helps avoid need in StringsBuffer
// when processing this dangerous errors!

// use this class if exception can be handled
class system_error : public status_exception
{
private:
	int errorCode;
public:
	system_error(const char* syscall, int error_code);

	static void raise(const char* syscall, int error_code);
	static void raise(const char* syscall);

	int getErrorCode() const
	{
		return errorCode;
	}

	static int getSystemError();
};

// use this class if exception can't be handled
// it will call abort() in DEV_BUILD to create core dump
class system_call_failed : public system_error
{
public:
	system_call_failed(const char* syscall, int error_code);

	static void raise(const char* syscall, int error_code);
	static void raise(const char* syscall);
};

class fatal_exception : public status_exception
{
public:
	explicit fatal_exception(const char* message);
	static void raiseFmt(const char* format, ...);
	const char* what() const throw();
	static void raise(const char* message);
};


// Serialize exception into status_vector
ISC_STATUS stuff_exception(ISC_STATUS *status_vector, const Firebird::Exception& ex) throw();

// Put status vector strings into strings buffer
void makePermanentVector(ISC_STATUS* perm, const ISC_STATUS* trans, FB_THREAD_ID thr = getThreadId()) throw();
void makePermanentVector(ISC_STATUS* v, FB_THREAD_ID thr = getThreadId()) throw();

// Catch synchronous exceptions in UNIX
#ifdef UNIX
void sync_signals_set(void*);
void sync_signals_reset();
#endif

}	// namespace Firebird


#endif	// FB_EXCEPTION_H
