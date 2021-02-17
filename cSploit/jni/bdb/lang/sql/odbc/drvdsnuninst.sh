#!/bin/sh
#
# Uninstall ODBC driver and system DSN
#    $1   driver name
#    $2   DSN name

which odbcinst > /dev/null || {
    echo >&2 "no usable odbcinst program"
    exit 1
}

odbcinst -u -d -n "$1" || true
odbcinst -u -l -s -n "$2" || true
