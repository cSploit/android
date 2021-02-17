@echo off

:	PROGRAM:	Class library integrity tests
:	MODULE:		string_test.cmd
:	DESCRIPTION:	test class Firebird::string
:
:  The contents of this file are subject to the Initial
:  Developer's Public License Version 1.0 (the "License");
:  you may not use this file except in compliance with the
:  License. You may obtain a copy of the License at
:  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
:
:  Software distributed under the License is distributed AS IS,
:  WITHOUT WARRANTY OF ANY KIND, either express or implied.
:  See the License for the specific language governing rights
:  and limitations under the License.
:
:  The Original Code was created by Alexander Peshkoff
:  for the Firebird Open Source RDBMS project.
:
:  Copyright (c) 2004 Alexander Peshkoff <peshkoff@mail.ru>
:  and all contributors signed below.
:
:  All Rights Reserved.
:  Contributor(s): ______________________________________.

: preprocessor defines
: DEV_BUILD makes single iteration, validating all string results.
:   When not defined, makes 100000 iterations (a few seconds runtime at P-4),
:   and reports time of that test.
: FIRESTR - use Firebird::string, if not defined - STL basic_string is used.
:   In the latter case some checks are not performed, because not supported by STL
:   or give AV with it (at least MS VC6 implementation).
: Without DEV_BUILD this checks are also not performed to give compareable results
:   for both string classes.

set cpp_files=string_test.cpp fb_string.cpp alloc.cpp locks.cpp ..\fb_exception.cpp
set inc_dirs=-I ..\..\include
set lib_files=user32.lib
set filesNdirs=%cpp_files% %inc_dirs% %lib_files%
set cc=cl -GR -GX

: This line tests our test using std::basic_string
: set flags=-DDEV_BUILD

: This line tests correctness of Firebird::string
set flags=-DFIRESTR -DDEV_BUILD

: This line tests speed of Firebird::string
: set flags=-Ox -DFIRESTR

: This line tests speed of std::basic_string for comparison
: set flags=-Ox

del /Q string_test.exe
%cc% %flags% %filesNdirs%
if errorlevel 1 exit
del /Q *.obj

string_test.exe
