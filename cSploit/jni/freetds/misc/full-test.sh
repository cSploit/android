#!/bin/sh

# test a test using watching for errors and logging all

log () {
	echo "@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@- $1 -@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@"
}

FILE=`echo "$1" | sed "s,^\\./,$PWD/,"`

#execute with valgrind
RES=0
VG=0
if test -f "$HOME/bin/vg_test"; then
	log "START $1"
	log "TEST 1"
	log "VALGRIND 1"
	classifier --timeout=600 --num-fd=3 "$HOME/bin/vg_test" "$@"
	RES=$?
	log "RESULT $RES"
	log "FILE $FILE"
	log "END $1"
	VG=1
fi

# try to execute normally
if test $RES != 0 -o $VG = 0; then
	log "START $1"
	log "TEST 1"
	classifier --timeout=600 "$@"
	RES=$?
	log "RESULT $RES"

	# log test executed to retrieve lately by main script
	log "FILE $FILE"
	log "END $1"
fi

# return always succes, test verified later
exit 0

