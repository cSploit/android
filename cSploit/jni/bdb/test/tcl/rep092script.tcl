# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# Rep092 script - multi-thread wake-ups in checking read-your-writes
# consistency.
#
# Usage: repscript clientdir token timeout rep_verbose verbose_type
# clientdir: client env directory
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript clientdir token timeout txn_flag rep_verbose verbose_type"

# Verify usage
if { $argc != 6 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set clientdir [ lindex $argv 0 ]
set token_chars [ lindex $argv 1 ]
set timeout [ lindex $argv 2 ]
set txn_flag [ lindex $argv 3 ]
set rep_verbose [ lindex $argv 4 ]
set verbose_type [ lindex $argv 5 ]
set verbargs "" 
if { $rep_verbose == 1 } {
	set verbargs " -verbose {$verbose_type on} "
}

# Join the client env.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
repladd 2
set cl_cmd "berkdb_env_noerr -home $clientdir $verbargs \
	-txn -rep_client -rep_transport \[list 2 replsend\]"
set clientenv [eval $cl_cmd]
error_check_good script_cenv_open [is_valid_env $clientenv] TRUE

set token [binary format H40 $token_chars]

set start [clock seconds]
if { $txn_flag } {
	set txn [$clientenv txn]
}
set count 0
while {[catch {$clientenv txn_applied -timeout $timeout $token} result]} {
	if {[is_substr $result DB_LOCK_DEADLOCK]} {
		incr count
		if { $txn_flag } {
			$txn abort
		}
		tclsleep 5
		if { $txn_flag } {
			set txn [$clientenv txn]
		}
	} else {
		error $result
	}
}
if { $txn_flag } {
	$txn commit
}
set duration [expr [clock seconds] - $start]
puts "RESULT: $result"
puts "DURATION: $duration"
puts "DEADLOCK_COUNT: $count"

$clientenv close
$queueenv close
