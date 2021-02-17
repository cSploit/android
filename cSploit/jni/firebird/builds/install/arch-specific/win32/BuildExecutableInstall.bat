::  Initial Developer's Public License.
::  The contents of this file are subject to the  Initial Developer's Public
::  License Version 1.0 (the "License"). You may not use this file except
::  in compliance with the License. You may obtain a copy of the License at
::    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
::  Software distributed under the License is distributed on an "AS IS" basis,
::  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
::  for the specific language governing rights and limitations under the
::  License.
::
::  The Original Code is copyright 2003-2004 Paul Reeves.
::
::  The Initial Developer of the Original Code is Paul Reeves
::
::  All Rights Reserved.
::
::=============================================================================
@echo off

@goto :MAIN
@goto :EOF
::============================================================================

:SET_PARAMS
@echo off
:: reset ERRLEV to clear error from last run in same cmd shell
set ERRLEV=0
:: Assume we are preparing a production build
set FBBUILD_BUILDTYPE=release
:: Don't ship pdb files by default
set FBBUILD_SHIP_PDB=no_pdb
:: Reset "make" vars to zero
set FBBUILD_ZIP_PACK=0
set FBBUILD_ISX_PACK=0
set FBBUILD_EMB_PACK=0

if not defined FB2_SNAPSHOT (set FB2_SNAPSHOT=0)

:: Set our package number at 0 and increment every
:: time we rebuild in a single session
if not defined FBBUILD_PACKAGE_NUMBER (
set FBBUILD_PACKAGE_NUMBER=0
) else (
set /A FBBUILD_PACKAGE_NUMBER+=1
)
@echo   Setting FBBUILD_PACKAGE_NUMBER to %FBBUILD_PACKAGE_NUMBER%

::If a suffix is defined (usually for an RC) ensure it is prefixed correctly.
if defined FBBUILD_FILENAME_SUFFIX (
if not "%FBBUILD_FILENAME_SUFFIX:~0,1%"=="_" (
(set FBBUILD_FILENAME_SUFFIX=_%FBBUILD_FILENAME_SUFFIX%)
)
)

:: See what we have on the command line

for %%v in ( %* )  do (
( if /I "%%v"=="DEBUG" (set FBBUILD_BUILDTYPE=debug) )
  ( if /I "%%v"=="PDB" (set FBBUILD_SHIP_PDB=ship_pdb) )
  ( if /I "%%v"=="ZIP" (set FBBUILD_ZIP_PACK=1) )
  ( if /I "%%v"=="ISX" (set FBBUILD_ISX_PACK=1) )
  ( if /I "%%v"=="EMB" (set FBBUILD_EMB_PACK=1) )
  ( if /I "%%v"=="ALL" ( (set FBBUILD_ZIP_PACK=1) & (set FBBUILD_ISX_PACK=1) & (set FBBUILD_EMB_PACK=1) ) )
)

:: Note FBBUILD_EMB_PACK appears to no be longer relevant. Before removing the code we will
:: deprecate it here.
set FBBUILD_EMB_PACK=0

:: Now check whether we are debugging the InnoSetup script

@if %FB2_ISS_DEBUG% equ 0 (@set ISS_BUILD_TYPE=iss_release) else (@set ISS_BUILD_TYPE=iss_debug)
@if %FB2_ISS_DEBUG% equ 0 (@set ISS_COMPRESS=compression) else (@set ISS_COMPRESS=nocompression)

(@set ISS_EXAMPLES=examples)
@if %FB2_ISS_DEBUG% equ 1 (
  @if %FB2_EXAMPLES% equ 0 (@set ISS_EXAMPLES=noexamples)
)

::Are we doing a snapshot build? If so we always do less work.
if "%FB2_SNAPSHOT%"=="1" (
  (set FB_ISS_EXAMPLES=noexamples)
  (set FBBUILD_ISX_PACK=0)
  (set FBBUILD_EMB_PACK=0)
)


:: Set up our final destination
set FBBUILD_INSTALL_IMAGES=%FB_ROOT_PATH%\builds\install_images
if not exist %FBBUILD_INSTALL_IMAGES% (mkdir %FBBUILD_INSTALL_IMAGES%)



:: Determine Product Status
set FBBUILD_PROD_STATUS=
@type %FB_ROOT_PATH%\src\jrd\build_no.h | findstr /I UNSTABLE  > nul && (
set FBBUILD_PROD_STATUS=DEV) || type %FB_ROOT_PATH%\src\jrd\build_no.h | findstr /I ALPHA > nul && (
set FBBUILD_PROD_STATUS=DEV) || type %FB_ROOT_PATH%\src\jrd\build_no.h | findstr /I BETA > nul && (
set FBBUILD_PROD_STATUS=PROD) || type %FB_ROOT_PATH%\src\jrd\build_no.h | findstr /I "Release Candidate" > nul && (
set FBBUILD_PROD_STATUS=PROD) || type %FB_ROOT_PATH%\src\jrd\build_no.h | findstr "RC" > nul && (
set FBBUILD_PROD_STATUS=PROD) || type %FB_ROOT_PATH%\src\jrd\build_no.h | findstr /I "Final" > nul && (
set FBBUILD_PROD_STATUS=PROD)

::End of SET_PARAMS
::-----------------
@goto :EOF


:CHECK_ENVIRONMENT
::================
:: Make sure we have everything we need. If something is missing then
:: let's bail out now.

sed --version | findstr version > nul
@if %ERRORLEVEL% GEQ 1 (
    call :ERROR Could not locate sed
    goto :EOF
) else (@echo     o sed found.)

if %FBBUILD_ZIP_PACK% EQU 1 (
  if not defined SEVENZIP (
    call :ERROR SEVENZIP environment variable is not defined.
    @goto :EOF
  ) else (@echo     o Compression utility found.)
)


if %FBBUILD_ISX_PACK% EQU 1 (
  if NOT DEFINED INNO5_SETUP_PATH (
    call :ERROR INNO5_SETUP_PATH variable not defined
    @goto :EOF
  ) else (@echo     o Inno Setup found at %INNO5_SETUP_PATH%.)
)


if not DEFINED FB_EXTERNAL_DOCS (
 @echo.
 @echo The FB_EXTERNAL_DOCS environment var is not defined
 @echo It should point to the directory containing the relevant release notes
 @echo in adobe pdf format.
 @echo.
 @echo Subsequent script execution will be cancelled.
 @echo.
 cancel_script > nul 2>&1
 goto :EOF
)

)

::End of CHECK_ENVIRONMENT
::------------------------
@goto :EOF


:SED_MAGIC
:: Do some sed magic to make sure that the final product
:: includes the version string in the filename.
:: If the Firebird Unix tools for Win32 aren't on
:: the path this will fail! Use of the cygwin tools has not
:: been tested and may produce unexpected results.
::========================================================
find "#define PRODUCT_VER_STRING" %FB_ROOT_PATH%\src\jrd\build_no.h > %temp%.\b$1.txt
sed -n -e s/\"//g -e s/"#define PRODUCT_VER_STRING "//w%temp%.\b$2.txt %temp%.\b$1.txt
for /f "tokens=*" %%a in ('type %temp%.\b$2.txt') do set FBBUILD_PRODUCT_VER_STRING=%%a

find "#define FB_MAJOR_VER" %FB_ROOT_PATH%\src\jrd\build_no.h > %temp%.\b$1.txt
sed -n -e s/\"//g -e s/"#define FB_MAJOR_VER "//w%temp%.\b$2.txt %temp%.\b$1.txt
for /f "tokens=*" %%a in ('type %temp%.\b$2.txt') do set FB_MAJOR_VER=%%a

find "#define FB_MINOR_VER" %FB_ROOT_PATH%\src\jrd\build_no.h > %temp%.\b$1.txt
sed -n -e s/\"//g -e s/"#define FB_MINOR_VER "//w%temp%.\b$2.txt %temp%.\b$1.txt
for /f "tokens=*" %%a in ('type %temp%.\b$2.txt') do set FB_MINOR_VER=%%a

find "#define FB_REV_NO" %FB_ROOT_PATH%\src\jrd\build_no.h > %temp%.\b$1.txt
sed -n -e s/\"//g -e s/"#define FB_REV_NO "//w%temp%.\b$2.txt %temp%.\b$1.txt
for /f "tokens=*" %%a in ('type %temp%.\b$2.txt') do set FB_REV_NO=%%a

find "#define FB_BUILD_NO" %FB_ROOT_PATH%\src\jrd\build_no.h > %temp%.\b$1.txt
sed -n -e s/\"//g -e s/"#define FB_BUILD_NO "//w%temp%.\b$2.txt %temp%.\b$1.txt
for /f "tokens=*" %%a in ('type %temp%.\b$2.txt') do set FB_BUILD_NO=%%a

set FBBUILD_FILE_ID=%FBBUILD_PRODUCT_VER_STRING%-%FBBUILD_PACKAGE_NUMBER%_%FB_TARGET_PLATFORM%

::@echo s/-2.0.0-/-%FBBUILD_PRODUCT_VER_STRING%-/ > %temp%.\b$3.txt
@echo s/define release/define %FBBUILD_BUILDTYPE%/ > %temp%.\b$3.txt
@echo s/define msvc_version 8/define msvc_version %MSVC_VERSION%/ >> %temp%.\b$3.txt
@echo s/define no_pdb/define %FBBUILD_SHIP_PDB%/ >> %temp%.\b$3.txt
::@echo s/define package_number=\"0\"/define package_number=\"%FBBUILD_PACKAGE_NUMBER%\"/ >> %temp%.\b$3.txt
@echo s/define iss_release/define %ISS_BUILD_TYPE%/ >> %temp%.\b$3.txt
@echo s/define examples/define %ISS_EXAMPLES%/ >> %temp%.\b$3.txt
@echo s/define compression/define %ISS_COMPRESS%/ >> %temp%.\b$3.txt
@echo s/FBBUILD_PRODUCT_VER_STRING/%FBBUILD_PRODUCT_VER_STRING%/ >> %temp%.\b$3.txt

sed -f  %temp%.\b$3.txt FirebirdInstall_30.iss > FirebirdInstall_%FBBUILD_FILE_ID%.iss

:: This is a better way of achieving what is done in make_all.bat, but we don't
:: test for sed in that script.
@sed /@UDF_COMMENT@/s/@UDF_COMMENT@/#/ < %FB_ROOT_PATH%\builds\install\misc\firebird.conf.in >  %FB_OUTPUT_DIR%\firebird.conf

set FBBUILD_FB30_CUR_VER=%FB_MAJOR_VER%.%FB_MINOR_VER%.%FB_REV_NO%
set FBBUILD_FB_CUR_VER=%FBBUILD_FB30_CUR_VER%
set FBBUILD_FB_LAST_VER=%FBBUILD_FB25_CUR_VER%

:: Now set some version strings of our legacy releases.
:: This helps us copy the correct documentation,
:: as well as set up the correct shortcuts
set FBBUILD_FB15_CUR_VER=1.5.6
set FBBUILD_FB20_CUR_VER=2.0.7
set FBBUILD_FB21_CUR_VER=2.1.5
set FBBUILD_FB25_CUR_VER=2.5.2

:: Now fix up the major.minor version strings in the readme files.
:: We place output in %FB_GEN_DIR%\readmes
@if not exist %FB_GEN_DIR%\readmes (@mkdir %FB_GEN_DIR%\readmes)
@for %%d in (ba cz de es fr hu it pl pt ru si ) do (
  @if not exist %FB_GEN_DIR%\readmes\%%d (@mkdir %FB_GEN_DIR%\readmes\%%d)
)

@echo s/\$MAJOR/%FB_MAJOR_VER%/g >  %temp%.\b$4.txt
@echo s/\$MINOR/%FB_MINOR_VER%/g >> %temp%.\b$4.txt
@echo s/\$RELEASE/%FB_REV_NO%/g  >> %temp%.\b$4.txt
@for %%f in (Readme.txt installation_readme.txt After_Installation.url) do (
	@echo   Processing version strings in %%f
	@sed -f  %temp%.\b$4.txt %%f > %FB_GEN_DIR%\readmes\%%f
)
@for %%d in (ba cz de es fr hu it pl pt ru si ) do (
  @pushd %%d
  @for /F %%f in ( '@dir /B /A-D *.txt'  ) do (
	@echo   Processing version strings in %%d\%%f
	@sed -f  %temp%.\b$4.txt %%f > %FB_GEN_DIR%\readmes\%%d\%%f
  )
  @popd
)

del %temp%.\b$?.txt


::End of SED_MAGIC
::----------------
@goto :EOF


:COPY_XTRA
:: system dll's we need
:: MSVC should be installed with redistributable packages.
::=====================
@echo   Copying MSVC runtime libraries...
if not exist %FB_OUTPUT_DIR%\system32 (mkdir %FB_OUTPUT_DIR%\system32)
@echo on
for %%f in ( msvcp%MSVC_VERSION%?.dll msvcr%MSVC_VERSION%?.dll  ) do ( 
if exist "%VCINSTALLDIR%\redist\%PROCESSOR_ARCHITECTURE%\Microsoft.VC%MSVC_VERSION%0.CRT\%%f" (
copy  "%VCINSTALLDIR%\redist\%PROCESSOR_ARCHITECTURE%\Microsoft.VC%MSVC_VERSION%0.CRT\%%f" %FB_OUTPUT_DIR%\ 
)
)

@echo off


@if %ERRORLEVEL% GEQ 1 ( (call :ERROR Copying MSVC runtime library failed with error %ERRORLEVEL% ) & (goto :EOF))

:: grab some missing bits'n'pieces from different parts of the source tree
::=========================================================================
@echo   Copying ib_util etc
copy %FB_ROOT_PATH%\src\extlib\ib_util.h %FB_OUTPUT_DIR%\include > nul || (call :WARNING Copying ib_util.h failed.)
copy %FB_ROOT_PATH%\lang_helpers\ib_util.pas %FB_OUTPUT_DIR%\include > nul || (call :WARNING Copying ib_util.pas failed.)

@implib.exe | findstr "Borland" > nul
@if errorlevel 0 (
if "%PROCESSOR_ARCHITECTURE%"=="x86" (
  @echo   Generating fbclient_bor.lib
  @implib %FB_OUTPUT_DIR%\lib\fbclient_bor.lib %FB_OUTPUT_DIR%\fbclient.dll > nul
)
)

@if "%FBBUILD_SHIP_PDB%"=="ship_pdb" (
  @echo   Copying pdb files...
  @copy %FB_TEMP_DIR%\%FBBUILD_BUILDTYPE%\fbserver\firebird.pdb %FB_OUTPUT_DIR%\ > nul
  @copy %FB_TEMP_DIR%\%FBBUILD_BUILDTYPE%\yvalve\fbclient.pdb %FB_OUTPUT_DIR%\ > nul
)

@echo   Started copying docs...
@rmdir /S /Q %FB_OUTPUT_DIR%\doc 2>nul
@mkdir %FB_OUTPUT_DIR%\doc
@copy %FB_ROOT_PATH%\ChangeLog %FB_OUTPUT_DIR%\doc\ChangeLog.txt >nul
@copy %FB_ROOT_PATH%\doc\*.* %FB_OUTPUT_DIR%\doc\ > nul
@if %ERRORLEVEL% GEQ 1 (
  call :ERROR COPY of main documentation tree failed with error %ERRORLEVEL%
  goto :EOF
)

@echo   Copying udf library scripts...
for %%v in ( ib_udf.sql ib_udf2.sql ) do (
  @copy %FB_ROOT_PATH%\src\extlib\%%v  %FB_OUTPUT_DIR%\UDF\%%v > nul
  @if %ERRORLEVEL% GEQ 1 (
    call :ERROR Copying %FB_ROOT_PATH%\src\extlib\%%v failed.
    goto :EOF
  )
)

for %%v in ( fbudf.sql fbudf.txt ) do (
  copy %FB_ROOT_PATH%\src\extlib\fbudf\%%v  %FB_OUTPUT_DIR%\UDF\%%v > nul
  @if %ERRORLEVEL% GEQ 1 (
    call :ERROR Copying %FB_ROOT_PATH%\src\extlib\%%v failed with error %ERRORLEVEL%
    goto :EOF
  )
)

::UDF upgrade script and doc
mkdir %FB_OUTPUT_DIR%\misc\upgrade\ib_udf 2>nul
@copy %FB_ROOT_PATH%\src\misc\upgrade\v2\ib_udf*.* %FB_OUTPUT_DIR%\misc\upgrade\ib_udf\ > nul

::INTL script
@copy %FB_ROOT_PATH%\src\misc\intl.sql %FB_OUTPUT_DIR%\misc\ > nul


@echo   Copying other documentation...
@copy  %FB_GEN_DIR%\readmes\installation_readme.txt %FB_OUTPUT_DIR%\doc\installation_readme.txt > nul
@copy %FB_OUTPUT_DIR%\doc\WhatsNew %FB_OUTPUT_DIR%\doc\WhatsNew.txt > nul
@del %FB_OUTPUT_DIR%\doc\WhatsNew


:: If we are not doing a final release then include stuff that is
:: likely to be of use to testers, especially as our release notes
:: may be incomplete or non-existent
@if /I "%FBBUILD_PROD_STATUS%"=="DEV" (
  @copy %FB_ROOT_PATH%\ChangeLog %FB_OUTPUT_DIR%\doc\ChangeLog.txt  > nul
)


@mkdir %FB_OUTPUT_DIR%\doc\sql.extensions 2>nul
@if %ERRORLEVEL% GEQ 2 ( (call :ERROR MKDIR for doc\sql.extensions dir failed) & (@goto :EOF))
@copy %FB_ROOT_PATH%\doc\sql.extensions\*.* %FB_OUTPUT_DIR%\doc\sql.extensions\ > nul
@if %ERRORLEVEL% GEQ 1 ( (call :ERROR Copying doc\sql.extensions failed  ) & (goto :EOF))

:: External docs aren't necessary for a snapshot build, so we don't throw
:: an error if FB_EXTERNAL_DOCS is not defined. On the other hand,
:: if the docs are available then we can include them.
if defined FB_EXTERNAL_DOCS (
@echo   Copying pdf docs...
@for %%v in ( Firebird-%FB_MAJOR_VER%.%FB_MINOR_VER%-QuickStart.pdf Firebird_v%FBBUILD_FB_CUR_VER%.ReleaseNotes.pdf ) do (
  @echo     ... %%v
  (@copy /Y %FB_EXTERNAL_DOCS%\%%v %FB_OUTPUT_DIR%\doc\%%v > nul) || (call :WARNING Copying %FB_EXTERNAL_DOCS%\%%v failed.)
)
)

:: Clean out text notes that are either not relevant to Windows or
:: are only of use to engine developers.
@for %%v in (  README.makefiles README.user README.user.embedded README.user.troubleshooting README.build.mingw.html README.build.msvc.html fb2-todo.txt cleaning-todo.txt install_win32.txt README.coding.style emacros-cross_ref.html firebird_conf.txt *.*~) do (
  @del %FB_OUTPUT_DIR%\doc\%%v 2>nul
)

:: Add license
for %%v in (IPLicense.txt IDPLicense.txt ) do (
    @copy %FB_ROOT_PATH%\builds\install\misc\%%v %FB_OUTPUT_DIR%\%%v > nul
)

:: And readme
@copy  %FB_GEN_DIR%\readmes\readme.txt %FB_OUTPUT_DIR%\ > nul

::  Walk through all docs and transform any that are not .txt, .pdf or .html to .txt
echo   Setting .txt filetype to ascii docs.
for /R %FB_OUTPUT_DIR%\doc %%v in (.) do (
  pushd %%v
  for /F %%W in ( 'dir /B /A-D' ) do (
    if /I "%%~xW" NEQ ".txt" (
      if /I "%%~xW" NEQ ".pdf" (
        if /I "%%~xW" NEQ ".htm" (
          if /I "%%~xW" NEQ ".html" (
            ren %%W %%W.txt
          )
        )
      )
    )
  )
  popd
)

:: Throw away any errorlevel left hanging around
@set | findstr win > nul

@echo   Completed copying docs.
::End of COPY_XTRA
::----------------
@goto :EOF


:BUILD_CRT_MSI
:: Generate runtimes as an MSI file.
:: This requires WiX 2.0 to be installed
::============
:: This is only relevent if we are shipping packages built with MSVS 2005 (MSVC8)
:: We have never shipped packages with MSVS 2008 (MSVC9) and we are hoping not 
:: to deploy MSI runtimes if we build with MSVC 2010 (MVC10)
if %MSVC_VERSION% EQU 8 (
if not exist %FB_OUTPUT_DIR%\system32\vccrt%MSVC_VERSION%_%FB_TARGET_PLATFORM%.msi (
    %WIX%\candle.exe -v0 %FB_ROOT_PATH%\builds\win32\msvc%MSVC_VERSION%\VCCRT_%FB_TARGET_PLATFORM%.wxs -out %FB_GEN_DIR%\vccrt_%FB_TARGET_PLATFORM%.wixobj
    %WIX%\light.exe %FB_GEN_DIR%\vccrt_%FB_TARGET_PLATFORM%.wixobj -out %FB_OUTPUT_DIR%\system32\vccrt%MSVC_VERSION%_%FB_TARGET_PLATFORM%.msi
) else (
    @echo   Using an existing build of %FB_OUTPUT_DIR%\system32\vccrt%MSVC_VERSION%_%FB_TARGET_PLATFORM%.msi
))

::End of BUILD_CRT_MSI
::--------------------
@goto :EOF


:IBASE_H
:: Concatenate header files into ibase.h
::======================================
:: o This section of code takes several header files, strips license
::   boiler plates and comments and inserts them into ibase.h for
::   distribution. The only drawback is that it strips all the comments.
:: o No error checking is done.
:: o Take note that different versions of sed use different
::   string delimiters. The firebird_tools version uses double quotes - ".
::   The cygwin one probably uses single quotes.
:: o The script 'strip_comments.sed' is taken from
::      http://sed.sourceforge.net/grabbag/scripts/testo.htm

setlocal
set OUTPATH=%FB_OUTPUT_DIR%\include
copy %FB_ROOT_PATH%\src\jrd\ibase.h %OUTPATH%\ibase.h > nul
for %%v in ( %FB_ROOT_PATH%\src\include\types_pub.h %FB_ROOT_PATH%\src\include\consts_pub.h %FB_ROOT_PATH%\src\dsql\sqlda_pub.h %FB_ROOT_PATH%\src\common\dsc_pub.h %FB_ROOT_PATH%\src\jrd\inf_pub.h %FB_ROOT_PATH%\src\jrd\blr.h ) do (
  del %OUTPATH%\%%~nxv 2> nul
  copy %%v %OUTPATH%\%%~nxv > nul
  sed -n -f strip_comments.sed %OUTPATH%\%%~nxv > %OUTPATH%\%%~nv.more || call :ERROR Stripping comments from %%v failed.

  more /s %OUTPATH%\%%~nv.more > %OUTPATH%\%%~nv.sed
)
move /y %OUTPATH%\ibase.h %OUTPATH%\ibase.sed
sed -e "/#include \"types_pub\.h\"/r %OUTPATH%\types_pub.sed" -e "/#include \"types_pub\.h\"/d" -e "/#include \"consts_pub\.h\"/r %OUTPATH%\consts_pub.sed" -e "/#include \"consts_pub\.h\"/d" -e "/#include \"..\/common\/dsc_pub\.h\"/r %OUTPATH%\dsc_pub.sed" -e "/#include \"..\/jrd\/dsc_pub\.h\"/d" -e "/#include \"..\/dsql\/sqlda_pub\.h\"/r %OUTPATH%\sqlda_pub.sed" -e "/#include \"..\/dsql\/sqlda_pub\.h\"/d" -e "/#include \"blr\.h\"/r %OUTPATH%\blr.sed" -e "/#include \"blr\.h\"/d" -e "/#include \"..\/jrd\/inf_pub\.h\"/r %OUTPATH%\inf_pub.sed" -e "/#include \"..\/jrd\/inf_pub\.h\"/d" %OUTPATH%\ibase.sed > %OUTPATH%\ibase.h
del %OUTPATH%\ibase.sed %OUTPATH%\types_pub.* %OUTPATH%\consts_pub.* %OUTPATH%\sqlda_pub.* %OUTPATH%\dsc_pub.* %OUTPATH%\inf_pub.* %OUTPATH%\blr.*
endlocal

::End of IBASE_H
::--------------
@goto :EOF


:DB_CONF
:: Generate sample databases file
::===============================
@echo   Creating sample databases.conf
copy %FB_ROOT_PATH%\builds\install\misc\databases.conf.in %FB_OUTPUT_DIR%\databases.conf > nul

::End of DB_CONF
::-----------------
@goto :EOF


:MISC
::==============================================
:: miscellany that doesn't belong anywhere else
::==============================================
::Metadata migration - presumably not needed for 3.0 so disabled for now
::mkdir %FB_OUTPUT_DIR%\misc\upgrade\metadata 2>nul
::@copy %FB_ROOT_PATH%\src\misc\upgrade\v2.1\metadata_* %FB_OUTPUT_DIR%\misc\upgrade\metadata > nul

:: Make sure that qli's help.fdb is available
::===============================================
@if not exist %FB_OUTPUT_DIR%\help\help.fdb (
    (@echo   Copying help.fdb for qli support)
    (@copy %FB_GEN_DIR%\dbs\help.fdb %FB_OUTPUT_DIR%\help\help.fdb > nul)
    (@if %ERRORLEVEL% GEQ 1 ( (call :ERROR Could not copy qli help database ) & (goto :EOF)))
)


::End of MISC
::-----------------
@goto :EOF


:FB_MSG
::=================================================================
:: firebird.msg is generated as part of the build process
:: in builds\win32 by build_msg.bat. Copying from there to output dir
::=================================================================
@if not exist %FB_OUTPUT_DIR%\firebird.msg (
    (@copy %FB_GEN_DIR%\firebird.msg %FB_OUTPUT_DIR%\firebird.msg > nul)
    (@if %ERRORLEVEL% GEQ 1 ( (call :ERROR Could not copy firebird.msg ) & (goto :EOF)))
)

::End of FB_MSG
::-------------
@goto :EOF


::TOUCH_LOCAL
::==========
:: Note 1: This doesn't seem to make any difference to whether local libraries are used.
:: Note 2: MS documentation was incorrectly interpreted. .local files should not be created
::         for libraries, they should be created for executables.
:: Create libname.local files for each locally installed library
::for %%v in ( fbclient msvcrt msvcp%VS_VER%0 )  do touch %FB_OUTPUT_DIR%\%%v.local
::@goto :EOF


:GEN_ZIP
::======
if %FBBUILD_ZIP_PACK% EQU 0 goto :EOF
:: Generate the directory tree to be zipped
set FBBUILD_ZIP_PACK_ROOT=%FB_ROOT_PATH%\builds\zip_pack_%FB_TARGET_PLATFORM%
if not exist %FBBUILD_ZIP_PACK_ROOT% @mkdir %FBBUILD_ZIP_PACK_ROOT% 2>nul
@del /s /q %FBBUILD_ZIP_PACK_ROOT%\ > nul
@copy /Y %FB_OUTPUT_DIR% %FBBUILD_ZIP_PACK_ROOT% > nul
for %%v in (bin doc doc\sql.extensions help include intl lib udf misc misc\upgrade misc\upgrade\ib_udf misc\upgrade\security misc\upgrade\metadata system32 plugins ) do (
    @mkdir %FBBUILD_ZIP_PACK_ROOT%\%%v 2>nul
    @dir /A-D %FB_OUTPUT_DIR%\%%v\*.* > nul 2>nul
    if not ERRORLEVEL 1 @copy /Y %FB_OUTPUT_DIR%\%%v\*.* %FBBUILD_ZIP_PACK_ROOT%\%%v\ > nul
)

@if %FB2_EXAMPLES% equ 1 for %%v in (examples examples\api examples\dyn examples\empbuild examples\include examples\stat examples\udf examples\build_win32 ) do (
    @mkdir %FBBUILD_ZIP_PACK_ROOT%\%%v 2>nul
    dir %FB_OUTPUT_DIR%\%%v\*.* > nul 2>nul
    if not ERRORLEVEL 1 @copy /Y %FB_OUTPUT_DIR%\%%v\*.* %FBBUILD_ZIP_PACK_ROOT%\%%v\ > nul
)


:: Now remove stuff that is not needed.
setlocal
set FB_RM_FILE_LIST=doc\installation_readme.txt bin\gpre_boot.exe bin\gpre_static.exe bin\gpre_embed.exe bin\gbak_embed.exe bin\isql_embed.exe bin\gds32.dll bin\btyacc.exe
if %FB2_SNAPSHOT% EQU 0 (set FB_RM_FILE_LIST=bin\fbembed.dll bin\fbembed.pdb %FB_RM_FILE_LIST%)
for %%v in ( %FB_RM_FILE_LIST% ) do (
  @del %FBBUILD_ZIP_PACK_ROOT%\%%v > nul 2>&1
)
endlocal

if %FB2_SNAPSHOT% EQU 1 (
  @copy %FB_ROOT_PATH%\builds\install\arch-specific\win32\readme_snapshot.txt %FBBUILD_ZIP_PACK_ROOT%\readme_snapshot.txt > nul
)

if not "%FBBUILD_SHIP_PDB%"=="ship_pdb" (
  @del /q %FBBUILD_ZIP_PACK_ROOT%\*.pdb > nul 2>&1
)

:: grab install notes for zip pack
@copy %FB_ROOT_PATH%\doc\install_win32.txt %FBBUILD_ZIP_PACK_ROOT%\doc\README_installation.txt > nul

::End of GEN_ZIP
::--------------
goto :EOF


:ZIP_PACK
::=======
if %FBBUILD_ZIP_PACK% EQU 0 goto :EOF
if "%FBBUILD_SHIP_PDB%" == "ship_pdb" (
    if exist %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%_pdb%FBBUILD_FILENAME_SUFFIX%.zip (
      @del %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%_pdb%FBBUILD_FILENAME_SUFFIX%.zip
    )
    set FBBUILD_ZIPFILE=%FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%_pdb%FBBUILD_FILENAME_SUFFIX%.zip

) else (
    if exist %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%%FBBUILD_FILENAME_SUFFIX%.zip (
      @del %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%%FBBUILD_FILENAME_SUFFIX%.zip
    )
    set FBBUILD_ZIPFILE=%FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%%FBBUILD_FILENAME_SUFFIX%.zip
)

@%SEVENZIP%\7z.exe a -r -tzip -mx9 %FBBUILD_ZIPFILE% %FBBUILD_ZIP_PACK_ROOT%\*.*
@echo   End of ZIP_PACK
@echo.
::----------------
@goto :EOF


:GEN_EMBEDDED
::===========
:: Generate the directory tree for the embedded zip pack
if %FBBUILD_EMB_PACK% EQU 0 goto :EOF
set FBBUILD_EMB_PACK_ROOT=%FB_ROOT_PATH%\builds\emb_pack_%FB_TARGET_PLATFORM%
@mkdir %FBBUILD_EMB_PACK_ROOT% 2>nul
@del /s /q %FBBUILD_EMB_PACK_ROOT%\ > nul

for %%v in (databases.conf firebird.conf firebird.msg) do (	@copy /Y %FB_OUTPUT_DIR%\%%v %FBBUILD_EMB_PACK_ROOT%\%%v > nul)

for %%v in ( doc intl udf ) do (@mkdir %FBBUILD_EMB_PACK_ROOT%\%%v 2>nul)

@copy /Y %FB_TEMP_DIR%\%FBBUILD_BUILDTYPE%\firebird\fbembed.* %FBBUILD_EMB_PACK_ROOT% > nul

for %%v in ( icuuc30 icudt30 icuin30  ) do (
@copy %FB_ICU_SOURCE_BIN%\%%v.dll %FBBUILD_EMB_PACK_ROOT% >nul
)

@copy /Y %FB_TEMP_DIR%\%FBBUILD_BUILDTYPE%\firebird\ib_util.dll %FBBUILD_EMB_PACK_ROOT% > nul
@copy /Y %FB_OUTPUT_DIR%\doc\Firebird*.pdf %FBBUILD_EMB_PACK_ROOT%\doc\ > nul
@copy /Y %FB_OUTPUT_DIR%\intl\*.* %FBBUILD_EMB_PACK_ROOT%\intl\ > nul
@copy /Y %FB_OUTPUT_DIR%\udf\*.* %FBBUILD_EMB_PACK_ROOT%\udf\ > nul
@copy /Y %FB_OUTPUT_DIR%\msvc*.* %FBBUILD_EMB_PACK_ROOT% > nul
if %MSVC_VERSION% EQU 8 (
  @copy /Y %FB_OUTPUT_DIR%\Microsoft.VC80.CRT.manifest %FBBUILD_EMB_PACK_ROOT% > nul
)

if "%FBBUILD_SHIP_PDB%"=="ship_pdb" (
  @copy /Y %FB_TEMP_DIR%\%FBBUILD_BUILDTYPE%\fbembed\fbembed.pdb %FBBUILD_EMB_PACK_ROOT% > nul
)


:: grab install notes for embedded zip pack
@copy %FB_ROOT_PATH%\doc\README.user.embedded %FBBUILD_EMB_PACK_ROOT%\doc\README_embedded.txt > nul
@copy %FB_ROOT_PATH%\doc\WhatsNew %FBBUILD_EMB_PACK_ROOT%\doc\WhatsNew.txt > nul

:: Add license
for %%v in (IPLicense.txt IDPLicense.txt ) do (
    @copy %FB_ROOT_PATH%\builds\install\misc\%%v %FBBUILD_EMB_PACK_ROOT%\%%v > nul
)

:: And readme
@copy  %FB_GEN_DIR%\readmes\readme.txt %FBBUILD_EMB_PACK_ROOT%\ > nul


::End of GEN_EMBEDDED
::-------------------
@goto :EOF


:EMB_PACK
::=======
if %FBBUILD_EMB_PACK% EQU 0 goto :EOF
:: Now we can zip it up and copy the package to the install images directory.
if "%FBBUILD_SHIP_PDB%" == "ship_pdb" (
  @del %FBBUILD_INSTALL_IMAGES%\%FBBUILD_FILE_ID%_embed_pdb%FBBUILD_FILENAME_SUFFIX%.zip > nul
  set FBBUILD_EMBFILE=%FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%_embed_pdb%FBBUILD_FILENAME_SUFFIX%.zip
) else (
  @del %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%_embed%FBBUILD_FILENAME_SUFFIX%.zip > nul
  set FBBUILD_EMBFILE=%FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_FILE_ID%_embed%FBBUILD_FILENAME_SUFFIX%.zip
)

@%SEVENZIP%\7z.exe a -r -tzip -mx9 %FBBUILD_EMBFILE% %FBBUILD_EMB_PACK_ROOT%\*.*


@echo   End of EMB_PACK
@echo.
::---------------
goto :EOF


:TOUCH_ALL
::========
::Set file timestamp to something meaningful.
::While building and testing this feature might be annoying, so we don't do it.
::==========================================================
setlocal
set TIMESTRING=0%PRODUCT_VER_STRING:~0,1%:0%PRODUCT_VER_STRING:~2,1%:0%PRODUCT_VER_STRING:~4,1%
@if /I "%BUILDTYPE%"=="release" (
    (@echo Touching release build files with %TIMESTRING% timestamp) & (touch -s -D -t%TIMESTRING% %FB_OUTPUT_DIR%\*.*)
    (if %FBBUILD_EMB_PACK% EQU 1 (@echo Touching release build files with %TIMESTRING% timestamp) & (touch -s -D -t%TIMESTRING% %FB_ROOT_PATH%\emb_pack\*.*) )
    (if %FBBUILD_ZIP_PACK% EQU 1 (@echo Touching release build files with %TIMESTRING% timestamp) & (touch -s -D -t%TIMESTRING% %FB_ROOT_PATH%\zip_pack\*.*) )
)
endlocal
::End of TOUCH_ALL
::----------------
@goto :EOF


:ISX_PACK
::=======
:: Now let's go and build the installable .exe
::
:: Note - define INNO5_SETUP_PATH with double quotes if it is installed into a path string using spaces.
:: eg set INNO5_SETUP_PATH="C:\Program Files\Inno Setup 5"
::
::=================================================
if %FBBUILD_ISX_PACK% NEQ 1 goto :EOF
@Echo   Now let's compile the InnoSetup scripts
@Echo.
%INNO5_SETUP_PATH%\iscc %FB_ROOT_PATH%\builds\install\arch-specific\win32\FirebirdInstall_%FBBUILD_FILE_ID%.iss
@echo.

@echo   End of ISX_PACK
@echo.
::---------------
@goto :EOF


:DO_MD5SUMS
::=========
:: Generate the md5sum checksum file
::==================================
if NOT DEFINED GNU_TOOLCHAIN (
  call :WARNING GNU_TOOLCHAIN variable not defined. Cannot generate md5 sums.
  @goto :EOF
)
@echo Generating md5sums for Firebird-%FBBUILD_PRODUCT_VER_STRING%-%FBBUILD_PACKAGE_NUMBER%

%GNU_TOOLCHAIN%\md5sum.exe %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_PRODUCT_VER_STRING%?%FBBUILD_PACKAGE_NUMBER%*.* > %FBBUILD_INSTALL_IMAGES%\Firebird-%FBBUILD_PRODUCT_VER_STRING%-%FBBUILD_PACKAGE_NUMBER%.md5sum

::---------------
@goto :EOF


:HELP
::===
@echo.
@echo.
@echo   Parameters can be passed in any order.
@echo   Currently the recognised params are:
@echo.
@echo       DEBUG  Use binaries from 'debug' dir, not 'release' dir.
@echo              (Requires a debug build. NOTE: A debug build is
@echo               not required to create packages with debug info.)
@echo.
@echo       PDB    Include pdb files.
@echo              (These files roughly double the size of the package.)
@echo.
@echo       ISX    Create installable binary from InnoSetup Extensions compiler.
@echo              (You need to set the INNO5_SETUP_PATH environment variable.)
@echo.
@echo       ZIP    Create Zip package.
@echo              (SEVENZIP is currently used and the SEVENZIP env var must be set.)
@echo.
::@echo       EMB    Create Embedded package.
::@echo              (SEVENZIP is currently used and the SEVENZIP env var must be set.)
::@echo.
::@echo       ALL    Build InnoSetup, Zip and Embedded packages.
@echo       ALL    Build InnoSetup and Zip packages.
@echo.
@echo       HELP   This help screen.
@echo.
@echo   In addition, the following environment variables are checked:
@echo.
@echo     FB2_ISS_DEBUG=1 - Prepare an InnoSetup script that is
@echo                       easier to debug
@echo.
@echo     FB2_EXAMPLES=0  - Don't include examples in the install kit.
@echo.
@echo.
@echo   Required Files
@echo.
@echo     To successfully package Firebird you will need to make sure several
@echo     packages are installed and correctly configured on your system.
@echo.
@echo     o InnoSetup is needed to create the binary installer. See the header
@echo       of the .iss file to see which minimum version is required.
@echo.
::@echo     o 7ZIP is required to create the zip and embedded packages
@echo     o 7ZIP is required to create the zip package
@echo.
@echo     o sed is required for packaging. Use the sed provided by
@echo       gnuwin32. The cygwin one is not guaranteed to work.
@echo.

::End of HELP
::-----------
@goto :EOF


:ERROR
::====
@echo.
@echo   Error in BuildExecutableInstall
@echo     %*
@echo.
popd
:: Attempt to execute a phony command. This will ensure
:: that ERRORLEVEL is set on exit.
cancel_script > nul 2>&1
:: And set ERRLEV in case we are called by run_all.bat
set ERRLEV=1
::End of ERROR
::------------
@goto :EOF


:WARNING
::======
@echo.
@echo   **** WARNING - Execution of a non-critical component failed.
@echo   %*
@echo.
if "%FBBUILD_PROD_STATUS%"=="PROD" (
@echo.
@echo   Production status is Final or Release Candidate
@echo   Error must be fixed before continuing
@echo.
cancel_script > nul 2>&1
) else (
@set | findstr win > nul 2>&1
)
@goto :EOF


:MAIN
::====

::Check if on-line help is required
for %%v in ( %1 %2 %3 %4 %5 %6 %7 %8 %9 )  do (
  ( @if /I "%%v"=="-h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="/h" (goto :HELP & goto :EOF) )
  ( @if /I "%%v"=="HELP" (goto :HELP & goto :EOF) )
)

pushd ..\..\..\win32
::This must be called from the directory it resides in.
@call setenvvar.bat
popd
@if errorlevel 1 (goto :END)

@if not defined FB2_ISS_DEBUG (set FB2_ISS_DEBUG=0)
@if not defined FB2_EXAMPLES (set FB2_EXAMPLES=1)

@Echo.
@Echo   Reading command-line parameters...
@(@call :SET_PARAMS %* )
@if "%ERRLEV%"=="1" (goto :ERROR %errorlevel% calling SET_PARAMS )

@Echo.
@Echo   Checking that all required components are available...
@(@call :CHECK_ENVIRONMENT ) || (@echo Error calling CHECK_ENVIRONMENT & @goto :EOF)
@Echo.

@Echo   Setting version number...
@(@call :SED_MAGIC ) || (@echo Error calling SED_MAGIC & @goto :EOF)
@Echo.

@Echo   Copying additional files needed for installation, documentation etc.
@(@call :COPY_XTRA ) || (@echo Error calling COPY_XTRA & @goto :EOF)
@Echo.

:: WIX is not necessary for a snapshot build, so we don't throw
:: an error if WIX is not defined. On the other hand,
:: if it is there anyway, use it.
if defined WIX (
@Echo   Building MSI runtimes
@(@call :BUILD_CRT_MSI ) || (@echo Error calling BUILD_CRT_MSI & @goto :EOF)
@Echo.
)

@Echo   Concatenating header files for ibase.h
@(@call :IBASE_H ) || (@echo Error calling IBASE_H & @goto :EOF)
@Echo.

@Echo   Writing databases conf
@(@call :DB_CONF ) || (@echo Error calling DB_CONF & @goto :EOF)
@Echo.
@Echo   Copying miscellany such as the QLI help database
@(@call :MISC ) || (@echo Error calling MISC & @goto :EOF)
@Echo.
@Echo   Copying firebird.msg
@(@call :FB_MSG ) || (@echo Error calling FB_MSG & @goto :EOF)
@Echo.

if %FBBUILD_EMB_PACK% EQU 1 (
@Echo   Generating image of embedded install
@(@call :GEN_EMBEDDED ) || (@echo Error calling GEN_EMBEDDED & @goto :EOF)
@Echo.
)

if %FBBUILD_ZIP_PACK% EQU 1 (
@echo   Generating image of zipped install
@(@call :GEN_ZIP ) || (@echo Error calling GEN_ZIP & @goto :EOF)
@echo.
)

::@Echo Creating .local files for libraries
::@(@call :TOUCH_LOCAL ) || (@echo Error calling TOUCH_LOCAL & @goto :EOF)
::@Echo.

@(@call :TOUCH_ALL ) || (@echo Error calling TOUCH_ALL & @goto :EOF)
@echo.

if %FBBUILD_ZIP_PACK% EQU 1 (
@echo   Zipping files for zip pack
@(@call :ZIP_PACK ) || (@echo Error calling ZIP_PACK & @goto :EOF)
@echo.
)

if %FBBUILD_EMB_PACK% EQU 1 (
@echo   Zipping files for embedded pack
@(@call :EMB_PACK ) || (@echo Error calling EMB_PACK & @goto :EOF)
@echo.
)

if %FBBUILD_ISX_PACK% EQU 1 (
@(@call :ISX_PACK ) || (@echo Error calling ISX_PACK & @goto :EOF)
@echo.
)

@(@call :DO_MD5SUMS ) || (@echo Error calling DO_MD5SUMS & @goto :EOF)


@echo.
@echo Completed building installation kit(s)
@echo.

::@if %FB2_ISS_DEBUG% equ 0 (ENDLOCAL)
::End of MAIN
::-----------
@goto :EOF


:END



