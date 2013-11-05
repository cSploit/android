/*
 *      PROGRAM:        Firebird Windows support
 *      MODULE:         dllinst.cpp
 *      DESCRIPTION:    DLL instance handle
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#if !defined(FB_DLL_INST) && defined(WIN_NT)

#define FB_DLL_INST

#include <firebird.h>

#include <windows.h>

namespace Firebird {

extern HINSTANCE hDllInst;
extern bool bEmbedded;

} // namespace

#endif // !FB_DLL_INST && WIN_NT
