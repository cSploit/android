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
set HTTPD_BIN_DIR=C:\Apache2
set GETTEXT_DIR=C:\svn-builder\djh-xp-vse2005\gettext
set TEST_DIR=M:\svn-auto-test

set HTTPD_SRC_DIR=..\httpd
set BDB_DIR=..\db4-win32
set NEON_DIR=..\neon
set ZLIB_DIR=..\zlib
set OPENSSL_DIR=..\openssl
set INTL_DIR=..\svn-libintl

REM Uncomment this if you want clean subversion build, after testing
REM set CLEAN_SVN=1

REM Uncomment this if you want disable ra_svn tests
REM set NO_RA_SVN=1

REM Uncomment this if you want disable ra_dav tests
REM set NO_RA_HTTP=1

set PATH=%GETTEXT_DIR%\bin;%PATH%
call C:\VCX2005\VC\vcvarsall.bat x86
