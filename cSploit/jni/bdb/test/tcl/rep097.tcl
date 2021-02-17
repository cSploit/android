# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep097
# TEST
# TEST	Replication and lease data durability test.
# TEST	Set leases on master and 2 clients.
# TEST	Have the original master go down and a client take over.
# TEST	Have the old master rejoin as client, but go down again.
# TEST	The other two sites do one txn, while the original master's
# TEST	LSN extends beyond due to running recovery.
# TEST	Original Master rejoins while new master fails.  Make sure remaining
# TEST	original site is elected, with the smaller LSN, but with txn data.
#
proc rep097 { method { tnum "097" } args } {
	source ./include.tcl

	# Valid for all access methods.  Other lease tests limit the
	# test because there is nothing method-specific being tested.
	# Use all methods for this basic test.
	if { $checking_valid_methods } {
		return "btree"
	}

	# This test depends on recovery, so can not be run with
	# in-memory logging or with rep files in-memory.
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}
	global repfiles_in_memory
	if { $repfiles_in_memory } {
		puts "Rep$tnum: Skipping for in-memory replication files."
		return
	}

	set args [convert_args $method $args]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"

	foreach r $test_recopts {
		puts "Rep$tnum ($method $r): Replication\
		    and durability of leases $msg."
		rep097_sub $method $tnum $r $args
	}
}

proc rep097_sub { method tnum recargs largs } {
	source ./include.tcl
	global testdir
	global databases_in_memory
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set env0dir $testdir/ENV0
	set env1dir $testdir/ENV1
	set env2dir $testdir/ENV2

	file mkdir $env0dir
	file mkdir $env1dir
	file mkdir $env2dir

	# Set leases for 3 sites, 3 second timeout, 0% clock skew
	set nsites 3
	set lease_to 3000000
	set lease_tosec [expr $lease_to / 1000000]
	set clock_fast 0
	set clock_slow 0
	set testfile test.db
	#
	# Since we have to use elections, the election code
	# assumes a 2-off site id scheme.
	# Open the site that will become master, due to priority.
	repladd 2
	set err_cmd(0) "none"
	set crash(0) 0
	set pri(0) 100
	set envcmd(0) "berkdb_env -create -txn nosync \
	    $verbargs -errpfx ENV0 -home $env0dir \
	    -rep_nsites $nsites -rep_lease \[list $lease_to\] -event \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set masterenv [eval $envcmd(0) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients.
	repladd 3
	set err_cmd(1) "none"
	set crash(1) 0
	set pri(1) 70
	set envcmd(1) "berkdb_env -create -txn nosync \
	    $verbargs -errpfx ENV1 -home $env1dir -rep_nsites $nsites \
	    -rep_lease \[list $lease_to $clock_fast $clock_slow\] -event \
	    -rep_client -rep_transport \[list 3 replsend\]"
	set clientenv [eval $envcmd(1) $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	repladd 4
	set err_cmd(2) "none"
	set crash(2) 0
	set pri(2) 30
	set envcmd(2) "berkdb_env -create -txn nosync \
	    $verbargs -errpfx ENV2 -home $env2dir \
	    -rep_nsites $nsites -rep_lease \[list $lease_to\] -event \
	    -rep_client -rep_transport \[list 4 replsend\]"
	set clientenv2 [eval $envcmd(2) $recargs]
	error_check_good client_env [is_valid_env $clientenv2] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 2} {$clientenv 3} {$clientenv2 4}"
	process_msgs $envlist

	#
	# Run election to get a master.  Leases prevent us from
	# simply assigning a master.
	#
	set msg "Rep$tnum.a"
	puts "\tRep$tnum.a: Run initial election."
	set nvotes $nsites
	set winner 0
	set elector [berkdb random_int 0 2]
	#
	# Note we send in a 0 for nsites because we set nsites back
	# when we started running with leases.  Master leases require
	# that nsites be set before calling rep_start, and master leases
	# require that the nsites arg to rep_elect be 0.
	#
	run_election envlist err_cmd pri crash $qdir $msg \
	    $elector 0 $nvotes $nsites $winner 0 NULL

	puts "\tRep$tnum.b: Spawn a child tclsh to do txn work."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep097script.tcl $testdir/rep097script.log \
		   $env0dir $env1dir $testfile $method &]

	# Let child run, create database and put a txn into it.
	# Process messages while we wait for the child to complete
	# its txn so that the clients can grant leases.
	puts "\tRep$tnum.c: Wait for child to write txn."
	while { [file exists $testdir/marker.db] == 0  } {
		tclsleep 1
	}
	set markerenv [berkdb_env -home $testdir -txn]
	error_check_good markerenv_open \
	    [is_valid_env $markerenv] TRUE
	set marker [berkdb_open -unknown -env $markerenv \
	    -auto_commit marker.db]
	set kd [$marker get CHILD1]
	while { [llength $kd] == 0 } {
		process_msgs $envlist
		tclsleep 1
		set kd [$marker get CHILD1]
	}
	process_msgs $envlist

	#
	# Close the master env handle (simulate a crash).  Run an election
	# with the remaining 2 sites.  
	#
	set msg "Rep$tnum.d"
	puts "\tRep$tnum.d: Run election after master crash."
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]
	set nvotes [expr $nsites - 1]
	set winner 1
	set elector 1
	run_election envlist err_cmd pri crash $qdir $msg \
	    $elector 0 $nvotes $nsites $winner 0 NULL

	#
	# Let child process know that the new master is elected.
	#
	error_check_good timestamp_done \
	    [$marker put PARENT1 [timestamp -r]] 0

	#
	# Wait for child to open db and write txn on new master.  We still
	# have two sites so leases should be able to be granted successfully.
	#
	set kd [$marker get CHILD2]
	while { [llength $kd] == 0 } {
		process_msgs $envlist
		tclsleep 1
		set kd [$marker get CHILD2]
	}
	process_msgs $envlist

	#
	# Restart the original master env as a client.
	# Synchronize with the rest of the group.
	#
	puts "\tRep$tnum.e: Resync newly rejoined site."
	set envstart0 [eval $envcmd(0) -recover]
	error_check_good orig_env [is_valid_env $envstart0] TRUE
	lappend envlist "$envstart0 2"
	process_msgs $envlist

	#
	# Close env again after synchronizing.
	#
	puts "\tRep$tnum.f: Abandon new site."
	$envstart0 log_flush
	error_check_good masterenv_close [$envstart0 close] 0
	set envlist [lreplace $envlist end end]

	#
	# Tell child process to write a txn with just these two sites again.
	#
	error_check_good timestamp_done \
	    [$marker put PARENT2 [timestamp -r]] 0

	set kd [$marker get CHILD3]
	while { [llength $kd] == 0 } {
		process_msgs $envlist
		tclsleep 1
		set kd [$marker get CHILD3]
	}
	process_msgs $envlist
	#
	# Child sends us the key it used as the data
	# of the CHILD3 key.  This key should be durable.
	#
	set key [lindex [lindex $kd 0] 1]

	#
	# Close the new master and restart the original site again.
	# Run an election between the remaining two sites.  The
	# original site should be ahead in LSN but behind in txns
	# and should lose the election.
	#
	puts "\tRep$tnum.g: Abandon new master and restart old site again."
	set envlist [lreplace $envlist 0 0]
	error_check_good clientenv_close [$clientenv close] 0
	set envstart1 [eval $envcmd(0) -recover]
	error_check_good orig_env [is_valid_env $envstart1] TRUE
	lappend envlist "$envstart1 2"

	#
	# Make sure recovered env is ahead of client2.
	#
	set c2file [stat_field $clientenv2 log_stat "Current log file number"]
	set c2off [stat_field $clientenv2 log_stat "Current log file offset"]
	set e1file [stat_field $envstart1 log_stat "Current log file number"]
	set e1off [stat_field $envstart1 log_stat "Current log file offset"]

	if { $e1file == $c2file } {
		error_check_good offchk [expr $e1off > $c2off] 1
	} else {
		error_check_good filechk [expr $e1file > $c2file] 1
	}

	set msg "Rep$tnum.h"
	puts "\tRep$tnum.h: Run election."
	set nvotes [expr $nsites - 1]
	set winner 2
	set elector 0
	run_election envlist err_cmd pri crash $qdir $msg \
	    $elector 0 $nvotes $nsites $winner 0 NULL

	#
	# Tell child to exit.
	#
	error_check_good timestamp_done \
	    [$marker put PARENT3 [timestamp -r]] 0

	set newmaster $clientenv2
	set newstate [stat_field $newmaster rep_stat "Role"]
	error_check_good newm $newstate "master"

	set masterdb [eval \
	    {berkdb_open_noerr -env $newmaster -rdonly $testfile}]
	error_check_good dbopen [is_valid_db $masterdb] TRUE
	check_leaseget $masterdb $key "-nolease" 0

	watch_procs $pid 5

	# Clean up.
	error_check_good marker_db_close [$marker close] 0
	error_check_good marker_env_close [$markerenv close] 0
	error_check_good masterdb_close [$masterdb close] 0
	error_check_good masterenv_close [$envstart1 close] 0
	error_check_good clientenv_close [$clientenv2 close] 0

	replclose $testdir/MSGQUEUEDIR

	# Check log file for failures.
	set errstrings [eval findfail $testdir/rep097script.log]
	foreach str $errstrings {
		puts "FAIL: error message in rep097 log file: $str"
	}
}

