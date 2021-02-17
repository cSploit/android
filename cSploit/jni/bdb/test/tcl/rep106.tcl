# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep106
# TEST
# TEST	Replication and basic lease test with site shutdowns.
# TEST	Set leases on master and 3 clients, 2 electable and 1 zero-priority.
# TEST	Do a lease operation and process to all clients.
# TEST	Shutdown 1 electable and perform another update.  Leases should work.
# TEST	Shutdown 1 electable and perform another update.  Should fail.
#
proc rep106 { method { tnum "106" } args } {
	source ./include.tcl
	global repfiles_in_memory

	# Valid for all access methods.  Other lease tests limit the
	# test because there is nothing method-specific being tested.
	# Use all methods for this basic test.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 4]

	# Run the body of the test with and without recovery,
	# Skip recovery with in-memory logging - it doesn't make sense.
	#
	# Also skip the case where the master is in-memory and any
	# client is on-disk.  If the master is in-memory,
	# the wrong site gets elected because on-disk envs write a log 
	# record when they create the env and in-memory ones do not
	# and the test wants to control which env gets elected.
	#
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			set master_logs [lindex $l 0]
			if { $master_logs == "in-memory" } {
				set client_logs [lsearch -exact $l "on-disk"]
				if { $client_logs != -1 } {
					puts "Skipping for in-memory master\
					    and on-disk client."
					continue
				}
			}

			puts "Rep$tnum ($method $r):\
			    Replication and master leases with shutdown $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client 1 logs are [lindex $l 1]"
			puts "Rep$tnum: Client 2 logs are [lindex $l 2]"
			puts "Rep$tnum: Client 3 logs are [lindex $l 3]"
			rep106_sub $method $tnum $l $r $args
		}
	}
}

proc rep106_sub { method tnum logset recargs largs } {
	source ./include.tcl
	global rep_verbose
	global repfiles_in_memory
	global testdir
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory == 1 } {
		set repmemargs " -rep_inmem_files "
	}

	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]
	set c3_logtype [lindex $logset 3]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set c3_logargs [adjust_logargs $c3_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]
	set c3_txnargs [adjust_txnargs $c3_logtype]

	# Set leases for 4 sites, 3 second timeout, 0% clock skew
	set nsites 4
	set lease_to 3000000
	set lease_tosec [expr $lease_to / 1000000]
	set clock_fast 0
	set clock_slow 0
	set testfile test.db
	#
	# Since we have to use elections, the election code
	# assumes a 2-off site id scheme.
	# Open a master.
	repladd 2
	set err_cmd(0) "none"
	set crash(0) 0
	set pri(0) 100
	#
	# Note that using the default clock skew should be the same
	# as specifying "no skew" through the API.  We want to
	# test both API usages here.
	#
	set envcmd(0) "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir \
	    -rep_nsites $nsites $repmemargs \
	    -rep_lease \[list $lease_to\] -event \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set masterenv [eval $envcmd(0) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open three clients.
	repladd 3
	set err_cmd(1) "none"
	set crash(1) 0
	set pri(1) 10
	set envcmd(1) "berkdb_env -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT -home $clientdir -rep_nsites $nsites \
	    -rep_lease \[list $lease_to $clock_fast $clock_slow\] $repmemargs \
	    -event -rep_client -rep_transport \[list 3 replsend\]"
	set clientenv [eval $envcmd(1) $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	repladd 4
	set err_cmd(2) "none"
	set crash(2) 0
	set pri(2) 10
	set envcmd(2) "berkdb_env -create $c2_txnargs $c2_logargs \
	    $verbargs -errpfx CLIENT2 -home $clientdir2 -rep_nsites $nsites \
	    -rep_lease \[list $lease_to\] $repmemargs \
	    -event -rep_client -rep_transport \[list 4 replsend\]"
	set clientenv2 [eval $envcmd(2) $recargs]
	error_check_good client_env [is_valid_env $clientenv2] TRUE

	repladd 5
	set err_cmd(3) "none"
	set crash(3) 0
	set pri(3) 0
	set envcmd(3) "berkdb_env -create $c3_txnargs $c3_logargs \
	    $verbargs -errpfx CLIENT3 -home $clientdir3 -rep_nsites $nsites \
	    -rep_lease \[list $lease_to\] $repmemargs \
	    -event -rep_client -rep_transport \[list 5 replsend\]"
	set clientenv3 [eval $envcmd(3) $recargs]
	error_check_good client_env [is_valid_env $clientenv3] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist \
	    "{$masterenv 2} {$clientenv 3} {$clientenv2 4} {$clientenv3 5}"
	process_msgs $envlist

	#
	# Run election to get a master.  Leases prevent us from
	# simply assigning a master.
	#
	set msg "Rep$tnum.a"
	puts "\tRep$tnum.a: Run initial election."
	set nvotes $nsites
	set winner 0
	setpriority pri $nsites $winner
	# proc setpriority overwrites.  We really want pri(3) to be 0.
	set pri(3) 0
	set elector [berkdb random_int 0 3]
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
	    rep106script.tcl $testdir/rep106script.log \
		   $masterdir $testfile $method &]

	# Let child run, create database and put a txn into it.
	# Process messages while we wait for the child to complete
	# its txn so that the clients can grant leases.
	puts "\tRep$tnum.c: Wait for child to write txn."
	while { 1 } {
		if { [file exists $testdir/marker.db] == 0  } {
			tclsleep 1
		} else {
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
			# Child sends us the key it used as the data
			# of the CHILD1 key.
			#
			set key [lindex [lindex $kd 0] 1]
			break
		}
	}
	set masterdb [eval \
	    {berkdb_open_noerr -env $masterenv -rdonly $testfile}]
	error_check_good dbopen [is_valid_db $masterdb] TRUE

	process_msgs $envlist
	set omethod [convert_method $method]
	set clientdb3 [eval {berkdb_open_noerr \
	    -env $clientenv3 $omethod -rdonly $testfile}]
	error_check_good dbopen [is_valid_db $clientdb3] TRUE

	set uselease ""
	set ignorelease "-nolease"
	puts "\tRep$tnum.d.0: Read with leases."
	check_leaseget $masterdb $key $uselease 0
	check_leaseget $clientdb3 $key $uselease 0
	puts "\tRep$tnum.d.1: Read ignoring leases."
	check_leaseget $masterdb $key $ignorelease 0
	check_leaseget $clientdb3 $key $ignorelease 0

	# 
	# Shut down electable client now.  Signal child process
	# with PARENT1 to write another txn.  Make sure leases still work.
	#
	puts "\tRep$tnum.e: Close electable client."
	$clientenv close
	set envlist "{$masterenv 2} {$clientenv2 4} {$clientenv3 5}"

	error_check_good timestamp_done \
	    [$marker put PARENT1 [timestamp -r]] 0

	set kd [$marker get CHILD2]
	while { [llength $kd] == 0 } {
		process_msgs $envlist
		tclsleep 1
		set kd [$marker get CHILD2]
	}
	process_msgs $envlist
	#
	# Child sends us the key it used as the data
	# of the CHILD2 key.
	#
	set key [lindex [lindex $kd 0] 1]
	puts "\tRep$tnum.e.0: Read with leases."
	check_leaseget $masterdb $key $uselease 0
	check_leaseget $clientdb3 $key $uselease 0

	# 
	# Shut down 2nd electable client now.  Signal child process
	# with PARENT2 to write another perm.  Leases should fail.
	#
	puts "\tRep$tnum.e: Close 2nd electable client."
	$clientenv2 close
	set envlist "{$masterenv 2} {$clientenv3 5}"

	# Child has committed the txn and we have processed it.  Now
	# signal the child process to put a checkpoint (so that we
	# write a perm record, but not a txn_commit and panic).
	# That will invalidate leases.
	error_check_good timestamp_done \
	    [$marker put PARENT2 [timestamp -r]] 0

	set kd [$marker get CHILD3]
	while { [llength $kd] == 0 } {
		tclsleep 1
		set kd [$marker get CHILD3]
	}
	process_msgs $envlist

	puts "\tRep$tnum.f.0: Read using leases fails."
	check_leaseget $masterdb $key $uselease REP_LEASE_EXPIRED
	puts "\tRep$tnum.f.1: Read ignoring leases."
	check_leaseget $masterdb $key $ignorelease 0

	watch_procs $pid 5

	process_msgs $envlist
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3 0 1 0

	# Clean up.
	error_check_good marker_db_close [$marker close] 0
	error_check_good marker_env_close [$markerenv close] 0
	error_check_good masterdb_close [$masterdb close] 0
	error_check_good clientdb_close [$clientdb3 close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv3 close] 0

	replclose $testdir/MSGQUEUEDIR

	# Check log file for failures.
	set errstrings [eval findfail $testdir/rep106script.log]
	foreach str $errstrings {
		puts "FAIL: error message in rep106 log file: $str"
	}
}

