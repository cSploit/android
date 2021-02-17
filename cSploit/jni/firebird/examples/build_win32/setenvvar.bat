:: This bat set the environment values
:: ROOT_PATH dos format path of the main directory 
:: DB_PATH unix format path of the main directory 
:: VS_VER VisualStudio version (msvc6|msvc7)

@echo off
::=================
:SET_DB_DIR

@cd ..\..
@for /f "delims=" %%a in ('@cd') do (set ROOT_PATH=%%a)
@cd %~dp0
for /f "tokens=*" %%a in ('@echo %ROOT_PATH:\=/%') do (set DB_PATH=%%a)

@msdev /? >nul 2>nul
@if not errorlevel 9009 ((set VS_VER=msvc6) & (goto :END))

@devenv /? >nul 2>nul
@if not errorlevel 9009 ((set VS_VER=msvc7) & (goto :END))

::===========
:HELP
@echo.
@echo    ERROR: There are not a visual studio valid version in your path. 
@echo    You need visual studio 6 or 7 to build Firebird
@echo. 
:: set errorlevel
@exit /B 1

:END
@echo.
@echo    vs_ver=%VS_VER%
@echo    db_path=%DB_PATH%
@echo    root_path=%ROOT_PATH%
@echo.