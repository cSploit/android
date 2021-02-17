# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Rep095 script - internal initialization use of shared region memory.
#
# Repscript exists to process the remaining messages for
# an internal initialization.  The test is set up for the
# parent process to start an internal initialization and
# for this child process to complete it by processing the
# remaining messages in the message queue.  This script 
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

set usage "repscript masterdir clientdir rep_verbose verbose_type"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set clientdir [ lindex $argv 1 ]
set rep_verbose [ lindex $argv 2 ]
set verbose_type [ lindex $argv 3 ]
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

puts "Everyone open"

# Join the marker env/database and let parent know child is set up.
set markerenv [berkdb_env -home $testdir -txn]
error_check_good markerenv_open [is_valid_env $markerenv] TRUE
set marker [berkdb_open -unknown -env $markerenv -auto_commit marker.db]
error_check_good putchildsetup [$marker put CHILDSETUP 2] 0

# Give parent process time to start the internal init.
while { [llength [$marker get PARENTPARTINIT]] == 0 } {
	tclsleep 1
}

# Process the remaining messages to finish the internal init in this process.
process_msgs "{$masterenv 1} {$clientenv 2}"

puts "Processed messages"

# Close everything.
error_check_good marker_db_close [$marker close] 0
error_check_good market_env_close [$markerenv close] 0
error_check_good script_master_close [$masterenv close] 0
error_check_good script_client_close [$clientenv close] 0
puts "Repscript completed successfully"
