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
IF ERRORLEVEL 1 EXIT /B 1

svnversion . /1.6.x | find "S" > nul:
IF ERRORLEVEL 1 (
  ECHO --- Building 1.6.x: Skipping bindings ---
  EXIT /B 0
)

PATH %PATH%;%TESTDIR%\bin
SET result=0


echo python win-tests.py -r -f fsfs --javahl "%TESTDIR%\tests"
python win-tests.py -r -f fsfs --javahl "%TESTDIR%\tests"
IF ERRORLEVEL 1 (
  echo [python reported error %ERRORLEVEL%]
  SET result=1
)

IF EXIST "%TESTDIR%\swig" rmdir /s /q "%TESTDIR%\swig"
mkdir "%TESTDIR%\swig\py-release\libsvn"
mkdir "%TESTDIR%\swig\py-release\svn"

xcopy "release\subversion\bindings\swig\python\*.pyd" "%TESTDIR%\swig\py-release\libsvn\*.pyd"
xcopy "release\subversion\bindings\swig\python\libsvn_swig_py\*.dll" "%TESTDIR%\swig\py-release\libsvn\*.dll"
xcopy "subversion\bindings\swig\python\*.py" "%TESTDIR%\swig\py-release\libsvn\*.py"
xcopy "subversion\bindings\swig\python\svn\*.py" "%TESTDIR%\swig\py-release\svn\*.py"

SET PYTHONPATH=%TESTDIR%\swig\py-release

python subversion\bindings\swig\python\tests\run_all.py
IF ERRORLEVEL 1 (
  echo [Python reported error %ERRORLEVEL%]
  SET result=1
)

exit /b %result%
