@echo off
set ERRLEV=0

:: Set env vars
@call setenvvar.bat

@if errorlevel 1 (call :ERROR Executing setenvvar.bat failed & goto :EOF)

:: verify that boot was run before

@if not exist %FB_GEN_DIR%\dbs\msg.fdb (goto :HELP_BOOT & goto :EOF)


@call set_build_target.bat %*

::==========
:: MAIN

@echo Building %FB_OBJ_DIR%

call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3 make_all_%FB_TARGET_PLATFORM%.log
if errorlevel 1 call :ERROR build failed - see make_all_%FB_TARGET_PLATFORM%.log for details

@if "%ERRLEV%"=="1" (
  @goto :EOF
) else (
  @call :MOVE
)
@goto :EOF

::===========
:MOVE
@echo Copying files to output
@set FB_OUTPUT_DIR=%FB_ROOT_PATH%\output_%FB_TARGET_PLATFORM%
@del %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\*.exp 2>nul
@del %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\*.lib 2>nul
@rmdir /q /s %FB_OUTPUT_DIR% 2>nul

@mkdir %FB_OUTPUT_DIR% 2>nul
@mkdir %FB_OUTPUT_DIR%\intl 2>nul
@mkdir %FB_OUTPUT_DIR%\udf 2>nul
@mkdir %FB_OUTPUT_DIR%\help 2>nul
@mkdir %FB_OUTPUT_DIR%\doc 2>nul
@mkdir %FB_OUTPUT_DIR%\doc\sql.extensions 2>nul
@mkdir %FB_OUTPUT_DIR%\include 2>nul
@mkdir %FB_OUTPUT_DIR%\include\firebird 2>nul
@mkdir %FB_OUTPUT_DIR%\lib 2>nul
@mkdir %FB_OUTPUT_DIR%\system32 2>nul
@mkdir %FB_OUTPUT_DIR%\plugins 2>nul

for %%v in ( icuuc30 icudt30 icuin30 ) do (
@copy %FB_ICU_SOURCE_BIN%\%%v.dll %FB_OUTPUT_DIR% >nul
)

@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\* %FB_OUTPUT_DIR% >nul
@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\intl\* %FB_OUTPUT_DIR%\intl >nul
@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\udf\* %FB_OUTPUT_DIR%\udf >nul
@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\system32\* %FB_OUTPUT_DIR%\system32 >nul
@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\plugins\*.dll %FB_OUTPUT_DIR%\plugins >nul
@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\yvalve\fbclient.lib %FB_OUTPUT_DIR%\lib\fbclient_ms.lib >nul
@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\ib_util\ib_util.lib %FB_OUTPUT_DIR%\lib\ib_util_ms.lib >nul

for %%v in (gpre_boot build_msg codes) do (
@del %FB_OUTPUT_DIR%\%%v.exe >nul
)

:: Firebird.conf, etc
@copy %FB_GEN_DIR%\firebird.msg %FB_OUTPUT_DIR% > nul
:: The line @UDF_COMMENT@ should be deleted from the target file.
findstr /V "@UDF_COMMENT@" %FB_ROOT_PATH%\builds\install\misc\firebird.conf.in > %FB_OUTPUT_DIR%\firebird.conf
@copy %FB_ROOT_PATH%\builds\install\misc\databases.conf.in %FB_OUTPUT_DIR%\databases.conf >nul
@copy %FB_ROOT_PATH%\builds\install\misc\fbintl.conf %FB_OUTPUT_DIR%\intl >nul
@copy %FB_ROOT_PATH%\builds\install\misc\plugins.conf %FB_OUTPUT_DIR% >nul
@copy %FB_ROOT_PATH%\src\utilities\ntrace\fbtrace.conf %FB_OUTPUT_DIR% >nul
@copy %FB_ROOT_PATH%\src\plugins\udr_engine\udr_engine.conf %FB_OUTPUT_DIR%\plugins\udr_engine.conf >nul
@copy %FB_ROOT_PATH%\builds\install\misc\IPLicense.txt %FB_OUTPUT_DIR% >nul
@copy %FB_ROOT_PATH%\builds\install\misc\IDPLicense.txt %FB_OUTPUT_DIR% >nul

:: DATABASES
@copy %FB_GEN_DIR%\dbs\security3.FDB %FB_OUTPUT_DIR%\security3.fdb >nul
@copy %FB_GEN_DIR%\dbs\HELP.fdb %FB_OUTPUT_DIR%\help\help.fdb >nul

:: DOCS
@copy %FB_ROOT_PATH%\ChangeLog %FB_OUTPUT_DIR%\doc\ChangeLog.txt >nul
@copy %FB_ROOT_PATH%\doc\WhatsNew %FB_OUTPUT_DIR%\doc\WhatsNew.txt >nul

:: READMES
@copy %FB_ROOT_PATH%\doc\README.* %FB_OUTPUT_DIR%\doc >nul
@copy %FB_ROOT_PATH%\doc\sql.extensions\README.* %FB_OUTPUT_DIR%\doc\sql.extensions >nul

:: HEADERS
:: Don't use this ibase.h unless you have to - we build it better in BuildExecutableInstall.bat
:: This variation doesn't clean up the license templates, and processes the component files in
:: a different order to that used in the production version. However, this version doesn't
:: have a dependancy upon sed while the production one does.
echo #pragma message("Non-production version of ibase.h.") > %FB_OUTPUT_DIR%\include\ibase.tmp
echo #pragma message("Using raw, unprocessed concatenation of header files.") >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\misc\ibase_header.txt >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\include\types_pub.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\common\dsc_pub.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\dsql\sqlda_pub.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\jrd\ibase.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\jrd\inf_pub.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\include\consts_pub.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\jrd\blr.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
type %FB_ROOT_PATH%\src\include\gen\iberror.h >> %FB_OUTPUT_DIR%\include\ibase.tmp
sed -f %FB_ROOT_PATH%\src\misc\headers.sed < %FB_OUTPUT_DIR%\include\ibase.tmp > %FB_OUTPUT_DIR%\include\ibase.h
del %FB_OUTPUT_DIR%\include\ibase.tmp > nul

:: Additional headers
copy %FB_ROOT_PATH%\src\extlib\ib_util.h %FB_OUTPUT_DIR%\include > nul
copy %FB_ROOT_PATH%\src\jrd\perf.h %FB_OUTPUT_DIR%\include >nul
copy %FB_ROOT_PATH%\src\include\gen\iberror.h %FB_OUTPUT_DIR%\include > nul

:: New API headers
copy %FB_ROOT_PATH%\src\include\firebird\*.h %FB_OUTPUT_DIR%\include\firebird > nul

:: UDF
copy %FB_ROOT_PATH%\src\extlib\ib_udf.sql %FB_OUTPUT_DIR%\udf > nul
copy %FB_ROOT_PATH%\src\extlib\ib_udf2.sql %FB_OUTPUT_DIR%\udf > nul
copy %FB_ROOT_PATH%\src\extlib\fbudf\fbudf.sql %FB_OUTPUT_DIR%\udf > nul

:: Installers
@copy %FB_INSTALL_SCRIPTS%\install_super.bat %FB_OUTPUT_DIR% >nul
@copy %FB_INSTALL_SCRIPTS%\install_classic.bat %FB_OUTPUT_DIR% >nul
@copy %FB_INSTALL_SCRIPTS%\uninstall.bat %FB_OUTPUT_DIR% >nul

:: MSVC runtime
if %MSVC_VERSION% == 10 (
@copy "%VS100COMNTOOLS%\..\..\VC\redist\%FB_VC10CRT_DIR%\Microsoft.VC100.CRT\msvcr100.dll" %FB_OUTPUT_DIR% >nul
) else (
if %MSVC_VERSION% == 9 (
@copy "%VS90COMNTOOLS%\..\..\VC\redist\%FB_PROCESSOR_ARCHITECTURE%\Microsoft.VC90.CRT\msvcr90.dll" %FB_OUTPUT_DIR% >nul
@copy "%VS90COMNTOOLS%\..\..\VC\redist\%FB_PROCESSOR_ARCHITECTURE%\Microsoft.VC90.CRT\msvcp90.dll" %FB_OUTPUT_DIR% >nul
@copy "%VS90COMNTOOLS%\..\..\VC\redist\%FB_PROCESSOR_ARCHITECTURE%\Microsoft.VC90.CRT\Microsoft.VC90.CRT.manifest" %FB_OUTPUT_DIR% >nul
) else (
if %MSVC_VERSION% == 8 (
@copy "%VS80COMNTOOLS%\..\..\VC\redist\%FB_PROCESSOR_ARCHITECTURE%\Microsoft.VC80.CRT\msvcr80.dll" %FB_OUTPUT_DIR% >nul
@copy "%VS80COMNTOOLS%\..\..\VC\redist\%FB_PROCESSOR_ARCHITECTURE%\Microsoft.VC80.CRT\msvcp80.dll" %FB_OUTPUT_DIR% >nul
@copy "%VS80COMNTOOLS%\..\..\VC\redist\%FB_PROCESSOR_ARCHITECTURE%\Microsoft.VC80.CRT\Microsoft.VC80.CRT.manifest" %FB_OUTPUT_DIR% >nul
)
)
)

@goto :EOF

::==============
:HELP_BOOT
@echo.
@echo    You must run make_boot.bat before running this script
@echo.
@goto :EOF

:ERROR
::====
@echo.
@echo   An error occurred while running make_all.bat -
@echo     %*
@echo.
set ERRLEV=1
cancel_script > nul 2>&1
::End of ERROR
::------------
@goto :EOF
