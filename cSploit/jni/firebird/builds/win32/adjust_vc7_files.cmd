@echo off
:: This is the helper script to convert MSVC8 build to MSVC7 build.
:: It changes build to use static CRT instead of DLL.
::
:: Use it after you converted your project files using VSPC
:: (http://sourceforge.net/projects/vspc/)
::
:: USAGE:
:: 1) adjust_vc7_files without parameters
::    fix all project files in MSVC7 direcotry
::
:: 2) adjust_vc7_files -L <filelist> 
::    fix all files in list
::
:: 3) adjust_vc7_files <filename>
::    fix a particular project file
::

if "%1" == "" (
  for %%i in (msvc7\*.vcproj) do call :adjust_files %%i

  goto :eof
)

if "%1" == "-L" (
  for /f %%i in (%2) do call :adjust_files %%i

  goto :eof
)


:adjust_files
echo Editing %1...
sed -e "s|RuntimeLibrary=\"3\"|RuntimeLibrary=\"1\"|" %1 > %1.sed_tmp
sed -e "s|RuntimeLibrary=\"2\"|RuntimeLibrary=\"0\"|" %1.sed_tmp > %1
del %1.sed_tmp
goto :eof
