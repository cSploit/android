#!/bin/sh
#
# Build script for cross compiling and packaging SQLite
# ODBC drivers and tools for Win32 using MinGW and NSIS.
# Tested on Fedora Core 3/5/8, Debian Etch, RHEL 5/6
#
# Cross toolchain and NSIS for Linux/i386/x86_64 can be fetched from
#  http://www.ch-werner.de/xtools/crossmingw64-0.3-1.i386.rpm
#  http://www.ch-werner.de/xtools/crossmingw64-0.3-1.x86_64.rpm
#  http://www.ch-werner.de/xtools/nsis-2.37-1.i386.rpm
# or
#  http://www.ch-werner.de/xtools/crossmingw64-0.3.i386.tar.bz2
#  http://www.ch-werner.de/xtools/crossmingw64-0.3.x86_64.tar.bz2
#  http://www.ch-werner.de/xtools/nsis-2.37-1_i386.tar.gz

# Some aspects of the build process can be controlled by shell variables:
#
#  NO_SQLITE2=1      omit building SQLite 2 and drivers for it
#  NO_TCCEXT=1       omit building TCC extension
#  WITH_SOURCES=1    add source directory to NSIS installer
#  SQLITE_DLLS=1     build and package drivers with SQLite 2/3 DLLs
#  SQLITE_DLLS=2     build drivers with refs to SQLite 2/3 DLLs
#                    SQLite3 driver can use System.Data.SQlite.dll

set -e

VER2=2.8.17
VER3=3.7.16.1
VER3X=3071601
VERZ=1.2.7
TCCVER=0.9.25

nov2=false
if test -n "$NO_SQLITE2" ; then
    nov2=true
    ADD_NSIS="-DWITHOUT_SQLITE2"
fi

notcc=false
if test -n "$NO_TCCEXT" ; then
    notcc=true
    ADD_NSIS="$ADD_NSIS -DWITHOUT_TCCEXT"
else
    export SQLITE_TCC_DLL="sqlite+tcc.dll"
fi

if test -f "$WITH_SEE" ; then
    export SEEEXT=see
    ADD_NSIS="$ADD_NSIS -DWITH_SEE=$SEEEXT"
    if test "$SQLITE_DLLS" = "2" ; then
	SQLITE_DLLS=1
    fi
fi

if test "$SQLITE_DLLS" = "2" ; then
    # turn on -DSQLITE_DYNLOAD in sqlite3odbc.c
    export ADD_CFLAGS="-DWITHOUT_SHELL=1 -DWITH_SQLITE_DLLS=2"
    ADD_NSIS="$ADD_NSIS -DWITHOUT_SQLITE3_EXE"
elif test -n "$SQLITE_DLLS" ; then
    export ADD_CFLAGS="-DWITHOUT_SHELL=1 -DWITH_SQLITE_DLLS=1"
    export SQLITE3_DLL="-Lsqlite3 -lsqlite3"
    export SQLITE3_EXE="sqlite3.exe"
    ADD_NSIS="$ADD_NSIS -DWITH_SQLITE_DLLS"
else
    export SQLITE3_A10N_O="sqlite3a10n.o"
    export SQLITE3_EXE="sqlite3.exe"
fi

if test -n "$WITH_SOURCES" ; then
    ADD_NSIS="$ADD_NSIS -DWITH_SOURCES"
fi

echo "=================="
echo "Preparing zlib ..."
echo "=================="
test -r zlib-${VERZ}.tar.gz || \
    wget -c http://zlib.net/zlib-${VERZ}.tar.gz || exit 1
rm -rf zlib-${VERZ}
tar xzf zlib-${VERZ}.tar.gz
ln -sf zlib-${VERZ} zlib

echo "===================="
echo "Preparing sqlite ..."
echo "===================="
( $nov2 && echo '*** skipped (NO_SQLITE2)' ) || true
$nov2 || test -r sqlite-${VER2}.tar.gz || \
    wget -c http://www.sqlite.org/sqlite-${VER2}.tar.gz
$nov2 || test -r sqlite-${VER2}.tar.gz || exit 1

$nov2 || rm -f sqlite
$nov2 || tar xzf sqlite-${VER2}.tar.gz
$nov2 || ln -sf sqlite-${VER2} sqlite

# enable sqlite_encode_binary et.al.
$nov2 || patch sqlite/main.mk <<'EOD'
--- sqlite.orig/main.mk	2005-04-24 00:43:23.000000000 +0200
+++ sqlite/main.mk	2006-03-16 14:29:55.000000000 +0100
@@ -55,7 +55,7 @@
 # Object files for the SQLite library.
 #
 LIBOBJ = attach.o auth.o btree.o btree_rb.o build.o copy.o date.o delete.o \
-         expr.o func.o hash.o insert.o \
+         expr.o func.o hash.o insert.o encode.o \
          main.o opcodes.o os.o pager.o parse.o pragma.o printf.o random.o \
          select.o table.o tokenize.o trigger.o update.o util.o \
          vacuum.o vdbe.o vdbeaux.o where.o tclsqlite.o
EOD

# display encoding
$nov2 || patch sqlite/src/shell.c <<'EOD'
--- sqlite.orig/src/shell.c	2005-04-24 00:43:22.000000000 +0200
+++ sqlite/src/shell.c	2006-05-23 08:22:01.000000000 +0200
@@ -1180,6 +1180,7 @@
   "   -separator 'x'       set output field separator (|)\n"
   "   -nullvalue 'text'    set text string for NULL values\n"
   "   -version             show SQLite version\n"
+  "   -encoding            show SQLite encoding\n"
   "   -help                show this text, also show dot-commands\n"
 ;
 static void usage(int showDetail){
@@ -1297,7 +1298,10 @@
     }else if( strcmp(z,"-echo")==0 ){
       data.echoOn = 1;
     }else if( strcmp(z,"-version")==0 ){
-      printf("%s\n", sqlite_version);
+      printf("%s\n", sqlite_libversion());
+      return 1;
+    }else if( strcmp(z,"-encoding")==0 ){
+      printf("%s\n", sqlite_libencoding());
       return 1;
     }else if( strcmp(z,"-help")==0 ){
       usage(1);
@@ -1330,9 +1334,9 @@
       char *zHome;
       char *zHistory = 0;
       printf(
-        "SQLite version %s\n"
+        "SQLite version %s encoding %s\n"
         "Enter \".help\" for instructions\n",
-        sqlite_version
+        sqlite_libversion(), sqlite_libencoding()
       );
       zHome = find_home_dir();
       if( zHome && (zHistory = malloc(strlen(zHome)+20))!=0 ){
EOD

# use open file dialog when no database name given
# need to link with -lcomdlg32 when enabled
true || patch sqlite/src/shell.c <<'EOD'
--- sqlite.orig/src/shell.c        2006-07-23 11:18:13.000000000 +0200
+++ sqlite/src/shell.c     2006-07-23 11:30:26.000000000 +0200
@@ -20,6 +20,10 @@
 #include "sqlite.h"
 #include <ctype.h>
 
+#if defined(_WIN32) && defined(DRIVER_VER_INFO)
+# include <windows.h>
+#endif
+
 #if !defined(_WIN32) && !defined(WIN32) && !defined(__MACOS__)
 # include <signal.h>
 # include <pwd.h>
@@ -1246,6 +1250,17 @@
   if( i<argc ){
     data.zDbFilename = argv[i++];
   }else{
+#if defined(_WIN32) && defined(DRIVER_VER_INFO)
+    static OPENFILENAME ofn;
+    static char zDbFn[1024];
+    ofn.lStructSize = sizeof(ofn);
+    ofn.lpstrFile = (LPTSTR) zDbFn;
+    ofn.nMaxFile = sizeof(zDbFn);
+    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
+    if( GetOpenFileName(&ofn) ){
+      data.zDbFilename = zDbFn;
+    } else
+#endif
     data.zDbFilename = ":memory:";
   }
   if( i<argc ){
EOD

# same but new module libshell.c
$nov2 || patch sqlite/main.mk <<'EOD'
--- sqlite.orig/main.mk        2007-01-10 19:30:52.000000000 +0100
+++ sqlite/main.mk     2007-01-10 19:33:39.000000000 +0100
@@ -54,7 +54,7 @@
 
 # Object files for the SQLite library.
 #
-LIBOBJ = attach.o auth.o btree.o btree_rb.o build.o copy.o date.o delete.o \
+LIBOBJ += attach.o auth.o btree.o btree_rb.o build.o copy.o date.o delete.o \
          expr.o func.o hash.o insert.o encode.o \
          main.o opcodes.o os.o pager.o parse.o pragma.o printf.o random.o \
          select.o table.o tokenize.o trigger.o update.o util.o \
EOD
$nov2 || cp -p sqlite/src/shell.c sqlite/src/libshell.c
$nov2 || patch sqlite/src/libshell.c <<'EOD'
--- sqlite.orig/src/libshell.c  2007-01-10 19:13:01.000000000 +0100
+++ sqlite/src/libshell.c  2007-01-10 19:25:56.000000000 +0100
@@ -20,6 +20,10 @@
 #include "sqlite.h"
 #include <ctype.h>
 
+#ifdef _WIN32
+# include <windows.h>
+#endif
+
 #if !defined(_WIN32) && !defined(WIN32) && !defined(__MACOS__)
 # include <signal.h>
 # include <pwd.h>
@@ -1205,7 +1209,7 @@
   strcpy(continuePrompt,"   ...> ");
 }
 
-int main(int argc, char **argv){
+int sqlite_main(int argc, char **argv){
   char *zErrMsg = 0;
   struct callback_data data;
   const char *zInitFile = 0;
@@ -1246,6 +1250,17 @@
   if( i<argc ){
     data.zDbFilename = argv[i++];
   }else{
+#if defined(_WIN32) && !defined(__TINYC__)
+    static OPENFILENAME ofn;
+    static char zDbFn[1024];
+    ofn.lStructSize = sizeof(ofn);
+    ofn.lpstrFile = (LPTSTR) zDbFn;
+    ofn.nMaxFile = sizeof(zDbFn);
+    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
+    if( GetOpenFileName(&ofn) ){
+      data.zDbFilename = zDbFn;
+    } else
+#endif
     data.zDbFilename = ":memory:";
   }
   if( i<argc ){
EOD

$nov2 || rm -f sqlite/src/minshell.c
$nov2 || touch sqlite/src/minshell.c
$nov2 || patch sqlite/src/minshell.c <<'EOD'
--- sqlite.orig/src/minshell.c  2007-01-10 18:46:47.000000000 +0100
+++ sqlite/src/minshell.c  2007-01-10 18:46:47.000000000 +0100
@@ -0,0 +1,20 @@
+/*
+** 2001 September 15
+**
+** The author disclaims copyright to this source code.  In place of
+** a legal notice, here is a blessing:
+**
+**    May you do good and not evil.
+**    May you find forgiveness for yourself and forgive others.
+**    May you share freely, never taking more than you give.
+**
+*************************************************************************
+** This file contains code to implement the "sqlite" command line
+** utility for accessing SQLite databases.
+*/
+
+int sqlite_main(int argc, char **argv);
+
+int main(int argc, char **argv){
+  return sqlite_main(argc, argv);
+}
EOD

echo "====================="
echo "Preparing sqlite3 ..."
echo "====================="
test -r sqlite-src-${VER3X}.zip || \
    wget -c http://www.sqlite.org/sqlite-src-${VER3X}.zip
test -r sqlite-src-${VER3X}.zip || exit 1
test -r extension-functions.c ||
    wget -O extension-functions.c -c \
      'http://www.sqlite.org/contrib/download/extension-functions.c?get=25'
if test -r extension-functions.c ; then
  cp extension-functions.c extfunc.c
  patch < extfunc.patch
fi
test -r extfunc.c || exit 1

rm -f sqlite3
rm -rf sqlite-src-${VER3X}
unzip sqlite-src-${VER3X}.zip
ln -sf sqlite-src-${VER3X} sqlite3

patch sqlite3/main.mk <<'EOD'
--- sqlite3.orig/main.mk        2007-03-31 14:32:21.000000000 +0200
+++ sqlite3/main.mk     2007-04-02 11:04:50.000000000 +0200
@@ -67,7 +67,7 @@

 # All of the source code files.
 #
-SRC = \
+SRC += \
   $(TOP)/src/alter.c \
   $(TOP)/src/analyze.c \
   $(TOP)/src/attach.c \
EOD

# use open file dialog when no database name given
# need to link with -lcomdlg32 when enabled
true || patch sqlite3/src/shell.c <<'EOD'
--- sqlite3.orig/src/shell.c        2006-06-06 14:32:21.000000000 +0200
+++ sqlite3/src/shell.c     2006-07-23 11:04:50.000000000 +0200
@@ -21,6 +21,10 @@
 #include <ctype.h>
 #include <stdarg.h>
 
+#if defined(_WIN32) && defined(DRIVER_VER_INFO)
+# include <windows.h>
+#endif
+
 #if !defined(_WIN32) && !defined(WIN32) && !defined(__OS2__)
 # include <signal.h>
 # include <pwd.h>
@@ -1676,6 +1676,17 @@
   if( i<argc ){
     data.zDbFilename = argv[i++];
   }else{
+#if defined(_WIN32) && defined(DRIVER_VER_INFO)
+    static OPENFILENAME ofn;
+    static char zDbFn[1024];
+    ofn.lStructSize = sizeof(ofn);
+    ofn.lpstrFile = (LPTSTR) zDbFn;
+    ofn.nMaxFile = sizeof(zDbFn);
+    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
+    if( GetOpenFileName(&ofn) ){
+      data.zDbFilename = zDbFn;
+    } else
+#endif
 #ifndef SQLITE_OMIT_MEMORYDB
     data.zDbFilename = ":memory:";
 #else
EOD
# SQLite 3.5.1 Win32 mutex fix
test "$VER3" != "3.5.1" || patch sqlite3/src/mutex_w32.c <<'EOD'
--- sqlite3.orig/src/mutex_w32.c	2007-08-30 14:10:30
+++ sqlite3/src/mutex_w32.c	2007-09-04 22:31:3
@@ -141,6 +141,12 @@
   p->nRef++;
 }
 int sqlite3_mutex_try(sqlite3_mutex *p){
+  /* The TryEnterCriticalSection() interface is not available on all
+  ** windows systems.  Since sqlite3_mutex_try() is only used as an
+  ** optimization, we can skip it on windows. */
+  return SQLITE_BUSY;
+
+#if 0  /* Not Available */
   int rc;
   assert( p );
   assert( p->id==SQLITE_MUTEX_RECURSIVE || sqlite3_mutex_notheld(p) );
@@ -152,6 +158,7 @@
     rc = SQLITE_BUSY;
   }
   return rc;
+#endif
 }
 
 /*

EOD

# same but new module libshell.c
cp -p sqlite3/src/shell.c sqlite3/src/libshell.c
test "$VER3" != "3.7.14" -a "$VER3" != "3.7.14.1" -a "$VER3" != "3.7.15" 
  -a "$VER3" != "3.7.15.1" -a "$VER3" != "3.7.15.2" -a "$VER3" != "3.7.16" \
  -a "$VER3" != "3.7.16.1" \
  && patch sqlite3/src/libshell.c <<'EOD'
--- sqlite3.orig/src/libshell.c  2007-01-08 23:40:05.000000000 +0100
+++ sqlite3/src/libshell.c  2007-01-10 18:35:43.000000000 +0100
@@ -21,6 +21,10 @@
 #include <ctype.h>
 #include <stdarg.h>

+#ifdef _WIN32
+# include <windows.h>
+#endif
+
 #if !defined(_WIN32) && !defined(WIN32) && !defined(__OS2__)
 # include <signal.h>
 # include <pwd.h>
@@ -1774,7 +1778,7 @@
   strcpy(continuePrompt,"   ...> ");
 }
 
-int main(int argc, char **argv){
+int sqlite3_main(int argc, char **argv){
   char *zErrMsg = 0;
   struct callback_data data;
   const char *zInitFile = 0;
@@ -1816,6 +1820,17 @@
   if( i<argc ){
     data.zDbFilename = argv[i++];
   }else{
+#if defined(_WIN32) && !defined(__TINYC__)
+    static OPENFILENAME ofn;
+    static char zDbFn[1024];
+    ofn.lStructSize = sizeof(ofn);
+    ofn.lpstrFile = (LPTSTR) zDbFn;
+    ofn.nMaxFile = sizeof(zDbFn);
+    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
+    if( GetOpenFileName(&ofn) ){
+      data.zDbFilename = zDbFn;
+    } else
+#endif
 #ifndef SQLITE_OMIT_MEMORYDB
     data.zDbFilename = ":memory:";
 #else
EOD

test "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" \
  && patch sqlite3/src/libshell.c <<'EOD'
--- sqlite3.orig/src/libshell.c  2007-01-08 23:40:05.000000000 +0100
+++ sqlite3/src/libshell.c  2007-01-10 18:35:43.000000000 +0100
@@ -21,6 +21,10 @@
 #include <ctype.h>
 #include <stdarg.h>

+#ifdef _WIN32
+# include <windows.h>
+#endif
+
 #if !defined(_WIN32) && !defined(WIN32)
 # include <signal.h>
 # include <pwd.h>
@@ -1774,7 +1778,7 @@
   strcpy(continuePrompt,"   ...> ");
 }
 
-int main(int argc, char **argv){
+int sqlite3_main(int argc, char **argv){
   char *zErrMsg = 0;
   struct callback_data data;
   const char *zInitFile = 0;
@@ -1816,6 +1820,17 @@
   if( i<argc ){
     data.zDbFilename = argv[i++];
   }else{
+#if defined(_WIN32) && !defined(__TINYC__)
+    static OPENFILENAME ofn;
+    static char zDbFn[1024];
+    ofn.lStructSize = sizeof(ofn);
+    ofn.lpstrFile = (LPTSTR) zDbFn;
+    ofn.nMaxFile = sizeof(zDbFn);
+    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
+    if( GetOpenFileName(&ofn) ){
+      data.zDbFilename = zDbFn;
+    } else
+#endif
 #ifndef SQLITE_OMIT_MEMORYDB
     data.zDbFilename = ":memory:";
 #else
EOD

test "$VER3" = "3.7.15" -o "$VER3" = "3.7.15.1" -o "$VER3" = "3.7.15.2" \
  -o "$VER3" = "3.7.16" -o "$VER3" = "3.7.16.1" \
  && patch sqlite3/src/libshell.c <<'EOD'
--- sqlite3.orig/src/libshell.c  2012-12-12 14:42:10.000000000 +0100
+++ sqlite3/src/libshell.c  2012-12-13 12:14:57.000000000 +0100
@@ -36,6 +36,10 @@
 #include <ctype.h>
 #include <stdarg.h>
 
+#ifdef _WIN32
+#include <windows.h>
+#endif
+
 #if !defined(_WIN32) && !defined(WIN32)
 # include <signal.h>
 # if !defined(__RTP__) && !defined(_WRS_KERNEL)
@@ -2894,7 +2898,7 @@
   return argv[i];
 }
 
-int main(int argc, char **argv){
+int sqlite3_main(int argc, char **argv){
   char *zErrMsg = 0;
   struct callback_data data;
   const char *zInitFile = 0;
@@ -2996,6 +3000,17 @@
     }
   }
   if( data.zDbFilename==0 ){
+#if defined(_WIN32) && !defined(__TINYC__)
+    static OPENFILENAME ofn;
+    static char zDbFn[1024];
+    ofn.lStructSize = sizeof(ofn);
+    ofn.lpstrFile = (LPTSTR) zDbFn;
+    ofn.nMaxFile = sizeof(zDbFn);
+    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
+    if( GetOpenFileName(&ofn) ){
+      data.zDbFilename = zDbFn;
+    } else
+#endif
 #ifndef SQLITE_OMIT_MEMORYDB
     data.zDbFilename = ":memory:";
 #else
EOD

rm -f sqlite3/src/minshell.c
touch sqlite3/src/minshell.c
patch sqlite3/src/minshell.c <<'EOD'
--- sqlite3.orig/src/minshell.c  2007-01-10 18:46:47.000000000 +0100
+++ sqlite3/src/minshell.c  2007-01-10 18:46:47.000000000 +0100
@@ -0,0 +1,20 @@
+/*
+** 2001 September 15
+**
+** The author disclaims copyright to this source code.  In place of
+** a legal notice, here is a blessing:
+**
+**    May you do good and not evil.
+**    May you find forgiveness for yourself and forgive others.
+**    May you share freely, never taking more than you give.
+**
+*************************************************************************
+** This file contains code to implement the "sqlite" command line
+** utility for accessing SQLite databases.
+*/
+
+int sqlite3_main(int argc, char **argv);
+
+int main(int argc, char **argv){
+  return sqlite3_main(argc, argv);
+}
EOD

# amalgamation: add libshell.c 
test "$VER3" != "3.5.6" && test -r sqlite3/tool/mksqlite3c.tcl && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/tool/mksqlite3c.tcl	2007-04-02 14:20:10.000000000 +0200
+++ sqlite3/tool/mksqlite3c.tcl	2007-04-03 09:42:03.000000000 +0200
@@ -194,6 +194,7 @@
    where.c
 
    parse.c
+   libshell.c

    tokenize.c
    complete.c
EOD
test "$VER3" = "3.5.6" && test -r sqlite3/tool/mksqlite3c.tcl && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/tool/mksqlite3c.tcl	2007-04-02 14:20:10.000000000 +0200
+++ sqlite3/tool/mksqlite3c.tcl	2007-04-03 09:42:03.000000000 +0200
@@ -200,6 +200,7 @@
 
    main.c

+   libshell.c
    fts3.c
    fts3_hash.c
    fts3_porter.c
EOD

# patch: parse foreign key constraints on virtual tables
test "$VER3" != "3.6.15" -a "$VER3" != "3.6.16" -a "$VER3" != "3.6.17" \
  -a "$VER3" != "3.6.18" -a "$VER3" != "3.6.19" -a "$VER3" != "3.6.20" \
  -a "$VER3" != "3.6.21" -a "$VER3" != "3.6.22" -a "$VER3" != "3.6.23" \
  -a "$VER3" != "3.6.23.1" -a "$VER3" != "3.7.0" -a "$VER3" != "3.7.0.1" \
  -a "$VER3" != "3.7.1" -a "$VER3" != "3.7.2" -a "$VER3" != "3.7.3" \
  -a "$VER3" != "3.7.4" -a "$VER3" != "3.7.5" -a "$VER3" != "3.7.6" \
  -a "$VER3" != "3.7.6.1" -a "$VER3" != "3.7.6.2" -a "$VER3" != "3.7.6.3" \
  -a "$VER3" != "3.7.7" -a "$VER3" != "3.7.7.1" -a "$VER3" != "3.7.8" \
  -a "$VER3" != "3.7.9" -a "$VER3" != "3.7.10" -a "$VER3" != "3.7.11" \
  -a "$VER3" != "3.7.12" -a "$VER3" != "3.7.12.1" -a "$VER3" != "3.7.13" \
  -a "$VER3" != "3.7.14" -a "$VER3" != "3.7.14.1" -a "$VER3" != "3.7.15" \
  -a "$VER3" != "3.7.15.1" -a "$VER3" != "3.7.15.2" -a "$VER3" != "3.7.16" \
  -a "$VER3" != "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
diff -u sqlite3.orig/src/build.c sqlite3/src/build.c
--- sqlite3.orig/src/build.c	2007-01-09 14:53:04.000000000 +0100
+++ sqlite3/src/build.c	2007-01-30 08:14:41.000000000 +0100
@@ -2063,7 +2063,7 @@
   char *z;
 
   assert( pTo!=0 );
-  if( p==0 || pParse->nErr || IN_DECLARE_VTAB ) goto fk_end;
+  if( p==0 || pParse->nErr ) goto fk_end;
   if( pFromCol==0 ){
     int iCol = p->nCol-1;
     if( iCol<0 ) goto fk_end;
diff -u sqlite3.orig/src/pragma.c sqlite3/src/pragma.c
--- sqlite3.orig/src/pragma.c	2007-01-27 03:24:56.000000000 +0100
+++ sqlite3/src/pragma.c	2007-01-30 09:19:30.000000000 +0100
@@ -589,6 +589,9 @@
     pTab = sqlite3FindTable(db, zRight, zDb);
     if( pTab ){
       v = sqlite3GetVdbe(pParse);
+#ifndef SQLITE_OMIT_VIRTUAL_TABLE
+      if( pTab->pVtab ) sqlite3ViewGetColumnNames(pParse, pTab);
+#endif
       pFK = pTab->pFKey;
       if( pFK ){
         int i = 0; 
diff -u sqlite3.orig/src/vtab.c sqlite3/src/vtab.c
--- sqlite3.orig/src/vtab.c	2007-01-09 15:01:14.000000000 +0100
+++ sqlite3/src/vtab.c	2007-01-30 08:23:22.000000000 +0100
@@ -540,6 +540,9 @@
   int rc = SQLITE_OK;
   Table *pTab = db->pVTab;
   char *zErr = 0;
+#ifndef SQLITE_OMIT_FOREIGN_KEYS
+  FKey *pFKey;
+#endif
 
   sqlite3_mutex_enter(db->mutex);
   pTab = db->pVTab;
@@ -568,6 +571,15 @@
   }
   sParse.declareVtab = 0;
 
+#ifndef SQLITE_OMIT_FOREIGN_KEYS
+  assert( pTab->pFKey==0 );
+  pTab->pFKey = sParse.pNewTable->pFKey;
+  sParse.pNewTable->pFKey = 0;
+  for(pFKey=pTab->pFKey; pFKey; pFKey=pFKey->pNextFrom){
+    pFKey->pFrom=pTab;
+  }
+#endif
+
   if( sParse.pVdbe ){
     sqlite3VdbeFinalize(sParse.pVdbe);
   }
EOD

# patch: re-enable NO_TCL in tclsqlite.c (3.3.15)
patch -d sqlite3 -p1 <<'EOD'
diff -u sqlite3.orig/src/tclsqlite.c sqlite3/src/tclsqlite.c
--- sqlite3.orig/src/tclsqlite.c	2007-04-06 17:02:14.000000000 +0200
+++ sqlite3/src/tclsqlite.c	2007-04-10 07:47:49.000000000 +0200
@@ -14,6 +14,7 @@
 **
 ** $Id: mingw-cross-build.sh,v 1.76 2013/03/30 05:16:56 chw Exp chw $
 */
+#ifndef NO_TCL     /* Omit this whole file if TCL is unavailable */
 #include "tcl.h"
 
 /*
@@ -2264,3 +2265,5 @@
   return 0;
 }
 #endif /* TCLSH */
+
+#endif /* !defined(NO_TCL) */
EOD

# patch: Win32 locking and pager unlock, for SQLite3 < 3.4.0
true || patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/src/os_win.c	2007-04-11 19:52:04.000000000 +0200
+++ sqlite3/src/os_win.c	2007-05-08 06:57:06.000000000 +0200
@@ -1237,8 +1237,8 @@
   ** the PENDING_LOCK byte is temporary.
   */
   newLocktype = pFile->locktype;
-  if( pFile->locktype==NO_LOCK
-   || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
+  if( locktype==SHARED_LOCK
+   || (locktype==EXCLUSIVE_LOCK && pFile->locktype<PENDING_LOCK)
   ){
     int cnt = 3;
     while( cnt-->0 && (res = LockFile(pFile->h, PENDING_BYTE, 0, 1, 0))==0 ){
@@ -1289,6 +1289,18 @@
       newLocktype = EXCLUSIVE_LOCK;
     }else{
       OSTRACE2("error-code = %d\n", GetLastError());
+      if( !getReadLock(pFile) ){
+        /* This should never happen.  We should always be able to
+        ** reacquire the read lock */
+        OSTRACE1("could not re-get a SHARED lock.\n");
+	if( newLocktype==PENDING_LOCK || pFile->locktype==PENDING_LOCK ){
+          UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
+        }
+        if( pFile->locktype==RESERVED_LOCK ){
+          UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
+        }
+        newLocktype = NO_LOCK;
+      }
     }
   }
 
@@ -1362,6 +1374,7 @@
       /* This should never happen.  We should always be able to
       ** reacquire the read lock */
       rc = SQLITE_IOERR_UNLOCK;
+      locktype = NO_LOCK;
     }
   }
   if( type>=RESERVED_LOCK ){
EOD

# patch: Win32 locking and pager unlock, for SQLite3 >= 3.5.4 && <= 3.6.10
true || patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/src/os_win.c       2007-12-13 22:38:58.000000000 +0100
+++ sqlite3/src/os_win.c    2008-01-18 10:01:48.000000000 +0100
@@ -855,8 +855,8 @@
   ** the PENDING_LOCK byte is temporary.
   */
   newLocktype = pFile->locktype;
-  if( pFile->locktype==NO_LOCK
-   || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
+  if( locktype==SHARED_LOCK
+   || (locktype==EXCLUSIVE_LOCK && pFile->locktype<PENDING_LOCK)
   ){
     int cnt = 3;
     while( cnt-->0 && (res = LockFile(pFile->h, PENDING_BYTE, 0, 1, 0))==0 ){
@@ -907,7 +907,18 @@
       newLocktype = EXCLUSIVE_LOCK;
     }else{
       OSTRACE2("error-code = %d\n", GetLastError());
-      getReadLock(pFile);
+      if( !getReadLock(pFile) ){
+        /* This should never happen.  We should always be able to
+        ** reacquire the read lock */
+        OSTRACE1("could not re-get a SHARED lock.\n");
+        if( newLocktype==PENDING_LOCK || pFile->locktype==PENDING_LOCK ){
+          UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
+        }
+        if( pFile->locktype==RESERVED_LOCK ){
+          UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
+        }
+        newLocktype = NO_LOCK;
+      }
     }
   }
 
@@ -982,6 +993,7 @@
       /* This should never happen.  We should always be able to
       ** reacquire the read lock */
       rc = SQLITE_IOERR_UNLOCK;
+      locktype = NO_LOCK;
     }
   }
   if( type>=RESERVED_LOCK ){
EOD

# patch: Win32 locking and pager unlock, for SQLite3 >= 3.6.11 && < 3.7.0
true || patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/src/os_win.c    2009-02-15 14:07:09.000000000 +0100
+++ sqlite3/src/os_win.c    2009-02-20 16:39:48.000000000 +0100
@@ -922,7 +922,7 @@
   newLocktype = pFile->locktype;
   if(   (pFile->locktype==NO_LOCK)
      || (   (locktype==EXCLUSIVE_LOCK)
-         && (pFile->locktype==RESERVED_LOCK))
+         && (pFile->locktype<RESERVED_LOCK))
   ){
     int cnt = 3;
     while( cnt-->0 && (res = LockFile(pFile->h, PENDING_BYTE, 0, 1, 0))==0 ){
@@ -981,7 +981,18 @@
     }else{
       error = GetLastError();
       OSTRACE2("error-code = %d\n", error);
-      getReadLock(pFile);
+      if( !getReadLock(pFile) ){
+        /* This should never happen.  We should always be able to
+        ** reacquire the read lock */
+        OSTRACE1("could not re-get a SHARED lock.\n");
+        if( newLocktype==PENDING_LOCK || pFile->locktype==PENDING_LOCK ){
+          UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
+        }
+        if( pFile->locktype==RESERVED_LOCK ){
+          UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
+        }
+        newLocktype = NO_LOCK;
+      }
     }
   }
 
@@ -1057,6 +1068,7 @@
       /* This should never happen.  We should always be able to
       ** reacquire the read lock */
       rc = SQLITE_IOERR_UNLOCK;
+      locktype = NO_LOCK;
     }
   }
   if( type>=RESERVED_LOCK ){
EOD

# patch: compile fix for FTS3 as extension module
test "$VER3" != "3.6.21" -a "$VER3" != "3.6.22" -a "$VER3" != "3.6.23" \
  -a "$VER3" != "3.6.23.1" -a "$VER3" != "3.7.0" -a "$VER3" != "3.7.0.1" \
  -a "$VER3" != "3.7.1" -a "$VER3" != "3.7.2" -a "$VER3" != "3.7.3" \
  -a "$VER3" != "3.7.4" -a "$VER3" != "3.7.5" -a "$VER3" != "3.7.6" \
  -a "$VER3" != "3.7.6.1" -a "$VER3" != "3.7.6.2" -a "$VER3" != "3.7.6.3" \
  -a "$VER3" != "3.7.7" -a "$VER3" != "3.7.7.1" -a "$VER3" != "3.7.8" \
  -a "$VER3" != "3.7.9" -a "$VER3" != "3.7.10" -a "$VER3" != "3.7.11" \
  -a "$VER3" != "3.7.12" -a "$VER3" != "3.7.12.1" -a "$VER3" != "3.7.13" \
  -a "$VER3" != "3.7.14" -a "$VER3" != "3.7.14.1" -a "$VER3" != "3.7.15" \
  -a "$VER3" != "3.7.15.1" -a "$VER3" != "3.7.15.2" -a "$VER3" != "3.7.16" \
  -a "$VER3" != "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3.c 2008-02-02 17:24:34.000000000 +0100
+++ sqlite3/ext/fts3/fts3.c      2008-03-16 11:29:02.000000000 +0100
@@ -274,10 +274,6 @@
 
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
-#if defined(SQLITE_ENABLE_FTS3) && !defined(SQLITE_CORE)
-# define SQLITE_CORE 1
-#endif
-
 #include <assert.h>
 #include <stdlib.h>
 #include <stdio.h>
@@ -6389,7 +6385,7 @@
   return rc;
 }
 
-#if !SQLITE_CORE
+#ifndef SQLITE_CORE
 int sqlite3_extension_init(
   sqlite3 *db, 
   char **pzErrMsg,
EOD
test "$VER3" = "3.6.21" -o "$VER3" = "3.6.22" -o "$VER3" = "3.6.23" \
  -o "$VER3" = "3.6.23.1" -o "$VER3" = "3.7.0" -o "$VER3" = "3.7.0.1" \
  -o "$VER3" = "3.7.1" -o "$VER3" = "3.7.2" -o "$VER3" = "3.7.3" \
  -o "$VER3" = "3.7.4" -o "$VER3" = "3.7.5" -o "$VER3" = "3.7.6" \
  -o "$VER3" = "3.7.6.1" -o "$VER3" = "3.7.6.2" -o "$VER3" = "3.7.6.3" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3.c 2008-02-02 17:24:34.000000000 +0100
+++ sqlite3/ext/fts3/fts3.c      2008-03-16 11:29:02.000000000 +0100
@@ -274,10 +274,6 @@
 
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
-#if defined(SQLITE_ENABLE_FTS3) && !defined(SQLITE_CORE)
-# define SQLITE_CORE 1
-#endif
-
 #include "fts3Int.h"

 #include <assert.h>
@@ -6389,7 +6385,7 @@
   return rc;
 }
 
-#if !SQLITE_CORE
+#ifndef SQLITE_CORE
 int sqlite3_extension_init(
   sqlite3 *db, 
   char **pzErrMsg,
EOD
patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_porter.c  2008-02-01 16:40:34.000000000 +0100
+++ sqlite3/ext/fts3/fts3_porter.c       2008-03-16 11:34:50.000000000 +0100
@@ -31,6 +31,11 @@
 #include <string.h>
 #include <ctype.h>
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 #include "fts3_tokenizer.h"
 
 /*
--- sqlite3.orig/ext/fts3/fts3_tokenizer1.c      2007-11-23 18:31:18.000000000 +0100
+++ sqlite3/ext/fts3/fts3_tokenizer1.c   2008-03-16 11:35:37.000000000 +0100
@@ -31,6 +31,11 @@
 #include <string.h>
 #include <ctype.h>
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 #include "fts3_tokenizer.h"
 
 typedef struct simple_tokenizer {
EOD
test "$VER3" != "3.7.8" -a "$VER3" != "3.7.9" -a "$VER3" != "3.7.10" \
  -a "$VER3" != "3.7.11" -a "$VER3" != "3.7.12" -a "$VER3" != "3.7.12.1" \
  -a "$VER3" != "3.7.13" -a "$VER3" != "3.7.14" -a "$VER3" != "3.7.14.1" \
  -a "$VER3" != "3.7.15" -a "$VER3" != "3.7.15.1" -a "$VER3" != "3.7.15.2" \
  -a "$VER3" != "3.7.16" -a "$VER3" != "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_hash.c    2007-11-24 01:41:52.000000000 +0100
+++ sqlite3/ext/fts3/fts3_hash.c 2008-03-16 11:39:57.000000000 +0100
@@ -29,6 +29,11 @@
 #include <stdlib.h>
 #include <string.h>
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 #include "sqlite3.h"
 #include "fts3_hash.h"
EOD
test "$VER3" = "3.6.21" && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_write.c   2009-12-03 20:39:06.000000000 +0100
+++ sqlite3/ext/fts3/fts3_write.c        2010-01-05 07:59:27.000000000 +0100
@@ -20,6 +20,10 @@
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
 #include "fts3Int.h"
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 #include <stdlib.h>
EOD
test "$VER3" = "3.6.22" -o "$VER3" = "3.6.23" -o "$VER3" = "3.6.23.1" \
  -o "$VER3" = "3.7.0" -o "$VER3" = "3.7.0.1" \
  -o "$VER3" = "3.7.1" -o "$VER3" = "3.7.2" -o "$VER3" = "3.7.3" \
  -o "$VER3" = "3.7.4" -o "$VER3" = "3.7.5" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_write.c   2010-01-05 09:42:19.000000000 +0100
+++ sqlite3/ext/fts3/fts3_write.c        2010-01-05 09:55:25.000000000 +0100
@@ -20,6 +20,10 @@
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
 #include "fts3Int.h"
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 #include <stdlib.h>
@@ -2226,7 +2230,7 @@
 
   if( !zVal ){
     return SQLITE_NOMEM;
-  }else if( nVal==8 && 0==sqlite3_strnicmp(zVal, "optimize", 8) ){
+  }else if( nVal==8 && 0==strnicmp(zVal, "optimize", 8) ){
     rc = fts3SegmentMerge(p, -1);
     if( rc==SQLITE_DONE ){
       rc = SQLITE_OK;
@@ -2234,10 +2238,10 @@
       sqlite3Fts3PendingTermsClear(p);
     }
 #ifdef SQLITE_TEST
-  }else if( nVal>9 && 0==sqlite3_strnicmp(zVal, "nodesize=", 9) ){
+  }else if( nVal>9 && 0==strnicmp(zVal, "nodesize=", 9) ){
     p->nNodeSize = atoi(&zVal[9]);
     rc = SQLITE_OK;
-  }else if( nVal>11 && 0==sqlite3_strnicmp(zVal, "maxpending=", 9) ){
+  }else if( nVal>11 && 0==strnicmp(zVal, "maxpending=", 9) ){
     p->nMaxPendingData = atoi(&zVal[11]);
     rc = SQLITE_OK;
 #endif
EOD

test "$VER3" = "3.7.6" -o "$VER3" = "3.7.6.1" -o "$VER3" = "3.7.6.2" \
  -o "$VER3" = "3.7.6.3" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_write.c	2011-04-12 11:44:56.000000000 +0200
+++ sqlite3/ext/fts3/fts3_write.c	2011-04-13 08:00:51.000000000 +0200
@@ -20,6 +20,10 @@
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
 #include "fts3Int.h"
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 #include <stdlib.h>
@@ -2450,7 +2454,7 @@
 
   if( !zVal ){
     return SQLITE_NOMEM;
-  }else if( nVal==8 && 0==sqlite3_strnicmp(zVal, "optimize", 8) ){
+  }else if( nVal==8 && 0==strnicmp(zVal, "optimize", 8) ){
     rc = fts3SegmentMerge(p, FTS3_SEGCURSOR_ALL);
     if( rc==SQLITE_DONE ){
       rc = SQLITE_OK;
@@ -2458,10 +2462,10 @@
       sqlite3Fts3PendingTermsClear(p);
     }
 #ifdef SQLITE_TEST
-  }else if( nVal>9 && 0==sqlite3_strnicmp(zVal, "nodesize=", 9) ){
+  }else if( nVal>9 && 0==strnicmp(zVal, "nodesize=", 9) ){
     p->nNodeSize = atoi(&zVal[9]);
     rc = SQLITE_OK;
-  }else if( nVal>11 && 0==sqlite3_strnicmp(zVal, "maxpending=", 9) ){
+  }else if( nVal>11 && 0==strnicmp(zVal, "maxpending=", 9) ){
     p->nMaxPendingData = atoi(&zVal[11]);
     rc = SQLITE_OK;
 #endif
--- sqlite3.orig/ext/fts3/fts3_aux.c	2011-04-12 11:44:56.000000000 +0200
+++ sqlite3/ext/fts3/fts3_aux.c	2011-04-13 08:16:17.000000000 +0200
@@ -15,6 +15,10 @@
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
 #include "fts3Int.h"
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 
EOD

test "$VER3" = "3.6.21" -o "$VER3" = "3.6.22" -o "$VER3" = "3.6.23" \
  -o "$VER3" = "3.6.23.1" -o "$VER3" = "3.7.0" -o "$VER3" = "3.7.0.1" \
  -o "$VER3" = "3.7.1" -o "$VER3" = "3.7.2" -o "$VER3" = "3.7.3" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_snippet.c 2009-12-03 12:33:32.000000000 +0100
+++ sqlite3/ext/fts3/fts3_snippet.c      2010-01-05 08:03:51.000000000 +0100
@@ -14,6 +14,10 @@
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
 #include "fts3Int.h"
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 #include <ctype.h>
--- sqlite3.orig/ext/fts3/fts3_expr.c    2009-12-03 12:33:32.000000000 +0100
+++ sqlite3/ext/fts3/fts3_expr.c 2010-01-05 08:06:10.000000000 +0100
@@ -17,6 +17,11 @@
 */
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 /*
 ** By default, this module parses the legacy syntax that has been 
 ** traditionally used by fts3. Or, if SQLITE_ENABLE_FTS3_PARENTHESIS
@@ -445,7 +450,7 @@
     const char *zStr = pParse->azCol[ii];
     int nStr = (int)strlen(zStr);
     if( nInput>nStr && zInput[nStr]==':' 
-     && sqlite3_strnicmp(zStr, zInput, nStr)==0 
+     && memcmp(zStr, zInput, nStr)==0 
     ){
       iCol = ii;
       iColLen = (int)((zInput - z) + nStr + 1);
--- sqlite3.orig/ext/fts3/fts3_tokenizer.c       2009-12-07 17:38:46.000000000 +0100
+++ sqlite3/ext/fts3/fts3_tokenizer.c    2010-01-05 08:12:50.000000000 +0100
@@ -27,7 +27,7 @@
 
 #include "sqlite3ext.h"
 #ifndef SQLITE_CORE
-  SQLITE_EXTENSION_INIT1
+extern const sqlite3_api_routines *sqlite3_api;
 #endif
 
 #include "fts3Int.h"
@@ -166,7 +166,7 @@
   if( !z ){
     zCopy = sqlite3_mprintf("simple");
   }else{
-    if( sqlite3_strnicmp(z, "tokenize", 8) || fts3IsIdChar(z[8])){
+    if( strnicmp(z, "tokenize", 8) || fts3IsIdChar(z[8])){
       return SQLITE_OK;
     }
     zCopy = sqlite3_mprintf("%s", &z[8]);
EOD

test "$VER3" = "3.7.4" -o "$VER3" = "3.7.5" -o "$VER3" = "3.7.6" \
  -o "$VER3" = "3.7.6.1" -o "$VER3" = "3.7.6.2" -o "$VER3" = "3.7.6.3" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_snippet.c 2009-12-03 12:33:32.000000000 +0100
+++ sqlite3/ext/fts3/fts3_snippet.c      2010-01-05 08:03:51.000000000 +0100
@@ -14,6 +14,10 @@
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
 #include "fts3Int.h"
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 #include <ctype.h>
--- sqlite3.orig/ext/fts3/fts3_tokenizer.c       2009-12-07 17:38:46.000000000 +0100
+++ sqlite3/ext/fts3/fts3_tokenizer.c    2010-01-05 08:12:50.000000000 +0100
@@ -27,7 +27,7 @@
 
 #include "sqlite3ext.h"
 #ifndef SQLITE_CORE
-  SQLITE_EXTENSION_INIT1
+extern const sqlite3_api_routines *sqlite3_api;
 #endif
 
 #include "fts3Int.h"
--- sqlite3.orig/ext/fts3/fts3_expr.c    2009-12-03 12:33:32.000000000 +0100
+++ sqlite3/ext/fts3/fts3_expr.c 2010-01-05 08:06:10.000000000 +0100
@@ -17,6 +17,11 @@
 */
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 /*
 ** By default, this module parses the legacy syntax that has been 
 ** traditionally used by fts3. Or, if SQLITE_ENABLE_FTS3_PARENTHESIS
@@ -445,7 +450,7 @@
     const char *zStr = pParse->azCol[ii];
     int nStr = (int)strlen(zStr);
     if( nInput>nStr && zInput[nStr]==':' 
-     && sqlite3_strnicmp(zStr, zInput, nStr)==0 
+     && memcmp(zStr, zInput, nStr)==0 
     ){
       iCol = ii;
       iColLen = (int)((zInput - z) + nStr + 1);
--- sqlite3.orig/ext/fts3/fts3.c	2010-12-07 16:14:36.000000000 +0100
+++ sqlite3/ext/fts3/fts3.c	2010-12-16 11:59:02.000000000 +0100
@@ -702,8 +698,8 @@
   sqlite3_tokenizer *pTokenizer = 0;        /* Tokenizer for this table */
 
   assert( strlen(argv[0])==4 );
-  assert( (sqlite3_strnicmp(argv[0], "fts4", 4)==0 && isFts4)
-       || (sqlite3_strnicmp(argv[0], "fts3", 4)==0 && !isFts4)
+  assert( (strnicmp(argv[0], "fts4", 4)==0 && isFts4)
+       || (strnicmp(argv[0], "fts3", 4)==0 && !isFts4)
   );
 
   nDb = (int)strlen(argv[1]) + 1;
@@ -732,7 +728,7 @@
     /* Check if this is a tokenizer specification */
     if( !pTokenizer 
      && strlen(z)>8
-     && 0==sqlite3_strnicmp(z, "tokenize", 8) 
+     && 0==strnicmp(z, "tokenize", 8) 
      && 0==sqlite3Fts3IsIdChar(z[8])
     ){
       rc = sqlite3Fts3InitTokenizer(pHash, &z[9], &pTokenizer, pzErr);
@@ -744,8 +740,8 @@
         rc = SQLITE_NOMEM;
         goto fts3_init_out;
       }
-      if( nKey==9 && 0==sqlite3_strnicmp(z, "matchinfo", 9) ){
-        if( strlen(zVal)==4 && 0==sqlite3_strnicmp(zVal, "fts3", 4) ){
+      if( nKey==9 && 0==strnicmp(z, "matchinfo", 9) ){
+        if( strlen(zVal)==4 && 0==strnicmp(zVal, "fts3", 4) ){
           bNoDocsize = 1;
         }else{
           *pzErr = sqlite3_mprintf("unrecognized matchinfo: %s", zVal);
--- sqlite3.orig/ext/fts3/fts3_write.c	2010-12-16 12:08:45.000000000 +0100
+++ sqlite3/ext/fts3/fts3_write.c	2010-12-16 12:48:30.000000000 +0100
@@ -868,16 +868,16 @@
   assert( pnBlob);
 
   if( p->pSegments ){
-    rc = sqlite3_blob_reopen(p->pSegments, iBlockid);
-  }else{
-    if( 0==p->zSegmentsTbl ){
-      p->zSegmentsTbl = sqlite3_mprintf("%s_segments", p->zName);
-      if( 0==p->zSegmentsTbl ) return SQLITE_NOMEM;
-    }
-    rc = sqlite3_blob_open(
-       p->db, p->zDb, p->zSegmentsTbl, "block", iBlockid, 0, &p->pSegments
-    );
+    sqlite3_blob_close(p->pSegments);
+    p->pSegments = 0;
   }
+  if( 0==p->zSegmentsTbl ){
+    p->zSegmentsTbl = sqlite3_mprintf("%s_segments", p->zName);
+    if( 0==p->zSegmentsTbl ) return SQLITE_NOMEM;
+  }
+  rc = sqlite3_blob_open(
+     p->db, p->zDb, p->zSegmentsTbl, "block", iBlockid, 0, &p->pSegments
+  );
 
   if( rc==SQLITE_OK ){
     int nByte = sqlite3_blob_bytes(p->pSegments);
EOD

# patch: FTS3 again, for SQLite3 >= 3.6.8
test "$VER3" = "3.6.8" -o "$VER3" = "3.6.9" -o "$VER3" = "3.6.10" \
  -o "$VER3" = "3.6.11" -o "$VER3" = "3.6.12" -o "$VER3" = "3.6.13" \
  -o "$VER3" = "3.6.14" -o "$VER3" = "3.6.14.1" -o "$VER3" = "3.6.14.2" \
  -o "$VER3" = "3.6.15" -o "$VER3" = "3.6.16" -o "$VER3" = "3.6.17" \
  -o "$VER3" = "3.6.18" -o "$VER3" = "3.6.19" -o "$VER3" = "3.6.20" &&
  patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_expr.c	2009-01-01 15:06:13.000000000 +0100
+++ sqlite3/ext/fts3/fts3_expr.c	2009-01-14 09:55:13.000000000 +0100
@@ -57,6 +57,12 @@
 #define SQLITE_FTS3_DEFAULT_NEAR_PARAM 10
 
 #include "fts3_expr.h"
+
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 #include "sqlite3.h"
 #include <ctype.h>
 #include <string.h>
EOD
test "$VER3" = "3.6.17" -o "$VER3" = "3.6.18" -o "$VER3" = "3.6.19" \
  -o "$VER3" = "3.6.20" && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_expr.c   2009-01-01 15:06:13.000000000 +0100
+++ sqlite3/ext/fts3/fts3_expr.c        2009-01-14 09:55:13.000000000 +0100
@@ -428,7 +428,7 @@
     const char *zStr = pParse->azCol[ii];
     int nStr = strlen(zStr);
     if( nInput>nStr && zInput[nStr]==':' 
-     && sqlite3_strnicmp(zStr, zInput, nStr)==0 
+     && memcmp(zStr, zInput, nStr)==0 
     ){
       iCol = ii;
       iColLen = ((zInput - z) + nStr + 1);
EOD
# patch: compile fix for rtree as extension module
patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/rtree/rtree.c	2008-07-16 16:43:35.000000000 +0200
+++ sqlite3/ext/rtree/rtree.c	2008-07-17 08:59:53.000000000 +0200
@@ -2812,7 +2812,7 @@
   return rc;
 }
 
-#if !SQLITE_CORE
+#ifndef SQLITE_CORE
 int sqlite3_extension_init(
   sqlite3 *db,
   char **pzErrMsg,
EOD

# patch: compile fix for rtree as extension module
test "$VER3" = "3.7.3" -o "$VER3" = "3.7.4" -o "$VER3" = "3.7.5" \
  -o "$VER3" = "3.7.6" \
  -o "$VER3" = "3.7.6.1" -o "$VER3" = "3.7.6.2" -o "$VER3" = "3.7.6.3" \
  -o "$VER3" = "3.7.7" -o "$VER3" = "3.7.7.1" -o "$VER3" = "3.7.8" \
  -o "$VER3" = "3.7.9" -o "$VER3" = "3.7.10" -o "$VER3" = "3.7.11" \
  -o "$VER3" = "3.7.12" -o "$VER3" = "3.7.12.1" -o "$VER3" = "3.7.13" \
  -o "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" -o "$VER3" = "3.7.15" \
  -o "$VER3" = "3.7.15.1" -o "$VER3" = "3.7.15.2" -o "$VER3" = "3.7.16" \
  -o "$VER3" = "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/rtree/rtree.c	2010-10-16 10:53:54.000000000 +0200
+++ sqlite3/ext/rtree/rtree.c	2010-10-16 11:12:32.000000000 +0200
@@ -3193,6 +3193,8 @@
   return rc;
 }
 
+#ifdef SQLITE_CORE
+
 /*
 ** A version of sqlite3_free() that can be used as a callback. This is used
 ** in two places - as the destructor for the blob value returned by the
@@ -3257,6 +3259,8 @@
   );
 }
 
+#endif
+
 #ifndef SQLITE_CORE
 int sqlite3_extension_init(
   sqlite3 *db,
EOD

# patch: .read shell command
test "$VER3" = "3.7.6.1" -o "$VER3" = "3.7.6.2" -o "$VER3" = "3.7.6.3" \
  -o "$VER3" = "3.7.7" -o "$VER3" = "3.7.7.1" -o "$VER3" = "3.7.8" \
  -o "$VER3" = "3.7.9" -o "$VER3" = "3.7.10" -o "$VER3" = "3.7.11" \
  -o "$VER3" = "3.7.12" -o "$VER3" = "3.7.12.1" -o "$VER3" = "3.7.13" \
  -o "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/src/shell.c	2011-05-19 15:34:57.000000000 +0200
+++ sqlite3/src/shell.c	2011-06-09 13:36:13.000000000 +0200
@@ -1957,6 +1957,7 @@
     }else{
       rc = process_input(p, alt);
       fclose(alt);
+      if( rc ) rc = 1;
     }
   }else
 
EOD

# patch: FTS3 for 3.7.7 plus missing APIs in sqlite3ext.h/loadext.c
test "$VER3" = "3.7.7" -o "$VER3" = "3.7.7.1" -o "$VER3" = "3.7.8" \
  -o "$VER3" = "3.7.9" -o "$VER3" = "3.7.10" -o "$VER3" = "3.7.11" \
  -o "$VER3" = "3.7.12" -o "$VER3" = "3.7.12.1" -o "$VER3" = "3.7.13" \
  -o "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" -o "$VER3" = "3.7.15" \
  -o "$VER3" = "3.7.15.1" -o "$VER3" = "3.7.15.2" -o "$VER3" = "3.7.16" \
  -o "$VER3" = "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_aux.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/ext/fts3/fts3_aux.c	2011-06-25 06:44:08.000000000 +0200
@@ -14,6 +14,10 @@
 #include "fts3Int.h"
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 
EOD
test "$VER3" = "3.7.7" -o "$VER3" = "3.7.7.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/ext/fts3/fts3.c	2011-06-25 06:48:49.000000000 +0200
@@ -295,10 +295,6 @@
 #include "fts3Int.h"
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
-#if defined(SQLITE_ENABLE_FTS3) && !defined(SQLITE_CORE)
-# define SQLITE_CORE 1
-#endif
-
 #include <assert.h>
 #include <stdlib.h>
 #include <stddef.h>
@@ -3136,7 +3132,7 @@
   return rc;
 }
 
-#if !SQLITE_CORE
+#ifndef SQLITE_CORE
 int sqlite3_extension_init(
   sqlite3 *db, 
   char **pzErrMsg,
EOD
test "$VER3" = "3.7.8" -o "$VER3" = "3.7.9" -o "$VER3" = "3.7.10" \
  -o "$VER3" = "3.7.11" -o "$VER3" = "3.7.12" -o "$VER3" = "3.7.12.1" \
  -o "$VER3" = "3.7.13" -o "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" \
  -o "$VER3" = "3.7.15" -o "$VER3" = "3.7.15.1" -o "$VER3" = "3.7.15.2" \
  -o "$VER3" = "3.7.16" -o "$VER3" = "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3.c	2011-09-19 20:46:52.000000000 +0200
+++ sqlite3/ext/fts3/fts3.c	2011-09-20 09:47:40.000000000 +0200
@@ -295,10 +295,6 @@
 #include "fts3Int.h"
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
-#if defined(SQLITE_ENABLE_FTS3) && !defined(SQLITE_CORE)
-# define SQLITE_CORE 1
-#endif
-
 #include <assert.h>
 #include <stdlib.h>
 #include <stddef.h>
@@ -4826,7 +4822,7 @@
   }
 }
 
-#if !SQLITE_CORE
+#ifndef SQLITE_CORE
 /*
 ** Initialize API pointer table, if required.
 */
EOD
test "$VER3" = "3.7.7" -o "$VER3" = "3.7.7.1" -o "$VER3" = "3.7.8" \
  -o "$VER3" = "3.7.9" -o "$VER3" = "3.7.10" -o "$VER3" = "3.7.11" \
  -o "$VER3" = "3.7.12" -o "$VER3" = "3.7.12.1" -o "$VER3" = "3.7.13" \
  -o "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" -o "$VER3" = "3.7.15" \
  -o "$VER3" = "3.7.15.1" -o "$VER3" = "3.7.15.2" -o "$VER3" = "3.7.16" \
  -o "$VER3" = "3.7.16.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_expr.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/ext/fts3/fts3_expr.c	2011-06-25 06:47:00.000000000 +0200
@@ -18,6 +18,11 @@
 #include "fts3Int.h"
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
+
 /*
 ** By default, this module parses the legacy syntax that has been 
 ** traditionally used by fts3. Or, if SQLITE_ENABLE_FTS3_PARENTHESIS
--- sqlite3.orig/ext/fts3/fts3_snippet.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/ext/fts3/fts3_snippet.c	2011-06-25 06:45:47.000000000 +0200
@@ -13,7 +13,10 @@
 
 #include "fts3Int.h"
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
-
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 
EOD
test "$VER3" = "3.7.7" -o "$VER3" = "3.7.7.1" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/ext/fts3/fts3_tokenizer.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/ext/fts3/fts3_tokenizer.c	2011-06-25 06:50:19.000000000 +0200
@@ -25,7 +25,7 @@
 */
 #include "sqlite3ext.h"
 #ifndef SQLITE_CORE
-  SQLITE_EXTENSION_INIT1
+extern const sqlite3_api_routines *sqlite3_api;
 #endif
 #include "fts3Int.h"
 
--- sqlite3.orig/ext/fts3/fts3_write.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/ext/fts3/fts3_write.c	2011-06-25 06:45:05.000000000 +0200
@@ -20,6 +20,10 @@
 #include "fts3Int.h"
 #if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)
 
+#include "sqlite3ext.h"
+#ifndef SQLITE_CORE
+extern const sqlite3_api_routines *sqlite3_api;
+#endif
 #include <string.h>
 #include <assert.h>
 #include <stdlib.h>
--- sqlite3.orig/src/sqlite3ext.h	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/src/sqlite3ext.h	2011-06-25 07:28:06.000000000 +0200
@@ -212,6 +212,9 @@
   int (*wal_autocheckpoint)(sqlite3*,int);
   int (*wal_checkpoint)(sqlite3*,const char*);
   void *(*wal_hook)(sqlite3*,int(*)(void*,sqlite3*,const char*,int),void*);
+  int (*blob_reopen)(sqlite3_blob*,sqlite3_int64);
+  int (*vtab_config)(sqlite3*,int op,...);
+  int (*vtab_on_conflict)(sqlite3*);
 };
 
 /*
@@ -412,6 +415,9 @@
 #define sqlite3_wal_autocheckpoint     sqlite3_api->wal_autocheckpoint
 #define sqlite3_wal_checkpoint         sqlite3_api->wal_checkpoint
 #define sqlite3_wal_hook               sqlite3_api->wal_hook
+#define sqlite3_blob_reopen            sqlite3_api->blob_reopen
+#define sqlite3_vtab_config            sqlite3_api->vtab_config
+#define sqlite3_vtab_on_conflict       sqlite3_api->vtab_on_conflict
 #endif /* SQLITE_CORE */
 
 #define SQLITE_EXTENSION_INIT1     const sqlite3_api_routines *sqlite3_api = 0;
--- sqlite3.orig/src/loadext.c	2011-06-24 09:06:08.000000000 +0200
+++ sqlite3/src/loadext.c	2011-06-25 07:29:59.000000000 +0200
@@ -84,6 +84,8 @@
 # define sqlite3_create_module 0
 # define sqlite3_create_module_v2 0
 # define sqlite3_declare_vtab 0
+# define sqlite3_vtab_config 0
+# define sqlite3_vtab_on_conflict 0
 #endif
 
 #ifdef SQLITE_OMIT_SHARED_CACHE
@@ -107,6 +109,7 @@
 #define sqlite3_blob_open      0
 #define sqlite3_blob_read      0
 #define sqlite3_blob_write     0
+#define sqlite3_blob_reopen    0
 #endif
 
 /*
@@ -372,6 +375,18 @@
   0,
   0,
 #endif
+#ifndef SQLITE_OMIT_INCRBLOB
+  sqlite3_blob_reopen,
+#else
+  0,
+#endif
+#ifndef SQLITE_OMIT_VIRTUALTABLE
+  sqlite3_vtab_config,
+  sqlite3_vtab_on_conflict,
+#else
+  0,
+  0,
+#endif
 };
 
 /*
EOD
test "$VER3" = "3.7.11" -o "$VER3" = "3.7.12" -o "$VER3" = "3.7.12.1" \
  -o "$VER3" = "3.7.13" -o "$VER3" = "3.7.14" -o "$VER3" = "3.7.14.1" \
  -o "$VER3" = "3.7.15" -o "$VER3" = "3.7.15.1" -o "$VER3" = "3.7.15.2" \
  && patch -d sqlite3 -p1 <<'EOD'
--- sqlite3.orig/src/sqlite3ext.h	2012-03-22 20:13:33.000000000 +0100
+++ sqlite3/src/sqlite3ext.h	2012-03-22 20:13:57.000000000 +0100
@@ -236,6 +236,7 @@
   int (*blob_reopen)(sqlite3_blob*,sqlite3_int64);
   int (*vtab_config)(sqlite3*,int op,...);
   int (*vtab_on_conflict)(sqlite3*);
+  int (*stricmp)(const char*,const char*);
 };
 
 /*
@@ -439,6 +440,7 @@
 #define sqlite3_blob_reopen            sqlite3_api->blob_reopen
 #define sqlite3_vtab_config            sqlite3_api->vtab_config
 #define sqlite3_vtab_on_conflict       sqlite3_api->vtab_on_conflict
+#define sqlite3_stricmp                sqlite3_api->stricmp
 #endif /* SQLITE_CORE */
 
 #define SQLITE_EXTENSION_INIT1     const sqlite3_api_routines *sqlite3_api = 0;
--- sqlite3.orig/src/loadext.c	2012-03-20 15:20:13.000000000 +0100
+++ sqlite3/src/loadext.c	2012-03-22 20:16:24.000000000 +0100
@@ -378,6 +378,7 @@
   sqlite3_blob_reopen,
   sqlite3_vtab_config,
   sqlite3_vtab_on_conflict,
+  sqlite3_stricmp,
 };
 
 /*
EOD

echo "===================="
echo "Preparing TinyCC ..."
echo "===================="
( $notcc && echo '*** skipped (NO_TCCEXT)' ) || true
$notcc || test -r tcc-${TCCVER}.tar.bz2 || \
    wget -c http://download.savannah.nongnu.org/releases/tinycc/tcc-${TCCVER}.tar.bz2
$notcc || test -r tcc-${TCCVER}.tar.bz2 || exit 1

$notcc || rm -rf tcc tcc-${TCCVER}
$notcc || tar xjf tcc-${TCCVER}.tar.bz2
$notcc || ln -sf tcc-${TCCVER} tcc
$notcc || patch -d tcc -p1 < tcc-${TCCVER}.patch

echo "========================"
echo "Cleanup before build ..."
echo "========================"
make -f Makefile.mingw-cross clean
$notv2 || make -C sqlite -f ../mf-sqlite.mingw-cross clean
make -C sqlite3 -f ../mf-sqlite3.mingw-cross clean
make -C sqlite3 -f ../mf-sqlite3fts.mingw-cross clean
make -C sqlite3 -f ../mf-sqlite3rtree.mingw-cross clean
make -f mf-sqlite3extfunc.mingw-cross clean

echo "============================="
echo "Building SQLite 2 ... ISO8859"
echo "============================="
( $nov2 && echo '*** skipped (NO_SQLITE2)' ) || true
$nov2 || make -C sqlite -f ../mf-sqlite.mingw-cross all
if test -n "$SQLITE_DLLS" ; then
    $nov2 || make -C sqlite -f ../mf-sqlite.mingw-cross sqlite.dll
fi

echo "================="
echo "Building zlib ..."
echo "================="

make -C zlib -f ../mf-zlib.mingw-cross all

echo "====================="
echo "Building SQLite 3 ..."
echo "====================="
make -C sqlite3 -f ../mf-sqlite3.mingw-cross all
test -r sqlite3/tool/mksqlite3c.tcl && \
  make -C sqlite3 -f ../mf-sqlite3.mingw-cross sqlite3.c
if test -r sqlite3/sqlite3.c -a -f "$WITH_SEE" ; then
    cat sqlite3/sqlite3.c "$WITH_SEE" >sqlite3.c
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_HAS_CODEC=1"
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_ACTIVATION_KEY=\\\"$SEE_KEY\\\""
    ADD_CFLAGS="$ADD_CFLAGS -DSEEEXT=\\\"$SEEEXT\\\""
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_API=static -DWIN32=1 -DNDEBUG=1 -DNO_TCL"
    ADD_CFLAGS="$ADD_CFLAGS -DTHREADSAFE=1 -DSQLITE_OMIT_EXPLAIN=1"
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_DLL=1 -DSQLITE_TRHEADSAFE=1"
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_OS_WIN=1 -DSQLITE_ASCII=1"
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_SOUNDEX=1"
    ADD_CFLAGS="$ADD_CFLAGS -DSQLITE_ENABLE_COLUMN_METADATA=1"
    ADD_CFLAGS="$ADD_CFLAGS -DWITHOUT_SHELL=1"
    export ADD_CFLAGS
    ADD_NSIS="$ADD_NSIS -DWITHOUT_SQLITE3_EXE"
    unset SQLITE3_A10N_O
    unset SQLITE3_EXE
fi
if test -n "$SQLITE_DLLS" ; then
    make -C sqlite3 -f ../mf-sqlite3.mingw-cross sqlite3.dll
fi

echo "==================="
echo "Building TinyCC ..."
echo "==================="
( $notcc && echo '*** skipped (NO_TCCEXT)' ) || true
$notcc || ( cd tcc ; sh mingw-cross-build.sh )
# copy SQLite headers into TCC install include directory
$notcc || $nov2 || cp -p sqlite/sqlite.h TCC/include
$notcc || cp -p sqlite3/sqlite3.h sqlite3/src/sqlite3ext.h TCC/include
# copy LGPL to TCC install doc directory
$notcc || cp -p tcc-${TCCVER}/COPYING TCC/doc
$notcc || cp -p tcc-${TCCVER}/README TCC/doc/readme.txt

echo "==============================="
echo "Building ODBC drivers and utils"
echo "==============================="
if $nov2 ; then
    make -f Makefile.mingw-cross all_no2
else
    make -f Makefile.mingw-cross
fi
make -f Makefile.mingw-cross sqlite3odbc${SEEEXT}nw.dll

echo "=========================="
echo "Building SQLite 2 ... UTF8"
echo "=========================="
( $nov2 && echo '*** skipped (NO_SQLITE2)' ) || true
$nov2 || make -C sqlite -f ../mf-sqlite.mingw-cross clean
$nov2 || make -C sqlite -f ../mf-sqlite.mingw-cross ENCODING=UTF8 all
if test -n "$SQLITE_DLLS" ; then
    $nov2 || \
       make -C sqlite -f ../mf-sqlite.mingw-cross ENCODING=UTF8 sqliteu.dll
fi

echo "========================="
echo "Building drivers ... UTF8"
echo "========================="
( $nov2 && echo '*** skipped (NO_SQLITE2)' ) || true
$nov2 || make -f Makefile.mingw-cross sqliteodbcu.dll sqliteu.exe

echo "==================================="
echo "Building SQLite3 FTS extensions ..."
echo "==================================="
make -C sqlite3 -f ../mf-sqlite3fts.mingw-cross clean all
mv sqlite3/sqlite3_mod_fts*.dll .

echo "====================================="
echo "Building SQLite3 rtree extensions ..."
echo "====================================="
make -C sqlite3 -f ../mf-sqlite3rtree.mingw-cross clean all
mv sqlite3/sqlite3_mod_rtree.dll .

echo "========================================"
echo "Building SQLite3 extension functions ..."
echo "========================================"
make -f mf-sqlite3extfunc.mingw-cross clean all

echo "============================"
echo "Building DLL import defs ..."
echo "============================"
# requires wine: create .def files with tiny_impdef.exe
# for all .dll files which provide SQLite
( $notcc && echo '*** skipped (NO_TCCEXT)' ) || true
$notcc || $nov2 || wine TCC/tiny_impdef.exe \
  sqliteodbc.dll -o TCC/lib/sqlite.def
$notcc || $nov2 || wine TCC/tiny_impdef.exe \
  sqliteodbcu.dll -o TCC/lib/sqliteu.def
$notcc || wine TCC/tiny_impdef.exe sqlite3odbc.dll -o TCC/lib/sqlite3.def

if test -n "$SQLITE_DLLS" ; then
    $nov2 || mv sqlite/sqlite.dll .
    $nov2 || mv sqlite/sqliteu.dll .
    mv sqlite3/sqlite3.dll .
fi

if test -n "$SQLITE_DLLS" ; then
    $notcc || $nov2 || wine TCC/tiny_impdef.exe \
      sqlite.dll -o TCC/lib/sqlite.def
    $notcc || $nov2 || wine TCC/tiny_impdef.exe \
      sqliteu.dll -o TCC/lib/sqliteu.def
    $notcc || wine TCC/tiny_impdef.exe sqlite3.dll -o TCC/lib/sqlite3.def
fi

echo "======================="
echo "Cleanup after build ..."
echo "======================="
$nov2 || make -C sqlite -f ../mf-sqlite.mingw-cross clean
$nov2 || rm -f sqlite/sqlite.exe
mv sqlite3/sqlite3.c sqlite3/sqlite3.amalg
make -C sqlite3 -f ../mf-sqlite3.mingw-cross clean
rm -f sqlite3/sqlite3.exe
make -C sqlite3 -f ../mf-sqlite3fts.mingw-cross clean
make -C sqlite3 -f ../mf-sqlite3rtree.mingw-cross clean
mv sqlite3/sqlite3.amalg sqlite3/sqlite3.c
make -f mf-sqlite3extfunc.mingw-cross semiclean

echo "==========================="
echo "Creating NSIS installer ..."
echo "==========================="
cp -p README readme.txt
unix2dos < license.terms > license.txt || todos < license.terms > license.txt
$notcc || unix2dos -k TCC/doc/COPYING || unix2dos -p TCC/doc/COPYING || \
  todos -p TCC/doc/COPYING
$notcc || unix2dos -k TCC/doc/readme.txt || unix2dos -p TCC/doc/readme.txt || \
  todos -p TCC/doc/readme.txt
makensis $ADD_NSIS sqliteodbc.nsi

