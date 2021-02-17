@echo off

@call setenvvar.bat
@if errorlevel 1 (goto :END)
@if not defined FB_BIN_DIR (@call set_build_target.bat %*)

@if "%1"=="msg" goto MSG

@if exist "%FB_GEN_DIR%\dbs\msg.fdb" del "%FB_GEN_DIR%\dbs\msg.fdb"

@echo create database '%FB_GEN_DB_DIR%/dbs/msg.fdb'; | "%FB_BIN_DIR%\isql" -q
@set FB_MSG_ISQL=@"%FB_BIN_DIR%\isql" -q %FB_GEN_DB_DIR%/dbs/msg.fdb -i %FB_ROOT_PATH%\src\msgs\
@%FB_MSG_ISQL%msg.sql
@echo.
@echo loading facilities
@%FB_MSG_ISQL%facilities2.sql
@echo loading sql states
@%FB_MSG_ISQL%sqlstates.sql
@echo loading locales
@%FB_MSG_ISQL%locales.sql
@echo loading history
@%FB_MSG_ISQL%history2.sql
@echo loading messages
@%FB_MSG_ISQL%messages2.sql
@echo loading symbols
@%FB_MSG_ISQL%symbols2.sql
@echo loading system errors
@%FB_MSG_ISQL%system_errors2.sql
@echo loading French translation
@%FB_MSG_ISQL%transmsgs.fr_FR2.sql
@echo loading German translation
@%FB_MSG_ISQL%transmsgs.de_DE2.sql

@if "%1"=="db" goto END

:MSG
@echo Building message file...
::@%FB_BIN_DIR%\build_msg -D %FB_GEN_DB_DIR%\dbs\msg.fdb -p %FB_GEN_DB_DIR% -f firebird.msg -L all
@%FB_BIN_DIR%\build_msg -D %FB_GEN_DB_DIR%\dbs\msg.fdb -p %FB_GEN_DB_DIR% -f firebird.msg
@copy %FB_GEN_DIR%\firebird.msg %FB_BIN_DIR% > nul

@echo Building codes header...
@%FB_BIN_DIR%\codes %FB_ROOT_PATH%\src\include\gen %FB_ROOT_PATH%\lang_helpers

:END
