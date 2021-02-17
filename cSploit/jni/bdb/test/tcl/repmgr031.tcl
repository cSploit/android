# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
# Test for ack policies that vary throughout the group, and that change
# dynamically.
# 
proc repmgr031 { { niter 1 } { tnum "031" } } {
	
	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"

	set viewopts { noview view }
	foreach v $viewopts {
		puts "Repmgr$tnum ($method $v): repmgr varying ack policy test."
		repmgr031_sub $method $niter $tnum $v
	}
}

proc repmgr031_sub { method niter tnum viewopt } {
	global rep_verbose
	global testdir

	set rv off
	if { $rep_verbose == 1 } {
		set rv on
	}

	env_cleanup $testdir
	
	if { $viewopt == "view" } {
		foreach {portA portB portC portV} [available_ports 4] {}
	} else {
		foreach {portA portB portC} [available_ports 3] {}
	}

	set dirA $testdir/A
	set dirB $testdir/B
	set dirC $testdir/C
	
	file mkdir $dirA
	file mkdir $dirB
	file mkdir $dirC

	if { $viewopt == "view" } {
		set dirV $testdir/V
		file mkdir $dirV
	}

	puts -nonewline "\tRepmgr$tnum: Set up a group of 3:"
	set envA [berkdb env -create -errpfx A -home $dirA -txn -rep -thread \
		      -recover -verbose [list rep $rv] -event]
	$envA rep_config {mgrelections off}
	$envA repmgr -local [list 127.0.0.1 $portA] -start master -ack none
	puts -nonewline "." ; 	flush stdout
	
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
		      -recover -verbose [list rep $rv]]
	$envB rep_config {mgrelections off}
	$envB repmgr -local [list 127.0.0.1 $portB] \
	    -remote [list 127.0.0.1 $portA] -start client
	await_startup_done $envB
	puts -nonewline "." ; 	flush stdout
	
	set envC [berkdb env -create -errpfx C -home $dirC -txn -rep -thread \
		      -recover -verbose [list rep $rv] -event]
	$envC rep_config {mgrelections off}
	$envC repmgr -local [list 127.0.0.1 $portC] \
	    -remote [list 127.0.0.1 $portA] -start client -ack none
	await_startup_done $envC
	puts "."

	if { $viewopt == "view" } {
		puts "\tRepmgr$tnum: Add a view."
		set viewcb ""
		set view_envcmd "berkdb env -create -errpfx D -home $dirV \
		    -txn -rep -thread -recover -verbose \[list rep $rv\] \
		    -rep_view \[list $viewcb\]"
		set envD [eval $view_envcmd]
		$envD rep_config {mgrelections off}
		$envD repmgr -local [list 127.0.0.1 $portV] \
		    -remote [list 127.0.0.1 $portA] -start client
		await_startup_done $envD
	}	

	puts "\tRepmgr$tnum: Shut down site B."
	$envB close

	puts "\tRepmgr$tnum: Write updates and check perm_failed event."
	# Initially sites A and C both have "none" policy.  Site C won't even
	# bother to send an ack, which is just fine with site A.
	# 
	$envA event_info -clear
	eval rep_test $method $envA NULL $niter 0 0 0
	error_check_good nofailure \
	    [string length [find_event  [$envA event_info] perm_failed]] 0

	# Change ack policy at site A.  Site C will have to be notified of this
	# change, or else the lack of an ack would cause a perm failure.
	# 
	$envA repmgr -ack quorum
	eval rep_test $method $envA NULL $niter 0 0 0
	error_check_good nofailure \
	    [string length [find_event  [$envA event_info] perm_failed]] 0

	puts "\tRepmgr$tnum: Shut down site A, make site C new master."
	# Even though site C started sending acks for site A's benefit, its own
	# ack policy should still be "none".  With no other sites running, that
	# will be the only way to avoid perm_failed event.
	# 
	$envA close
	$envC repmgr -start master -msgth 0
	eval rep_test $method $envC NULL $niter 0 0 0
	error_check_good nofailure \
	    [string length [find_event  [$envC event_info] perm_failed]] 0
	
	puts "\tRepmgr$tnum: Change ack policy to quorum."
	$envC repmgr -ack quorum
	eval rep_test $method $envC NULL $niter 0 0 0
	error_check_bad failure \
	    [string length [find_event  [$envC event_info] perm_failed]] 0

	puts "\tRepmgr$tnum: Start site B, to provide acks needed by site C"
	$envC event_info -clear
	set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread \
		      -recover -verbose [list rep $rv]]
	$envB rep_config {mgrelections off}
	$envB repmgr -local [list 127.0.0.1 $portB] -start client
	await_startup_done $envB
	
	eval rep_test $method $envC NULL $niter 0 0 0
	error_check_good failure \
	    [string length [find_event  [$envC event_info] perm_failed]] 0

	if { $viewopt == "view" } {
		$envD close
	}
	$envB close
	$envC close
}
