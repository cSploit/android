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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project, based on previous work done
 *  by Eugeney Putilin <evgeneyputilin at mail.ru>,
 *  Vlad Khorsun <hvlad at users.sourceforge.net> and
 *  Roman Rokytskyy <roman at rokytskyy.de>.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Eugeney Putilin <evgeneyputilin at mail.ru>
 *  Vlad Khorsun <hvlad at users.sourceforge.net>
 *  Roman Rokytskyy <roman at rokytskyy.de>
 */

#ifndef FIREBIRD_API_H
#define FIREBIRD_API_H

#define FB_NULL	0L


namespace Firebird {


typedef unsigned int uint;
typedef short int16;
typedef int int32;

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) && !defined(__GNUC__)
typedef __int64 int64;
#else
typedef long long int int64;
#endif

typedef char Utf8;		// Utf8* used as nul-terminated string

// ISC compatible handle
#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
typedef unsigned int Handle;
#else
typedef void* Handle;
#endif


}	// namespace Firebird


#endif	// FIREBIRD_API_H
