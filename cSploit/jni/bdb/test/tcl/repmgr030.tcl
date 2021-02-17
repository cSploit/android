# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr030
# TEST	repmgr multiple client-to-client peer test.
# TEST
# TEST	Start an appointed master, three clients and a view.  The third client
# TEST	configures the two other clients and view as peers and delays client
# TEST	sync.  Add some data and confirm that the third client uses first client
# TEST	as a peer.  Close the master so that the first client now becomes the
# TEST	the master.  Add some more data and confirm that the third client now
# TEST	uses the second client as a peer.  Close the current master so that the
# TEST	second client becomes master and the third client uses the view as a
# TEST	peer.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr030 { { niter 100 } { tnum "030" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr multiple c2c peer test."
	repmgr030_sub $method $niter $tnum $args
}

proc repmgr030_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 5

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
	set clientdir3 $testdir/CLIENTDIR3
	set viewdir $testdir/VIEWDIR

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3
	file mkdir $viewdir

	#
	# Use longer ack timeout, shorter rep_request and heartbeats to 
	# trigger rerequests to make sure enough clients are keeping up to
	# acknowledge removing a site.
	#
	set ack_timeout 3000000
	set req_min 4000
	set req_max 128000
	set hbsend 200000
	set hbmon 500000

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread -event"
	set masterenv [eval $ma_envcmd]
	$masterenv rep_request $req_min $req_max
	$masterenv repmgr -ack all -pri 100 \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -timeout [list ack $ack_timeout] \
	    -timeout [list heartbeat_send $hbsend] \
	    -timeout [list heartbeat_monitor $hbmon] -start master

	# Open three clients, setting first two as peers of the third and
	# configuring third for delayed sync.
	puts "\tRepmgr$tnum.b: Start three clients, third with three peers."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread -event"
	set clientenv [eval $cl_envcmd]
	$clientenv rep_request $req_min $req_max
	$clientenv repmgr -ack all -pri 80 \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 3]] \
	    -timeout [list ack $ack_timeout] \
	    -timeout [list heartbeat_send $hbsend] \
	    -timeout [list heartbeat_monitor $hbmon] -start client
	await_startup_done $clientenv

	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread -event"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 rep_request $req_min $req_max
	$clientenv2 repmgr -ack all -pri 60 \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 3]] \
	    -timeout [list ack $ack_timeout] \
	    -timeout [list heartbeat_send $hbsend] \
	    -timeout [list heartbeat_monitor $hbmon] -start client
	await_startup_done $clientenv2

	# Put view in middle of peer list to ensure it is only used when
	# there are no participants available.
	set cl3_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT3 -home $clientdir3 -txn -rep -thread -event"
	set clientenv3 [eval $cl3_envcmd]
	$clientenv3 rep_request $req_min $req_max
	$clientenv3 repmgr -ack all -pri 50 \
	    -local [list 127.0.0.1 [lindex $ports 3]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1] peer] \
	    -remote [list 127.0.0.1 [lindex $ports 4] peer] \
	    -remote [list 127.0.0.1 [lindex $ports 2] peer] \
	    -timeout [list ack $ack_timeout] \
	    -timeout [list heartbeat_send $hbsend] \
	    -timeout [list heartbeat_monitor $hbmon] -start client
	await_startup_done $clientenv3

	# Open a view.
	puts "\tRepmgr$tnum.c: Start a view."
	set viewcb ""
	set view_envcmd "berkdb_env_noerr -create $verbargs \
	    -rep_view \[list $viewcb \] -errpfx VIEW -home $viewdir -event \
	    -txn -rep -thread"
	set viewenv [eval $view_envcmd]
	$viewenv rep_request $req_min $req_max
	$viewenv repmgr -ack all -pri 80 \
	    -local [list 127.0.0.1 [lindex $ports 4]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -timeout [list ack $ack_timeout] \
	    -timeout [list heartbeat_send $hbsend] \
	    -timeout [list heartbeat_monitor $hbmon] -start client
	await_startup_done $viewenv

	# Internally, repmgr does the following to determine the peer
	# to use: it scans the internal list of remote sites, selecting
	# the first one that is marked as a peer and that is not the
	# current master.  If it can't find a participant but there is
	# a view, it selects the first view.

	puts "\tRepmgr$tnum.d: Configure third client for delayed sync."
	$clientenv3 rep_config {delayclient on}

	puts "\tRepmgr$tnum.e: Check third client used first client as peer."
	set creqs [stat_field $clientenv rep_stat "Client service requests"]
	set c2reqs [stat_field $clientenv2 rep_stat "Client service requests"]
	error_check_good got_client_reqs [expr {$creqs > 0}] 1
	error_check_good no_client2_reqs [expr {$c2reqs == 0}] 1

	puts "\tRepmgr$tnum.f: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	puts "\tRepmgr$tnum.g: Shut down master, first client takes over."
	error_check_good masterenv_close [$masterenv close] 0
	await_expected_master $clientenv

	puts "\tRepmgr$tnum.h: Run some more transactions at new master."
	eval rep_test $method $clientenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.i: Sync delayed third client."
	error_check_good rep_sync [$clientenv3 rep_sync] 0
	# Give sync requests a bit of time to show up in stats.
	tclsleep 1

	puts "\tRepmgr$tnum.j: Check third client used second client as peer."
	set c2reqs [stat_field $clientenv2 rep_stat "Client service requests"]
	error_check_good got_client2_reqs [expr {$c2reqs > 0}] 1

	puts "\tRepmgr$tnum.k: Remove master site."
	# Remove master so that the last remaining peer client can be elected
	# master with its lower priority.  This also reduces the size of
	# the replication group so that the remaining sites can generate
	# enough votes for a successful election.
	$clientenv repmgr -remove [list 127.0.0.1 [lindex $ports 0]]
	await_event $clientenv site_removed
	# Give site remove gmdb operation some time to propagate.
	tclsleep 2

	puts "\tRepmgr$tnum.j: Shut down client, second client takes over."
	error_check_good clientenv_close [$clientenv close] 0
	await_expected_master $clientenv2

	puts "\tRepmgr$tnum.l: Run more transactions at latest master."
	eval rep_test $method $clientenv2 NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.m: Sync delayed third client."
	error_check_good rep_sync [$clientenv3 rep_sync] 0

	puts "\tRepmgr$tnum.n: Check third client used view as peer."
	# Give sync requests a bit of time to show up in stats.
	tclsleep 1
	set vreqs2 [stat_field $viewenv rep_stat "Client service requests"]
	error_check_good got_view_reqs [expr {$vreqs2 > 0}] 1

	puts "\tRepmgr$tnum.o: Verify client database contents."
	rep_verify $clientdir2 $clientenv2 $clientdir3 $clientenv3 1 1 1
	rep_verify $clientdir2 $clientenv2 $viewdir $viewenv 1 1 1

	error_check_good view_close [$viewenv close] 0
	error_check_good client3_close [$clientenv3 close] 0
	error_check_good client2_close [$clientenv2 close] 0
}
