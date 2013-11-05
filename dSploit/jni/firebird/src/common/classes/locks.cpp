/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			locks.cpp
 *	DESCRIPTION:	Win32 Mutex support compatible with
 *					old OS versions (like Windows 95)
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
 *  Copyright (c) 2003 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "../../include/firebird.h"
#include "../../jrd/common.h"

#if defined(WIN_NT)
// minimum win32 version: win98 / winnt4 SP3
#define _WIN32_WINNT 0x0403
#endif

#include "../../common/classes/locks.h"
#include "../../common/thd.h"


namespace Firebird {

#if defined(WIN_NT)

// Win9X support
#ifdef WIN9X_SUPPORT

// NS: This code is adapted from from KernelEx project, with the explicit
// permission from the author. KernelEx project aims to provide Windows XP
// compatibility layer for Windows 98 and Windows ME. For further information
// please refer to http://www.sourceforge.net/projects/kernelex/

static const K32OBJ_CRITICAL_SECTION = 4;
static const TDBX_WIN98 = 0x50;
static const TDBX_WINME = 0x80;

typedef struct _CRIT_SECT     // Size = 0x20
{
	BYTE      Type;           // 00 = 4: K32_OBJECT_CRITICAL_SECTION
	int       RecursionCount; // 04 initially 0, incremented on lock
	void*     OwningThread;   // 08 pointer to TDBX
	DWORD     un3;            // 0C
	int       LockCount;      // 10 initially 1, decremented on lock
	struct _CRIT_SECT* Next;  // 14
	void*     PLst;           // 18 list of processes using it?
	struct _WIN_CRITICAL_SECTION* UserCS; // 1C pointer to user defined CRITICAL_SECTION
} CRIT_SECT, *PCRIT_SECT;

typedef struct _WIN_CRITICAL_SECTION
{
	BYTE Type; //= 4: K32_OBJECT_CRITICAL_SECTION
	PCRIT_SECT crit;
	DWORD un1;
	DWORD un2;
	DWORD un3;
	DWORD un4;
} WIN_CRITICAL_SECTION, *PWIN_CRITICAL_SECTION;

static DWORD tdbx_offset;

__declspec(naked) BOOL WINAPI TryEnterCrst(CRIT_SECT* crit)
{
__asm {
	mov edx, [esp+4]
	xor eax, eax
	inc eax
	xor ecx, ecx
	cmpxchg [edx+10h], ecx ;if (OP1==eax) { OP1=OP2; ZF=1; } else { eax=OP1; ZF=0 }
	;mov ecx, ppTDBXCur
	mov ecx, fs:[18h]
	add ecx, [tdbx_offset]
	mov ecx, [ecx] ;ecx will contain TDBX now
	cmp eax, 1
	jnz L1
	;critical section was unowned => successful lock
	mov [edx+8], ecx
	inc dword ptr [edx+4]
	ret 4
L1:
	cmp [edx+8], ecx
	jnz L2
	;critical section owned by this thread
	dec dword ptr [edx+10h]
	inc dword ptr [edx+4]
	xor eax, eax
	inc eax
	ret 4
L2:
	;critical section owned by other thread - do nothing
	xor eax, eax
	ret 4
	}
}

BOOL WINAPI TryEnterCriticalSection_Win9X(CRITICAL_SECTION* cs)
{
	WIN_CRITICAL_SECTION* mycs = (WIN_CRITICAL_SECTION*) cs;
	if (mycs->Type != K32OBJ_CRITICAL_SECTION)
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, NULL);

	return TryEnterCrst(mycs->crit);
}

#endif

// On Win 98 and Win ME TryEnterCriticalSection is defined, but not implemented
// So direct linking to it won't hurt and will signal our incompatibility with Win 95
TryEnterCS::tTryEnterCriticalSection* TryEnterCS::m_funct = &TryEnterCriticalSection;

static TryEnterCS tryEnterCS;

TryEnterCS::TryEnterCS()
{
// Win9X support
#ifdef WIN9X_SUPPORT
	OSVERSIONINFO OsVersionInfo;

	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx((LPOSVERSIONINFO) &OsVersionInfo))
	{
		if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
			OsVersionInfo.dwMajorVersion == 4)
		{
			// Windows 98
			if (OsVersionInfo.dwMinorVersion == 10)
			{
				tdbx_offset = TDBX_WIN98;
				m_funct = TryEnterCriticalSection_Win9X;
			}

			// Windows ME
			if (OsVersionInfo.dwMinorVersion == 90)
			{
				tdbx_offset = TDBX_WINME;
				m_funct = TryEnterCriticalSection_Win9X;
			}
		}
	}
#endif
}

void Spinlock::init()
{
	SetCriticalSectionSpinCount(&spinlock, 4000);
}

#else //posix mutex

pthread_mutexattr_t Mutex::attr;

void Mutex::initMutexes()
{
	// Throw exceptions on errors, but they will not be processed in init
	// (first constructor). Better logging facilities are required here.
	int rc = pthread_mutexattr_init(&attr);
	if (rc < 0)
		system_call_failed::raise("pthread_mutexattr_init", rc);

	rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	if (rc < 0)
		system_call_failed::raise("pthread_mutexattr_settype", rc);
}

#endif

} // namespace Firebird
