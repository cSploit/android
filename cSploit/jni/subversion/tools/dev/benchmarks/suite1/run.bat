:: Licensed to the Apache Software Foundation (ASF) under one
:: or more contributor license agreements.  See the NOTICE file
:: distributed with this work for additional information
:: regarding copyright ownership.  The ASF licenses this file
:: to you under the Apache License, Version 2.0 (the
:: "License"); you may not use this file except in compliance
:: with the License.  You may obtain a copy of the License at
::
::   http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing,
:: software distributed under the License is distributed on an
:: "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
:: KIND, either express or implied.  See the License for the
:: specific language governing permissions and limitations
:: under the License.

@ECHO OFF
SETLOCAL EnableDelayedExpansion

:: Where are the svn binaries you want to benchmark?
SET SVN_1_6=C:\path\to\1.6-svn\bin\svn
SET SVN_trunk=C:\path\to\trunk-svn\bin\svn

SET benchmark=%CD%\benchmark.py

SET my_datetime=%date%-%time%
SET my_datetime=%my_datetime: =_%
SET my_datetime=%my_datetime:/=_%
SET my_datetime=%my_datetime::=%
SET my_datetime=%my_datetime:.=%
SET my_datetime=%my_datetime:,=%
SET parent=%my_datetime%
SET inital_workdir=%CD%
mkdir "%parent%"
cd "%parent%"
ECHO %CD%

GOTO main

:batch
  SET levels=%1
  SET spread=%2
  SET N=%3
  SET pre=%levels%x%spread%_
  ECHO.
  ECHO.---------------------------------------------------------------------
  ECHO.
  ECHO.Results for dir levels: %levels%  spread: %spread%
  CALL "%benchmark%" --svn="%SVN_1_6%" run %pre%1.6 %levels% %spread% %N% > NUL
  CALL "%benchmark%" --svn="%SVN_trunk%" run %pre%trunk %levels% %spread% %N% > NUL
  CALL "%benchmark%" compare %pre%1.6 %pre%trunk
  GOTO :EOF

:main
SET N=6
SET al=5
SET as=5
SET bl=25
SET bs=1
SET cl=1
SET cs=100

::::DEBUG
::SET N=1
::SET al=1
::SET as=1
::SET bl=2
::SET bs=1
::SET cl=1
::SET cs=2
::::DEBUG

SET started=%date%-%time%
ECHO.Started at %started%
ECHO.

CALL :batch %al% %as% %N%
CALL :batch %bl% %bs% %N%
CALL :batch %cl% %cs% %N%

ECHO.
ECHO.=========================================================================
ECHO.
FOR %%F IN (*x*_1.6) DO SET all_1.6=!all_1.6! %%F
CALL "%benchmark%" combine total_1.6 %all_1.6% > NUL
FOR %%F IN (*x*_trunk) DO SET all_trunk=!all_trunk! %%F
CALL "%benchmark%" combine total_trunk %all_trunk% > NUL

ECHO.comparing averaged totals..."
CALL "%benchmark%" compare total_1.6 total_trunk

ECHO.
ECHO.Had started at %started%,
ECHO.       done at %date%-%time%
ECHO %CD%

cd "%inital_workdir%"
IF EXIST %parent%\total_trunk rmdir /S /Q "%parent%"

ENDLOCAL
