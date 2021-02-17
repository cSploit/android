#! /bin/sh
#
# Run test 3.

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
$UTILITY_FILES -f ../../src3/client.c
test "$?" -eq 0 || {
	echo "FAIL: buildclient failed."
	exit 1
}

msg "BUILDING SERVER #1"
CFLAGS="$COMPILE_FLAGS -DSERVER1"; export CFLAGS
buildserver -r BERKELEY-DB  $DVERBOSE_FLAG -t -o server1 \
  -s TestThread1:TestThread1 $UTILITY_FILES -f ../../src3/server.c
test "$?" -eq 0 || {
	echo "FAIL: buildserver failed."
	exit 1
}

msg "BUILDING SERVER #2"
CFLAGS="$COMPILE_FLAGS -DSERVER2"; export CFLAGS
buildserver -r BERKELEY-DB  $DVERBOSE_FLAG -t -o server2 \
   -s TestThread2:TestThread2 $UTILITY_FILES -f ../../src3/server.c
test "$?" -eq 0 || {
	echo "FAIL: buildserver failed."
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
if [ $1 == 1 ]; then 
        kill_thread=-k
fi
./client $kill_thread $DVERBOSE_FLAG 
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

# Killing a thread can result in expected database "run recovery" errors
if [ $1 == 1 ]; then 
        exit $exitval
fi

# Copy out any server errors.
test -s stderr && {
        echo "STDERR:"
	cat stderr
	echo "FAIL: stderr file not empty"
	exitval=1
}

exit $exitval
