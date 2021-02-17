@rem Script to build libnet under "Visual Studio .NET Command Prompt".
@rem Dependencies are:
@rem   winpcap, specifically, the winpcap developer pack

@setlocal
@set MYCOMPILE=cl /nologo /MD /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE
@set MYLINK=link /nologo
@set MYMT=mt /nologo

@rem relative to C code in src/
@set WINPCAP=..\..\..\WpdPack

copy win32\libnet.h include\
copy win32\stdint.h include\libnet\
copy win32\config.h include\
copy win32\getopt.h include\

cd src
%MYCOMPILE% /I..\include /I%WINPCAP%\Include libnet_a*.c libnet_build_*.c libnet_c*.c libnet_dll.c libnet_error.c libnet_i*.c libnet_link_win32.c libnet_p*.c libnet_raw.c libnet_resolve.c libnet_version.c libnet_write.c
%MYLINK% /DLL /libpath:%WINPCAP%\Lib  /out:libnet.dll *.obj Advapi32.lib
if exist libnet.dll.manifest^
  %MYMT% -manifest libnet.dll.manifest -outputresource:libnet.dll;2
cd ..

exit /b %errorlevel%

