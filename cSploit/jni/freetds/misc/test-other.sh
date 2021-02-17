#!/bin/sh

# Additional make check test
# check other tools (like Perl and PHP)
# odbc MUST be configured to point to source driver

# stop on errors
set -e

verbose=no
do_perl=no
do_php=yes

# check for perl existence
if perl -e 'exit 0' > /dev/null 2>&1; then
	do_perl=yes
fi

for param
do
	case $param in
	--verbose)
		verbose=yes ;;
	--no-perl)
		do_perl=no ;;
	--no-php)
		do_php=no ;;
	--perl-only)
		do_perl=yes
		do_php=no
		;;
	--php-only)
		do_perl=no
		do_php=yes
		;;
	--help)
		echo "Usage: $0 [OPTION]..."
		echo '  --help          this help'
		echo '  --verbose       verbose output'
		echo '  --no-perl       do not execute Perl test'
		echo '  --no-php        do not execute PHP test'
		echo '  --perl-only     only Perl test'
		echo '  --php-only      only PHP test'
		exit 0
		;;
	*)
		echo 'Option not supported!' 1>&2
		exit 1
		;;
	esac
done

log () {
        echo "@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@- $1 -@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@"
}

# set correct directory
DIR=`dirname $0`
cd "$DIR/.."

# save directory
ORIGDIR="$PWD"

# init log
LOG="$PWD/test-other.log"
rm -f "$LOG"
echo "log started" >> "$LOG"

## assure make
#make all
#echo "make all ok" >> "$LOG"

tUID=`cat PWD | grep '^UID=' | sed 's,^....,,g'`
tPWD=`cat PWD | grep '^PWD=' | sed 's,^....,,g'`
tSRV=`cat PWD | grep '^SRV=' | sed 's,^....,,g'`
tDB=`cat PWD | grep '^DB=' | sed 's,^...,,g'`

# prepare odbc.ini redirection
rm -f odbc.ini
for f in .libs/libtdsodbc.so .libs/libtdsodbc.sl .libs/libtdsodbc.dll .libs/libtdsodbc.dylib; do
	if test -r "$PWD/src/odbc/$f"; then
		echo "[$tSRV]
Driver = $PWD/src/odbc/$f
Database = $tDB
Servername = $tSRV

[xx_$tSRV]
Driver = $PWD/src/odbc/$f
Database = $tDB
Servername = $tSRV" > odbc.ini
		ODBCINI="$PWD/odbc.ini"
		SYSODBCINI="$PWD/odbc.ini"
		export ODBCINI SYSODBCINI
		break;
	fi
done

# Perl
if test $do_perl = yes; then
	DBI_DSN="dbi:ODBC:$tSRV"
	DBI_USER="$tUID"
	DBI_PASS="$tPWD"
	export DBI_DSN DBI_USER DBI_PASS
	# TODO better way
	ODBCHOME=/usr
	export ODBCHOME
	for f in DBD-ODBC-*.tar.gz; do
		DIR=`echo "$f" | sed s,.tar.gz$,,g`
		echo Testing $DIR
		if ! test -d "$DIR"; then
			tar zxvf "$DIR.tar.gz"
			# try to apply patch for Sybase
			cd "$DIR"
			{ patch -p1 || true ; } <<EOF
diff -ru DBD-ODBC-1.13/t/07bind.t DBD-ODBC-1.13my/t/07bind.t
--- DBD-ODBC-1.13/t/07bind.t	2005-02-20 10:09:17.039561424 +0100
+++ DBD-ODBC-1.13my/t/07bind.t	2004-12-18 15:19:11.000000000 +0100
@@ -133,7 +133,7 @@
 		  # expect!
 		  \$row[2] = "";
 	       }
-	       if (\$row[2] ne \$_->[2]) {
+	       if (\$row[2] ne \$_->[2] && (\$dbname ne "sql server" || \$row[2] ne " ") ) {
 		  print "Column C value failed! bind value = \$bind_val, returned values = \$row[0]|\$row[1]|\$row[2]|\$row[3]\\n";
 		  return undef;
 	       }
EOF
			# fix for DBD::ODBC and Perl 5.8.8
			echo "--- DBD-ODBC-1.13.orig/dbdimp.c	2004-11-05 04:19:36.000000000 +0100
+++ DBD-ODBC-1.13/dbdimp.c	2007-01-16 09:38:20.477774620 +0100
@@ -2921,11 +2921,11 @@
     * */
    else if (is_inout != phs->is_inout) {
       croak(\"Can't rebind or change param %s in/out mode after first bind (%d => %d)\",
 	    phs->name, phs->is_inout, is_inout);
    }
-   else if (maxlen && maxlen != phs->maxlen) {
+   else if (maxlen && maxlen > phs->maxlen) {
       croak(\"Can't change param %s maxlen (%ld->%ld) after first bind\",
 	    phs->name, phs->maxlen, maxlen);
    }
 
    if (!is_inout) {    /* normal bind to take a (new) copy of current value    */
" | patch -p1 || true
			cd t
			{ patch -p0 || true ; }  <<EOF
--- 20SqlServer.t.orig  2005-08-09 13:33:30.000000000 +0200
+++ 20SqlServer.t       2005-08-09 13:37:36.000000000 +0200
@@ -419,7 +419,7 @@

    \$dbh->{odbc_async_exec} = 1;
    # print "odbc_async_exec is: \$dbh->{odbc_async_exec}\\n";
-   is(\$dbh->{odbc_async_exec}, 1, "test odbc_async_exec attribute set");
+   is(\$dbh->{odbc_async_exec}, 0, "test odbc_async_exec attribute NOT set");

    # not sure if this should be a test.  May have permissions problems, but it's the only sample
    # of the error handler stuff I have.
EOF
			cd ../..
		fi
		test -d "$DIR"
		cd "$DIR"
		grep -v 'prompt(' Makefile.PL > Makefile.PL.tmp
		cat Makefile.PL.tmp > Makefile.PL
		test -r Makefile || LANG=C perl Makefile.PL
		LANG=C make clean
		RES=0
		log "START DBD::ODBC make"
		test -r Makefile || LANG=C classifier perl Makefile.PL || RES=$?
		if test $RES = 0; then
			LANG=C classifier make || RES=$?
		fi
		log "RESULT $RES"
		log "END DBD::ODBC make"
		if test $RES = 0; then
			log "START DBD::ODBC test"
			if test $verbose = yes; then
				LANG=C classifier make test TEST_VERBOSE=1 || RES=$?
			else
				LANG=C classifier make test || RES=$?
			fi
			log "RESULT $RES"
			log "END DBD::ODBC test"
		fi
		if test $RES != 0; then
			exit $RES
		fi
	done
fi

# PHP
cd "$ORIGDIR"
FILE='php5.2-latest.tar.bz2'
if test $do_php = yes -a -f "$FILE"; then
	# need to recompile ??
	if test ! -x phpinst/bin/php -o "$FILE" -nt phpinst/bin/php; then
		rm -rf php5.2-201* phpinst lib
		bunzip2 -c "$FILE" | tar xvf -
		DIR=`echo php5.2-201*`
		MAINDIR=$PWD
		mkdir lib
		cp src/dblib/.libs/lib*.s[ol]* lib
		cp src/tds/.libs/libtds.a lib
#		cp src/tds/.libs/lib*.s[ol]* lib
		cd $DIR
		CFLAGS='-O0 -pipe' ./configure --prefix=$MAINDIR/phpinst --disable-all --with-mssql=$MAINDIR  --with-unixODBC=/usr
		make -j4
		make install
		cd ..
	fi

	cd phptests
	echo "<?php \$server = \"$tSRV\"; \$user = \"$tUID\"; \$pass = \"$tPWD\"; ?>" > pwd.inc

	ERR=""
	for f in *.php; do
		echo Testing PHP script $f
		log "START PHP test $f"
		RES=0
		../phpinst/bin/php -q $f 2>&1 || { RES=$?; ERR="$ERR $f"; }
		log "RESULT $RES"
		log "FILE $PWD/$f"
		log "END PHP test $f"
	done
	if test "$ERR" != ""; then
		echo "Some script failed:$ERR"
		exit 1
	fi
fi

echo "all tests ok!!!" >> "$LOG"
exit 0

