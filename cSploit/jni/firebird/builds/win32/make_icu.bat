@echo off


:: Set env vars
@call setenvvar.bat

@call set_build_target.bat %* icu

::==========
:: MAIN

@echo Extracting pre-built ICU

%FB_ROOT_PATH%\extern\icu\icu.exe -y > make_icu_%FB_TARGET_PLATFORM%.log

if errorlevel 1 call :ERROR build failed - see make_icu_%FB_TARGET_PLATFORM%.log for details
@goto :EOF



:ERROR
::====
@echo.
@echo   An error occurred while running make_icu.bat -
@echo     %*
@echo.
set ERRLEV=1
cancel_script > nul 2>&1
::End of ERROR
::------------
@goto :EOF
