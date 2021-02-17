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

IF NOT EXIST ..\config.bat GOTO noconfig
call ..\config.bat

set FS_TYPE=%1
set RA_TYPE=%2

REM By default, return zero
set ERR=0

if "%RA_TYPE%"=="ra_local" goto ra_local
if "%RA_TYPE%"=="ra_svn"   goto ra_svn
if "%RA_TYPE%"=="ra_dav"   goto ra_dav

echo Unknown ra method '%RA_TYPE%'
EXIT 3

:ra_local
python win-tests.py %TEST_DIR% -f %FS_TYPE% -c -r 
if ERRORLEVEL 1 set ERR=1
EXIT %ERR%

:ra_svn
python win-tests.py %TEST_DIR% -f %FS_TYPE% -c -r -u svn://localhost
if ERRORLEVEL 1 set ERR=1
EXIT %ERR%

:ra_dav
python win-tests.py %TEST_DIR% -f %FS_TYPE% -c -r --httpd-dir="%HTTPD_BIN_DIR%" --httpd-port 1234
if ERRORLEVEL 1 set ERR=1
EXIT %ERR%

:noconfig
echo File config.bat not found. Please copy it from config.bat.tmpl and tweak for you.
EXIT 2
