#! /bin/sh
#
# MVCC tests, consists of 3 rounds which test:
# 1. No MVCC
# 2. MVCC enabled by DB_CONFIG
# 3. MVCC enable by flags.

msg()
{
	test "$DVERBOSE" == 1 && {
		echo "========"
		echo "======== $1"
		echo "========"
	}
}

init_tmadmin()
{
tmadmin << END_OF_TMADMIN
	crdl -z $TLOGDEVICE -b 500
	crlog -m cluster3
END_OF_TMADMIN
}


# Everything else is done in run/bin.
cd $RUN/bin

# If testing DB_CONFIG enabled MVCC move the file to the 
# environment directory
if [ $1 -eq 1 ]; then
    cp ../../src5/DB_CONFIG ../data/DB_CONFIG
fi

# The CFLAGS variable defines the pre-processor defines -- start with
# whatever the user set, and add our own stuff.
#
# For debugging output, add -DDVERBOSE 

test "$DVERBOSE" == 1 && {
	COMPILE_FLAGS="-DDVERBOSE"
	DVERBOSE_FLAG="-v"
}
COMPILE_FLAGS="$CFLAGS $COMPILE_FLAGS -g -I../../.."
UTILITY_FILES="-f ../../utilities/bdb_xa_util.c"

msg "BUILDING CLIENT"
CFLAGS="$COMPILE_FLAGS"; export CFLAGS
buildclient -r BERKELEY-DB $DVERBOSE_FLAG -o client \
$UTILITY_FILES -f ../../src5/client.c
test "$?" -eq 0 || {
	echo "FAIL: buildclient failed."
	exit 1
}

msg "BUILDING SERVER for DATABASE 1"
# Build the server database with DB_MULITVERSION
if [ $1 -eq 2 ]; then
	COMPILE_FLAGS="-DMVCC_FLAG $COMPILE_FLAGS"
fi
CFLAGS="$COMPILE_FLAGS"; export CFLAGS
buildserver -r BERKELEY-DB $DVERBOSE_FLAG -t -o server1 \
  -s read_db1 -s write_db1 $UTILITY_FILES -f ../../src5/server.c
test "$?" -eq 0 || {
	echo "FAIL: buildserver 1 failed."
	exit 1
}

msg "BUILDING THE RESOURCE MANAGER."
buildtms -o DBRM -r BERKELEY-DB

init_tmadmin

# Boot Tuxedo.
# You should see something like:
#
# Booting admin processes ...
#
# exec BBL -A :
#         process id=13845 ... Started.
#
# Booting server processes ...
#
# exec DBRM -A :
#         process id=13846 ... Started.
# exec DBRM -A :
#         process id=13847 ... Started.
# exec server1 -A :
#         process id=13848 ... Started.
# exec server2 -A :
#         process id=13849 ... Started.
# 5 processes started.
msg "BOOTING TUXEDO."
tmboot -y

exitval=0
./client -t $1 $DVERBOSE_FLAG 2>> stderr
test "$?" -ne 0 && {
	echo "FAIL: client failed"
	exitval=1
	break;
}

msg "SHUTTING DOWN THE TRANSACTION MANAGER."
echo 'y' | tmshutdown

# Copy out any server output.
echo "STDOUT:"
cat stdout  

# Copy out any server errors.
test -s stderr && {
        echo "STDERR:"
	cat stderr
	echo "FAIL: stderr file not empty"
	exitval=1
}

exit $exitval
