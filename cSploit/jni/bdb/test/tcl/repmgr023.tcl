# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	repmgr023
# TEST	Test of JOIN_FAILURE event for repmgr applications.
# TEST
# TEST	Run for btree only because access method shouldn't matter.

proc repmgr023 { { niter 50 } { tnum 023 } args } {

	source ./include.tcl
	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	# QNX does not support fork() in a multi-threaded environment.
	if { $is_qnx_test } {
		puts "Skipping repmgr$tnum on QNX."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): Test of JOIN_FAILURE event."
	repmgr023_sub $method $niter $tnum $args
}

proc repmgr023_sub { method niter tnum  largs } {
	global testdir
	global util_path
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir
	file mkdir [set dira $testdir/SITE_A]
	file mkdir [set dirb $testdir/SITE_B]
	file mkdir [set dirc $testdir/SITE_C]
	foreach { porta portb portc } [available_ports 3] {}

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	set cmda "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs -rep -thread \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_A \
	    -home $dira"
	set enva [eval $cmda]
	$enva repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $porta] -start master

	set cmdb "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs -rep -thread \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_B \
	    -home $dirb"
	set envb [eval $cmdb]
	$envb repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $portb] -start client \
	    -remote [list 127.0.0.1 $porta]
	puts "\tRepmgr$tnum.a: wait for client B to sync with master."
	await_startup_done $envb

	set cmdc "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs -rep -thread \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_C \
	    -home $dirc"
	set envc [eval $cmdc]
	$envc repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $portc] -start client \
	    -remote [list 127.0.0.1 $porta]
	puts "\tRepmgr$tnum.b: wait for client C to sync with master."
	await_startup_done $envc


	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$enva test force noarchive_timeout

	# Run rep_test in the master.
	puts "\tRepmgr$tnum.c: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $enva NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.d: Close client."
	set last_client_log [get_logfile $envc last]
	$envc close

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master.
		puts "\tRepmgr$tnum.e: Running rep_test in replicated env."
		eval rep_test $method $enva NULL $niter $start 0 0 $largs
		incr start $niter

		puts "\tRepmgr$tnum.f: Run db_archive on master."
		$enva log_flush
		$enva test force noarchive_timeout
		set res [eval exec $util_path/db_archive -d -h $dira]
		set first_master_log [get_logfile $enva first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}

	puts "\tRepmgr$tnum.g: Restart client."
	set envc [eval $cmdc -recover -event]
	$envc rep_config {autoinit off}
	$envc repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $portc] -start client \
	    -remote [list 127.0.0.1 $porta]

	# Since we've turned off auto-init, but are too far behind to sync, we
	# expect a join_failure event.
	# 
	await_condition {[expr [stat_field $envc rep_stat "Startup complete"] \
			      || [is_event_present $envc join_failure]]}

	error_check_good failed [is_event_present $envc join_failure] 1

	# Do a few more transactions at the master, and see that the client is
	# still OK (i.e., simply that it's still accessible, that we can read
	# data there), although of course it can't receive the new data.
	# 
	puts "\tRepmgr$tnum.h: Put more new transactions, which won't\
		get to client C"
	eval rep_test $method $enva NULL $niter $start 0 0 $largs
	incr start $niter

	# Env a (the master) will match env b, but env c will not match.
	rep_verify $dira $enva $dirb $envb 0 1 1
	rep_verify $dira $enva $dirc $envc 0 0 1

	if {$databases_in_memory} {
		set dbname { "" "test.db" }
	} else {
		set dbname  "test.db"
	}
	set dbp [eval berkdb open \
		     -env $envc [convert_method $method] $largs $dbname]
	set dbc [$dbp cursor]
	$dbc get -first
	$dbc get -next
	$dbc get -next
	set result [$dbc get -next]
	error_check_good got_data [llength $result] 1
	error_check_good got_data [llength [lindex $result 0]] 2
	$dbc close
	$dbp close

	# Shut down the master, so as to force client B to take over.  Since we
	# didn't do any log archiving at B, client C should now be able to sync
	# up again.
	# 
	puts "\tRepmgr$tnum.i: Shut down master, client C should sync up."
	$enva close
	await_startup_done $envc 40

	$envc close
	$envb close
	set test_be_quiet ""
}
