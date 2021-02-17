@echo off
map root z:=cdrom\data:ndk\sdk0007\ndk
PATH=L:\APP\BC50\BIN
rem set PGM=RETURN_BLOCK_OF_TREE
rem set PGM=vlist
rem set PGM=volres
rem set PGM=semaphor
rem set PGM=scanvolr
rem set PGM=scantree
rem set PGM=nwwhoami
rem set PGM=bcastmd
rem set PGM=getsynt
rem set PGM=readsynt
set PGM=readsdef
set SDK=Z:\NWSDK
set LIBDIR=%SDK%\LIB\WIN32\BORLAND
rem BCC32 -c getopt.c
BCC32 -DN_PLAT_MSW4 -c -I%SDK%\INCLUDE %PGM%.c >err
BCC32 %PGM%.obj getopt.obj %LIBDIR%\netwin32.lib %LIBDIR%\locwin32.lib %LIBDIR%\calwin32.lib %LIBDIR%\clxwin32.lib
