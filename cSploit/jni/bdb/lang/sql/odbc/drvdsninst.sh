#!/bin/sh
#
# Install ODBC driver and system DSN
#    $1   driver name
#    $2   DSN name
#    $3   libtool .la file
#    $4   directory where libtool installs shared library

which odbcinst > /dev/null || {
    echo >&2 "no usable odbcinst program"
    exit 1
}

DRVNAME=$1
DRVNAME_BR='['"$1"']'

DSNNAME=$2
DSNNAME_BR='['"$2"']'

. $3 || exit 1

for n in $library_names ; do
    true
done
if [ -z "$n" ] ; then
    echo >&2 "no shared library name for driver $DRVNAME in $3"
    exit 1
fi
DRVSHLIB="$4/$n"
if [ ! -r "$DRVSHLIB" ] ; then
    echo >&2 "no shared library for driver $DRVNAME in $4"
    exit 1
fi

cat > /tmp/drvinst.$$ << __EOD__
$DRVNAME_BR
Description=$DRVNAME
Driver=$DRVSHLIB
Setup=$DRVSHLIB
FileUsage=1
__EOD__

odbcinst -q -d -n "$DRVNAME" | fgrep "$DRVNAME_BR" > /dev/null || {
    odbcinst -i -d -n "$DRVNAME" -f /tmp/drvinst.$$ || true
}
rm -f /tmp/drvinst.$$

cat > /tmp/dsninst.$$ << __EOD__
$DSNNAME_BR
Driver=$DRVNAME
__EOD__

odbcinst -q -s -n "$DSNNAME" | fgrep "$DSNNAME_BR" > /dev/null || {
    odbcinst -i -l -s -n "$DSNNAME" -f /tmp/dsninst.$$ || true
}
rm -f /tmp/dsninst.$$
