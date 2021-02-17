# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr011
# TEST	repmgr two site strict majority test.
# TEST
# TEST	Test each 2site_strict option's behavior for master loss and for
# TEST	client site removal.  With 2site_strict=on, make sure remaining
# TEST	site does not take over as master and that the client site can be
# TEST	removed and rejoin the group.  With 2site_strict=off, make sure
# TEST	remaining site does take over as master and make sure the deferred
# TEST	election logic prevents the rejoining site from immediately taking
# TEST	over as master before fully rejoining the repgroup. 
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr011 { { niter 100 } { tnum "011" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr two site strict majority test."
	repmgr011_sub $method $niter $tnum $args
}

proc repmgr011_sub { method niter tnum largs } {
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

	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $clientdir
	file mkdir $clientdir2
	puts "\tRepmgr$tnum.a: 2site_strict=on (default) test."
	puts "\tRepmgr$tnum.a1: Start first site as master."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.a2: Start second site as client."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv2

	#
	# Use of -ack all guarantees replication is complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.a3: Run first set of transactions at master."
	set start 0
	eval rep_test $method $clientenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.a4: Shut down master, wait, then verify no master."
	error_check_good client_close [$clientenv close] 0
	tclsleep 20
	error_check_bad c2_master [stat_field $clientenv2 rep_stat "Master"] 1

	puts "\tRepmgr$tnum.a5: Restart first site as master"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -start master
	await_expected_master $clientenv

	puts "\tRepmgr$tnum.a6: Run another set of transactions at master."
	eval rep_test $method $clientenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.a7: Shut down and remove client."
	error_check_good client_close [$clientenv2 close] 0
	$clientenv repmgr -remove  [list 127.0.0.1 [lindex $ports 1]]

	puts "\tRepmgr$tnum.a8: Run another set of transactions at master."
	eval rep_test $method $clientenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.a9: Restart removed client."
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start elect
	await_startup_done $clientenv2
	# Allow time for rejoin group membership database updates to complete.
	tclsleep 2

	puts "\tRepmgr$tnum.a10: Run another set of transactions at master."
	eval rep_test $method $clientenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.a11: Verify client database contents."
	rep_verify $clientdir $clientenv $clientdir2 $clientenv2 1 1 1

	puts "\tRepmgr$tnum.b: 2site_strict=off test."
	puts "\tRepmgr$tnum.b1: Turn off 2site_strict on both sites."
	$clientenv rep_config {mgr2sitestrict off}
	$clientenv2 rep_config {mgr2sitestrict off}

	puts "\tRepmgr$tnum.b2: Shut down master, second site takes over."
	error_check_good client_close [$clientenv close] 0
	await_expected_master $clientenv2

	puts "\tRepmgr$tnum.b3: Run a set of transactions at new master."
	eval rep_test $method $clientenv2 NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.b4: Restart first site as client."
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -start elect
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.b5: Run another set of transactions at master."
	eval rep_test $method $clientenv2 NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.b6: Shut down and remove client."
	error_check_good client_close [$clientenv close] 0
	$clientenv2 repmgr -remove  [list 127.0.0.1 [lindex $ports 0]]

	puts "\tRepmgr$tnum.b7: Run another set of transactions at master."
	eval rep_test $method $clientenv2 NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.b8: Restart removed client."
	set clientenv [eval $cl_envcmd]
	#
	# Test the deferred election when rejoining a 2SITE_STRICT=off
	# repgroup by asking for an election.
	#
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -start elect
	await_startup_done $clientenv
	error_check_good c2strict [$clientenv rep_config \
	    {mgr2sitestrict off}] 0
	#
	# Allow time for rejoin group membership database updates and
	# deferred election.
	#
	tclsleep 2

	puts "\tRepmgr$tnum.b9: Run another set of transactions at master."
	eval rep_test $method $clientenv2 NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.b10: Verify client database contents."
	rep_verify $clientdir2 $clientenv2 $clientdir $clientenv 1 1 1

	error_check_good client_close [$clientenv close] 0
	error_check_good client2_close [$clientenv2 close] 0
}
