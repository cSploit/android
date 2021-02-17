@echo off
set FB_DBG=
set FB_OBJ_DIR=release
set FB_CLEAN=/build
set FB_ICU=
set FB_VC10CRT_DIR=%FB_PROCESSOR_ARCHITECTURE%

for %%v in ( %* )  do (
  ( if /I "%%v"=="DEBUG" ( (set FB_DBG=TRUE) && (set FB_OBJ_DIR=debug) ) )
  ( if /I "%%v"=="CLEAN" (set FB_CLEAN=/rebuild) )
  ( if /I "%%v"=="ICU" ( (set FB_ICU=1) && (set FB_DBG=) ) )
  ( if /I "%%v"=="LIBTOMMATH" (set FB_DBG=) )
)

if %MSVC_VERSION% GEQ 7 (set FB_OBJ_DIR=%FB_TARGET_PLATFORM%\%FB_OBJ_DIR%)
if %MSVC_VERSION% == 10 ( if %FB_VC10CRT_DIR% == AMD64 ( set FB_VC10CRT_DIR=x64))

@set FB_BIN_DIR=%FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\firebird\

@echo    Executed %0
@echo.
