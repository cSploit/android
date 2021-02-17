#!/bin/bash

# These commands prepare for win32 distribution

# It uses MingW to cross compile
# Useful for testing ODBC under Windows
# With --ntwdblib option you can compile dblib tests against MS dblib so
# to test tests with MS dblib

errore() {
	echo $* >&2
	exit 1
}

NTWDBLIB=no
TYPE=win32
HOSTS='i686-w64-mingw32 i586-mingw32msvc i386-mingw32'
ARCHIVE='tar jcvf "freetds-$PACKAGE_VERSION.$TYPE.tar.bz2" "freetds-$PACKAGE_VERSION"'
PACK=yes
ONLINE=no

for param
do
	case $param in
	--win64)
		HOSTS='x86_64-w64-mingw32 amd64-mingw32msvc x86_64-pc-mingw32'
		TYPE=win64
		;;
	--zip)
		ARCHIVE='zip -r9 "freetds-$PACKAGE_VERSION.$TYPE.zip" "freetds-$PACKAGE_VERSION"'
		;;
	--gzip)
		ARCHIVE='tar zcvf "freetds-$PACKAGE_VERSION.$TYPE.tar.gz" "freetds-$PACKAGE_VERSION"'
		;;
	--ntwdblib)
		NTWDBLIB=yes
		;;
	--no-pack)
		PACK=no
		;;
	--online)
		PACK=no
		ONLINE=yes
		;;
	--help)
		echo "Usage: $0 [OPTION]..."
		echo '  --help          this help'
		echo '  --win64         compile for win64'
		echo '  --zip           compress with zip'
		echo '  --gzip          compress with gzip'
		echo '  --ntwdblib      use ntwdblib instead of our'
		echo "  --no-pack       don't clean and pack files"
		echo "  --online        build on same directory"
		exit 0
		;;
	*)
		echo 'Option not supported! Try --help' 1>&2
		exit 1
		;;
	esac
done

# search valid HOST
HOST=
for h in $HOSTS; do
	if $h-gcc --help > /dev/null 2> /dev/null; then
		HOST=$h
		break
	fi
done
if test x$HOST = x; then
	echo "Valid host not found in $HOSTS" 1>&2
	exit 1
fi

PACKAGE_VERSION=
eval `grep '^PACKAGE_VERSION=' configure`
if test "$PACKAGE_VERSION" = ""; then
	VERSION=
	eval `grep '^ \?VERSION=' configure`
	PACKAGE_VERSION="$VERSION"
fi
test "$PACKAGE_VERSION" != "" || errore "PACKAGE_VERSION not found"
if test $ONLINE = no; then
	test -r "freetds-$PACKAGE_VERSION.tar.gz" -o -r "freetds-$PACKAGE_VERSION.tar.bz2" || make dist
	test -r "freetds-$PACKAGE_VERSION.tar.gz" -o -r "freetds-$PACKAGE_VERSION.tar.bz2" || errore "package not found"
	rm -rf "freetds-$PACKAGE_VERSION"
	if test -r "freetds-$PACKAGE_VERSION.tar.gz"; then
		gunzip -dc "freetds-$PACKAGE_VERSION.tar.gz" | tar xvf -
	else
		bunzip2 -dc "freetds-$PACKAGE_VERSION.tar.bz2" | tar xvf -
	fi
	cd "freetds-$PACKAGE_VERSION" || errore "Directory not found"
else
	make clean
fi
if ! $HOST-gcc --help > /dev/null 2> /dev/null; then
	echo $HOST-gcc not found >&2
	exit 1
fi

# build
trap 'echo Error at line $LINENO' ERR
set -e
CFLAGS='-O2 -g -pipe' ./configure --host=$HOST --build=i686-pc-linux-gnu
cp libtool libtool.tmp
cat libtool.tmp | sed -e 's,file format pe-i386,file format pe-(x86-64|i386),' > libtool
make -j4 2> errors.txt
TESTS_ENVIRONMENT=true make -j4 check 2>> errors.txt

# compile dblib test against MS dblib
if test "$NTWDBLIB" = "yes"; then
	cd src/dblib
	# generate new libsybdb.dll.a
	[ -f .libs/libsybdb.dll.a ] || errore "library libsybdb.dll.a not found"

	# replace libsybdb.dll.a
	cat > ntwdblib.def <<"_EOF"
EXPORTS
SQLDebug
abort_xact
bcpDefaultPrefix
bcp_batch
bcp_bind
bcp_colfmt
bcp_collen
bcp_colptr
bcp_columns
bcp_control
bcp_done
bcp_exec
bcp_init
bcp_moretext
bcp_readfmt
bcp_sendrow
bcp_setl
bcp_writefmt
build_xact_string
close_commit
commit_xact
dbadata
dbadlen
dbaltbind
dbaltcolid
dbaltlen
dbaltop
dbalttype
dbaltutype
dbanullbind
dbbcmd
dbbind
dbbylist
dbcancel
dbcanquery
dbchange
dbclose
dbclrbuf
dbclropt
dbcmd
dbcmdrow
dbcolbrowse
dbcolinfo
dbcollen
dbcolname
dbcolntype
dbcolsource
dbcoltype
dbcolutype
dbconnectionread
dbconvert
dbcount
dbcurcmd
dbcurrow
dbcursor
dbcursorbind
dbcursorclose
dbcursorcolinfo
dbcursorfetch
dbcursorfetchex
dbcursorinfo
dbcursorinfoex
dbcursoropen
dbdata
dbdataready
dbdatecrack
dbdatlen
dbdead
dbenlisttrans
dbenlistxatrans
dberrhandle
dbexit
dbfcmd
dbfirstrow
dbfreebuf
dbfreelogin
dbfreequal
dbgetchar
dbgetmaxprocs
dbgetoff
dbgetpacket
dbgetrow
dbgettime
dbgetuserdata
dbhasretstat
dbinit
dbisavail
dbiscount
dbisopt
dblastrow
dblocklib
dblogin
dbmorecmds
dbmoretext
dbmsghandle
dbname
dbnextrow
dbnullbind
dbnumalts
dbnumcols
dbnumcompute
dbnumorders
dbnumrets
dbopen
dbordercol
dbprhead
dbprocerrhandle
dbprocinfo
dbprocmsghandle
dbprrow
dbprtype
dbqual
dbreadpage
dbreadtext
dbresults
dbretdata
dbretlen
dbretname
dbretstatus
dbrettype
dbrows
dbrowtype
dbrpcexec
dbrpcinit
dbrpcparam
dbrpcsend
dbrpwclr
dbrpwset
dbserverenum
dbsetavail
dbsetlname
dbsetlogintime
dbsetlpacket
dbsetmaxprocs
dbsetnull
dbsetopt
dbsettime
dbsetuserdata
dbsqlexec
dbsqlok
dbsqlsend
dbstrcpy
dbstrlen
dbtabbrowse
dbtabcount
dbtabname
dbtabsource
dbtsnewlen
dbtsnewval
dbtsput
dbtxptr
dbtxtimestamp
dbtxtsnewval
dbtxtsput
dbunlocklib
dbupdatetext
dbuse
dbvarylen
dbwillconvert
dbwinexit
dbwritepage
dbwritetext
open_commit
remove_xact
start_xact
stat_xact
_EOF
	$HOST-dlltool --dllname ntwdblib.dll --input-def ntwdblib.def --output-lib .libs/libsybdb.dll.a
	cd ../..

	# replace includes
	cd include
	rm -rf OLD
	mkdir OLD
	mv dblib.h sqldb.h sqlfront.h sybdb.h syberror.h sybfront.h OLD
	cp $HOME/cpp/freetds/dblib/sqldb.h sqldb.h
	cp $HOME/cpp/freetds/dblib/sqlfront.h sqlfront.h
	cd ..

	# rebuild tests
	cd src/dblib/unittests
	rm -f *.o *.exe
	make clean
	perl -pi.orig -e '$_ =~ s/$/ -DDBNTWIN32/ if (/^CPPFLAGS\s*=/)' Makefile
	TESTS_ENVIRONMENT=true make -j4 check 2>> ../../../errors.txt
	cd ../../../include
	rm -f sqldb.h sqlfront.h
	mv OLD/* .
	rmdir OLD
	cd ..
fi
cat errors.txt | grep -v '^.libs/lt-\|^mkdir: cannot create directory ..libs.: File exists$' | perl -ne 'if (/visibility attribute not supported in this configuration/) {$ignore=1;$_=""} else {$ignore=0} ; print $old if !$ignore; $old=$_; END { print $old }' > ../${TYPE}_errors.txt
rm errors.txt

if test "$PACK" = "no"; then
	echo "No packaging requested, exiting"
	exit 0
fi

rm -rf include doc
find -name Makefile.in  |xargs rm
find -name \*.in  | grep -v 'unittests\|PWD' | xargs rm
find \( -type f -o -type l \) -a \( ! -name \*.bat -a ! -name \*.exe -a ! -name \*.def -a ! -name \*.dll -a ! -name \*.be -a ! -name \*.conf -a ! -name \*.exp -a ! -name \*.bin -a ! -name \*.in -a ! -name \*.sql \) | xargs rm

find -name \*.exe | grep '/\.libs/' | while read X; do mv "${X%/.libs/*}/.libs/${X#*/.libs/}" "${X%/.libs/*}/${X#*/.libs/}"; done
find -type d -exec rmdir {} \; 2> /dev/null || true
find src -name .libs -type d | while read X; do mv "$X"/* "${X%/.libs}"; done
# copy dll inside unitests directories
find src -name \*.dll | while read X; do D=${X%/*}; test -d "$D/unittests" && cp $X "$D/unittests"; mv $X src/apps; done
mv src/apps/libtdsodbc-0.dll win32/FreeTDS.dll
sed 's,Debug\\,,' < win32/installfreetds.bat > win32/installfreetds.bat.new && mv -f win32/installfreetds.bat.new win32/installfreetds.bat

# remove empty directories
find -type d -exec rmdir {} \; 2> /dev/null || true
find -type d -exec rmdir {} \; 2> /dev/null || true
find -type d -exec rmdir {} \; 2> /dev/null || true

# strip any dll or exe
find -name \*.dll | xargs $HOST-strip
find -name \*.exe | xargs $HOST-strip

echo '@echo off
del err.txt ok.txt
for %%f in (*.exe) do call :processa %%f
goto fine

:processa
echo processing %1 ...
echo start test %1>out.txt
set OK=1
%1 >> out.txt 2>&1
if errorlevel 1 set OK=0
echo ok %OK%
echo end test %1>>out.txt
echo.>>out.txt
echo.>>out.txt
echo.>>out.txt
echo.>>out.txt

if "%OK%"=="0" type out.txt >> err.txt
if "%OK%"=="1" type out.txt >> ok.txt

:fine
' > src/odbc/unittests/go.bat
unix2dos src/odbc/unittests/go.bat
ln src/odbc/unittests/go.bat src/dblib/unittests/go.bat

cd ..
eval "$ARCHIVE"
