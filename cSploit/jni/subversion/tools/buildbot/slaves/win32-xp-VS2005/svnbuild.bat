REM     Licensed to the Apache Software Foundation (ASF) under one
REM     or more contributor license agreements.  See the NOTICE file
REM     distributed with this work for additional information
REM     regarding copyright ownership.  The ASF licenses this file
REM     to you under the Apache License, Version 2.0 (the
REM     "License"); you may not use this file except in compliance
REM     with the License.  You may obtain a copy of the License at
REM    
REM       http://www.apache.org/licenses/LICENSE-2.0
REM    
REM     Unless required by applicable law or agreed to in writing,
REM     software distributed under the License is distributed on an
REM     "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
REM     KIND, either express or implied.  See the License for the
REM     specific language governing permissions and limitations
REM     under the License.

@echo off
IF NOT EXIST ..\config.bat GOTO noconfig
call ..\config.bat

cmd.exe /c call ..\svnclean.bat

set PARAMS=-t vcproj --vsnet-version=2005 --with-berkeley-db=%BDB_DIR% --with-zlib=%ZLIB_DIR% --with-httpd=%HTTPD_SRC_DIR% --with-neon=%NEON_DIR% --with-libintl=%INTL_DIR%
REM set PARAMS=-t vcproj --vsnet-version=2005 --with-berkeley-db=%BDB_DIR% --with-zlib=%ZLIB_DIR% --with-httpd=%HTTPD_SRC_DIR% --with-neon=%NEON_DIR%
IF NOT "%OPENSSL_DIR%"=="" set PARAMS=%PARAMS% --with-openssl=%OPENSSL_DIR%

python gen-make.py %PARAMS%
IF ERRORLEVEL 1 GOTO ERROR

REM MSDEV.COM %HTTPD_SRC_DIR%\apache.dsw /MAKE "BuildBin - Win32 Release"
REM IF ERRORLEVEL 1 GOTO ERROR

rem MSBUILD subversion_vcnet.sln /t:__ALL_TESTS__ /p:Configuration=Debug
MSBUILD subversion_vcnet.sln /t:__ALL_TESTS__ /p:Configuration=Release
IF ERRORLEVEL 1 GOTO ERROR
MSBUILD subversion_vcnet.sln /t:__SWIG_PYTHON__ /p:Configuration=Release
IF ERRORLEVEL 1 GOTO ERROR
MSBUILD subversion_vcnet.sln /t:__SWIG_PERL__ /p:Configuration=Release
IF ERRORLEVEL 1 GOTO ERROR
MSBUILD subversion_vcnet.sln /t:__JAVAHL__ /p:Configuration=Release
IF ERRORLEVEL 1 GOTO ERROR

EXIT 0

REM ----------------------------------------------------
:ERROR
ECHO.
ECHO *** Whoops, something choked.
ECHO.
CD ..
EXIT 1

:noconfig
echo File config.bat not found. Please copy it from config.bat.tmpl and tweak for you.
EXIT 2
