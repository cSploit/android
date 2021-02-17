# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr018
# TEST	Check repmgr stats.
# TEST
# TEST	Start an appointed master and one client. Shut down the client,
# TEST	run some transactions at the master and verify that there are
# TEST	acknowledgement failures and one dropped connection. Shut down
# TEST	and restart client again and verify that there are two dropped
# TEST	connections.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr018 { { niter 20 } { tnum "018" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): Test of repmgr stats."
	repmgr018_sub $method $niter $tnum $args
}

proc repmgr018_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 2

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs -errpfx MASTER \
	    -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	# Open a client
	puts "\tRepmgr$tnum.b: Start a client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.c: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	error_check_good perm_no_failed_stat \
	    [stat_field $masterenv repmgr_stat "Acknowledgement failures"] 0

	# Remember number of connections dropped here for later test.
	# Note that normal group membership initialization can result
	# in a dropped connection at this point.
	set drop1 [stat_field $masterenv repmgr_stat "Connections dropped"]
	$clientenv close

	# Just do a few transactions (i.e., 3 of them), because each one is
	# expected to time out, and if we did many the test would take a long
	# time (with no benefit).
	#
	puts "\tRepmgr$tnum.d: Run transactions with no client."
	eval rep_test $method $masterenv NULL 3 $niter $niter 0 $largs

	error_check_bad perm_failed_stat \
	    [stat_field $masterenv repmgr_stat "Acknowledgement failures"] 0

	# Wait up to 30 seconds when testing for dropped connections. This
	# corresponds to the master connection_retry timeout.
	set max_wait 30
	await_condition {[stat_field $masterenv repmgr_stat \
	    "Connections dropped"] > $drop1} $max_wait
	set drop2 [stat_field $masterenv repmgr_stat "Connections dropped"]

	# Bring the client back up, and down, a couple times, to test resetting
	# of stats.
	#
	puts "\tRepmgr$tnum.e: Shut down client (pause), check dropped connection."
	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv
	$clientenv close

	await_condition {[stat_field $masterenv repmgr_stat \
	    "Connections dropped"] > $drop2} $max_wait
	$masterenv repmgr_stat -clear

	puts "\tRepmgr$tnum.f: Shut down, pause, check dropped connection (reset)."
	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv
	$clientenv close

	await_condition {[stat_field $masterenv repmgr_stat \
	    "Connections dropped"] == 1} $max_wait

	error_check_good masterenv_close [$masterenv close] 0
}
