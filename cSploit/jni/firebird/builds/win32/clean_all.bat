@echo off

@call setenvvar.bat
@if errorlevel 1 (goto :END)
@echo Cleaning temporary directories...
@rmdir /S /Q %FB_OUTPUT_DIR% 2>nul
@rmdir /S /Q %FB_TEMP_DIR% 2>nul

@echo Cleaning gen...
@rmdir /S /Q %FB_GEN_DIR% 2>nul

@echo Cleaning INTL...
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\%FB_TARGET_PLATFORM%\release\bin 2>nul
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\%FB_TARGET_PLATFORM%\release\lib 2>nul
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\%FB_TARGET_PLATFORM%\release 2>nul
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\%FB_TARGET_PLATFORM% 2>nul

:: Removing this might screw up parallel builds.
:: So let's be sure we mean it.
if defined FB_CLEAN_SHARED (
@echo Cleaning INTL Shared dirs...
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\include 2>nul
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\source\data\out 2>nul
@del /Q %FB_ROOT_PATH%\extern\icu\source\extra\uconv\resources\*.res 2>nul
@del %FB_ROOT_PATH%\extern\icu\source\extra\uconv\pkgdatain.txt 2>nul
@del %FB_ROOT_PATH%\extern\icu\source\stubdata\stubdatabuilt.txt 2>nul
@rmdir /S /Q %FB_ROOT_PATH%\extern\icu\source\test\testdata\out 2>nul
)

:: This really does clean the icu stuff - but is it too much? Cleaning less
:: thoroughly than this speeds up the build process if a previous build has been
:: made for the same target.
if defined FB_INTLREALCLEAN (
@echo Thoroughly scrubbing all remnants of INTL release dirs...
for /R %FB_ROOT_PATH%\extern\icu\source\ %%a in (release) do (rmdir /q /s "%%a" 2>nul)
set FB_INTLREALCLEAN=
)


@echo Cleaning install and build files...
@del *%FB_TARGET_PLATFORM%.log 2>nul
@del *.manifest 2>nul


if defined FB_CLEAN_SHARED (
@echo Cleaning shared gen and dsql files...
@del %FB_ROOT_PATH%\src\include\gen\autoconfig.h 2>nul
@del %FB_ROOT_PATH%\src\dsql\dsql.tab.h 2>nul
@del %FB_ROOT_PATH%\src\dsql\parse.cpp 2>nul
)

@del %FB_ROOT_PATH%\builds\win32\msvc6\*.plg 2>nul
@rmdir /s /q %FB_ROOT_PATH%\builds\win32\install_image 2>nul


@echo Completed executing %0
@echo.

