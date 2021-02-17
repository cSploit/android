::
:: This bat file doesn't use cd, all the paths are full paths.
:: with this convention this bat file is position independent
:: and it will be easier to move the place of somefiles.
::

@echo off
set ERRLEV=0

:CHECK_ENV
@call setenvvar.bat
@if errorlevel 1 (goto :END)

@call set_build_target.bat %*


::===========
:MAIN
@echo.
@echo Copy autoconfig.h
@del %FB_ROOT_PATH%\src\include\gen\autoconfig.h 2> nul
@copy %FB_ROOT_PATH%\src\include\gen\autoconfig_msvc.h %FB_ROOT_PATH%\src\include\gen\autoconfig.h > nul

@echo Creating directories
@rmdir /s /q %FB_GEN_DIR% 2>nul
:: Remove previously generated output, and recreate the directory hierarchy. 
for %%v in ( alice auth burp dsql gpre isql jrd misc msgs qli examples yvalve) do (
  @mkdir %FB_GEN_DIR%\%%v 
)

@rmdir /s /q %FB_GEN_DIR%\utilities 2>nul

@mkdir %FB_GEN_DIR%\utilities 2>nul
@mkdir %FB_GEN_DIR%\utilities\gstat 2>nul
@mkdir %FB_GEN_DIR%\auth\SecurityDatabase 2>nul
@mkdir %FB_GEN_DIR%\gpre\std 2>nul

::=======
call :btyacc
if "%ERRLEV%"=="1" goto :END

call :LibTomMath
if "%ERRLEV%"=="1" goto :END

@echo Generating DSQL parser...
@call parse.bat %*
if "%ERRLEV%"=="1" goto :END

::=======
call :gpre_boot
if "%ERRLEV%"=="1" goto :END

::=======
@echo Preprocessing the source files needed to build gbak, gpre and isql...
@call preprocess.bat BOOT

::=======
call :engine
if "%ERRLEV%"=="1" goto :END

call :gbak
if "%ERRLEV%"=="1" goto :END

call :gpre
if "%ERRLEV%"=="1" goto :END

call :isql
if "%ERRLEV%"=="1" goto :END

@findstr /V "@UDF_COMMENT@" %FB_ROOT_PATH%\builds\install\misc\firebird.conf.in > %FB_BIN_DIR%\firebird.conf

@copy %FB_ROOT_PATH%\extern\icu\icudt???.dat %FB_BIN_DIR% >nul 2>&1
@copy %FB_ICU_SOURCE_BIN%\*.dll %FB_BIN_DIR% >nul 2>&1

::=======
@call :databases

::=======
@echo Preprocessing the entire source tree...
@call preprocess.bat

::=======
@call :msgs
if "%ERRLEV%"=="1" goto :END

@call :codes
if "%ERRLEV%"=="1" goto :END

::=======
@call create_msgs.bat msg
::=======

@call :NEXT_STEP
@goto :END


::===================
:: BUILD btyacc
:btyacc
@echo.
@echo Building btyacc (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3Boot btyacc_%FB_TARGET_PLATFORM%.log btyacc
if errorlevel 1 call :boot2 btyacc
goto :EOF

::===================
:: BUILD LibTomMath
:LibTomMath
@echo.
@call set_build_target.bat %* libtommath
@echo Building LibTomMath (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\extern\libtommath\libtommath_MSVC%MSVC_VERSION% libtommath_%FB_TARGET_PLATFORM%.log libtommath
if errorlevel 1 call :boot2 LibTomMath
@call set_build_target.bat %*
goto :EOF

::===================
:: BUILD gpre_boot
:gpre_boot
@echo.
@echo Building gpre_boot (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3Boot gpre_boot_%FB_TARGET_PLATFORM%.log gpre_boot
if errorlevel 1 call :boot2 gpre_boot
goto :EOF

::===================
:: BUILD engine
:engine
@echo.
@echo Building engine (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3 engine_%FB_TARGET_PLATFORM%.log engine12
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3 engine_%FB_TARGET_PLATFORM%.log ib_util
if errorlevel 1 call :boot2 engine
@goto :EOF

::===================
:: BUILD gbak
:gbak
@echo.
@echo Building gbak (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3 gbak_%FB_TARGET_PLATFORM%.log gbak
if errorlevel 1 call :boot2 gbak
@goto :EOF

::===================
:: BUILD gpre
:gpre
@echo.
@echo Building gpre (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3 gpre_%FB_TARGET_PLATFORM%.log gpre
if errorlevel 1 call :boot2 gpre
@goto :EOF

::===================
:: BUILD isql
:isql
@echo.
@echo Building isql (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3 isql_%FB_TARGET_PLATFORM%.log isql
if errorlevel 1 call :boot2 isql
@goto :EOF

::===================
:: ERROR boot
:boot2
echo.
echo Error building %1, see %1_%FB_TARGET_PLATFORM%.log
echo.
set ERRLEV=1
goto :EOF


::===================
:: BUILD messages
:msgs
@echo.
@echo Building build_msg (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3Boot build_msg_%FB_TARGET_PLATFORM%.log build_msg
if errorlevel 1 goto :msgs2
@goto :EOF
:msgs2
echo.
echo Error building build_msg, see build_msg_%FB_TARGET_PLATFORM%.log
echo.
set ERRLEV=1
goto :EOF

::===================
:: BUILD codes
:codes
@echo.
@echo Building codes (%FB_OBJ_DIR%)...
@call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3Boot codes_%FB_TARGET_PLATFORM%.log codes
if errorlevel 1 goto :codes2
@goto :EOF
:codes2
echo.
echo Error building codes, see codes_%FB_TARGET_PLATFORM%.log
echo.
set ERRLEV=1
goto :EOF

::==============
:databases
@rmdir /s /q %FB_GEN_DIR%\dbs 2>nul
@mkdir %FB_GEN_DIR%\dbs 2>nul

@echo create database '%FB_GEN_DB_DIR%\dbs\security3.fdb'; | "%FB_BIN_DIR%\isql" -q
@"%FB_BIN_DIR%\isql" -q %FB_GEN_DB_DIR%/dbs/security3.fdb -i %FB_ROOT_PATH%\src\dbs\security.sql
@copy %FB_GEN_DIR%\dbs\security3.fdb %FB_GEN_DIR%\dbs\security.fdb > nul

@%FB_BIN_DIR%\gbak -r %FB_ROOT_PATH%\builds\misc\metadata.gbak %FB_GEN_DB_DIR%/dbs/metadata.fdb

@call create_msgs.bat db

@%FB_BIN_DIR%\gbak -r %FB_ROOT_PATH%\builds\misc\help.gbak %FB_GEN_DB_DIR%/dbs/help.fdb
@copy %FB_GEN_DIR%\dbs\metadata.fdb %FB_GEN_DIR%\dbs\yachts.lnk > nul

@goto :EOF


::==============
:NEXT_STEP
@echo.
@echo    You may now run make_all.bat [DEBUG] [CLEAN]
@echo.
@goto :EOF

:END
