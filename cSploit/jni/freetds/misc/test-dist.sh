#!/bin/sh

# This script test that Makefiles are able to produce a good
# distribution
#
# It create a test-dist.log file in the main directory, you
# can tail this file to see the progress

# stop on errors
set -e

for param
do
	case $param in
	--no-unittests)
		TESTS_ENVIRONMENT=true
		export TESTS_ENVIRONMENT
		;;
	--help)
		echo "Usage: $0 [OPTION]..."
		echo '  --help          this help'
		echo '  --no-unittests  do not execute unittests'
		exit 0
		;;
	*)
		echo 'Option not supported!' 1>&2
		exit 1
		;;
	esac
done

# do not create logs so diskcheck test does not fail
unset TDSDUMP || true

# set correct directory
DIR=`dirname $0`
cd "$DIR/.."

# remove old distributions
touch freetds-dummy
find freetds-* -type d ! -perm -200 -exec chmod u+w {} ';'
rm -rf freetds-*

# save directory
ORIGDIR="$PWD"

# init log
LOG="$PWD/test-dist.log"
rm -f "$LOG"
echo "log started" >> "$LOG"

# make distribution
test -f Makefile || sh autogen.sh
make dist
echo "make distribution ok" >> "$LOG"

# untar to test it, should already contains documentation
DIR=`echo freetds-*.tar.bz2 | sed s,.tar.bz2$,,g`
bzip2 -dc freetds-*.tar.bz2 | tar xf -
test -d "$DIR"
cd "$DIR"
echo "untar ok" >> "$LOG"

# assure you do not compile documentation again
mkdir fakebin
PATH="$PWD/fakebin:$PATH"
export PATH
cd fakebin
echo "#!/bin/sh
echo \"\$0 should not be called\" >&2
exit 1" > openjade
cp openjade doxygen
cp openjade txt2man
cp openjade autoheader
# perl is used by some perl rules
cp openjade perl
chmod +x openjade doxygen txt2man autoheader perl
cd ..
if ! openjade --help; then true; else echo 'succedeed ?'; false; fi
if ! doxygen --help; then true; else echo 'succeeded ?'; false; fi
if ! txt2man --help; then true; else echo 'succeeded ?'; false; fi
if ! autoheader --help; then true; else echo 'succeeded ?'; false; fi
if ! perl --help; then true; else echo 'succeeded ?'; false; fi
echo "fakebin ok" >> "$LOG"

# direct make install (without make all)
mkdir install
INSTALLDIR="$PWD/install"
mkdir build
cd build
# --enable-msdblib --enable-sybase-compat can cause also problems, try to compile with both
../configure --prefix="$INSTALLDIR" --enable-msdblib --enable-sybase-compat --disable-libiconv
# make clean should not cause problems here
make clean
make install
cd ..
echo "direct make install ok" >> "$LOG"

# cleanup
rm -rf install build

# again with dist and autogen
mkdir build
cd build
../autogen.sh
make dist
cd ..
rm -rf build
echo "make dist ok" >> "$LOG"

# test if make clean clean too much
mkdir install
./configure --prefix="$PWD/install"
make clean
make dist

# finally big test. I hope you have a fast machine :)
cd ..
rm -rf "$DIR"
bzip2 -dc freetds-*.tar.bz2 | tar xf -
cd "$DIR"
./configure
if test ! -e PWD -a -e "$ORIGDIR/../PWD"; then
	cp "$ORIGDIR/../PWD" .
fi
if test ! -e PWD -a -e "$ORIGDIR/PWD"; then
	cp "$ORIGDIR/PWD" .
fi
make distcheck
echo "make distcheck ok" >> "$LOG"

# cleanup
cd "$ORIGDIR"
chmod -R 777 "$DIR"
rm -rf "$DIR"

# check rpm
RPMCMD=rpm
if rpmbuild --help > /dev/null 2>&1; then
	RPMCMD=rpmbuild
fi
if $RPMCMD --help > /dev/null 2>&1; then
	$RPMCMD -ta freetds-*.tar.bz2 || exit 1
	echo "rpm test ok" >> "$LOG"
else
	echo "rpm test skipped, no rpm detected" >> "$LOG"
fi

echo "all tests ok!!!" >> "$LOG"

