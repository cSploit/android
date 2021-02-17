# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Rep102 script - HANDLE_DEAD and exclusive databases
#
# Repscript exists to open/close the exclusive database
# while a master is using it exclusively.  This script 
# requires a one-master and one-client setup.
#
# Usage: repscript masterdir clientdir rep_verbose verbose_type
# masterdir: master env directory
# clientdir: client env directory
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript masterdir clientdir testfile rep_verbose verbose_type"

# Verify usage
if { $argc != 5 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set clientdir [ lindex $argv 1 ]
set testfile [ lindex $argv 2 ]
set rep_verbose [ lindex $argv 3 ]
set verbose_type [ lindex $argv 4 ]
set verbargs "" 
if { $rep_verbose == 1 } {
	set verbargs " -verbose {$verbose_type on} "
}

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# We need to set up our own machids.
# Add 1 for master env id, and 2 for the clientenv id.
#
repladd 1
repladd 2

# Join the master env.
set ma_cmd "berkdb_env_noerr -home $masterdir $verbargs \
	-txn -rep_master -rep_transport \[list 1 replsend\]"
set masterenv [eval $ma_cmd]
error_check_good script_menv_open [is_valid_env $masterenv] TRUE

puts "Master open"

# Join the client env.
set cl_cmd "berkdb_env_noerr -home $clientdir $verbargs \
	-txn -rep_client -rep_transport \[list 2 replsend\]"
set clientenv [eval $cl_cmd]
error_check_good script_cenv_open [is_valid_env $clientenv] TRUE

puts "Everyone open.  Now open $testfile"

# Now open the database and hold the open database handle.
set t [$clientenv txn -nowait]
set cdb [eval {berkdb_open} -env $clientenv -txn $t $testfile]
error_check_good cdb_open [is_valid_db $db] TRUE
$t commit

# Join the marker env/database and let parent know child is set up.
set markerenv [berkdb_env -home $testdir -txn]
error_check_good markerenv_open [is_valid_env $markerenv] TRUE
set marker [berkdb_open -unknown -env $markerenv -auto_commit marker.db]
error_check_good putchildsetup [$marker put CHILDSETUP 2] 0

# Give parent process time to start the internal init.
while { [llength [$marker get PARENTPARTINIT]] == 0 } {
	tclsleep 1
}

# Sleep one more second to give parent process time to hang processing
# the messages.
tclsleep 1

# Try to access handle.  Confirm it gets a dead handle.
puts "Client access gets DB_REP_HANDLE_DEAD."
set ret [catch {eval $cdb stat -faststat} res]
if { $ret == 0 } {
	$cdb close
	error "Looking for DB_REP_HANDLE_DEAD: got $res"
}
error_check_good cdb_stat $ret 1
error_check_good cdb_stat_err [is_substr $res "DB_REP_HANDLE_DEAD"] 1
error_check_good cdb_close [$cdb close] 0

# Close everything.
error_check_good marker_db_close [$marker close] 0
error_check_good market_env_close [$markerenv close] 0
error_check_good script_master_close [$masterenv close] 0
error_check_good script_client_close [$clientenv close] 0
puts "Repscript completed successfully"
