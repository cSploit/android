@echo off
setlocal
set REMOVE_SERVICE=remove -z
set STOP_SERVICE=stop
if not "%1"=="" (
set STOP_SERVICE=%STOP_SERVICE% -n %1
set REMOVE_SERVICE=%REMOVE_SERVICE% -n %1
)
instsvc %STOP_SERVICE%
instsvc %REMOVE_SERVICE%
endlocal

if "%1"=="" (instreg remove -z)
