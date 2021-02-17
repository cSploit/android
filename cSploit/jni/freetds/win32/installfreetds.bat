@echo off
REM This shell script installs the FreeTDS ODBC driver
set dest=%windir%\System
IF EXIST %windir%\System32\ODBCCONF.exe SET dest=%windir%\System32
ECHO Installing to %dest%
COPY /Y Debug\FreeTDS.dll %dest%\FreeTDS.dll
CD %dest%
%dest%\ODBCCONF INSTALLDRIVER "FreeTDS|Driver=FreeTDS.dll|Setup=FreeTDS.dll||"
%dest%\ODBCCONF CONFIGDRIVER "FreeTDS" "APILevel=1" 
%dest%\ODBCCONF CONFIGDRIVER "FreeTDS" "ConnectFunctions=YYY" 
%dest%\ODBCCONF CONFIGDRIVER "FreeTDS" "DriverODBCVer=02.50"
%dest%\ODBCCONF CONFIGDRIVER "FreeTDS" "FileUsage=0" 
%dest%\ODBCCONF CONFIGDRIVER "FreeTDS" "SQLLevel=1" 
%dest%\ODBCCONF CONFIGDRIVER "FreeTDS" "CPTimeout=60"
