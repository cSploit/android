# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  repmgr029
# TEST  Test repmgr group membership: create, join, re-join and remove from
# TEST  repmgr group and observe changes in group membership database.
# TEST
proc repmgr029 { } {
	source ./include.tcl
	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	puts "Repmgr029: Repmgr Group Membership operations."
	z1
	z2
	z3
	z4
	z5
	z6
	z7
	z8
	z9
	z10
	z11
	z12
	z13
	z14
	z15
	z16
	z17
	z18
	z19
	z20
	z21
	z22
}

# See that a joining site that names a non-master as helper gets a
# "forward" response, and manages to then get to the true master.
#
# Note: there's a bit of a race here, depending on the condition of
# site B at the time C tries to join.  That should eventually be
# tightened up.
proc z3 {} {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {port0 port1 port2 port3 port4 port5} [available_ports 6] {}

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3
	set clientdir4 $testdir/CLIENTDIR4
	set clientdir5 $testdir/CLIENTDIR5

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3
	file mkdir $clientdir4
	file mkdir $clientdir5

	puts "\tRepmgr029.z3.a: Primordial creation, Start Master site 0"
	set env1 [berkdb env -create -errpfx MASTER -home $masterdir -txn \
	    -rep -thread -recover -verbose [list rep $rv]]
	$env1 repmgr -local [list 127.0.0.1 $port0] -start master
	error_check_good nsites_A [$env1 rep_get_nsites] 1

	puts "\tRepmgr029.z3.b: Simple join request, \
	    client 1 points directly at master"
	set env2 [berkdb env -create -errpfx CLIENT -home $clientdir -txn \
	    -rep -thread -recover -verbose [list rep $rv]]
	$env2 rep_config {mgr2sitestrict on}
	$env2 repmgr -local [list 127.0.0.1 $port1] \
	    -remote [list 127.0.0.1 $port0] -start client
	await_startup_done $env2
	error_check_good nsites_A2 [$env1 rep_get_nsites] 2
	error_check_good nsites_B2 [$env2 rep_get_nsites] 2

	puts "\tRepmgr029.z3.c: Join request forwarding, start client 2."
	set env3 [berkdb env -create -errpfx CLIENT2 -home $clientdir2 -txn \
	    -rep -thread -recover -verbose [list rep $rv]]
	$env3 rep_config {mgr2sitestrict on}
	$env3 repmgr -local [list 127.0.0.1 $port2] \
	    -remote [list 127.0.0.1 $port1]
	set done no
	while {!$done} {
		if {[catch {$env3 repmgr -start client} msg]} {
			puts $msg
			tclsleep 1
		} else {
			set done yes
		}
	}
	await_startup_done $env3
	error_check_good nsites_A3 [$env1 rep_get_nsites] 3
	error_check_good nsites_B3 [$env2 rep_get_nsites] 3
	error_check_good nsites_C3 [$env3 rep_get_nsites] 3

	puts "\tRepmgr029.z3.d: Master cannot be removed \
	    (by itself, or as requested from a client)"
	set ret [catch {$env1 repmgr -remove [list 127.0.0.1 $port0]} result]
	error_check_bad no_failure $ret 0
	error_check_match unavail $result "*DB_REP_UNAVAIL*"
	set ret [catch {$env2 repmgr -remove [list 127.0.0.1 $port0]} result]
	error_check_bad no_failure2 $ret 0
	error_check_match unavail2 $result "*DB_REP_UNAVAIL*"

	set db [berkdb open -env $env1 -thread __db.rep.system __db.membership]

	puts "\tRepmgr029.z3.e: Join request rejected for lack of acks"
	puts "\t\tRepmgr029.z3.e.1: Close client 1 and 2."
	error_check_good s_3_close [$env3 close] 0
	error_check_good s_2_close [$env2 close] 0

	puts "\t\tRepmgr029.z3.e.2: Start client 3."
	set env4 [berkdb env -create -errpfx CLIENT3 -home $clientdir3 \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	set ret [catch {$env4 repmgr -local [list 127.0.0.1 $port3] \
	    -remote [list 127.0.0.1 $port0] -start client} result]
	error_check_bad no_failure3 $ret 0
	error_check_match unavail3 $result "*DB_REP_UNAVAIL*"

	set prev_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.e.3: Check previous GMDB version $prev_vers"
	set SITE_ADDING 1
	set SITE_PRESENT 4
	error_check_good site_3_adding [repmgr029_gmdb_status $db 127.0.0.1 $port3] \
	    $SITE_ADDING

	puts "\t\tRepmgr029.z3.e.4: limbo resolution, restart client 1."
	set env2 [berkdb env -create -errpfx CLIENT -home $clientdir -txn \
	    -rep -thread -recover -verbose [list rep $rv] -event]
	# no helper should be needed this time.
	$env2 repmgr -local [list 127.0.0.1 $port1] -start client
	await_startup_done $env2 50

	puts "\t\tRepmgr029.z3.e.5: normal txn at master"
	set niter 1
	rep_test btree $env1 NULL $niter 0 0 0 
	set new_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.e.6: NEW GMDB version $new_vers"
	error_check_good version_incr $new_vers [expr $prev_vers + 1]
	error_check_good site_3_added [repmgr029_gmdb_status $db 127.0.0.1 $port3] \
	    $SITE_PRESENT

	puts "\t\tRepmgr029.z3.e.7: client 3 rejoins."
	$env4 repmgr -start client

	await_startup_done $env4 60

	# To verify that the GMDB has been updated on client side.
	puts "\t\tRepmgr029.z3.e.8: Verify the GMDB on the client 3."
	set db3 [berkdb open -env $env4 -thread __db.rep.system __db.membership]
	error_check_good vers [repmgr029_gmdb_version $db3] $new_vers
	$db3 close

	# This test case verify a scenario where (1) try another (different)
	# join request, still with insufficient acks, and see that it doesn't
	# load up another limbo; and then (2) with acks working,
	# a second request finishes off the first and then succeeds.
	# I guess we also need to try simply retrying the first addition.
	puts "\tRepmgr029.z3.f: Join request rejected for lack of acks"
	puts "\t\tRepmgr029.z3.f.1: Close client 1."
	error_check_good s_1_close [$env2 close] 0

	set prev_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.f.2: Check current GMDB version $prev_vers"

	puts "\t\tRepmgr029.z3.f.3: Start client 4."
	set env5 [berkdb env -create -errpfx CLIENT4 -home $clientdir4 \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	set ret [catch {$env5 repmgr -local [list 127.0.0.1 $port4] \
	    -remote [list 127.0.0.1 $port0] -start client} result]
	error_check_bad no_failure4 $ret 0
	error_check_match unavail4 $result "*DB_REP_UNAVAIL*"

	set prev_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.f.4: Check current GMDB version $prev_vers"
	error_check_good site_4_adding [repmgr029_gmdb_status $db 127.0.0.1 $port4] \
	    $SITE_ADDING

	puts "\t\tRepmgr029.z3.f.5: Start client 5."
	set env6 [berkdb env -create -errpfx CLIENT5 -home $clientdir5 \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	set ret [catch {$env6 repmgr -local [list 127.0.0.1 $port5] \
	    -remote [list 127.0.0.1 $port0] -start client} result]
	error_check_bad no_failure5 $ret 0
	error_check_match unavail5 $result "*DB_REP_UNAVAIL*"

	set prev_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.f.6: Check current GMDB version $prev_vers"
	# [M]: There is no gm status for client 5 so far. Let alone the "ADDING".
	#error_check_good site_5_adding [repmgr029_gmdb_status $db 127.0.0.1 $port5] \
	#    $SITE_ADDING

	puts "\t\tRepmgr029.z3.f.7: limbo resolution, restart client 1."
	set env2 [berkdb env -create -errpfx CLIENT -home $clientdir -txn \
	    -rep -thread -recover -verbose [list rep $rv] -event]
	# no helper should be needed this time.
	$env2 repmgr -local [list 127.0.0.1 $port1] -start client
	await_startup_done $env2 50
	puts "\t\tRepmgr029.z3.f.8: normal txn at master"
	set niter 1
	rep_test btree $env1 NULL $niter 0 0 0

	set new_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.f.9: NEW GMDB version $new_vers"
	error_check_good version_incr $new_vers [expr $prev_vers + 1]

	puts "\t\tRepmgr029.z3.f.10: client 5 rejoins."
	$env6 repmgr -start client
	await_startup_done $env6 60

	set new_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.f.11: NEW GMDB version $new_vers"
	# Check for client 5, which has gm status as "ADDED"
	error_check_good site_5_added [repmgr029_gmdb_status $db 127.0.0.1 $port5] \
	    $SITE_PRESENT
	#[M]: So far gm status for client 4 is "ADDED"
	error_check_good site_4_added [repmgr029_gmdb_status $db 127.0.0.1 $port4] \
	    $SITE_PRESENT
	#[M]: We'd like to check the gm status on the client 4 sides.
	#     No Way! as client 4 has not been start up and sync.
	# puts "\t\tRepmgr029.z3.e.8: Verify the GMDB on the client 4."
	# set db4 [berkdb open -env $env5 -thread __db.rep.system __db.membership]
	# error_check_good vers [repmgr029_gmdb_version $db4] $new_vers

	puts "\t\tRepmgr029.z3.f.12: client 4 rejoins."
	$env5 repmgr -start client
	await_startup_done $env5 100

	set new_vers [repmgr029_gmdb_version $db]
	puts "\t\tRepmgr029.z3.f.13: NEW GMDB version $new_vers"

	puts "\tRepmgr029.z3.h: Remove (downed) client 3, from master"
	$env1 repmgr -remove [list 127.0.0.1 $port3]
	error_check_good site_3_removed [repmgr029_gmdb_status $db 127.0.0.1 $port3] 0
	error_check_good db_close [$db close] 0
	error_check_good s_1_close [$env2 close] 0
	error_check_good s_3_close [$env4 close] 0
	error_check_good s_4_close [$env5 close] 0
	error_check_good s_5_close [$env6 close] 0
	error_check_good s_0_close [$env1 close] 0
	puts "\tRepmgr029.z3.i: End OF Repmgr029"
}

# Remove a live site from a group, and see that the site gets a
# LOCAL_SITE_REMOVED event, and the other sites get SITE_REMOVED.
# Test removing a view site and a participant site.
# 
proc z6 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC portV} [available_ports 4] {}
	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	set dirV $testdir/V
	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirV

	puts -nonewline "\tRepmgr029.z6.a: Build 4-site group with one view."
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envA repmgr -local [list 127.0.0.1 $portA creator] -start elect
	error_check_good nsites_a [$envA rep_get_nsites] 1
	puts -nonewline ".";	flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start elect
	await_startup_done $envB
	error_check_good nsites_b [$envB rep_get_nsites] 2
	puts -nonewline ".";	flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start elect
	await_startup_done $envC
	error_check_good nsites_c [$envC rep_get_nsites] 3
	puts -nonewline ".";	flush stdout

	set viewcb ""
	set envV [berkdb env -create -errpfx V -home $dirV -txn -rep -thread \
	    -rep_view $viewcb -verbose [list rep $rv] -event]
	$envV repmgr -local [list 127.0.0.1 $portV] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envV
	# View site does not increment nsites.
	error_check_good nsites_v [$envC rep_get_nsites] 3
	puts ".";	flush stdout

	set mgmdb [berkdb open \
	    -env $envA -thread __db.rep.system __db.membership]

	puts "\tRepmgr029.z6.b: Remove (live) view site V with request from B."
	repmgr029_remove_site_from_helper $envA $envB $envV $portV $mgmdb

	puts "\tRepmgr029.z6.c: Remove (live) site C with request from B."
	repmgr029_remove_site_from_helper $envA $envB $envC $portC $mgmdb

	error_check_good s_b_close [$envB close] 0
	$mgmdb close
	error_check_good s_a_close [$envA close] 0
}

# See that SITE_ADDED events are fired appropriately.
proc z8 { } {
	global rep_verbose
	global testdir
	
	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}
	
	env_cleanup $testdir
	foreach {portA portB portC} [available_ports 3] {}
	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	puts "\tRepmgr029.z8: Create primordial site"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envA repmgr -local [list 127.0.0.1 $portA creator] -start elect

	puts "\tRepmgr029.z8: Add client, check for event at master"
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start elect
	set ev [find_event [$envA event_info] site_added]
	error_check_good ev_a [llength $ev] 2
	set eid [lindex $ev 1]
	error_check_good ev_a_eid $eid [repmgr029_get_eid $envA $portB]
	await_startup_done $envB

	puts "\tRepmgr029.z8: Add another client, check for events at both other sites"
	$envA event_info -clear
	$envB event_info -clear
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start elect
	await_startup_done $envC

	set ev [find_event [$envA event_info] site_added]
	error_check_good ev_a2 [llength $ev] 2
	set eid [lindex $ev 1]
	error_check_good ev_a_eid2 $eid [repmgr029_get_eid $envA $portC]

	await_event $envB site_added
	set ev [find_event [$envB event_info] site_added]
	error_check_good ev_b [llength $ev] 2
	set eid [lindex $ev 1]
	error_check_good ev_b_eid $eid [repmgr029_get_eid $envB $portC]

	$envC close
	$envB close
	$envA close
}

# Remove a site, starting at the site to be removed.  See that we at least shut
# down threads (if not also fire event in this case).
# 
proc z7 { } {
	global rep_verbose
	global testdir
	
	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}
	
	env_cleanup $testdir
	foreach {portA portB portC} [available_ports 3] {}
	
	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	
	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	puts -nonewline "\tRepmgr029.z7: Set up a group of 3, A (master), B, C"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	puts -nonewline "." ; 	flush stdout
	
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts -nonewline "." ; 	flush stdout

	set eid_B_at_A [repmgr029_get_eid $envA $portB]
	
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	puts "."

	puts "\tRepmgr029.z7: Remove site B itself"
	$envB repmgr -remove  [list 127.0.0.1 $portB]
	await_event $envB local_site_removed

	set master_ev [find_event [$envA event_info] site_removed]
	error_check_good site_a_event_eid [lindex $master_ev 1] $eid_B_at_A
	error_check_good site_a_list [llength [repmgr029_get_eid $envA $portB]] 0

	$envB close
	$envC close
	$envA close
}

# See that a join request is rejected if insufficient acks.  (It should
# remain in the db as "adding" though, and apps should be able to query
# nsites to find out that it's been incremented.)
# 
proc z4 {} {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {port0 port1 port2} [available_ports 3] {}

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	puts -nonewline "\tRepmgr029.z4.a: Start the master."
	set env1 [berkdb_env -create -errpfx MASTER -home $masterdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env1 repmgr -local [list 127.0.0.1 $port0] -start master
	error_check_good nsites_1 [$env1 rep_get_nsites] 1
	puts ".";	flush stdout

	puts -nonewline "\tRepmgr029.z4.b: Start first client."
	set env2 [berkdb_env -create -errpfx CLIENT -home $clientdir -txn \
	    -rep -thread -recover -verbose [list rep $rv]]
	$env2 rep_config {mgr2sitestrict on}
	$env2 repmgr -local [list 127.0.0.1 $port1] \
	    -remote [list 127.0.0.1 $port0] -start client
	await_startup_done $env2
	error_check_good nsites_2 [$env2 rep_get_nsites] 2
	puts ".";	flush stdout

	puts "\tRepmgr029.z4.c: Close the first client."
	error_check_good s_2_close [$env2 close] 0

	puts "\tRepmgr029.z4.d: Start the second client."
	set env3 [berkdb_env -create -errpfx CLIENT2 -home $clientdir2 \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env3 rep_config {mgr2sitestrict on}
	set ret [catch {$env3 repmgr -local [list 127.0.0.1 $port2] \
	    -remote [list 127.0.0.1 $port0] -start client} result]
	error_check_bad no_failure $ret 0
	error_check_match unavail $result "*DB_REP_UNAVAIL*"
	puts "\tRepmgr029.z4.e: The second join failed as expected, \
	    since the first client is down"

	puts -nonewline "\tRepmgr029.z4.f: restart the first client."
	set env2 [berkdb_env -errpfx CLIENT -home $clientdir -txn -rep \
	    -thread -recover -create -verbose [list rep $rv]]
	$env2 rep_config {mgr2sitestrict on}
	$env2 repmgr -local [list 127.0.0.1 $port1] -start client
	await_startup_done $env2
	puts ".";	flush stdout

	puts "\tRepmgr029.z4.g: try to join the second client again"
	if {[catch {$env3 repmgr -start client} result] && \
	    [string match "*REP_UNAVAIL*" $result]} {
		puts "\tRepmgr029.z4.h: pause and try again"
		tclsleep 3
		$env3 repmgr -start client
	}
	await_startup_done $env3 100
	error_check_good nsites_3 [$env3 rep_get_nsites] 3

	error_check_good s_3_close [$env3 close] 0
	error_check_good s_2_close [$env2 close] 0
	error_check_good s_1_close [$env1 close] 0
}

# Cold-boot an established group, without specifying any remote sites, and see
# that they can elect a master (demonstrating that they have recorded each
# others' addresses).
# 
proc z5 {} {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {port0 port1 port2} [available_ports 3] {}

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	puts -nonewline "\tRepmgr029.z5.a: Set up a group of 3, one master and two clients."
	set env1 [berkdb env -create -errpfx MASTER -home $masterdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env1 repmgr -local [list 127.0.0.1 $port0] -start master
	error_check_good nsites_1 [$env1 rep_get_nsites] 1
	puts -nonewline "." ; 	flush stdout

	set env2 [berkdb env -create -errpfx CLIENT -home $clientdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env2 repmgr -local [list 127.0.0.1 $port1] \
	    -remote [list 127.0.0.1 $port0] -start client
	await_startup_done $env2
	error_check_good nsites_2 [$env2 rep_get_nsites] 2
	puts -nonewline "." ; 	flush stdout

	set env3 [berkdb env -create -errpfx CLIENT2 -home $clientdir2 \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env3 repmgr -local [list 127.0.0.1 $port2] \
	    -remote [list 127.0.0.1 $port0] -start client
	await_startup_done $env3
	error_check_good nsites_3 [$env1 rep_get_nsites] 3
	puts "." ; 	flush stdout

	puts "\tRepmgr029.z5: Shut down all sites and then restart with election"
	error_check_good s_2_close [$env2 close] 0
	error_check_good s_3_close [$env3 close] 0
	error_check_good s_1_close [$env1 close] 0

	set env1 [berkdb env -create -errpfx A -home $masterdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env1 repmgr -local [list 127.0.0.1 $port0] -start elect -pri 100
	set env2 [berkdb env -create -errpfx B -home $clientdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env2 repmgr -local [list 127.0.0.1 $port1] -start elect -pri 200
	set env3 [berkdb env -create -errpfx C -home $clientdir2 \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env3 repmgr -local [list 127.0.0.1 $port2] -start elect -pri 140

	puts "\tRepmgr029.z5: Wait for election to choose a new master"
	await_condition {[repmgr029_known_master $env1 $env2 $env3]}
	error_check_good nsites_1 [$env1 rep_get_nsites] 3
	error_check_good nsites_2 [$env2 rep_get_nsites] 3
	error_check_good nsites_3 [$env3 rep_get_nsites] 3

	error_check_good s_3_close [$env3 close] 0
	error_check_good s_1_close [$env1 close] 0
	error_check_good s_2_close [$env2 close] 0
}

# Remove a site while it is disconnected, and see if it can get an event when it
# tries to reconnect.  (2nd try)
proc z2 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC portD portE} [available_ports 5] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	set dirD $testdir/D
	set dirE $testdir/E

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirD
	file mkdir $dirE

	puts -nonewline "\tRepmgr029.z2.a: Set up a group of 5: A, B, C, D, E"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	error_check_good nsites_a [$envA rep_get_nsites] 1
	puts -nonewline ".";	flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	error_check_good nsites_b [$envB rep_get_nsites] 2
	puts -nonewline ".";	flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	error_check_good nsites_c [$envC rep_get_nsites] 3
	puts -nonewline "." ;	flush stdout

	# It is ideal to increase the await time when the group size is large.
	set envD [berkdb env -create -errpfx D -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envD repmgr -local [list 127.0.0.1 $portD] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envD 100
	error_check_good nsites_d [$envD rep_get_nsites] 4
	puts -nonewline "." ; 	flush stdout

	set envE [berkdb env -create -errpfx E -home $dirE -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envE repmgr -local [list 127.0.0.1 $portE] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envE 200
	error_check_good nsites_e [$envE rep_get_nsites] 5
	puts "." ; 	flush stdout

	puts "\tRepmgr029.z2.b: shut down sites D and E"
	error_check_good s_d_close [$envD close] 0
	error_check_good s_e_close [$envE close] 0

	puts "\tRepmgr029.z2.c: remove site D from the group"
	$envA repmgr -remove  [list 127.0.0.1 $portD]
	error_check_good rm_at_a \
	    [string length [repmgr029_site_list_status $envA $portD]] 0

	puts "\tRepmgr029.z2.d: shut down all remaining sites"
	error_check_good s_b_close [$envB close] 0
	error_check_good s_c_close [$envC close] 0
	error_check_good s_a_close [$envA close] 0

	puts -nonewline "\tRepmgr029.z2.e: start up just D and E \
	    (neither of which know that D has been removed)"
	set envD [berkdb env -create -errpfx D -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envD repmgr -local [list 127.0.0.1 $portD] -start elect\
	    -timeout {connection_retry 2000000}
	# Should comments out the await here, otherwise, envD cannot join
	#await_startup_done $envD 
	puts -nonewline ".";	flush stdout

	set envE [berkdb env -create -errpfx E -home $dirE -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envE repmgr -local [list 127.0.0.1 $portE] -start elect
	await_condition {[expr [stat_field $envD \
	    rep_stat "Messages processed"] > 0]}
	puts ".";	flush stdout

	puts -nonewline "\tRepmgr029.z2.f: Start sites A, B, and C"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start elect -pri 200
	puts -nonewline "." ; 	flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] -start elect -pri 150
	puts -nonewline "." ; 	flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] -start elect -pri 100
	await_startup_done $envC 1000
	puts "." ; 	flush stdout

	puts "\tRepmgr029.z2.g: wait for site D to notice that it has been removed"
	await_event $envD local_site_removed

	# Yikes!  This is not going to work!  Site D will be rejected before it
	# gets a chance to have its database updated!  :-(
	# [M] NOW: A is the master, B, C, E get sync with A, but D does not.

	error_check_good s_d_close [$envD close] 0
	error_check_good s_e_close [$envE close] 0
	error_check_good s_c_close [$envC close] 0
	error_check_good s_b_close [$envB close] 0
	error_check_good s_a_close [$envA close] 0
}

# Remove a site while it is down.  When it starts up again, it should rejoin.
proc z1 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC} [available_ports 3] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	puts -nonewline "\tRepmgr029.z1.a: Set up a group of 3: A, B, C"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	error_check_good nsitesA [$envA rep_get_nsites] 1
	puts -nonewline ".";	flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	error_check_good nsitesB [$envB rep_get_nsites] 2
	puts -nonewline ".";	flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	error_check_good nsitesC [$envC rep_get_nsites] 3
	puts ".";	flush stdout

	puts "\tRepmgr029.z1.b: Shut down site B, and remove it from the group."
	error_check_good s_b_close [$envB close] 0

	$envA repmgr -remove  [list 127.0.0.1 $portB]
	error_check_good rm_at_a \
	    [string length [repmgr029_site_list_status $envA $portB]] 0

	puts "\tRepmgr029.z1.c: restart B"
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -timeout {connection_retry 2000000} -start client
	await_startup_done $envB

	# make sure we haven't fired a LOCAL_SITE_REMOVED event to B
	set ev [find_event [$envB event_info] local_site_removed]
	error_check_good site_b_not_removed [string length $ev] 0

	# Now try it again, only this time the auto-rejoin fails due to lack of
	# acks, so B should shut down and fire LOCAL_SITE_REMOVED event.  TODO:
	# should we have some sort of stat query so that the application can
	# tell whether threads are running?  Or is that just redundant with the
	# event?
	puts "\tRepmgr029.z1.d: shut down and remove site B again"
	error_check_good s_b_close [$envB close] 0
	$envA repmgr -remove [list 127.0.0.1 $portB]

	puts "\tRepmgr029.z1.e: shut down site C, and then restart B"
	error_check_good s_c_close [$envC close] 0
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -timeout {connection_retry 2000000} -start client
	await_event $envB local_site_removed

	error_check_good s_b_close [$envB close] 0
	error_check_good s_a_close [$envA close] 0
}

# Test "sharing", by constructing a situation where a site that's been down for
# a while has an obsolete, too-high notion of nsites.  On a cold boot, if that
# site is needed, it would spoil the election by requiring too many votes,
# unless it gets a hint from other sites.
#
# Create a group of 6 sites, A, B, C, D, E, F.  Make sure F knows nsites is 6;
# then shut it down.  Remove E; now nsites is 5 (A, B, C, D, f).  Then remove D;
# nsites is 4 (A, B, C, f).  Now shut down everyone, and then reboot only A, B,
# and F (leave C down).  Try to elect a master.
# 
proc z9 { } {
	global rep_verbose
	global testdir

	set rv [ expr $rep_verbose ? on : off ]

	env_cleanup $testdir
	foreach {portA portB portC portD portE portF} [available_ports 6] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	set dirD $testdir/D
	set dirE $testdir/E
	set dirF $testdir/F

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirD
	file mkdir $dirE
	file mkdir $dirF
    
	puts -nonewline "\tRepmgr029.z9: Set up a group of 6"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	puts -nonewline "." ; 	flush stdout
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts -nonewline "." ; 	flush stdout
	set envC [berkdb env -create -errpfx B -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	puts -nonewline "." ; 	flush stdout
	set envD [berkdb env -create -errpfx B -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envD repmgr -local [list 127.0.0.1 $portD] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envD 30
	puts -nonewline "." ; 	flush stdout
	set envE [berkdb env -create -errpfx B -home $dirE -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envE repmgr -local [list 127.0.0.1 $portE] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envE 30
	puts -nonewline "." ; 	flush stdout
	set envF [berkdb env -create -errpfx B -home $dirF -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envF repmgr -local [list 127.0.0.1 $portF] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envF 40
	puts "."

	puts "\tRepmgr029.z9: Shut down site F"
	$envF close

	puts "\tRepmgr029.z9: Remove site E"
	$envE close
	$envA repmgr -remove [list 127.0.0.1 $portE]

	puts "\tRepmgr029.z9: Remove site D"
	$envD close
	$envA repmgr -remove [list 127.0.0.1 $portD]

	puts "\tRepmgr029.z9: Shut down site C"
	$envC close

	puts "\tRepmgr029.z9: Bounce the master"
	$envA close
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start elect

	# We now have a group of 4, with only A and B running.  That's not
	# enough to elect a master.

	puts "\tRepmgr029.z9: Restart site F"
	set envF [berkdb env -create -errpfx F -home $dirF -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envF repmgr -local [list 127.0.0.1 $portF] -start elect

	# There are now 3 sites running, in a 4-site group.  That should be
	# enough to elect a master, if site F can be advised of the fact that
	# the group size has been reduced.

	# Wait for an election to complete.
	await_condition {[repmgr029_known_master $envA $envF $envB]} 30

	$envA close
	$envB close
	$envF close
}

# See that a membership list gets restored after an interrupted internal init
# and check that we get the expected error if a user defines the local site
# inconsistently with the internal init restored list.
proc z10 { } {
	global rep_verbose
	global testdir
	global tclsh_path
	global test_path

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	# Define an extra port for error case.
	foreach {portA portB portC portD portE} [available_ports 5] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	set dirD $testdir/D

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirD

	set pagesize 4096
	set log_max [expr $pagesize * 8]

	puts "\tRepmgr029.z10: Set up a group of 4, A (master), B, C, D"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC

	set envD [berkdb env -create -errpfx D -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envD repmgr -local [list 127.0.0.1 $portD] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envD

	puts "\tRepmgr029.z10: Shut down C and D and generate\
	    enough churn to force internal init on both sites"
	set log_endC [get_logfile $envC last]
	$envC close
	set log_endD [get_logfile $envD last]
	$envD close

	set max_log_end $log_endC
	if { $log_endD > $log_endC } {
		set max_log_end log_endD
	}
	set niter 50
	while { [get_logfile $envA first] <= $max_log_end } {
		$envA test force noarchive_timeout
		rep_test btree $envA NULL $niter 0 0 0 -pagesize $pagesize
		$envA log_flush
		$envA log_archive -arch_remove
	}

	# Use separate process so that it works even on Windows.
	# Inhibit master from sending any PAGE messages.
	puts "\tRepmgr029.z10: Restart site C in a separate process"
	$envA test abort no_pages
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    repmgr029script.tcl $testdir/repmgr029scriptC.log \
	    $dirC $portC $rv "C" &]
	watch_procs $pid 5

	puts "\tRepmgr029.z10: Restart site D in a separate process"
	set pid2 [exec $tclsh_path $test_path/wrap.tcl \
	    repmgr029script.tcl $testdir/repmgr029scriptD.log \
	    $dirD $portD $rv "D" &]
	watch_procs $pid2 5

	puts "\tRepmgr029.z10: Shut down the rest of the group"
	$envB close
	$envA close

	puts "\tRepmgr029.z10: Restart site C alone"
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] -start elect

	puts "\tRepmgr029.z10: Check list of known sites on C: A, B, D"
	set l [$envC repmgr_site_list]
	foreach p [list $portA $portB $portD] {
		set sought [list 127.0.0.1 $p]
		error_check_good port$p \
		    [expr [lsearch -glob $l [concat * $sought *]] >= 0] 1
	}
	$envC close

	puts "\tRepmgr029.z10: Restart site D with a different local site port"
	# Do not use errpfx, which hides internal error messages.
	set envD [berkdb env -create -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	error_check_bad diff_local [catch \
	    {$envD repmgr -local [list 127.0.0.1 $portE] -start elect} msg] 0
	puts "\tRepmgr029.z10: Check for inconsistent local site error on D"
	error_check_good errchk [is_substr $msg \
	    "Current local site conflicts with earlier definition"] 1
	$envD close
}

# See that a client notices a membership change that happens while it is
# disconnected (via the internal init completion trigger).
proc z11 { } {
	global rep_verbose
	global testdir
	global tclsh_path
	
	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}
	
	env_cleanup $testdir
	foreach {portA portB portC portD} [available_ports 4] {}
	
	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	set dirD $testdir/D
	
	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirD

	set pagesize 4096
	set log_max [expr $pagesize * 8]

	puts -nonewline "\tRepmgr029.z11: Set up a group initially of size 3"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	puts -nonewline "." ; 	flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts -nonewline "." ; 	flush stdout
	
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	puts "."

	puts "\tRepmgr029.z11: Shut down C"
	$envC close

	puts "\tRepmgr029.z11: Join new site D"
	set envD [berkdb env -create -errpfx D -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv] -log_max $log_max]
	$envD repmgr -local [list 127.0.0.1 $portD] \
	    -remote [list 127.0.0.1 $portA] -start client

	puts "\tRepmgr029.z11: Generate enough churn to force internal init at C later"
	set tail [get_logfile $envA last]
	set niter 50
	while { [get_logfile $envA first] <= $tail } {
		$envA test force noarchive_timeout
		rep_test btree $envA NULL $niter 0 0 0 -pagesize $pagesize
		$envA log_flush
		$envA log_archive -arch_remove
	}

	puts "\tRepmgr029.z11: Restart site C"
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] -start elect
	await_startup_done $envC

	puts "\tRepmgr029.z11: Check list of known sites"
	set l [$envC repmgr_site_list]
	foreach p [list $portA $portB $portD] {
		set sought [list 127.0.0.1 $p]
		error_check_good port$p \
		    [expr [lsearch -glob $l [concat * $sought *]] >= 0] 1
	}
	$envC close
	$envD close
	$envB close
	$envA close
}

# Exercise the new connection-related event types.
proc z12 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC} [available_ports 3] {}

	set dirA $testdir/A
	set dirB $testdir/B

	file mkdir $dirA
	file mkdir $dirB

	puts "\tRepmgr029.z12: Start primordial master site A"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master \
	    -timeout {connection_retry 2000000}

	puts "\tRepmgr029.z12: Add new client site B"
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -remote [list 127.0.0.1 $portA] \
	    -local [list 127.0.0.1 $portB] -start client
	await_startup_done $envB

	puts "\tRepmgr029.z12: Check connection events"
	set ev [find_event [$envA event_info] connection_established]
	error_check_good ev_len [llength $ev] 2
	error_check_good ev_eid [lindex $ev 1] [repmgr029_get_eid $envA $portB]
	set ev [find_event [$envB event_info] connection_established]
	error_check_good ev_len2 [llength $ev] 2
	error_check_good ev_eid2 [lindex $ev 1] [repmgr029_get_eid $envB $portA]

	puts "\tRepmgr029.z12: Shut down site B, observe event at site A"
	$envB close
	set ev [await_event $envA connection_broken]
	error_check_good ev_len3 [llength $ev] 2
	set evinfo [lindex $ev 1]
	error_check_good ev_len3b [llength $evinfo] 2
	foreach {eid err} $evinfo {}
	error_check_good ev_eid3 $eid [repmgr029_get_eid $envA $portB]
	puts "\t\tRepmgr029.z12: (connection_broken error code is $err)"

	set ev [await_event $envA connection_retry_failed]
	error_check_good ev_len3 [llength $ev] 2
	set evinfo [lindex $ev 1]
	error_check_good ev_len3c [llength $evinfo] 2
	foreach {eid err} $evinfo {}
	error_check_good ev_eid3 $eid [repmgr029_get_eid $envA $portB]
	puts "\t\tRepmgr029.z12: (retry_failed error code is $err)"

	puts "\tRepmgr029.z12: Shut down site A, then restart B"
	$envA close
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] -start elect \
	    -timeout {connection_retry 2000000}
	# new event instances should continue to be fired indefinitely.  For
	# now, consider '3' to be close enough to infinity.
	for { set i 1 } { $i <= 3 } { incr i } {
		puts "\tRepmgr029.z12: Observe event ($i)"
		set ev [await_event $envB connection_retry_failed]
		error_check_good ev_eid4 [lindex $ev 1 0] [repmgr029_get_eid $envB $portA]
		error_check_good never_estd \
		    [string length [find_event [$envB event_info] \
		    connection_established]] 0
		# According to our definition of "connection broken" you can't
		# "break" what you never had.
		error_check_good never_broken \
		    [string length [find_event [$envB event_info] \
		    connection_broken]] 0
		$envB event_info -clear
	}
	$envB close
}

# Make sure applications aren't bothered by perm failed events from failed GMDB
# operations.
proc z13 { } {
	global rep_verbose
	global testdir
	
	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}
	
	env_cleanup $testdir
	foreach {portA portB portC} [available_ports 3] {}
	
	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	
	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	puts -nonewline "\tRepmgr029.z13: Create first 2 sites"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
		      -recover -verbose [list rep $rv] -event]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	puts -nonewline "." ; 	flush stdout
	
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
		      -recover -verbose [list rep $rv]]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts "."

	puts "\tRepmgr029.z13: Shut down site B, try to add third site, site C"
	$envB close

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	set ret [catch {$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client} result]
	error_check_bad no_failure $ret 0
	error_check_match unavail $result "*DB_REP_UNAVAIL*"

	puts "\tRepmgr029.z13: Make sure site A application didn't see a perm failure"
	error_check_good no_failure \
	    [string length [find_event [$envA event_info] perm_failed]] 0

	$envC close
	$envA close
}

# Make sure we can add/remove sites even when ALL policy is in effect.
proc z14 { } {
	global rep_verbose
	global testdir
	
	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}
	
	foreach {portA portB portC} [available_ports 3] {}
	
	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	
	foreach policy {all allpeers} {
		env_cleanup $testdir
		file mkdir $dirA
		file mkdir $dirB
		file mkdir $dirC

		puts "\tRepmgr029.z14: Using \"$policy\" ack policy"
		puts "\tRepmgr029.z14: Create first site A"
		set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
		    -recover -verbose [list rep $rv] -event]
		$envA repmgr -local [list 127.0.0.1 $portA] -start master -ack $policy

		puts "\tRepmgr029.z14: Add 2nd site, B"
		set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
		    -recover -verbose [list rep $rv]]
		$envB repmgr -local [list 127.0.0.1 $portB] \
		    -remote [list 127.0.0.1 $portA] -start client -ack $policy
		await_startup_done $envB

		puts "\tRepmgr029.z14: Add 3rd site, C"
		set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
		    -recover -verbose [list rep $rv]]
		$envC repmgr -local [list 127.0.0.1 $portC] \
		    -remote [list 127.0.0.1 $portA] -start client -ack $policy
		await_startup_done $envC

		puts "\tRepmgr029.z14: Remove site B"
		$envC repmgr -remove [list 127.0.0.1 $portB]
		error_check_good removed \
		    [string length [repmgr029_site_list_status $envA $portB]] 0

		$envB close
		$envC close
		$envA close
	}
}

# Rescind a pending (previously incomplete) change, and check effect on nsites.
proc z15 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC portD portE} [available_ports 5] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	set dirD $testdir/D
	set dirE $testdir/E

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirD
	file mkdir $dirE

	puts -nonewline "\tRepmgr029.z15: Create initial group of 4 sites"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	puts -nonewline "." ; flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts -nonewline "." ; flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	puts -nonewline "." ; flush stdout

	set envD [berkdb env -create -errpfx D -home $dirD -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envD repmgr -local [list 127.0.0.1 $portD] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envD
	puts "."

	puts "\tRepmgr029.z15: Shut down C and D, and try to add E"
	$envC close
	$envD close
	set envE [berkdb env -create -errpfx E -home $dirE -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	set ret [catch {$envE repmgr -local [list 127.0.0.1 $portE] \
	    -remote [list 127.0.0.1 $portA] -start client} result]
	error_check_bad no_failure $ret 0
	error_check_match unavail $result "*DB_REP_UNAVAIL*"
	error_check_good nsites [$envA rep_get_nsites] 5
	await_condition {[expr [$envB rep_get_nsites] == 5]}

	puts "\tRepmgr029.z15: Rescind the addition of site E, by removing it"
	$envA repmgr -remove [list 127.0.0.1 $portE]
	error_check_good nsites2 [$envA rep_get_nsites] 4
	await_condition {[expr [$envB rep_get_nsites] == 4]}

	puts -nonewline "\tRepmgr029.z15: Restart sites C and D"
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] -start client
	await_startup_done $envC
	puts -nonewline "." ; flush stdout

	set envD [berkdb env -create -errpfx D -home $dirD -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envD repmgr -local [list 127.0.0.1 $portD] -start client
	await_startup_done $envD
	puts "."

	puts "\tRepmgr029.z15: Try adding new site E again,\
	    this time it should succeed"
	# Note that it was not necessary to bounce the env handle.
	$envE repmgr -start client
	error_check_good nsites [$envA rep_get_nsites] 5
	await_condition {[expr [$envB rep_get_nsites] == 5]}
	await_startup_done $envE

	puts "\tRepmgr029.z15: Shut down C and D again,\
	    and this time try removing site E"
	$envC close
	$envD close
	set ret [catch {$envA repmgr -remove [list 127.0.0.1 $portE]} result]
	error_check_bad no_failure2 $ret 0
	error_check_match unavail2 $result "*DB_REP_UNAVAIL*"
	error_check_good nsites2 [$envA rep_get_nsites] 5
	error_check_good nsites3 [$envB rep_get_nsites] 5

	set db [berkdb open -env $envA -thread __db.rep.system __db.membership]
	set SITE_DELETING 2
	error_check_good deleting \
	    [repmgr029_gmdb_status $db 127.0.0.1 $portE] $SITE_DELETING
	$db close

	puts "\tRepmgr029.z15: See that site E fired event for as little\
	    as DELETING status"
	await_event $envE local_site_removed
	$envE close

	puts "\tRepmgr029.z15: Rescind the removal of site E"
	# The only way add site E is to have it start and try to join.  Someday
	# (maybe even before code freeze) it will be possible to restart the
	# zombie carcass in the same env handle.
	set envE [berkdb env -create -errpfx E -home $dirE -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envE repmgr -local [list 127.0.0.1 $portE] -start client
	error_check_good nsites4 [$envA rep_get_nsites] 5
	error_check_good nsites5 [$envB rep_get_nsites] 5

	$envE close
	$envB close
	$envA close
}

# See that removing a non-existent site acts as a no-op, and doesn't yield an
# error. 
proc z16 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	# Group size will be three, but allocate an extra port to act as the
	# non-existent sites.
	foreach {portA portB portC portD} [available_ports 4] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	puts -nonewline "\tRepmgr029.z16: Create a group of 3 sites"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA] -start master
	puts -nonewline "." ; flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts -nonewline "." ; flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	puts "."

	puts "\tRepmgr029.z16: Remove non-existent site"
	$envB repmgr -remove [list 127.0.0.1 $portD]
	error_check_good nsites [$envA rep_get_nsites] 3
	error_check_good nsites [$envB rep_get_nsites] 3
	error_check_good nsites [$envC rep_get_nsites] 3

	# While we're on the topic of removing sites, let's try having a site
	# remove itself.
	puts "\tRepmgr029.z16: Have site C remove itself"
	$envC repmgr -remove [list 127.0.0.1 $portC]
	error_check_good nsites [$envA rep_get_nsites] 2
	await_event $envC local_site_removed

	$envC close
	$envB close
	$envA close
}

# Exercise group creation with non-default ack policies.
proc z17 { } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	foreach {portA portB portC} [available_ports 3] {}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C

	foreach policy {one onepeer all allpeers allavailable none} {
		env_cleanup $testdir

		file mkdir $dirA
		file mkdir $dirB
		file mkdir $dirC

		puts -nonewline "\tRepmgr029.z17: Create a group of 3 sites using\
		    `$policy' ack policy"
		set envA [berkdb env -create -errpfx A -home $dirA -txn \
		    -rep -thread -verbose [list rep $rv]]
		$envA repmgr -local [list 127.0.0.1 $portA creator] \
		    -start elect -ack $policy
		puts -nonewline "." ; flush stdout

		set envB [berkdb env -create -errpfx B -home $dirB -txn \
		    -rep -thread -verbose [list rep $rv] -event]
		$envB repmgr -local [list 127.0.0.1 $portB] \
		    -remote [list 127.0.0.1 $portA] -start elect -ack $policy
		await_startup_done $envB
		puts -nonewline "." ; flush stdout

		set envC [berkdb env -create -errpfx C -home $dirC -txn \
		    -rep -thread -verbose [list rep $rv] -event]
		$envC repmgr -local [list 127.0.0.1 $portC] \
		    -remote [list 127.0.0.1 $portA] -start elect
		await_startup_done $envC
		puts "."

		puts "\tRepmgr029.z17: Remove both clients."
		$envA repmgr -remove [list 127.0.0.1 $portB]
		error_check_good nsites [$envA rep_get_nsites] 2
		await_event $envB local_site_removed
		$envB close
		$envA repmgr -remove [list 127.0.0.1 $portC]
		error_check_good nsites [$envA rep_get_nsites] 1
		await_event $envC local_site_removed
		$envC close

		$envA close
	}
}

#
#  Add new site to existing group, already populated via hot backup
#  a. Start site A as group creator.
#  b. Start site B as client, and wait it for sync.
#  c. Hot backup the site B's environment to directory C,
#     and start up site C using the directory C.
#  d. Check membership at site C 
#
proc z18 { } {
	global rep_verbose
	global testdir
	global util_path

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC} [available_ports 3] {}

	set dirA $testdir/dirA
	set dirB $testdir/dirB
	set dirC $testdir/dirC

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	puts -nonewline "\tRepmgr029.z18.a: Start site A as master."
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA creator] \
	    -start master
	error_check_good nsites_A [$envA rep_get_nsites] 1
	puts "." ; flush stdout

	puts -nonewline "\tRepmgr029.z18.b. Start site B"
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -verbose [list rep $rv]]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	error_check_good nsites_B [$envB rep_get_nsites] 2
	puts "." ; flush stdout

	puts "\tRepmgr029.z18.c.1: Hot backup the site B's environment to $dirC"
	# Ensure $dirC is empty before hot backup.
	set files [glob -nocomplain $dirC/*]
	error_check_good no_files [llength $files] 0

	eval exec $util_path/db_hotbackup -vh $dirB -b $dirC

	puts "\tRepmgr029.z18.c.2: Start up site C in $dirC."
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	error_check_good nsites_C [$envC rep_get_nsites] 3

	puts "\tRepmgr029.z18.c.3: Verify site C starts without internal init."
	error_check_good no_pages [stat_field $envC rep_stat "Pages received"] 0

	error_check_good siteC_close [$envC close] 0
	error_check_good siteB_close [$envB close] 0
	error_check_good siteA_close [$envA close] 0
}

#
# Initiate group change during long-running transaction at master 
# (waits for transaction to abort)
# a. start site A as master
# b. begin a transaction, write a record
# c. start a separate process to add a second site ("B") to the group
# d. in the transaction in b, write a record and sleep for a second in a loop.
#    Would run into deadlock
# e. abort the txn when the deadlock occurs
# f. after that, the joining operation in the other thread should complete
#    successfully.
#
proc z19 {} {
	global rep_verbose
	global testdir
	global tclsh_path
	global test_path

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB} [available_ports 2] {}

	set dirA $testdir/dirA
	set dirB $testdir/dirB

	file mkdir $dirA
	file mkdir $dirB

	puts "\tRepmgr029.z19.a: Start up site A as master "
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv]]
	$envA repmgr -local [list 127.0.0.1 $portA creator] -start master
	error_check_good nsites_A [$envA rep_get_nsites] 1

	puts "\tRepmgr029.z19.b: Begin txn and open db on master."
	set txn [$envA txn]
	error_check_good txn_begin [is_valid_txn $txn $envA] TRUE
	set testfile repmg029.db
	set oflags {-create -btree -mode 0755 -thread -env $envA \
	    -txn $txn $testfile}
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	puts "\tRepmgr029.z19.c: Add site B in another process"
	set pid [exec $tclsh_path $test_path/wrap.tcl repmgr029script2.tcl \
	    $testdir/repmgr029script2.log $dirB $portB $dirA $portA &]

	puts "\tRepmgr029.z19.d: Write data in the txn, expecting deadlock"
	set maxcount 100
	for { set count 0 } { $count < $maxcount } { incr count } {
		set key $count
		set data "gmdb data"
		if { [catch {$db put -txn $txn $key $data} ret] } {
			error_check_good put_deadlock \
			    [is_substr $ret DB_LOCK_DEADLOCK] 1
			break
		} else {
			tclsleep 1
		}
	}
	error_check_good put_deadlock [is_substr $ret DB_LOCK_DEADLOCK] 1
	error_check_good txn_abort [$txn abort] 0

	puts "\tRepmgr029.z19.e: Confirm B has joined."
	for { set count 0 } { $count < $maxcount } { incr count } {
		if { [$envA rep_get_nsites] > 1 } {
			break
		} else {
			tclsleep 1
		}
	}

	watch_procs $pid 5
	error_check_good db_close [$db close] 0
	error_check_good master_close [$envA close] 0

	# Check output file of the sub-process for failures.
	set file repmgr029script2.log
	set errstrings [eval findfail $testdir/$file]
	foreach str $errstrings {
		puts "$str"
	}
	error_check_good errstrings_llength [llength $errstrings] 0
}

# Test setting and unsetting local site.
proc z20 {} {
	global rep_verbose
	global testdir
	global tclsh_path
	global test_path

	env_cleanup $testdir
	foreach {portA portB} [available_ports 2] {}
	set dirA $testdir/dirA
	set dirB $testdir/dirB
	file mkdir $dirA
	file mkdir $dirB
	set envA_cmd "berkdb env -create -home $dirA -txn -rep \
	    -thread -recover"
	set envB_cmd "berkdb env -create -home $dirB -txn -rep \
	    -thread -recover"

	puts "\tRepmgr029.z20.a: Set local site"
	make_dbconfig $dirA \
	    [list [list repmgr_site 127.0.0.1 $portA db_local_site on]]
	set envA [eval $envA_cmd]
	$envA repmgr -start master
	error_check_good local_a [$envA repmgr_get_local_site] \
	    "127.0.0.1 $portA"

	puts "\tRepmgr029.z20.b: Set a local site and a non-local site"
	make_dbconfig $dirB \
	    [list [list repmgr_site 127.0.0.1 $portB db_local_site on] \
	    [list repmgr_site 127.0.0.1 $portA db_local_site off] \
	    [list repmgr_site 127.0.0.1 $portA db_bootstrap_helper on]]
	set envB [eval $envB_cmd]
	$envB repmgr -start client
	error_check_good local_b [$envB repmgr_get_local_site] \
	    "127.0.0.1 $portB"
	error_check_good sites_b [$envB rep_get_nsites] 2
	$envB close

	puts "\tRepmgr029.z20.c: Set a non-local site and a local site"
	env_cleanup $dirB
	make_dbconfig $dirB \
	    [list [list repmgr_site 127.0.0.1 $portA db_local_site off] \
	    [list repmgr_site 127.0.0.1 $portA db_bootstrap_helper on] \
	    [list repmgr_site 127.0.0.1 $portB db_local_site on]]
	set envB [eval $envB_cmd]
	$envB repmgr -start client
	error_check_good local_c [$envB repmgr_get_local_site] \
	    "127.0.0.1 $portB"
	error_check_good sites_c [$envB rep_get_nsites] 2
	$envB close

	puts "\tRepmgr029.z20.d: Cancel and reset local site (error)"
	env_cleanup $dirB
	make_dbconfig $dirB \
	    [list [list repmgr_site 127.0.0.1 $portA db_local_site on] \
	    [list repmgr_site 127.0.0.1 $portA db_local_site off] \
	    [list repmgr_site 127.0.0.1 $portA db_bootstrap_helper on] \
	    [list repmgr_site 127.0.0.1 $portB db_local_site on]]
	error_check_bad local_d [ catch { eval $envB_cmd } msg ] 0
	error_check_good local_msg_d [is_substr $msg \
	    "A previously given local site may not be unset"] 1

	puts "\tRepmgr029.z20.e: Replace the local site (error)"
	env_cleanup $dirB
	make_dbconfig $dirB \
	    [list [list repmgr_site 127.0.0.1 $portA db_local_site on] \
	    [list repmgr_site 127.0.0.1 $portB db_local_site on] \
	    [list repmgr_site 127.0.0.1 $portA db_bootstrap_helper on]]
	error_check_bad local_e [ catch { eval $envB_cmd } msg ] 0
	error_check_good local_msg_e [is_substr $msg \
	    "A (different) local site has already been set"] 1

	$envA close
}

# Test receiving limbo gmdb update for the running site itself.  If its status
# is deleting after reloading the latest gmdb, it should be removed from the 
# group.  If its status is adding, it should rejoin soon.
proc z21 {} {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	foreach {portA portB portC portD} [available_ports 4] {}

	set dirA $testdir/dirA
	set dirB $testdir/dirB
	set dirC $testdir/dirC
	set dirD $testdir/dirD

	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC
	file mkdir $dirD

	set SITE_ADDING 1
	set SITE_DELETING 2
	set SITE_PRESENT 4

	puts -nonewline "\tRepmgr029.z21.a: Start A, B, C"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
	    -recover -verbose [list rep $rv] -event]
	$envA repmgr -local [list 127.0.0.1 $portA creator] -start master
	error_check_good nsites_A [$envA rep_get_nsites] 1
	puts -nonewline "." ; flush stdout

	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	error_check_good nsites_B [$envB rep_get_nsites] 2
	puts -nonewline "." ; flush stdout

	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
	    -verbose [list rep $rv] -event]
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envC
	error_check_good nsites_C [$envC rep_get_nsites] 3
	puts "." ; flush stdout

	foreach status [list $SITE_ADDING $SITE_DELETING] {
		puts "\tRepmgr029.z21.$status.a: Start D"
		set envD [berkdb env -create -home $dirD -txn \
		    -rep -thread -verbose [list rep $rv] -event]
		$envD repmgr -local [list 127.0.0.1 $portD] \
		    -remote [list 127.0.0.1 $portA] -start client
		await_startup_done $envD
		error_check_good nsites_D [$envD rep_get_nsites] 4

		if { $status == $SITE_ADDING } {
			set status_str "adding"
		} else {
			set status_str "deleting"
		}
		puts "\tRepmgr029.z21.$status.b: Update D in gmdb from\
		    present to $status_str"
		set db [berkdb open -env $envA -thread -auto_commit \
		    __db.rep.system __db.membership]
		error_check_good site_D_added\
		    [repmgr029_gmdb_status $db 127.0.0.1 $portD] $SITE_PRESENT
		repmgr029_gmdb_update_site $db 127.0.0.1 $portD $status

		puts "\tRepmgr029.z21.$status.c: Sync in-memory gmdb on A, B, C"
		# Replicate gmdb changes and more updates to clients.
		rep_test btree $envA NULL 10 0 0 0

		# Sync the in-memory sites info and array with the on-disk gmdb.
		catch {$envA repmgr -start elect -msgth 2}
		catch {$envB repmgr -start elect -msgth 2}
		catch {$envC repmgr -start elect -msgth 2}

		puts "\tRepmgr029.z21.$status.d: Sync in-memory gmdb on D"
		set ret [catch {$envD repmgr -start elect -msgth 2} result]
		error_check_bad has_failure $ret 0
		puts -nonewline "\tRepmgr029.z21.$status.e: "
		if { $status == $SITE_ADDING } {
			# Expected error to start repmgr on a running listener,
			# but we finish reloading gmdb by this.
			error_check_match bad_pram [is_substr $result \
			    "repmgr is already started"] 1
			puts "D has rejoined the group"
			await_condition {[expr [repmgr029_gmdb_status \
			    $db 127.0.0.1 $portD] == $SITE_PRESENT]} 40
		} else {
			puts "D can't rejoin the group"
			# Get DB_DELETED during reloading gmdb.
			error_check_match unavail $result "*DB_REP_UNAVAIL*"
		}

		puts "\tRepmgr029.z21.$status.f: Remove D"
		$envA repmgr -remove [list 127.0.0.1 $portD]
		rep_test btree $envA NULL 10 0 0 0
		await_event $envB site_removed
		await_event $envC site_removed
		await_event $envA site_removed
		$envD close
		env_cleanup $testdir/dirD
	}

	$db close
	$envB close
	$envC close
	$envA close
}

# Test an offline and limbo site's attempts to start repmgr and rejoin the
# group.  If it is adding, it should rejoin soon.  Otherwise, it should be
# removed when it is absent from the group. 
proc z22 {} {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } { set rv on }

	env_cleanup $testdir
	foreach {port0 port1} [available_ports 2] {}

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set SITE_ADDING 1
	set SITE_PRESENT 4

	puts -nonewline "\tRepmgr029.z22.a: Start master and client"
	set env1 [berkdb_env -create -errpfx MASTER -home $masterdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]
	$env1 repmgr -local [list 127.0.0.1 $port0] -start master
	puts -nonewline "."
	flush stdout

	set env2 [berkdb_env_noerr -create -errpfx CLIENT -home $clientdir \
	    -txn -rep -thread -recover -verbose [list rep $rv] -event]
	$env2 repmgr -local [list 127.0.0.1 $port1] \
	    -remote [list 127.0.0.1 $port0] -start client -pri 0
	await_startup_done $env2
	puts "."
	flush stdout

	puts "\tRepmgr029.z22.b: Close and reopen the master environment"
	$env1 close
	set env1 [berkdb_env -create -errpfx MASTER -home $masterdir \
	    -txn -rep -thread -recover -verbose [list rep $rv]]

	puts "\tRepmgr029.z22.c: Update client's status to be adding on master"
	set db [berkdb open -env $env1 -thread -auto_commit \
	    __db.rep.system __db.membership]
	error_check_good client_added \
	    [repmgr029_gmdb_status $db 127.0.0.1 $port1] $SITE_PRESENT
	repmgr029_gmdb_update_site $db 127.0.0.1 $port1 $SITE_ADDING

	puts "\tRepmgr029.z22.d: Start repmgr on master"
	$env1 repmgr -local [list 127.0.0.1 $port0] -start master

	puts "\tRepmgr029.z22.e: Wait for client to rejoin the group"
	await_condition {[expr \
	    [repmgr029_gmdb_status $db 127.0.0.1 $port1] == $SITE_PRESENT]} 50
	$db close
	set env2_ev [find_event [$env2 event_info] local_site_removed]
	error_check_good no_removal [string length $env2_ev] 0

	puts "\tRepmgr029.z22.f: Close all"
	$env2 close
	$env1 close
}

proc repmgr029_dump_db { e } {
	set db [berkdb open -env $e -thread __db.rep.system __db.membership]
	set c [$db cursor]
	set format_version [lindex [$c get -first] 0 1]
	binary scan $format_version II fmt vers
	puts "format $fmt version $vers"
	while {[llength [set r [$c get -next]]] > 0} {
		set k [lindex $r 0 0]
		set v [lindex $r 0 1]
		binary scan $k I len
		set hostname [string range $k 4 [expr 2 + $len]]
		binary scan $hostname A* host
		binary scan [string range $k [expr 4 + $len] end] S port
		if { $fmt < 2 } {
			binary scan $v I status
			puts "{$host $port} status $status"
		} else {
			binary scan $v "II" status flags
			puts "{$host $port} status $status flags $flags"
		}
	}
	$c close
	$db close
}

proc repmgr029_get_eid { e port } {
	set sle [repmgr029_get_site_list_entry $e $port]
	if { [string length $sle] == 0} {
		return ""
	}
	return [lindex $sle 0]
}

proc repmgr029_get_site_list_entry { e port } {
	foreach sle [$e repmgr_site_list] {
		set p [lindex $sle 2]
		if { $p == $port } {
			return $sle
		}
	}
	return ""
}

proc repmgr029_gmdb_status { db host port } {
	set l [string length $host]
	set key [binary format Ia*cS [expr $l + 1] $host 0 $port]
	set kvlist [$db get $key]
	if {[llength $kvlist] == 0} {
		return 0
	}
	set kvpair [lindex $kvlist 0]
	set val [lindex $kvpair 1]
	binary scan $val I status
	return $status
}

# The proc is only used to reliably create hard-to-reproduce test cases.
# Otherwise, gmdb should never be manipulated directly.
proc repmgr029_gmdb_update_site { db host port status } {
	set l [string length $host]
	set key [binary format Ia*cS [expr $l + 1] $host 0 $port]
	set data [binary format II $status 0]
	$db put $key $data

	set key [binary format IS 0 0]
	set kvlist [$db get $key]
	set kvpair [lindex $kvlist 0]
	set val [lindex $kvpair 1]
	binary scan $val II format version
	set data [binary format II $format [expr $version + 1] ]
	$db put $key $data
}

proc repmgr029_gmdb_version { db } {
	set key [binary format IS 0 0]
	set kvlist [$db get $key]
	set kvpair [lindex $kvlist 0]
	set val [lindex $kvpair 1]
	binary scan $val II format version
	return $version
}

proc repmgr029_known_master { e1 e2 e3 } {
	foreach e [list $e1 $e2 $e3] {
		set m [stat_field $e rep_stat "Master environment ID"]
		if {$m == -2} {
			return no
		}
	}
	return yes
}

proc repmgr029_site_list_status { e port } {
	set sle [repmgr029_get_site_list_entry $e $port]
	if { [string length $sle] == 0 } {
		return ""
	}
	return [lindex $sle 3]
}

proc repmgr029_sync_sites { dir1 dir2 db nkeys ndata } {
	global util_path
	set in_sync 0

	while { !$in_sync } {
		set e1_stat [exec $util_path/db_stat -h $dir1 -d $db]
		set e1_nkeys [is_substr $e1_stat \
		    "$nkeys\tNumber of unique keys in the tree"]
		set e1_ndata [is_substr $e1_stat \
		    "$nkeys\tNumber of data items in the tree"]
		set e2_stat [exec $util_path/db_stat -h $dir2 -d $db]
		set e2_nkeys [is_substr $e2_stat \
		    "$ndata\tNumber of unique keys in the tree"]
		set e2_ndata [is_substr $e2_stat \
		    "$ndata\tNumber of data items in the tree"]
		if { $e1_nkeys == $e2_nkeys && $e1_ndata == $e2_ndata } {
			set in_sync 1
		} else {
			tclsleep 1
		}
	}
}

#
# Remove a site via a non-master helper site, test resulting site lists
# and events, and close the removed site.
#
proc repmgr029_remove_site_from_helper { masterenv \
    helperenv remenv remport masgmdb } {
	set eid_rem_at_mas [repmgr029_get_eid $masterenv $remport]
	set eid_rem_at_help [repmgr029_get_eid $helperenv $remport]

	# Remove site and make sure removal is reflected in master gmdb.
	$helperenv repmgr -remove [list 127.0.0.1 $remport]
	error_check_good site_removed \
	    [repmgr029_gmdb_status $masgmdb 127.0.0.1 $remport] 0

	# Make sure master site_removed event is fired.
	set master_ev [find_event [$masterenv event_info] site_removed]
	error_check_good master_event [llength $master_ev] 2
	error_check_good master_event_eid [lindex $master_ev 1] $eid_rem_at_mas
	error_check_good master_list [llength [repmgr029_get_eid \
	    $masterenv $remport]] 0

	# Make sure removed site gets local_site_removed event.
	await_event $remenv local_site_removed
	error_check_good s_rem_close [$remenv close] 0

	# Make sure helper site gets site_removed event and gmdb update.
	await_condition {[expr [string length [repmgr029_site_list_status \
	    $helperenv $remport]] == 0]}
	set helper_ev [find_event [$helperenv event_info] site_removed]
	error_check_good helper_event [llength $helper_ev] 2
	error_check_good helper_event_eid [lindex $helper_ev 1] \
	    $eid_rem_at_help
	error_check_good helper_list [llength [repmgr029_get_eid \
	    $helperenv $remport]] 0
}
