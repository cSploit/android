#! /bin/sh
#
# Build the configuration file for single thread tests --
#	We do this work in the shell script because we have to fill in
#	lots of shell variables.
#
# Usage: ./tuxconfig.sh <IPCKEY>

if test $# -ne 1; then
    echo "Usage: ./tuxconfig.sh <IPCKEY>\n"
    exit 1
fi
	IPCKEY=$1
	MACHINE_NAME=`uname -n`
	cat > $RUN/config/ubb.cfg << END_OF_UBB_FILE
*RESOURCES
IPCKEY		$IPCKEY
DOMAINID	domain3
MASTER		cluster3
MAXACCESSERS	10
MAXSERVERS	5
MAXSERVICES	10
MODEL		SHM
LDBAL		N

*MACHINES
DEFAULT:
		APPDIR="$APPDIR"
		TUXCONFIG="$TUXCONFIG"
		TLOGDEVICE="$TLOGDEVICE"
		TUXDIR="$TUXDIR"
# Machine name is 30 characters max
"$MACHINE_NAME"	LMID=cluster3

*GROUPS
# Group name is 30 characters max
group_tm	LMID=cluster3 GRPNO=1 TMSNAME=DBRM TMSCOUNT=2 OPENINFO="BERKELEY-DB:$RUN/data"

*SERVERS
DEFAULT:
		CLOPT="-A"

# Server name is 78 characters max (same for any pathname)
server1		SRVGRP=group_tm SRVID=1 MAXGEN=3 RESTART=Y
server2		SRVGRP=group_tm SRVID=2 MAXGEN=3 RESTART=Y

*SERVICES
# Service name is 15 characters max
# server1
TestTxn1
# server2
TestTxn2
END_OF_UBB_FILE
	tmloadcf -y $RUN/config/ubb.cfg
