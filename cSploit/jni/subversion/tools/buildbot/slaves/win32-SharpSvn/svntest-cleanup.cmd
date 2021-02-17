@echo off
REM ================================================================
REM   Licensed to the Apache Software Foundation (ASF) under one
REM   or more contributor license agreements.  See the NOTICE file
REM   distributed with this work for additional information
REM   regarding copyright ownership.  The ASF licenses this file
REM   to you under the Apache License, Version 2.0 (the
REM   "License"); you may not use this file except in compliance
REM   with the License.  You may obtain a copy of the License at
REM  
REM     http://www.apache.org/licenses/LICENSE-2.0
REM  
REM   Unless required by applicable law or agreed to in writing,
REM   software distributed under the License is distributed on an
REM   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
REM   KIND, either express or implied.  See the License for the
REM   specific language governing permissions and limitations
REM   under the License.
REM ================================================================

SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

CALL ..\svn-config.cmd

IF NOT EXIST "..\deps\" MKDIR "..\deps"

PUSHD ..\deps
ECHO Checking dependencies in %CD%

IF NOT EXIST "imports\" (
  svn co --username guest --password "" http://sharpsvn.open.collab.net/svn/sharpsvn/trunk/imports imports
)
IF NOT EXIST build\imports.done (
  copy /y imports\dev-default.build default.build
  nant build %NANTARGS%
  IF ERRORLEVEL 1 (
    exit /B 1
  )
  del release\bin\*svn* release\bin\_*.*
  echo. > build\imports.done
)

POPD

PUSHD "%TEMP%"
IF NOT ERRORLEVEL 1 (
  rmdir /s /q "%TEMP%" 2> nul:
)
POPD

taskkill /im svn.exe /f 2> nul:
taskkill /im svnadmin.exe /f 2> nul:
taskkill /im svnserve.exe /f 2> nul:
taskkill /im svnrdump.exe /f 2> nul:
taskkill /im svnsync.exe /f 2> nul:
taskkill /im httpd.exe /f 2> nul:
IF EXIST "%TESTDIR%\tests\subversion\tests\cmdline\httpd\" (
  rmdir /s /q  "%TESTDIR%\tests\subversion\tests\cmdline\httpd"
)

del "%TESTDIR%\tests\*.log" 2> nul:


exit /B 0
