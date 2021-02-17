# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr025
# TEST	repmgr heartbeat rerequest test.
# TEST
# TEST	Start an appointed master site and one client.  Use a test hook
# TEST	to inhibit PAGE_REQ processing at the master (i.e., "lose" some
# TEST	messages). 
# TEST	Start a second client that gets stuck in internal init.  Wait
# TEST	long enough to rely on the heartbeat rerequest to request the
# TEST	missing pages, rescind the test hook and verify that all
# TEST	data appears on both clients.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr025 { { niter 100 } { tnum "025" } args } {

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

	puts "Repmgr$tnum ($method): repmgr heartbeat rerequest test."
	repmgr025_sub $method $niter $tnum $args
}

proc repmgr025_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global util_path
	global verbose_type
	set nsites 3

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]
	set omethod [convert_method $method]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# First just establish the group, because a new client can't join a
	# group while the master is in the middle of a txn.
	puts "\tRepmgr$tnum.a: Create a group of three."
	set common "berkdb_env_noerr -create $verbargs \
            -txn -rep -thread -recover -log_buffer $log_buf -log_max $log_max"
	set ma_envcmd "$common -errpfx MASTER -home $masterdir"
	set cl_envcmd "$common -errpfx CLIENT -home $clientdir"
	set cl2_envcmd "$common -errpfx CLIENT2 -home $clientdir2"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] -start client
	await_startup_done $clientenv
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] -start client
	await_startup_done $clientenv2
	$clientenv close
	$clientenv2 close
	$masterenv close

	# Open a master.
	puts "\tRepmgr$tnum.b: Start a master."
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -timeout {heartbeat_send 500000}
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	# Open first client
	puts "\tRepmgr$tnum.c: Start first client."
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -timeout {heartbeat_monitor 1100000}
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.d: Add some data to master and commit."
	# Add enough data to move into a new log file, so that we can force an
	# internal init when we restart client2 later.
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	set log_end [lindex [lsort $res] end]

	set dbname test.db
	set mdb [eval {berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv} $largs {$dbname}]
	set done false
	set start 0
	$masterenv test force noarchive_timeout
	while { !$done } {
		eval rep_test $method $masterenv $mdb $niter $start 0 0 $largs
		incr start $niter
		$masterenv log_archive -arch_remove
		set res [exec $util_path/db_archive -l -h $masterdir]
		if { [lsearch -exact $res $log_end] == -1 } {
			set done true
		}
	}

	puts "\tRepmgr$tnum.e: Inhibit PAGE_REQ processing at master."
	$masterenv test abort no_pages

	# Open second client.  The test hook will cause
	# this client to be stuck in internal init until the updates
	# are committed, so do not await_startup_done here.
	puts "\tRepmgr$tnum.f: Start second client."
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -timeout {heartbeat_monitor 1100000}
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -start client

	puts "\tRepmgr$tnum.g: Test for page requests from rerequest thread."
	# Wait 5 seconds (significantly longer than heartbeat send time) to
	# process all page requests resulting from master transactions.
	set max_wait 5
	tclsleep $max_wait
	set init_pagereq [stat_field $clientenv2 rep_stat "Pages requested"]
	# Any further page requests can only be from the heartbeat rerequest
	# because we processed all other lingering page requests above.
	await_condition {[stat_field $clientenv2 rep_stat \
	    "Pages requested"] > $init_pagereq} $max_wait

	puts "\tRepmgr$tnum.h: Rescind test hook, finish client startup."
	$masterenv test abort none
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.i: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	error_check_good mdb_close [$mdb close] 0
	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}
