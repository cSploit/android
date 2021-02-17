#!/bin/sh

# MacOS X do-hickie to run ff2csv.py, with parameters, by double-click.


flags="hq"
Usage () {
    args="$*"
    if [[ -n "$args" ]] ; then
        echo >&2 "$args"
    fi
    echo >&2 "Usage: $0 [-$flags] [querysetfile [csvfile]]
Run ff2csv.py, fetching and summarizing SVN bug status."
}
while getopts $flags flag; do
    case "$flag" in
        h|q) Usage; exit 0;;
    esac
done

# we want to run in the same folder as this script, not
# the users home folder
cd `dirname $0`


date=`date +%m%d`
./ff2csv.py ${1:-query-set-1-$date.tsv} ${2:-core-history-$date.csv}
