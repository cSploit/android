# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Rep110 script - open an environment handle and then close it.
# This tests a codepath with NOWAIT set where the handle_cnt
# was getting messed up if we are in internal init and would
# need to wait.
#
# Usage: repscript clientdir verb
# clientdir: client env directory
# verb: verbose setting
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript clientdir verb"

# Verify usage
if { $argc != 2 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set clientdir [ lindex $argv 0 ]
set rep_verbose [ lindex $argv 1 ]

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]     
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# Join the client env.  We expect an error on the open,
# so that is the only thing we do.
repladd 1
repladd 2
set envid 2
if { $rep_verbose } {
	set cl2_cmd "berkdb_env_noerr -home $clientdir \
		-errfile /dev/stderr -errpfx CLIENT.child \
	 	-verbose {rep on} \
	 	-txn -rep_client -rep_transport \[list $envid replsend\]"
} else {
	set cl2_cmd "berkdb_env_noerr -home $clientdir \
		-errfile /dev/stderr -errpfx CLIENT.child \
		-txn -rep_client -rep_transport \[list $envid replsend\]"
}

#
# We expect a DB_REP_LOCKOUT error returned from this.
#
set stat [catch {eval $cl2_cmd} ret]
error_check_good stat $stat 1
error_check_good ret [is_substr $ret DB_REP_LOCKOUT] 1

tclsleep 1

return
