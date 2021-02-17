@ECHO OFF

::
:: build-all-msvc.bat --
::
:: Multi-Platform Build Tool for MSVC
::

REM
REM This batch script is used to build the SQLite DLL for multiple platforms
REM and configurations using MSVC.  The built SQLite DLLs, their associated
REM import libraries, and optionally their symbols files, are placed within
REM the directory specified on the command line, in sub-directories named for
REM their respective platforms and configurations.  This batch script must be
REM run from inside a Visual Studio Command Prompt for the desired version of
REM Visual Studio ^(the initial platform configured for the command prompt does
REM not really matter^).  Exactly one command line argument is required, the
REM name of an existing directory to be used as the final destination directory
REM for the generated output files, which will be placed in sub-directories
REM created therein.  Ideally, the directory specified should be empty.
REM
REM Example:
REM
REM                        CD /D C:\dev\sqlite\core
REM                        tool\build-all-msvc.bat C:\Temp
REM
REM In the example above, "C:\dev\sqlite\core" represents the root of the
REM source tree for SQLite and "C:\Temp" represents the final destination
REM directory for the generated output files.
REM
REM There are several environment variables that may be set to modify the
REM behavior of this batch script and its associated Makefile.  The list of
REM platforms to build may be overriden by using the PLATFORMS environment
REM variable, which should contain a list of platforms ^(e.g. x86 x86_amd64
REM x86_arm^).  All platforms must be supported by the version of Visual Studio
REM being used.  The list of configurations to build may be overridden by
REM setting the CONFIGURATIONS environment variable, which should contain a
REM list of configurations to build ^(e.g. Debug Retail^).  Neither of these
REM variable values may contain any double quotes, surrounding or embedded.
REM Finally, the NCRTLIBPATH and NSDKLIBPATH environment variables may be set
REM to specify the location of the CRT and SDK, respectively, needed to compile
REM executables native to the architecture of the build machine during any
REM cross-compilation that may be necessary, depending on the platforms to be
REM built.  These values in these two variables should be surrounded by double
REM quotes if they contain spaces.
REM
REM Please note that the SQLite build process performed by the Makefile
REM associated with this batch script requires both Gawk ^(gawk.exe^) and Tcl
REM 8.5 ^(tclsh85.exe^) to be present in a directory contained in the PATH
REM environment variable unless a pre-existing amalgamation file is used.
REM
SETLOCAL

REM SET __ECHO=ECHO
REM SET __ECHO2=ECHO
REM SET __ECHO3=ECHO
IF NOT DEFINED _AECHO (SET _AECHO=REM)
IF NOT DEFINED _CECHO (SET _CECHO=REM)
IF NOT DEFINED _VECHO (SET _VECHO=REM)

%_AECHO% Running %0 %*

REM SET DFLAGS=/L

%_VECHO% DFlags = '%DFLAGS%'

SET FFLAGS=/V /F /G /H /I /R /Y /Z

%_VECHO% FFlags = '%FFLAGS%'

SET ROOT=%~dp0\..
SET ROOT=%ROOT:\\=\%

%_VECHO% Root = '%ROOT%'

REM
REM NOTE: The first and only argument to this batch file should be the output
REM       directory where the platform-specific binary directories should be
REM       created.
REM
SET BINARYDIRECTORY=%1

IF NOT DEFINED BINARYDIRECTORY (
  GOTO usage
)

%_VECHO% BinaryDirectory = '%BINARYDIRECTORY%'

SET DUMMY=%2

IF DEFINED DUMMY (
  GOTO usage
)

REM
REM NOTE: From this point, we need a clean error level.  Reset it now.
REM
CALL :fn_ResetErrorLevel

REM
REM NOTE: Change the current directory to the root of the source tree, saving
REM       the current directory on the directory stack.
REM
%__ECHO2% PUSHD "%ROOT%"

IF ERRORLEVEL 1 (
  ECHO Could not change directory to "%ROOT%".
  GOTO errors
)

REM
REM NOTE: This batch file requires the ComSpec environment variable to be set,
REM       typically to something like "C:\Windows\System32\cmd.exe".
REM
IF NOT DEFINED ComSpec (
  ECHO The ComSpec environment variable must be defined.
  GOTO errors
)

REM
REM NOTE: This batch file requires the VcInstallDir environment variable to be
REM       set.  Tyipcally, this means this batch file needs to be run from an
REM       MSVC command prompt.
REM
IF NOT DEFINED VCINSTALLDIR (
  ECHO The VCINSTALLDIR environment variable must be defined.
  GOTO errors
)

REM
REM NOTE: If the list of platforms is not already set, use the default list.
REM
IF NOT DEFINED PLATFORMS (
  SET PLATFORMS=x86 x86_amd64 x86_arm
)

%_VECHO% Platforms = '%PLATFORMS%'

REM
REM NOTE: If the list of configurations is not already set, use the default
REM       list.
REM
IF NOT DEFINED CONFIGURATIONS (
  SET CONFIGURATIONS=Debug Retail
)

%_VECHO% Configurations = '%CONFIGURATIONS%'

REM
REM NOTE: Setup environment variables to translate between the MSVC platform
REM       names and the names to be used for the platform-specific binary
REM       directories.
REM
SET amd64_NAME=x64
SET arm_NAME=ARM
SET x64_NAME=x64
SET x86_NAME=x86
SET x86_amd64_NAME=x64
SET x86_arm_NAME=ARM
SET x86_x64_NAME=x64

%_VECHO% amd64_Name = '%amd64_NAME%'
%_VECHO% arm_Name = '%arm_NAME%'
%_VECHO% x64_Name = '%x64_NAME%'
%_VECHO% x86_Name = '%x86_NAME%'
%_VECHO% x86_amd64_Name = '%x86_amd64_NAME%'
%_VECHO% x86_arm_Name = '%x86_arm_NAME%'
%_VECHO% x86_x64_Name = '%x86_x64_NAME%'

REM
REM NOTE: Check for the external tools needed during the build process ^(i.e.
REM       those that do not get compiled as part of the build process itself^)
REM       along the PATH.
REM
FOR %%T IN (gawk.exe tclsh85.exe) DO (
  SET %%T_PATH=%%~dp$PATH:T
)

REM
REM NOTE: The Gawk executable "gawk.exe" is required during the SQLite build
REM       process unless a pre-existing amalgamation file is used.
REM
IF NOT DEFINED gawk.exe_PATH (
  ECHO The Gawk executable "gawk.exe" is required to be in the PATH.
  GOTO errors
)

REM
REM NOTE: The Tcl 8.5 executable "tclsh85.exe" is required during the SQLite
REM       build process unless a pre-existing amalgamation file is used.
REM
IF NOT DEFINED tclsh85.exe_PATH (
  ECHO The Tcl 8.5 executable "tclsh85.exe" is required to be in the PATH.
  GOTO errors
)

REM
REM NOTE: Set the TOOLPATH variable to contain all the directories where the
REM       external tools were found in the search above.
REM
SET TOOLPATH=%gawk.exe_PATH%;%tclsh85.exe_PATH%

%_VECHO% ToolPath = '%TOOLPATH%'

REM
REM NOTE: Check for MSVC 2012 because the Windows SDK directory handling is
REM       slightly different for that version.
REM
IF "%VisualStudioVersion%" == "11.0" (
  REM
  REM NOTE: If the Windows SDK library path has already been set, do not set
  REM       it to something else later on.
  REM
  IF NOT DEFINED NSDKLIBPATH (
    SET SET_NSDKLIBPATH=1
  )
) ELSE (
  CALL :fn_UnsetVariable SET_NSDKLIBPATH
)

REM
REM NOTE: Check if this is the Windows Phone SDK.  If so, a different batch
REM       file is necessary to setup the build environment.  Since the variable
REM       values involved here may contain parenthesis, using GOTO instead of
REM       an IF block is required.
REM
IF DEFINED WindowsPhoneKitDir GOTO set_vcvarsall_phone
SET VCVARSALL=%VCINSTALLDIR%\vcvarsall.bat
GOTO set_vcvarsall_done
:set_vcvarsall_phone
SET VCVARSALL=%VCINSTALLDIR%\WPSDK\WP80\vcvarsphoneall.bat
:set_vcvarsall_done

REM
REM NOTE: This is the outer loop.  There should be exactly one iteration per
REM       platform.
REM
FOR %%P IN (%PLATFORMS%) DO (
  REM
  REM NOTE: Using the MSVC platform name, lookup the simpler platform name to
  REM       be used for the name of the platform-specific binary directory via
  REM       the environment variables setup earlier.
  REM
  CALL :fn_CopyVariable %%P_NAME PLATFORMNAME

  REM
  REM NOTE: This is the inner loop.  There should be exactly one iteration.
  REM       This loop is necessary because the PlatformName environment
  REM       variable was set above and that value is needed by some of the
  REM       commands contained in the inner loop.  If these commands were
  REM       directly contained in the outer loop, the PlatformName environment
  REM       variable would be stuck with its initial empty value instead.
  REM
  FOR /F "tokens=2* delims==" %%D IN ('SET PLATFORMNAME') DO (
    REM
    REM NOTE: Attempt to clean the environment of all variables used by MSVC
    REM       and/or Visual Studio.  This block may need to be updated in the
    REM       future to account for additional environment variables.
    REM
    CALL :fn_UnsetVariable DevEnvDir
    CALL :fn_UnsetVariable ExtensionSdkDir
    CALL :fn_UnsetVariable Framework35Version
    CALL :fn_UnsetVariable FrameworkDir
    CALL :fn_UnsetVariable FrameworkDir32
    CALL :fn_UnsetVariable FrameworkVersion
    CALL :fn_UnsetVariable FrameworkVersion32
    CALL :fn_UnsetVariable FSHARPINSTALLDIR
    CALL :fn_UnsetVariable INCLUDE
    CALL :fn_UnsetVariable LIB
    CALL :fn_UnsetVariable LIBPATH
    CALL :fn_UnsetVariable Platform
    REM CALL :fn_UnsetVariable VCINSTALLDIR
    CALL :fn_UnsetVariable VSINSTALLDIR
    CALL :fn_UnsetVariable WindowsPhoneKitDir
    CALL :fn_UnsetVariable WindowsSdkDir
    CALL :fn_UnsetVariable WindowsSdkDir_35
    CALL :fn_UnsetVariable WindowsSdkDir_old

    REM
    REM NOTE: Reset the PATH here to the absolute bare minimum required.
    REM
    SET PATH=%TOOLPATH%;%SystemRoot%\System32;%SystemRoot%

    FOR %%B IN (%CONFIGURATIONS%) DO (
      REM
      REM NOTE: When preparing the debug build, set the DEBUG and MEMDEBUG
      REM       environment variables to be picked up by the MSVC makefile
      REM       itself.
      REM
      IF /I "%%B" == "Debug" (
        SET DEBUG=2
        SET MEMDEBUG=1
      ) ELSE (
        CALL :fn_UnsetVariable DEBUG
        CALL :fn_UnsetVariable MEMDEBUG
      )

      REM
      REM NOTE: Launch a nested command shell to perform the following steps:
      REM
      REM       1. Setup the MSVC environment for this platform using the
      REM          official batch file.
      REM
      REM       2. Make sure that no stale build output files are present.
      REM
      REM       3. Build the "sqlite3.dll" and "sqlite3.lib" binaries for this
      REM          platform.
      REM
      REM       4. Copy the "sqlite3.dll" and "sqlite3.lib" binaries for this
      REM          platform to the platform-specific directory beneath the
      REM          binary directory.
      REM
      "%ComSpec%" /C (
        REM
        REM NOTE: Attempt to setup the MSVC environment for this platform.
        REM
        %__ECHO3% CALL "%VCVARSALL%" %%P

        IF ERRORLEVEL 1 (
          ECHO Failed to call "%VCVARSALL%" for platform %%P.
          GOTO errors
        )

        REM
        REM NOTE: If this batch file is not running in "what-if" mode, check to
        REM       be sure we were actually able to setup the MSVC environment
        REM       as current versions of their official batch file do not set
        REM       the exit code upon failure.
        REM
        IF NOT DEFINED __ECHO3 (
          IF NOT DEFINED WindowsPhoneKitDir (
            IF NOT DEFINED WindowsSdkDir (
              ECHO Cannot build, Windows SDK not found for platform %%P.
              GOTO errors
            )
          )
        )

        REM
        REM NOTE: When using MSVC 2012, the native SDK path cannot simply use
        REM       the "lib" sub-directory beneath the location specified in the
        REM       WindowsSdkDir environment variable because that location does
        REM       not actually contain the necessary library files for x86.
        REM       This must be done for each iteration because it relies upon
        REM       the WindowsSdkDir environment variable being set by the batch
        REM       file used to setup the MSVC environment.
        REM
        IF DEFINED SET_NSDKLIBPATH (
          IF DEFINED WindowsPhoneKitDir (
            CALL :fn_CopyVariable WindowsPhoneKitDir NSDKLIBPATH
            CALL :fn_AppendVariable NSDKLIBPATH \lib\x86
          ) ELSE IF DEFINED WindowsSdkDir (
            CALL :fn_CopyVariable WindowsSdkDir NSDKLIBPATH
            CALL :fn_AppendVariable NSDKLIBPATH \lib\win8\um\x86
          )
        )

        REM
        REM NOTE: Unless prevented from doing so, invoke NMAKE with the MSVC
        REM       makefile to clean any stale build output from previous
        REM       iterations of this loop and/or previous runs of this batch
        REM       file, etc.
        REM
        IF NOT DEFINED NOCLEAN (
          %__ECHO% nmake -f Makefile.msc clean

          IF ERRORLEVEL 1 (
            ECHO Failed to clean for platform %%P.
            GOTO errors
          )
        ) ELSE (
          REM
          REM NOTE: Even when the cleaning step has been disabled, we still
          REM       need to remove the build output for the files we are
          REM       specifically wanting to build for each platform.
          REM
          %__ECHO% DEL /Q sqlite3.dll sqlite3.lib sqlite3.pdb
        )

        REM
        REM NOTE: Call NMAKE with the MSVC makefile to build the "sqlite3.dll"
        REM       binary.  The x86 compiler will be used to compile the native
        REM       command line tools needed during the build process itself.
        REM       Also, disable looking for and/or linking to the native Tcl
        REM       runtime library.
        REM
        %__ECHO% nmake -f Makefile.msc sqlite3.dll XCOMPILE=1 USE_NATIVE_LIBPATHS=1 NO_TCL=1 %NMAKE_ARGS%

        IF ERRORLEVEL 1 (
          ECHO Failed to build %%B "sqlite3.dll" for platform %%P.
          GOTO errors
        )

        REM
        REM NOTE: Copy the "sqlite3.dll" file to the appropriate directory for
        REM       the build and platform beneath the binary directory.
        REM
        %__ECHO% XCOPY sqlite3.dll "%BINARYDIRECTORY%\%%B\%%D\" %FFLAGS% %DFLAGS%

        IF ERRORLEVEL 1 (
          ECHO Failed to copy "sqlite3.dll" to "%BINARYDIRECTORY%\%%B\%%D\".
          GOTO errors
        )

        REM
        REM NOTE: Copy the "sqlite3.lib" file to the appropriate directory for
        REM       the build and platform beneath the binary directory.
        REM
        %__ECHO% XCOPY sqlite3.lib "%BINARYDIRECTORY%\%%B\%%D\" %FFLAGS% %DFLAGS%

        IF ERRORLEVEL 1 (
          ECHO Failed to copy "sqlite3.lib" to "%BINARYDIRECTORY%\%%B\%%D\".
          GOTO errors
        )

        REM
        REM NOTE: Copy the "sqlite3.pdb" file to the appropriate directory for
        REM       the build and platform beneath the binary directory unless we
        REM       are prevented from doing so.
        REM
        IF NOT DEFINED NOSYMBOLS (
          %__ECHO% XCOPY sqlite3.pdb "%BINARYDIRECTORY%\%%B\%%D\" %FFLAGS% %DFLAGS%

          IF ERRORLEVEL 1 (
            ECHO Failed to copy "sqlite3.pdb" to "%BINARYDIRECTORY%\%%B\%%D\".
            GOTO errors
          )
        )
      )
    )
  )

  REM
  REM NOTE: Handle any errors generated during the nested command shell.
  REM
  IF ERRORLEVEL 1 (
    GOTO errors
  )
)

REM
REM NOTE: Restore the saved current directory from the directory stack.
REM
%__ECHO2% POPD

IF ERRORLEVEL 1 (
  ECHO Could not restore directory.
  GOTO errors
)

REM
REM NOTE: If we get to this point, we have succeeded.
REM
GOTO no_errors

:fn_ResetErrorLevel
  VERIFY > NUL
  GOTO :EOF

:fn_SetErrorLevel
  VERIFY MAYBE 2> NUL
  GOTO :EOF

:fn_CopyVariable
  SETLOCAL
  IF NOT DEFINED %1 GOTO :EOF
  IF "%2" == "" GOTO :EOF
  SET __ECHO_CMD=ECHO %%%1%%
  FOR /F "delims=" %%V IN ('%__ECHO_CMD%') DO (
    SET VALUE=%%V
  )
  ENDLOCAL && SET %2=%VALUE%
  GOTO :EOF

:fn_UnsetVariable
  IF NOT "%1" == "" (
    SET %1=
    CALL :fn_ResetErrorLevel
  )
  GOTO :EOF

:fn_AppendVariable
  SET __ECHO_CMD=ECHO %%%1%%
  IF DEFINED %1 (
    FOR /F "delims=" %%V IN ('%__ECHO_CMD%') DO (
      SET %1=%%V%~2
    )
  ) ELSE (
    SET %1=%~2
  )
  SET __ECHO_CMD=
  CALL :fn_ResetErrorLevel
  GOTO :EOF

:usage
  ECHO.
  ECHO Usage: %~nx0 ^<binaryDirectory^>
  ECHO.
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
