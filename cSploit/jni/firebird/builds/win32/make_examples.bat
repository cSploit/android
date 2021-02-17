::@echo off


:: Set env vars
@call setenvvar.bat
@if errorlevel 1 (goto :EOF)

@call set_build_target.bat %*

:: verify that boot was run before
@if not exist %FB_BIN_DIR%\isql.exe (goto :HELP_BOOT & goto :EOF)
@if not exist %FB_BIN_DIR%\gpre.exe (goto :HELP_BOOT & goto :EOF)
@if not exist %FB_BIN_DIR%\fbclient.dll (goto :HELP_BOOT & goto :EOF)

::Uncomment this to build intlemp
::set FB2_INTLEMP=1

::===========
:MAIN
@echo BUILD_EMPBUILD
@call :BUILD_EMPBUILD

@echo.
@echo Building %FB_OBJ_DIR%
if "%VS_VER%"=="msvc6" (
    @call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird2 examples_%FB_TARGET_PLATFORM%.log empbuild intlbld
) else (
    @call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3_Examples empbuild_%FB_TARGET_PLATFORM%.log empbuild
    @if defined FB2_INTLEMP (
      @call compile.bat %FB_ROOT_PATH%\builds\win32\%VS_VER%\Firebird3_Examples intlbuild_%FB_TARGET_PLATFORM%.log intlbuild
    )
)
@echo.
@call :MOVE
@call :BUILD_EMPLOYEE
@call :MOVE2
@goto :EOF

::===========
:BUILD_EMPBUILD
@echo.
@echo Building empbuild.fdb
@copy /y %FB_ROOT_PATH%\examples\empbuild\*.sql   %FB_GEN_DIR%\examples\ > nul
@copy /y %FB_ROOT_PATH%\examples\empbuild\*.inp   %FB_GEN_DIR%\examples\ > nul

@echo.
:: Here we must use cd because isql does not have an option to set a base directory
@cd %FB_GEN_DIR%\examples
@echo   Creating empbuild.fdb...
@echo.
@del empbuild.fdb 2> nul
@%FB_BIN_DIR%\isql -i empbld.sql


if defined FB2_INTLEMP (
@echo   Creating intlbuild.fdb...
@echo.
@copy %FB_ROOT_PATH%\builds\install\misc\fbintl.conf %FB_BIN_DIR%\intl >nul
@del intlbuild.fdb 2> nul
@%FB_BIN_DIR%\isql -i intlbld.sql
)

@cd %FB_ROOT_PATH%\builds\win32
@echo.
@echo path = %FB_GEN_DB_DIR%\examples
@echo   Preprocessing empbuild.e...
@echo.
@%FB_BIN_DIR%\gpre.exe -r -m -n -z %FB_ROOT_PATH%\examples\empbuild\empbuild.e %FB_GEN_DIR%\examples\empbuild.c -b %FB_GEN_DB_DIR%/examples/

if defined FB2_INTLEMP (
@echo   Preprocessing intlbld.e...
@echo.
@%FB_BIN_DIR%\gpre.exe -r -m -n -z %FB_ROOT_PATH%\examples\empbuild\intlbld.e %FB_GEN_DIR%\examples\intlbld.c -b %FB_GEN_DB_DIR%/examples/
)

@goto :EOF


::===========
:MOVE
@echo.
@rmdir /q /s %FB_OUTPUT_DIR%\examples 2>nul
@mkdir %FB_OUTPUT_DIR%\examples
@mkdir %FB_OUTPUT_DIR%\examples\api
@mkdir %FB_OUTPUT_DIR%\examples\dbcrypt
@mkdir %FB_OUTPUT_DIR%\examples\build_unix
@mkdir %FB_OUTPUT_DIR%\examples\build_win32
@mkdir %FB_OUTPUT_DIR%\examples\empbuild
@mkdir %FB_OUTPUT_DIR%\examples\include
@mkdir %FB_OUTPUT_DIR%\examples\interfaces
@mkdir %FB_OUTPUT_DIR%\examples\package
@mkdir %FB_OUTPUT_DIR%\examples\stat
@mkdir %FB_OUTPUT_DIR%\examples\udf
@mkdir %FB_OUTPUT_DIR%\examples\udr

@echo Moving files to output directory
@copy %FB_ROOT_PATH%\examples\* %FB_OUTPUT_DIR%\examples > nul
@ren %FB_OUTPUT_DIR%\examples\readme readme.txt > nul
@copy %FB_ROOT_PATH%\examples\api\* %FB_OUTPUT_DIR%\examples\api > nul
@copy %FB_ROOT_PATH%\examples\dbcrypt\* %FB_OUTPUT_DIR%\examples\dbcrypt > nul
@copy %FB_ROOT_PATH%\examples\build_unix\* %FB_OUTPUT_DIR%\examples\build_unix > nul
@copy %FB_ROOT_PATH%\examples\build_win32\* %FB_OUTPUT_DIR%\examples\build_win32 > nul
:: @copy %FB_ROOT_PATH%\examples\empbuild\* %FB_OUTPUT_DIR%\examples\empbuild > nul
@copy %FB_ROOT_PATH%\examples\empbuild\employe2.sql %FB_OUTPUT_DIR%\examples\empbuild > nul
@copy %FB_ROOT_PATH%\examples\include\* %FB_OUTPUT_DIR%\examples\include > nul
@copy %FB_ROOT_PATH%\examples\interfaces\* %FB_OUTPUT_DIR%\examples\interfaces > nul
@copy %FB_ROOT_PATH%\examples\package\* %FB_OUTPUT_DIR%\examples\package > nul
@copy %FB_ROOT_PATH%\examples\stat\* %FB_OUTPUT_DIR%\examples\stat > nul
@copy %FB_ROOT_PATH%\examples\udf\* %FB_OUTPUT_DIR%\examples\udf > nul
@copy %FB_ROOT_PATH%\examples\udr\* %FB_OUTPUT_DIR%\examples\udr > nul
@copy %FB_ROOT_PATH%\src\extlib\ib_udf* %FB_OUTPUT_DIR%\examples\udf > nul
@copy %FB_ROOT_PATH%\src\extlib\fbudf\* %FB_OUTPUT_DIR%\examples\udf > nul

::@copy %FB_GEN_DIR%\examples\empbuild.c %FB_OUTPUT_DIR%\examples\empbuild\ > nul
::@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\examples\empbuild.exe %FB_GEN_DIR%\examples\empbuild.exe > nul
::if defined FB2_INTLEMP (
::if "%VS_VER%"=="msvc6" (
::@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\examples\intlbld.exe %FB_GEN_DIR%\examples\intlbuild.exe > nul
::) else (
::@copy %FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\examples\intlbuild.exe %FB_GEN_DIR%\examples\intlbuild.exe > nul
::)
::)
@goto :EOF

::===========
:: only to test if it works
:BUILD_EMPLOYEE
@echo.
@echo Building employee.fdb
:: Here we must use cd because isql does not have an option to set a base directory
:: and empbuild.exe uses isql
@set FB_SAVE_PATH=%PATH%
@set PATH=%FB_BIN_DIR%;%PATH%
:: This allows us to use the new engine in embedded mode to build
:: the employee database.
@set FIREBIRD=%FB_BIN_DIR%

@cd %FB_GEN_DIR%\examples
@del %FB_GEN_DIR%\examples\employee.fdb 2>nul
@%FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\empbuild\empbuild.exe %FB_GEN_DB_DIR%/examples/employee.fdb

@if defined FB2_INTLEMP (
@echo Building intlemp.fdb
  @del %FB_GEN_DIR%\examples\intlemp.fdb 2>nul
  @del isql.tmp 2>nul
  @echo s;intlemp.fdb;%SERVER_NAME%:%FB_GEN_DIR%\examples\intlemp.fdb;g > isql.tmp
  @%FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\intlbuild\intlbuild.exe %FB_GEN_DB_DIR%/examples/intlemp.fdb
)
@set FIREBIRD=
@set PATH=%FB_SAVE_PATH%
@cd %FB_ROOT_PATH%\builds\win32

@goto :EOF

::==============
:MOVE2
@copy %FB_GEN_DIR%\examples\employee.fdb %FB_OUTPUT_DIR%\examples\empbuild\ > nul

if defined FB2_INTLEMP (
  if exist %FB_GEN_DIR%\examples\intlemp.fdb (
  @copy %FB_GEN_DIR%\examples\intlemp.fdb %FB_OUTPUT_DIR%\examples\empbuild\ > nul
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
@echo   Error  - %*
@echo.
cancel_script > nul 2>&1
::End of ERROR
::------------
@goto :EOF

