#!/bin/sh
#
# Build everything for Win32/Win64

rm -rf dist
mkdir dist
exec >mkall.log 2>&1
set -x
if test -r VERSION ; then
    VER32=$(tr -d '.' <VERSION)
    VER=$(cat VERSION)
else
    VER32="0"
    VER="0.0"
fi

if test $(arch) = "x86_64" ; then
    CC32="gcc -m32 -march=i386 -mtune=i386"
    SH32="linux32 sh"
else
    CC32=gcc
    SH32=sh
fi

NO_SQLITE2=1 NO_TCCEXT=1 SQLITE_DLLS=2 CC=$CC32 $SH32 mingw-cross-build.sh
mv sqliteodbc.exe dist/sqliteodbc_dl-$VER32.exe

CC=$CC32 $SH32 mingw-cross-build.sh
mv sqliteodbc.exe dist/sqliteodbc-$VER32.exe

NO_SQLITE2=1 SQLITE_DLLS=2 sh mingw64-cross-build.sh
mv sqliteodbc_w64.exe dist/sqliteodbc_w64_dl-$VER32.exe

sh mingw64-cross-build.sh
mv sqliteodbc_w64.exe dist/sqliteodbc_w64-$VER32.exe

test -r ../sqliteodbc-$VER.tar.gz && cp -p ../sqliteodbc-$VER.tar.gz dist
