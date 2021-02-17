# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr012
# TEST	repmgr heartbeat test.
# TEST
# TEST	Start an appointed master and one client site. Set heartbeat
# TEST	send and monitor values and process some transactions. Stop
# TEST	sending heartbeats from master and verify that client sees
# TEST	a dropped connection.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr012 { { niter 100 } { tnum "012" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr heartbeat test."
	repmgr012_sub $method $niter $tnum $args
}

proc repmgr012_sub { method niter tnum largs } {
	source ./include.tcl
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
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	# Open a client.
	puts "\tRepmgr$tnum.b: Start a client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.c: Run first set of transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	puts "\tRepmgr$tnum.d: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	# Timeouts are in microseconds, heartbeat monitor should be
	# longer than heartbeat_send.
	# If we have a machine that's really slow, like our AIX hosts
	# and QNX hosts are, double the times. 
	puts "\tRepmgr$tnum.e: Set heartbeat timeouts."
	if { $is_aix_test || $is_qnx_test } {
		$masterenv repmgr -timeout {heartbeat_send 200000}
		$clientenv repmgr -timeout {heartbeat_monitor 360000}
	} else {
		$masterenv repmgr -timeout {heartbeat_send 100000}
		$clientenv repmgr -timeout {heartbeat_monitor 180000}
	}

	puts "\tRepmgr$tnum.f: Run second set of transactions at master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.g: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	# Make sure client reacts to the lost master connection by holding an
	# election.  To do so, first check initial value of stats, then make
	# sure it increases.
	# 
	set init_eh [stat_field $clientenv rep_stat "Elections held"]
	set init_cd [stat_field $clientenv repmgr_stat "Connections dropped"]

	# Make sure client notices the lack of heartbeat.  Since the client's
	# heartbeat monitoring granularity is < 1 second, if we wait up to 5
	# seconds that ought to give it plenty of time to notice and react.
	# 
	puts "\tRepmgr$tnum.h: Remove master heartbeat and wait."
	$masterenv repmgr -timeout {heartbeat_send 0}
	set max_wait 5
	await_condition {[stat_field $clientenv rep_stat \
	    "Elections held"] > $init_eh} $max_wait
	error_check_good conndrop [expr \
	    [stat_field $clientenv repmgr_stat "Connections dropped"] \
	    > $init_cd] 1

	error_check_good client_close [$clientenv close] 0
	error_check_good master_close [$masterenv close] 0
}
